#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include "pop_llvm_warnings.hpp"

#include "../lex_synt_lib_common/assert.hpp"
#include "constexpr_function_evaluator.hpp"

namespace U
{

namespace
{

// TODO - add possibility to setup these values, using compiler options.
constexpr size_t g_max_data_stack_size= 1024u * 1024u * 64u - 1u; // 64 Megabytes will be enough for stack.
constexpr size_t g_constants_segment_offset= g_max_data_stack_size + 1u;
constexpr size_t g_max_constants_stack_size =1024u * 1024u * 64u; // 64 Megabytes will be enough for constants.
constexpr size_t g_max_call_stack_depth = 1024u;

}

ConstexprFunctionEvaluator::ConstexprFunctionEvaluator( const llvm::DataLayout& data_layout )
	: data_layout_(data_layout)
{}

ConstexprFunctionEvaluator::Result ConstexprFunctionEvaluator::Evaluate(
	llvm::Function* const llvm_function,
	const llvm::ArrayRef<const llvm::Constant*> args )
{
	stack_.resize(16u); // reserve null pointer

	U_ASSERT( args.size() == llvm_function->getFunctionType()->getNumParams() );

	llvm::Type* return_type= llvm_function->getReturnType();
	size_t s_ret_ptr= 0u;

	// Fill arguments
	size_t i= 0u;
	for( const llvm::Argument& arg : llvm_function->args() )
	{
		if( arg.getType()->isPointerTy() )
		{
			llvm::GenericValue val;

			if( arg.hasStructRetAttr() )
			{
				U_ASSERT(i == 0u);
				return_type= arg.getType()->getPointerElementType();

				s_ret_ptr= stack_.size();
				const size_t new_stack_size= stack_.size() + size_t( data_layout_.getTypeAllocSize(return_type) );
				if( new_stack_size >= g_max_data_stack_size )
				{
					ReportDataStackOverflow();
					continue;
				}
				stack_.resize( new_stack_size );

				val.IntVal= llvm::APInt( 64u, uint64_t(s_ret_ptr) );
			}
			else
				val.IntVal= llvm::APInt( 64u, uint64_t( MoveConstantToStack( *args[i] ) ) );

			instructions_map_[ &arg ]= val;
		}
		else
			instructions_map_[ &arg ]= GetVal( args[i] );

		++i;
	}

	const llvm::GenericValue res= CallFunction( *llvm_function, 0u );

	Result result;
	result.errors= std::move(errors_);
	errors_= {};

	if( return_type->isIntegerTy() )
		result.result_constant= llvm::Constant::getIntegerValue( return_type, res.IntVal );
	else if( return_type->isFloatTy() )
		result.result_constant= llvm::ConstantFP::get( return_type, double(res.FloatVal) );
	else if( return_type->isDoubleTy() )
		result.result_constant= llvm::ConstantFP::get( return_type, res.DoubleVal );
	else if( return_type->isVoidTy() )
		result.result_constant= llvm::UndefValue::get( return_type ); // TODO - set correct value
	else if( return_type->isArrayTy() || return_type->isStructTy() )
		result.result_constant= CreateInitializerForStructElement( return_type, s_ret_ptr );
	else if( return_type->isPointerTy() )
		errors_.push_back( "returning pointer in constexpr function" );
	else U_ASSERT(false);

	instructions_map_.clear();
	stack_.clear();
	external_constant_mapping_.clear();
	constants_stack_.clear();

	return result;
}

llvm::GenericValue ConstexprFunctionEvaluator::CallFunction( const llvm::Function& llvm_function, const size_t stack_depth )
{
	if( llvm_function.getBasicBlockList().empty() )
	{
		errors_.push_back( "executing function \"" + std::string(llvm_function.getName()) + "\" with no body" );
		return llvm::GenericValue();
	}
	if( stack_depth > g_max_call_stack_depth )
	{
		errors_.push_back( "Max call stack depth (" + std::to_string( g_max_call_stack_depth ) + ") reached" );
		return llvm::GenericValue();
	}

	const size_t prev_stack_size= stack_.size();

	const llvm::BasicBlock* prev_basic_block= nullptr;
	const llvm::BasicBlock* current_basic_block= &llvm_function.getBasicBlockList().front();

	const llvm::Instruction* instruction= &*current_basic_block->begin();
	while( errors_.empty() )
	{
		switch( instruction->getOpcode() )
		{
		case llvm::Instruction::Alloca:
			ProcessAlloca(instruction);
			break;

		case llvm::Instruction::Load:
			ProcessLoad(instruction);
			break;

		case llvm::Instruction::Store:
			ProcessStore(instruction);
			break;

		case llvm::Instruction::GetElementPtr:
			ProcessGEP(instruction);
			break;

		case llvm::Instruction::Call:
			ProcessCall( instruction, stack_depth );
			break;

		case llvm::Instruction::Br:
			{
				prev_basic_block= current_basic_block;
				if( instruction->getNumOperands() == 1u ) // Unconditional
					current_basic_block= llvm::dyn_cast<llvm::BasicBlock>(instruction->getOperand(0u));
				else
				{
					const llvm::GenericValue val= GetVal(instruction->getOperand(0u));
					current_basic_block= llvm::dyn_cast<llvm::BasicBlock>(instruction->getOperand( val.IntVal.getBoolValue() ? 2u : 1u ));
				}
				instruction= &*current_basic_block->begin();
			}
			continue; // Continue loop without advancing instruction.

		case llvm::Instruction::PHI:
			{
				const auto phi_node= llvm::dyn_cast<llvm::PHINode>(instruction);

				for (unsigned int i= 0u; i < phi_node->getNumIncomingValues(); ++i )
				{
					if( phi_node->getIncomingBlock(i) == prev_basic_block)
					{
						instructions_map_[instruction]= GetVal( phi_node->getIncomingValue(i) );
						break;
					}
					U_ASSERT( i + 1u != phi_node->getNumIncomingValues() );
				}
			}
			break;

		case llvm::Instruction::Ret:
			{
				llvm::GenericValue res;
				if( instruction->getNumOperands() != 0u )
					res= GetVal( instruction->getOperand(0u) );

				stack_.resize( prev_stack_size );
				return res;
			}

		case llvm::Instruction::Unreachable:
			errors_.push_back( "executing Unreachable instruction" );
			return llvm::GenericValue();

		default:
			if( instruction->getNumOperands() == 1u )
				ProcessUnaryArithmeticInstruction(instruction);
			else if( instruction->getNumOperands() == 2u )
				ProcessBinaryArithmeticInstruction(instruction);
			else
			{
				errors_.push_back( std::string( "executing unknown instruction \"" ) + instruction->getOpcodeName() + "\"" );
				return llvm::GenericValue();
			}
		};

		instruction= instruction->getNextNode();
	}
	return llvm::GenericValue();
}

size_t ConstexprFunctionEvaluator::MoveConstantToStack( const llvm::Constant& constant )
{
	const auto prev_it= external_constant_mapping_.find( &constant );
	if( prev_it != external_constant_mapping_.end() )
		return prev_it->second;

	if( !constant.getType()->isSized() )
		return 0u; // Constant have incomplete type.

	// Use separate stack for constants, because we can push constants to it in any time.

	const size_t stack_offset= constants_stack_.size();
	const size_t new_stack_size= constants_stack_.size() + size_t( data_layout_.getTypeAllocSize( constant.getType() ) );
	if( new_stack_size >= g_max_constants_stack_size )
	{
		ReportConstantsStackOverflow();
		return 0u;
	}
	constants_stack_.resize( new_stack_size );

	external_constant_mapping_[&constant]= stack_offset + g_constants_segment_offset;

	CopyConstantToStack( constant, stack_offset );

	return stack_offset + g_constants_segment_offset;
}

void ConstexprFunctionEvaluator::CopyConstantToStack( const llvm::Constant& constant, const size_t stack_offset )
{
	// TODO - check big-endian/little-endian correctness.

	llvm::Type* const constant_type= constant.getType();
	if( const auto struct_type= llvm::dyn_cast<llvm::StructType>( constant_type ) )
	{
		const llvm::StructLayout& struct_layout= *data_layout_.getStructLayout( struct_type );

		unsigned int i= 0u;
		for( llvm::Type* const element_type : struct_type->elements() )
		{
			llvm::Constant* const element= constant.getAggregateElement(i);
			const size_t element_offset= size_t(struct_layout.getElementOffset(i));

			if( element_type->isPointerTy() )
			{
				if( llvm::dyn_cast<llvm::PointerType>(element_type)->getElementType()->isFunctionTy() )
					errors_.push_back( "passing function pointer to constexpr function" );
				else
				{
					const size_t element_ptr= MoveConstantToStack( *llvm::dyn_cast<llvm::GlobalVariable>(element)->getInitializer() );
					std::memcpy( constants_stack_.data() + stack_offset + element_offset, &element_ptr, sizeof(size_t) );
				}
			}
			else
				CopyConstantToStack( *element, stack_offset + element_offset );

			++i;
		}
	}
	else if( const auto array_type= llvm::dyn_cast<llvm::ArrayType>(constant_type) )
	{
		const size_t element_size= size_t( data_layout_.getTypeAllocSize( array_type->getElementType() ) );
		for( unsigned int i= 0u; i < array_type->getNumElements(); ++i )
			CopyConstantToStack( *constant.getAggregateElement(i), stack_offset + i * element_size );
	}
	else if( constant_type->isIntegerTy() )
	{
		const uint64_t val= constant.getUniqueInteger().getLimitedValue();
		std::memcpy( constants_stack_.data() + stack_offset, &val, size_t(data_layout_.getTypeAllocSize( constant_type )) );
	}
	else if( constant_type->isFloatTy() )
	{
		const float val= llvm::dyn_cast<llvm::ConstantFP>(&constant)->getValueAPF().convertToFloat();
		std::memcpy( constants_stack_.data() + stack_offset, &val, sizeof(float) );
	}
	else if( constant_type->isDoubleTy() )
	{
		const double val= llvm::dyn_cast<llvm::ConstantFP>(&constant)->getValueAPF().convertToDouble();
		std::memcpy( constants_stack_.data() + stack_offset, &val, sizeof(double) );
	}
	else if( constant_type->isPointerTy() )
	{
		const llvm::PointerType* const pointer_type= llvm::dyn_cast<llvm::PointerType>(constant_type);
		if( pointer_type->getElementType()->isFunctionTy() )
			errors_.push_back( "passing function pointer to constexpr function" );
		else U_ASSERT(false);
		std::memset( constants_stack_.data() + stack_offset, 0, size_t(data_layout_.getTypeAllocSize( constant_type )) );
	}
	else U_ASSERT(false);
}

llvm::Constant* ConstexprFunctionEvaluator::CreateInitializerForStructElement( llvm::Type* const type, const size_t element_ptr )
{
	if( type->isIntegerTy() )
	{
		uint64_t val; // TODO - check big-endian/little-endian correctness.
		std::memcpy( &val, stack_.data() + element_ptr, sizeof(val) );
		return llvm::Constant::getIntegerValue( type, llvm::APInt( type->getIntegerBitWidth(), val ) );
	}
	else if( type->isFloatTy() )
	{
		float val;
		std::memcpy( &val, stack_.data() + element_ptr, sizeof(val) );
		return llvm::ConstantFP::get( type, static_cast<double>(val) );
	}
	else if( type->isDoubleTy() )
	{
		double val;
		std::memcpy( &val, stack_.data() + element_ptr, sizeof(val) );
		return llvm::ConstantFP::get( type, val );
	}
	else if( type->isArrayTy() )
	{
		llvm::ArrayType* const array_type= llvm::dyn_cast<llvm::ArrayType>(type);
		llvm::Type* const element_type= array_type->getElementType();
		const size_t element_size= size_t(data_layout_.getTypeAllocSize(element_type));

		std::vector<llvm::Constant*> initializers( size_t(array_type->getNumElements()), nullptr );
		for( unsigned int i= 0u; i < array_type->getNumElements(); ++i )
			initializers[i]= CreateInitializerForStructElement( array_type->getElementType(), element_ptr + i * element_size );

		return llvm::ConstantArray::get( array_type, initializers );
	}
	else if( type->isStructTy() )
	{
		llvm::StructType* const struct_type= llvm::dyn_cast<llvm::StructType>(type);
		const llvm::StructLayout& struct_layout= *data_layout_.getStructLayout( struct_type );

		ClassFieldsVector<llvm::Constant*> initializers( struct_type->getNumElements(), nullptr );
		for( unsigned int i= 0u; i < struct_type->getNumElements(); ++i )
			initializers[i]= CreateInitializerForStructElement( struct_type->getElementType(i), element_ptr + size_t(struct_layout.getElementOffset(i)) );

		return llvm::ConstantStruct::get( struct_type, initializers );
	}
	else if( type->isPointerTy() )
	{
		errors_.push_back( "building constant pointer" );
		return llvm::UndefValue::get( type );
	}
	else U_ASSERT(false);

	return nullptr;
}

llvm::GenericValue ConstexprFunctionEvaluator::GetVal( const llvm::Value* const val )
{
	llvm::GenericValue res;
	if( const auto constant_fp= llvm::dyn_cast<llvm::ConstantFP>(val) )
	{
		if( val->getType()->isFloatTy() )
			res.FloatVal= constant_fp->getValueAPF().convertToFloat();
		else if( val->getType()->isDoubleTy() )
			res.DoubleVal= constant_fp->getValueAPF().convertToDouble();
		else U_ASSERT(false);
	}
	else if( const auto constant_int= llvm::dyn_cast<llvm::ConstantInt>(val) )
		res.IntVal= constant_int->getValue();
	else if( const auto global_variable= llvm::dyn_cast<llvm::GlobalVariable>( val ) )
		res.IntVal= llvm::APInt( 64u, MoveConstantToStack( *global_variable->getInitializer() ) );
	else if( const auto constant_aggregate= llvm::dyn_cast<llvm::ConstantAggregate>(val) )
		res.IntVal= llvm::APInt( 64u, uint64_t( MoveConstantToStack( *constant_aggregate ) ) );
	else if( const auto constant_data_sequential= llvm::dyn_cast<llvm::ConstantDataSequential>(val) )
		res.IntVal= llvm::APInt( 64u, uint64_t( MoveConstantToStack( *constant_data_sequential ) ) );
	else if( llvm::dyn_cast<llvm::Function>(val) != nullptr )
		errors_.push_back( "accessing function pointer" );
	else if( auto constant_expression= llvm::dyn_cast<llvm::ConstantExpr>( val ) )
	{
		// Process here constant GEP.
		if( constant_expression->getOpcode() == llvm::Instruction::GetElementPtr )
		{
			const llvm::GenericValue ptr= GetVal( constant_expression->getOperand(0u) );

			uint64_t index_accumulated= 0u;
			llvm::Type* aggregate_type= constant_expression->getOperand(0u)->getType()->getPointerElementType();
			for( unsigned int op= 2u; op < constant_expression->getNumOperands(); ++op )
			{
				const llvm::GenericValue index= GetVal( constant_expression->getOperand(op) );

				if( aggregate_type->isArrayTy() )
				{
					llvm::Type* const element_type= aggregate_type->getArrayElementType();
					index_accumulated+= index.IntVal.getLimitedValue() * data_layout_.getTypeAllocSize( element_type );
					aggregate_type= element_type;
				}
				else if( aggregate_type->isStructTy() )
				{
					const llvm::StructLayout& struct_layout= *data_layout_.getStructLayout( llvm::dyn_cast<llvm::StructType>(aggregate_type) );
					const unsigned int element_index= static_cast<unsigned int>(index.IntVal.getLimitedValue());
					index_accumulated+= struct_layout.getElementOffset( element_index );
					aggregate_type= aggregate_type->getStructElementType( element_index );
				}
				else U_ASSERT(false);
			}
			llvm::GenericValue new_ptr;
			new_ptr.IntVal= ptr.IntVal + llvm::APInt( ptr.IntVal.getBitWidth(), index_accumulated );
			return new_ptr;
		}
		else U_ASSERT(false);
	}
	else
	{
		U_ASSERT( instructions_map_.find( val ) != instructions_map_.end() );
		res= instructions_map_[val];
	}
	return res;
}

void ConstexprFunctionEvaluator::ProcessAlloca( const llvm::Instruction* const instruction )
{
	llvm::Type* const element_type= llvm::dyn_cast<llvm::PointerType>(instruction->getType())->getElementType();

	const size_t stack_offset= stack_.size();
	const size_t new_stack_size= stack_.size() + size_t(data_layout_.getTypeAllocSize( element_type ));
	if( new_stack_size >= g_max_data_stack_size )
	{
		ReportDataStackOverflow();
		return;
	}
	stack_.resize( new_stack_size );

	llvm::GenericValue val;
	val.IntVal= llvm::APInt( data_layout_.getPointerSizeInBits(), uint64_t(stack_offset) );
	instructions_map_[ instruction ]= val;
}

void ConstexprFunctionEvaluator::ProcessLoad( const llvm::Instruction* const instruction )
{
	const llvm::GenericValue address_val= GetVal( instruction->getOperand(0u) );

	const size_t offset= size_t(address_val.IntVal.getLimitedValue());
	const unsigned char* data_ptr= nullptr;
	if( offset >= g_constants_segment_offset )
		data_ptr= constants_stack_.data() + ( offset - g_constants_segment_offset );
	else
		data_ptr= stack_.data() + offset;

	llvm::Type* const element_type= instruction->getType();
	llvm::GenericValue val;
	if( element_type->isIntegerTy() )
	{
		uint64_t buff[4];
		std::memcpy( buff, data_ptr, size_t(data_layout_.getTypeStoreSize( element_type )) );

		val.IntVal= llvm::APInt( element_type->getIntegerBitWidth() , buff );
	}
	else if( element_type->isFloatTy() )
		std::memcpy( &val.FloatVal, data_ptr, sizeof(float) );
	else if( element_type->isDoubleTy() )
		std::memcpy( &val.DoubleVal, data_ptr, sizeof(double) );
	else if( element_type->isPointerTy() )
	{
		uint64_t ptr;
		std::memcpy( &ptr, data_ptr, size_t(data_layout_.getTypeAllocSize( element_type )) );
		val.IntVal= llvm::APInt( 64u , ptr );
	}
	else U_ASSERT(false);

	instructions_map_[ instruction ]= val;
}

void ConstexprFunctionEvaluator::ProcessStore( const llvm::Instruction* const instruction )
{
	const llvm::Value* const address= instruction->getOperand(1u);
	U_ASSERT( instructions_map_.find( address ) != instructions_map_.end() );
	const llvm::GenericValue& address_val= instructions_map_[address];

	const size_t offset= size_t(address_val.IntVal.getLimitedValue());
	unsigned char* data_ptr= nullptr;
	if( offset >= g_constants_segment_offset )
		data_ptr= constants_stack_.data() + ( offset - g_constants_segment_offset );
	else
		data_ptr= stack_.data() + offset;

	const llvm::GenericValue val= GetVal( instruction->getOperand(0u) );

	llvm::Type* const element_type= llvm::dyn_cast<llvm::PointerType>(address->getType())->getElementType();
	if( element_type->isIntegerTy() )
	{
		const uint64_t limited_value= val.IntVal.getLimitedValue();
		std::memcpy( data_ptr, &limited_value, size_t(data_layout_.getTypeStoreSize( element_type )) );
	}
	else if( element_type->isFloatTy() )
		std::memcpy( data_ptr, &val.FloatVal, size_t(data_layout_.getTypeAllocSize( element_type )) );
	else if( element_type->isDoubleTy() )
		std::memcpy( data_ptr, &val.DoubleVal, size_t(data_layout_.getTypeAllocSize( element_type )) );
	else if( element_type->isPointerTy() )
	{
		const uint64_t ptr= val.IntVal.getLimitedValue();
		std::memcpy( data_ptr, &ptr, size_t(data_layout_.getTypeAllocSize( element_type )) );
	}
	else U_ASSERT(false);
}

void ConstexprFunctionEvaluator::ProcessGEP( const llvm::Instruction* const instruction )
{
	U_ASSERT( instruction->getNumOperands() == 3u ); // Currently compiler generates only 3-operand GEP commands.

	const llvm::GenericValue ptr= GetVal( instruction->getOperand(0u) );
	const llvm::GenericValue index= GetVal( instruction->getOperand(2u) );

	llvm::Type* const aggregate_type= llvm::dyn_cast<llvm::PointerType>(instruction->getOperand(0u)->getType())->getElementType();

	llvm::GenericValue new_ptr;
	if( aggregate_type->isArrayTy() )
	{
		llvm::Type* const element_type= llvm::dyn_cast<llvm::ArrayType>( aggregate_type )->getElementType();
		new_ptr.IntVal=
			ptr.IntVal +
			llvm::APInt( ptr.IntVal.getBitWidth(), index.IntVal.getLimitedValue() ) *
			llvm::APInt( ptr.IntVal.getBitWidth(), data_layout_.getTypeAllocSize( element_type ) );
	}
	else if( aggregate_type->isStructTy() )
	{
		const llvm::StructLayout& struct_layout= *data_layout_.getStructLayout( llvm::dyn_cast<llvm::StructType>(aggregate_type) );
		new_ptr.IntVal=
			ptr.IntVal +
			llvm::APInt( ptr.IntVal.getBitWidth(), struct_layout.getElementOffset( static_cast<unsigned int>(index.IntVal.getLimitedValue()) ) );
	}
	else U_ASSERT(false);

	instructions_map_[instruction]= new_ptr;
}

void ConstexprFunctionEvaluator::ProcessCall( const llvm::Instruction* const instruction, const size_t stack_depth )
{
	const llvm::Function* const function= llvm::dyn_cast<llvm::Function>(instruction->getOperand(instruction->getNumOperands() - 1u)); // Function is last operand
	U_ASSERT(function != nullptr);
	U_ASSERT( function->arg_size() == instruction->getNumOperands() - 1u );

	if(function->getName() == "llvm.dbg.declare")
		return;
	if( function->getIntrinsicID() == llvm::Intrinsic::memcpy || function->getIntrinsicID() == llvm::Intrinsic::memmove )
	{
		ProcessMemmove( instruction );
		return;
	}

	InstructionsMap new_instructions_map;

	const size_t prev_stack_size= stack_.size();

	unsigned int i= 0u;
	for( const llvm::Argument& arg : function->args() )
	{
		new_instructions_map[ &arg ]= GetVal( instruction->getOperand(i) );
		++i;
	}

	instructions_map_.swap(new_instructions_map);
	llvm::GenericValue result_val= CallFunction( *function, stack_depth + 1u );
	instructions_map_.swap(new_instructions_map);

	if( !function->getReturnType()->isVoidTy() )
		instructions_map_[instruction]= result_val;

	stack_.resize( prev_stack_size ); // Drop temporary byval arguments.
}

void ConstexprFunctionEvaluator::ProcessMemmove( const llvm::Instruction* const instruction )
{
	const size_t dst_offset= size_t( GetVal( instruction->getOperand(0u) ).IntVal.getLimitedValue() );
	const size_t src_offset= size_t( GetVal( instruction->getOperand(1u) ).IntVal.getLimitedValue() );
	const size_t size= size_t( GetVal( instruction->getOperand(2u) ).IntVal.getLimitedValue() );

	unsigned char* const dst_ptr=
		( dst_offset >= g_constants_segment_offset )
			? ( constants_stack_.data() + ( dst_offset - g_constants_segment_offset ) )
			: ( stack_.data() + dst_offset );
	const unsigned char* const src_ptr=
		( src_offset >= g_constants_segment_offset )
			? ( constants_stack_.data() + ( src_offset - g_constants_segment_offset ) )
			: ( stack_.data() + src_offset );

	std::memmove( dst_ptr, src_ptr, size );
}

void ConstexprFunctionEvaluator::ProcessUnaryArithmeticInstruction( const llvm::Instruction* const instruction )
{
	const llvm::GenericValue op= GetVal( instruction->getOperand(0u) );

	llvm::Type* const dst_type= instruction->getType();
	llvm::Type* const src_type= instruction->getOperand(0u)->getType();
	llvm::GenericValue val;
	switch(instruction->getOpcode())
	{
	case llvm::Instruction::SExt:
		U_ASSERT(dst_type->isIntegerTy());
		val.IntVal= op.IntVal.sext(dst_type->getIntegerBitWidth());
		break;

	case llvm::Instruction::ZExt:
		U_ASSERT(dst_type->isIntegerTy());
		val.IntVal= op.IntVal.zext(dst_type->getIntegerBitWidth());
		break;

	case llvm::Instruction::Trunc:
		U_ASSERT(dst_type->isIntegerTy());
		val.IntVal= op.IntVal.trunc(dst_type->getIntegerBitWidth());
		break;

	case llvm::Instruction::FPExt:
		if( dst_type->isFloatTy() )
		{
			if( src_type->isFloatTy() ) val.FloatVal= op.FloatVal;
			else U_ASSERT(false);
		}
		else if( dst_type->isDoubleTy() )
		{
			if( src_type->isFloatTy() ) val.DoubleVal= llvm::APFloat(op.FloatVal).convertToDouble();
			else U_ASSERT(false);
		}
		else U_ASSERT(false);
		break;

	case llvm::Instruction::FPTrunc:
		if( dst_type->isFloatTy() )
		{
			if( src_type->isFloatTy() ) val.FloatVal= op.FloatVal;
			else if( src_type->isDoubleTy() ) val.FloatVal= llvm::APFloat(op.DoubleVal).convertToFloat();
			else U_ASSERT(false);
		}
		else if( dst_type->isDoubleTy() )
		{
			if( src_type->isDoubleTy() ) val.DoubleVal= op.DoubleVal;
			else U_ASSERT(false);
		}
		else U_ASSERT(false);
		break;

	case llvm::Instruction::SIToFP:
		U_ASSERT(src_type->isIntegerTy());
		if( dst_type->isFloatTy() )
			val.FloatVal= llvm::APIntOps::RoundSignedAPIntToFloat(op.IntVal);
		else if( dst_type->isDoubleTy() )
			val.DoubleVal= llvm::APIntOps::RoundSignedAPIntToDouble(op.IntVal);
		else U_ASSERT(false);
		break;

	case llvm::Instruction::UIToFP:
		U_ASSERT(src_type->isIntegerTy());
		if( dst_type->isFloatTy() )
			val.FloatVal= llvm::APIntOps::RoundAPIntToFloat(op.IntVal);
		else if( dst_type->isDoubleTy() )
			val.DoubleVal= llvm::APIntOps::RoundAPIntToDouble(op.IntVal);
		else U_ASSERT(false);
		break;

	// TODO - is this correct way to convert floats to ints?
	case llvm::Instruction::FPToSI:
		U_ASSERT(dst_type->isIntegerTy());
		if( src_type->isFloatTy() )
			val.IntVal= llvm::APIntOps::RoundFloatToAPInt( op.FloatVal, dst_type->getIntegerBitWidth() );
		else if( src_type->isDoubleTy() )
			val.IntVal= llvm::APIntOps::RoundDoubleToAPInt( op.DoubleVal, dst_type->getIntegerBitWidth() );
		else U_ASSERT(false);
		break;

	case llvm::Instruction::FPToUI:
		U_ASSERT(dst_type->isIntegerTy());
		if( src_type->isFloatTy() )
			val.IntVal= llvm::APIntOps::RoundFloatToAPInt( op.FloatVal, dst_type->getIntegerBitWidth() );
		else if( src_type->isDoubleTy() )
			val.IntVal= llvm::APIntOps::RoundDoubleToAPInt( op.DoubleVal, dst_type->getIntegerBitWidth() );
		else U_ASSERT(false);
		break;

	case llvm::Instruction::BitCast:
		val.IntVal= llvm::APInt( dst_type->getIntegerBitWidth(), op.IntVal.getLimitedValue() );
		break;

	default:
		errors_.push_back( std::string( "executing unknown unary instruction \"" ) + instruction->getOpcodeName() + "\"" );
		break;
	}

	instructions_map_[instruction]= val;
}

void ConstexprFunctionEvaluator::ProcessBinaryArithmeticInstruction( const llvm::Instruction* const instruction )
{
	const llvm::GenericValue op0= GetVal( instruction->getOperand(0u) );
	const llvm::GenericValue op1= GetVal( instruction->getOperand(1u) );

	llvm::Type* const type= instruction->getOperand(0u)->getType();
	llvm::GenericValue val;
	switch(instruction->getOpcode())
	{
	case llvm::Instruction::Add:
		U_ASSERT(type->isIntegerTy());
		val.IntVal= op0.IntVal + op1.IntVal;
		break;

	case llvm::Instruction::Sub:
		U_ASSERT(type->isIntegerTy());
		val.IntVal= op0.IntVal - op1.IntVal;
		break;

	case llvm::Instruction::Mul:
		U_ASSERT(type->isIntegerTy());
		val.IntVal= op0.IntVal * op1.IntVal;
		break;

	case llvm::Instruction::SDiv:
		U_ASSERT(type->isIntegerTy());
		if( op1.IntVal.getBoolValue() )
			val.IntVal= op0.IntVal.sdiv(op1.IntVal);
		else
			errors_.push_back( "constexpr division by zero" );
		break;

	case llvm::Instruction::UDiv:
		U_ASSERT(type->isIntegerTy());
		if( op1.IntVal.getBoolValue() )
			val.IntVal= op0.IntVal.udiv(op1.IntVal);
		else
			errors_.push_back( "constexpr division by zero" );
		break;

	case llvm::Instruction::SRem:
		U_ASSERT(type->isIntegerTy());
		if( op1.IntVal.getBoolValue() )
			val.IntVal= op0.IntVal.srem(op1.IntVal);
		else
			errors_.push_back( "constexpr division by zero" );
		break;

	case llvm::Instruction::URem:
		U_ASSERT(type->isIntegerTy());
		if( op1.IntVal.getBoolValue() )
			val.IntVal= op0.IntVal.urem(op1.IntVal);
		else
			errors_.push_back( "constexpr division by zero" );
		break;

	case llvm::Instruction::And:
		U_ASSERT(type->isIntegerTy());
		val.IntVal= op0.IntVal & op1.IntVal;
		break;

	case llvm::Instruction::Or:
		U_ASSERT(type->isIntegerTy());
		val.IntVal= op0.IntVal | op1.IntVal;
		break;

	case llvm::Instruction::Xor:
		U_ASSERT(type->isIntegerTy());
		val.IntVal= op0.IntVal ^ op1.IntVal;
		break;

	case llvm::Instruction::Shl:
		U_ASSERT(type->isIntegerTy());
		val.IntVal= op0.IntVal.shl(op1.IntVal);
		break;

	case llvm::Instruction::AShr:
		U_ASSERT(type->isIntegerTy());
		val.IntVal= op0.IntVal.ashr(op1.IntVal);
		break;

	case llvm::Instruction::LShr:
		U_ASSERT(type->isIntegerTy());
		val.IntVal= op0.IntVal.lshr(op1.IntVal);
		break;

	case llvm::Instruction::FAdd:
		if( type->isFloatTy() )
			val.FloatVal= ( llvm::APFloat(op0.FloatVal) + llvm::APFloat(op1.FloatVal) ).convertToFloat();
		else if( type->isDoubleTy() )
			val.DoubleVal= ( llvm::APFloat(op0.DoubleVal) + llvm::APFloat(op1.DoubleVal) ).convertToDouble();
		else U_ASSERT(false);
		break;

	case llvm::Instruction::FSub:
		if( type->isFloatTy() )
			val.FloatVal= ( llvm::APFloat(op0.FloatVal) - llvm::APFloat(op1.FloatVal) ).convertToFloat();
		else if( type->isDoubleTy() )
			val.DoubleVal= ( llvm::APFloat(op0.DoubleVal) - llvm::APFloat(op1.DoubleVal) ).convertToDouble();
		else U_ASSERT(false);
		break;

	case llvm::Instruction::FMul:
		if( type->isFloatTy() )
			val.FloatVal= ( llvm::APFloat(op0.FloatVal) * llvm::APFloat(op1.FloatVal) ).convertToFloat();
		else if( type->isDoubleTy() )
			val.DoubleVal= ( llvm::APFloat(op0.DoubleVal) * llvm::APFloat(op1.DoubleVal) ).convertToDouble();
		else U_ASSERT(false);
		break;

	case llvm::Instruction::FDiv:
		if( type->isFloatTy() )
			val.FloatVal= ( llvm::APFloat(op0.FloatVal) / llvm::APFloat(op1.FloatVal) ).convertToFloat();
		else if( type->isDoubleTy() )
			val.DoubleVal= ( llvm::APFloat(op0.DoubleVal) / llvm::APFloat(op1.DoubleVal) ).convertToDouble();
		else U_ASSERT(false);
		break;

	case llvm::Instruction::FRem:
		{
			if( type->isFloatTy() )
			{
				llvm::APFloat result_val(op0.FloatVal);
				result_val.mod( llvm::APFloat(op1.FloatVal));
				val.FloatVal= result_val.convertToFloat();
			}
			else if( type->isDoubleTy() )
			{
				llvm::APFloat result_val(op0.DoubleVal);
				result_val.mod( llvm::APFloat(op1.DoubleVal) );
				val.DoubleVal= result_val.convertToDouble();
			}
			else U_ASSERT(false);
		}
		break;

	case llvm::Instruction::ICmp:
		U_ASSERT(type->isIntegerTy());
		switch(llvm::dyn_cast<llvm::CmpInst>(instruction)->getPredicate())
		{
		case llvm::CmpInst::ICMP_EQ : val.IntVal= op0.IntVal.eq (op1.IntVal); break;
		case llvm::CmpInst::ICMP_NE : val.IntVal= op0.IntVal.ne (op1.IntVal); break;
		case llvm::CmpInst::ICMP_UGT: val.IntVal= op0.IntVal.ugt(op1.IntVal); break;
		case llvm::CmpInst::ICMP_UGE: val.IntVal= op0.IntVal.uge(op1.IntVal); break;
		case llvm::CmpInst::ICMP_ULT: val.IntVal= op0.IntVal.ult(op1.IntVal); break;
		case llvm::CmpInst::ICMP_ULE: val.IntVal= op0.IntVal.ule(op1.IntVal); break;
		case llvm::CmpInst::ICMP_SGT: val.IntVal= op0.IntVal.sgt(op1.IntVal); break;
		case llvm::CmpInst::ICMP_SGE: val.IntVal= op0.IntVal.sge(op1.IntVal); break;
		case llvm::CmpInst::ICMP_SLT: val.IntVal= op0.IntVal.slt(op1.IntVal); break;
		case llvm::CmpInst::ICMP_SLE: val.IntVal= op0.IntVal.sle(op1.IntVal); break;
		default: U_ASSERT(false); break;
		};
		break;

	case llvm::Instruction::FCmp:
		U_ASSERT(type->isFloatingPointTy());
		{
			llvm::APFloat::cmpResult cmp_result;
			if( type->isFloatTy() )
				cmp_result= llvm::APFloat(op0.FloatVal).compare(llvm::APFloat(op1.FloatVal));
			else
				cmp_result= llvm::APFloat(op0.DoubleVal).compare(llvm::APFloat(op1.DoubleVal));
			switch(llvm::dyn_cast<llvm::CmpInst>(instruction)->getPredicate())
			{
			// see llvm-3.7.1.src/lib/IR/ConstantFold.cpp:1752
			default: U_ASSERT(false); break;
			case llvm::FCmpInst::FCMP_FALSE: val.IntVal= llvm::APInt( 1u, 0u ); break;
			case llvm::FCmpInst::FCMP_TRUE : val.IntVal= llvm::APInt( 1u, 1u ); break;
			case llvm::FCmpInst::FCMP_UNO  : val.IntVal= llvm::APInt( 1u, cmp_result == llvm::APFloat::cmpUnordered ); break;
			case llvm::FCmpInst::FCMP_ORD  : val.IntVal= llvm::APInt( 1u, cmp_result != llvm::APFloat::cmpUnordered ); break;
			case llvm::FCmpInst::FCMP_UEQ  : val.IntVal= llvm::APInt( 1u, cmp_result == llvm::APFloat::cmpUnordered || cmp_result == llvm::APFloat::cmpEqual ); break;
			case llvm::FCmpInst::FCMP_OEQ  : val.IntVal= llvm::APInt( 1u, cmp_result == llvm::APFloat::cmpEqual ); break;
			case llvm::FCmpInst::FCMP_UNE  : val.IntVal= llvm::APInt( 1u, cmp_result != llvm::APFloat::cmpEqual ); break;
			case llvm::FCmpInst::FCMP_ONE  : val.IntVal= llvm::APInt( 1u, cmp_result == llvm::APFloat::cmpLessThan || cmp_result == llvm::APFloat::cmpGreaterThan ); break;
			case llvm::FCmpInst::FCMP_ULT  : val.IntVal= llvm::APInt( 1u, cmp_result == llvm::APFloat::cmpUnordered || cmp_result == llvm::APFloat::cmpLessThan ); break;
			case llvm::FCmpInst::FCMP_OLT  : val.IntVal= llvm::APInt( 1u, cmp_result == llvm::APFloat::cmpLessThan ); break;
			case llvm::FCmpInst::FCMP_UGT  : val.IntVal= llvm::APInt( 1u, cmp_result == llvm::APFloat::cmpUnordered || cmp_result == llvm::APFloat::cmpGreaterThan ); break;
			case llvm::FCmpInst::FCMP_OGT  : val.IntVal= llvm::APInt( 1u, cmp_result == llvm::APFloat::cmpGreaterThan ); break;
			case llvm::FCmpInst::FCMP_ULE  : val.IntVal= llvm::APInt( 1u, cmp_result != llvm::APFloat::cmpGreaterThan ); break;
			case llvm::FCmpInst::FCMP_OLE  : val.IntVal= llvm::APInt( 1u, cmp_result == llvm::APFloat::cmpLessThan || cmp_result == llvm::APFloat::cmpEqual ); break;
			case llvm::FCmpInst::FCMP_UGE  : val.IntVal= llvm::APInt( 1u, cmp_result != llvm::APFloat::cmpLessThan ); break;
			case llvm::FCmpInst::FCMP_OGE  : val.IntVal= llvm::APInt( 1u, cmp_result == llvm::APFloat::cmpGreaterThan || cmp_result == llvm::APFloat::cmpEqual ); break;
			}
		}
		break;

	default:
		errors_.push_back( std::string("executing unknown binary instruction \"") + instruction->getOpcodeName() + "\"" );
		break;
	};

	instructions_map_[instruction]= val;
}

void ConstexprFunctionEvaluator::ReportDataStackOverflow()
{
	errors_.push_back( "Max data stack size (" + std::to_string( g_max_data_stack_size ) + ") reached");
}

void ConstexprFunctionEvaluator::ReportConstantsStackOverflow()
{
	errors_.push_back( "Max constants stack size (" + std::to_string( g_max_constants_stack_size ) + ") reached" );
}

} // namespace U
