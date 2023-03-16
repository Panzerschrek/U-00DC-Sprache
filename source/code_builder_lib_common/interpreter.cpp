#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Operator.h>
#include "pop_llvm_warnings.hpp"

#include "../lex_synt_lib_common/assert.hpp"
#include "interpreter.hpp"

namespace U
{

namespace
{

// TODO - add possibility to setup these values, using compiler options.
constexpr size_t g_max_data_stack_size= 1024u * 1024u * 64u - 16u; // 64 Megabytes will be enough for stack.
constexpr size_t g_constants_segment_offset= g_max_data_stack_size + 16u;
constexpr size_t g_max_globals_stack_size =1024u * 1024u * 64u; // 64 Megabytes will be enough for constants.
constexpr size_t g_max_call_stack_depth = 1024u;

}

Interpreter::Interpreter( const llvm::DataLayout& data_layout )
	: data_layout_(data_layout), pointer_size_in_bits_( data_layout_.getPointerSizeInBits() )
{}

Interpreter::ResultConstexpr Interpreter::EvaluateConstexpr(
	llvm::Function* const llvm_function,
	const llvm::ArrayRef<const llvm::Constant*> args )
{
	stack_.resize(16u); // reserve null pointer

	U_ASSERT( args.size() == llvm_function->getFunctionType()->getNumParams() );

	llvm::Type* return_type= llvm_function->getReturnType();

	size_t s_ret_ptr= 0u;

	// Fill arguments
	size_t i= 0u;
	for( const llvm::Argument& param : llvm_function->args() )
	{
		if( param.getType()->isPointerTy() )
		{
			if( const auto s_ret_type= param.getParamStructRetType() )
			{
				U_ASSERT(i == 0u);
				return_type= s_ret_type;

				s_ret_ptr= stack_.size();
				const size_t new_stack_size= stack_.size() + size_t( data_layout_.getTypeAllocSize(return_type) );
				if( new_stack_size >= g_max_data_stack_size )
				{
					ReportDataStackOverflow();
					continue;
				}
				stack_.resize( new_stack_size );

				llvm::GenericValue val;
				val.IntVal= llvm::APInt( 64u, uint64_t(s_ret_ptr) );
				instructions_map_[ &param ]= std::move(val);
			}
			else
			{
				// Assume this is reference param.
				llvm::GenericValue val;
				val.IntVal= llvm::APInt( 64u, uint64_t( MoveConstantToStack( *args[i] ) ) );
				instructions_map_[ &param ]= std::move(val);
			}
		}
		else if( param.getType() == args[i]->getType() )
			instructions_map_[ &param ]= GetVal( args[i] );
		else U_ASSERT(false);

		++i;
	}

	U_ASSERT( !return_type->isPointerTy() ); // Currently can return only values.

	const llvm::GenericValue res= CallFunction( *llvm_function, 0u );

	ResultConstexpr result;
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
		result.result_constant= ReadConstantFromStack( return_type, s_ret_ptr );
	else if( return_type->isPointerTy() )
		errors_.push_back( "returning pointer in constexpr function" );
	else U_ASSERT(false);

	instructions_map_.clear();
	stack_.clear();
	external_constant_mapping_.clear();
	globals_stack_.clear();

	return result;
}

Interpreter::ResultGeneric Interpreter::EvaluateGeneric(
	llvm::Function* const llvm_function,
	const llvm::ArrayRef<llvm::GenericValue> args )
{
	stack_.resize(16u); // reserve null pointer

	U_ASSERT( args.size() == llvm_function->getFunctionType()->getNumParams() );

	// Fill arguments
	size_t i= 0u;
	for( const llvm::Argument& param : llvm_function->args() )
	{
		U_ASSERT( ! param.getType()->isPointerTy() );
		instructions_map_[ &param ]= args[i];

		++i;
	}

	Interpreter::ResultGeneric res;
	res.result= CallFunction( *llvm_function, 0u );

	res.errors= std::move(errors_);
	errors_= {};

	instructions_map_.clear();
	stack_.clear();

	// Preserve globals and external constants here.

	return res;
}

void Interpreter::RegisterCustomFunction( const llvm::StringRef name, const CustomFunction function )
{
	custom_functions_.insert_or_assign( name, function );
}

void Interpreter::ReadExecutinEngineData( void* const dst, const uint64_t address, const size_t size ) const
{
	const size_t offset= size_t(address);
	const std::byte* data_ptr= nullptr;
	if( offset >= g_constants_segment_offset )
		data_ptr= globals_stack_.data() + ( address - g_constants_segment_offset );
	else
		data_ptr= stack_.data() + address;

	std::memcpy( dst, data_ptr, size );
}

llvm::GenericValue Interpreter::CallFunction( const llvm::Function& llvm_function, const size_t stack_depth )
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
			ProcessCall( llvm::dyn_cast<llvm::CallInst>(instruction), stack_depth );
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

				for (uint32_t i= 0u; i < phi_node->getNumIncomingValues(); ++i )
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

		case llvm::Instruction::Select:
			{
				const llvm::GenericValue bool_val= GetVal(instruction->getOperand(0u));
				instructions_map_[instruction]= GetVal( instruction->getOperand( bool_val.IntVal.getBoolValue() ? 1u : 2u ) );
			}
			break;

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

size_t Interpreter::MoveConstantToStack( const llvm::Constant& constant )
{
	const auto prev_it= external_constant_mapping_.find( &constant );
	if( prev_it != external_constant_mapping_.end() )
		return prev_it->second;

	if( !constant.getType()->isSized() )
		return 0u; // Constant have incomplete type.

	// Use separate stack for constants, because we can push constants to it in any time.

	const size_t stack_offset= globals_stack_.size();
	const size_t new_stack_size= globals_stack_.size() + size_t( data_layout_.getTypeAllocSize( constant.getType() ) );
	if( new_stack_size >= g_max_globals_stack_size )
	{
		ReportGlobalsStackOverflow();
		return 0u;
	}
	globals_stack_.resize( new_stack_size );

	external_constant_mapping_[&constant]= stack_offset + g_constants_segment_offset;

	CopyConstantToStack( constant, stack_offset );

	return stack_offset + g_constants_segment_offset;
}

void Interpreter::CopyConstantToStack( const llvm::Constant& constant, const size_t stack_offset )
{
	// TODO - check big-endian/little-endian correctness.

	llvm::Type* const constant_type= constant.getType();
	if( const auto struct_type= llvm::dyn_cast<llvm::StructType>( constant_type ) )
	{
		const llvm::StructLayout& struct_layout= *data_layout_.getStructLayout( struct_type );

		uint32_t i= 0u;
		for( llvm::Type* const element_type : struct_type->elements() )
		{
			llvm::Constant* const element= constant.getAggregateElement(i);
			const size_t element_offset= size_t(struct_layout.getElementOffset(i));

			if( element_type->isPointerTy() )
			{
				size_t element_ptr= 0;
				if( llvm::ConstantExpr* const constant_expression= llvm::dyn_cast<llvm::ConstantExpr>( element ) )
				{
					if( constant_expression->getOpcode() == llvm::Instruction::GetElementPtr )
						element_ptr= size_t( BuildGEP( constant_expression ).IntVal.getLimitedValue() );
					else U_ASSERT(false);
				}
				else if( const auto global_variable= llvm::dyn_cast<llvm::GlobalVariable>(element) )
					element_ptr= MoveConstantToStack( *global_variable->getInitializer() );
				else if( element->isNullValue() )
					element_ptr= 0;
				else if( const auto function= llvm::dyn_cast<llvm::Function>(element) )
					element_ptr= reinterpret_cast<size_t>( function );
				else U_ASSERT(false);

				std::memcpy( globals_stack_.data() + stack_offset + element_offset, &element_ptr, sizeof(size_t) );
			}
			else
				CopyConstantToStack( *element, stack_offset + element_offset );

			++i;
		}
	}
	else if( const auto array_type= llvm::dyn_cast<llvm::ArrayType>(constant_type) )
	{
		const size_t element_size= size_t( data_layout_.getTypeAllocSize( array_type->getElementType() ) );
		for( uint32_t i= 0u; i < array_type->getNumElements(); ++i )
			CopyConstantToStack( *constant.getAggregateElement(i), stack_offset + i * element_size );
	}
	else if( constant_type->isIntegerTy() )
	{
		// TODO - check big-endian/little-endian correctness.
		const llvm::APInt val = constant.getUniqueInteger();
		if( val.getBitWidth() <= 64 || val.getBitWidth() % 64u == 0 )
			std::memcpy( globals_stack_.data() + stack_offset, val.getRawData(), size_t(data_layout_.getTypeAllocSize( constant_type )) );
		else U_ASSERT(false);
	}
	else if( constant_type->isFloatTy() )
	{
		const float val= llvm::dyn_cast<llvm::ConstantFP>(&constant)->getValueAPF().convertToFloat();
		std::memcpy( globals_stack_.data() + stack_offset, &val, sizeof(float) );
	}
	else if( constant_type->isDoubleTy() )
	{
		const double val= llvm::dyn_cast<llvm::ConstantFP>(&constant)->getValueAPF().convertToDouble();
		std::memcpy( globals_stack_.data() + stack_offset, &val, sizeof(double) );
	}
	else if( constant_type->isPointerTy() )
	{
		if( const auto function= llvm::dyn_cast<llvm::Function>(&constant) )
		{
			const uint64_t val= reinterpret_cast<size_t>(function);
			std::memcpy( globals_stack_.data() + stack_offset, &val, pointer_size_in_bits_ / 8 );
		}
		else
			std::memset( globals_stack_.data() + stack_offset, 0, size_t( data_layout_.getTypeAllocSize( constant_type ) ) );
	}
	else U_ASSERT(false);
}

llvm::Constant* Interpreter::ReadConstantFromStack( llvm::Type* const type, const size_t value_ptr )
{
	if( const auto integer_type= llvm::dyn_cast<llvm::IntegerType>(type) )
	{
		if( integer_type->getBitWidth() <= 64u )
		{
			uint64_t val; // TODO - check big-endian/little-endian correctness.
			std::memcpy( &val, stack_.data() + value_ptr, sizeof(val) );
			return llvm::Constant::getIntegerValue( type, llvm::APInt( integer_type->getBitWidth(), val ) );
		}
		else if ( integer_type->getBitWidth() % 64u == 0u )
			return
				llvm::Constant::getIntegerValue(
					type,
					llvm::APInt(
						uint32_t(integer_type->getBitWidth()),
						uint32_t(integer_type->getBitWidth() / sizeof(uint64_t)),
						reinterpret_cast<uint64_t*>(stack_.data() + value_ptr) ) );
		else U_ASSERT(false);
	}
	else if( type->isFloatTy() )
	{
		float val;
		std::memcpy( &val, stack_.data() + value_ptr, sizeof(val) );
		return llvm::ConstantFP::get( type, double(val) );
	}
	else if( type->isDoubleTy() )
	{
		double val;
		std::memcpy( &val, stack_.data() + value_ptr, sizeof(val) );
		return llvm::ConstantFP::get( type, val );
	}
	else if( const auto array_type= llvm::dyn_cast<llvm::ArrayType>(type) )
	{
		const size_t element_size= size_t(data_layout_.getTypeAllocSize(array_type->getElementType()));

		std::vector<llvm::Constant*> initializers( size_t(array_type->getNumElements()), nullptr );
		for( uint32_t i= 0u; i < array_type->getNumElements(); ++i )
			initializers[i]= ReadConstantFromStack( array_type->getElementType(), value_ptr + i * element_size );

		return llvm::ConstantArray::get( array_type, initializers );
	}
	else if( const auto struct_type= llvm::dyn_cast<llvm::StructType>(type) )
	{
		const llvm::StructLayout& struct_layout= *data_layout_.getStructLayout( struct_type );

		ClassFieldsVector<llvm::Constant*> initializers( struct_type->getNumElements(), nullptr );
		for( uint32_t i= 0u; i < struct_type->getNumElements(); ++i )
			initializers[i]= ReadConstantFromStack( struct_type->getElementType(i), value_ptr + size_t(struct_layout.getElementOffset(i)) );

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

llvm::GenericValue Interpreter::BuildGEP( const llvm::User* const instruction )
{
	U_ASSERT( instruction->getNumOperands() >= 2u );

	llvm::User::const_op_iterator op= instruction->op_begin();
	const llvm::User::const_op_iterator op_end= instruction->op_end();

	const llvm::GenericValue ptr= GetVal( op->get() );
	++op;

	const llvm::GenericValue first_index= GetVal( op->get() );
	++op;

	// TODO - check if this is correct cast.
	llvm::Type* const ptr_element_type= llvm::dyn_cast<llvm::GEPOperator>(instruction)->getSourceElementType();

	uint64_t offset_accumulated= first_index.IntVal.getLimitedValue() * data_layout_.getTypeAllocSize( ptr_element_type );
	llvm::Type* aggregate_type= ptr_element_type;

	for(; op != op_end; ++op)
	{
		const llvm::GenericValue index= GetVal( op->get() );

		if( const auto array_type= llvm::dyn_cast<llvm::ArrayType>(aggregate_type) )
		{
			const auto element_type= array_type->getElementType();
			offset_accumulated+= index.IntVal.getLimitedValue() * data_layout_.getTypeAllocSize( element_type );
			aggregate_type= element_type;
		}
		else if( const auto struct_type= llvm::dyn_cast<llvm::StructType>(aggregate_type) )
		{
			const llvm::StructLayout& struct_layout= *data_layout_.getStructLayout( struct_type );
			const uint32_t element_index= uint32_t(index.IntVal.getLimitedValue());
			offset_accumulated+= struct_layout.getElementOffset( element_index );
			aggregate_type= aggregate_type->getStructElementType( element_index );
		}
		else U_ASSERT(false);
	}

	llvm::GenericValue new_ptr;
	new_ptr.IntVal= ptr.IntVal + llvm::APInt( ptr.IntVal.getBitWidth(), offset_accumulated );
	return new_ptr;
}

llvm::GenericValue Interpreter::GetVal( const llvm::Value* const val )
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
	else if( llvm::dyn_cast<llvm::ConstantPointerNull>(val) != nullptr )
		res.IntVal= llvm::APInt( pointer_size_in_bits_, uint64_t(0) );
	else if( const auto global_variable= llvm::dyn_cast<llvm::GlobalVariable>( val ) )
		res.IntVal= llvm::APInt( 64u, MoveConstantToStack( *global_variable->getInitializer() ) );
	else if( const auto constant_struct= llvm::dyn_cast<llvm::ConstantStruct>( val ) )
	{
		res.AggregateVal.resize( constant_struct->getType()->getNumElements() );
		for( uint32_t i= 0u; i < res.AggregateVal.size(); ++i )
			res.AggregateVal[i]= GetVal( constant_struct->getAggregateElement(i) );
	}
	else if( const auto constant_array= llvm::dyn_cast<llvm::ConstantArray>( val ) )
	{
		res.AggregateVal.resize( size_t(constant_array->getType()->getNumElements()) );
		for( uint32_t i= 0u; i < res.AggregateVal.size(); ++i )
			res.AggregateVal[i]= GetVal( constant_array->getAggregateElement(i) );
	}
	else if( const auto constant_data_array= llvm::dyn_cast<llvm::ConstantDataArray>( val ) )
	{
		res.AggregateVal.resize( size_t(constant_data_array->getNumElements()) );
		for( uint32_t i= 0u; i < res.AggregateVal.size(); ++i )
			res.AggregateVal[i]= GetVal( constant_data_array->getAggregateElement(i) );
	}
	else if( const auto constant_zero= llvm::dyn_cast<llvm::ConstantAggregateZero>( val ) )
	{
		res.AggregateVal.resize( size_t(constant_zero->getElementCount().getKnownMinValue()) );
		for( uint32_t i= 0u; i < res.AggregateVal.size(); ++i )
			res.AggregateVal[i]= GetVal( constant_zero->getElementValue(i) );
	}
	else if (const auto undef_value= llvm::dyn_cast<llvm::UndefValue>( val ) )
	{
		// Udef values are possible but compiler may produce it where it can not break functional purity.
		// So, just fill undef value with zeros.
		if( val->getType()->isFloatTy() )
			res.FloatVal= 0.0f;
		else if( val->getType()->isDoubleTy() )
			res.DoubleVal= 0.0f;
		else if( val->getType()->isIntegerTy() )
			res.IntVal= undef_value->getUniqueInteger();
		else if( const auto struct_type= llvm::dyn_cast<llvm::StructType>( val->getType() ) )
		{
			res.AggregateVal.resize( struct_type->getNumElements() );
			for( uint32_t i= 0u; i < res.AggregateVal.size(); ++i )
				res.AggregateVal[i]= GetVal( undef_value->getElementValue(i) );
		}
		else if( const auto array_type= llvm::dyn_cast<llvm::ArrayType>( val->getType() ) )
		{
			res.AggregateVal.resize( size_t(array_type->getNumElements()) );
			for( uint32_t i= 0u; i < res.AggregateVal.size(); ++i )
				res.AggregateVal[i]= GetVal( undef_value->getElementValue(i) );
		}
		else U_ASSERT(false);
	}
	else if( const auto function= llvm::dyn_cast<llvm::Function>(val) )
		res.IntVal= llvm::APInt( pointer_size_in_bits_, reinterpret_cast<size_t>( function ) );
	else if( auto constant_expression= llvm::dyn_cast<llvm::ConstantExpr>( val ) )
	{
		if( constant_expression->getOpcode() == llvm::Instruction::GetElementPtr )
			res= BuildGEP( constant_expression );
		else U_ASSERT(false);
	}
	else
	{
		U_ASSERT( instructions_map_.find( val ) != instructions_map_.end() );
		res= instructions_map_[val];
	}
	return res;
}

void Interpreter::ProcessAlloca( const llvm::Instruction* const instruction )
{
	const auto alloca_instruction= llvm::dyn_cast<llvm::AllocaInst>(instruction);
	llvm::Type* const element_type= alloca_instruction->getAllocatedType();

	const size_t stack_offset= stack_.size();
	const size_t new_stack_size= stack_.size() + size_t(data_layout_.getTypeAllocSize( element_type ));
	if( new_stack_size >= g_max_data_stack_size )
	{
		ReportDataStackOverflow();
		return;
	}
	stack_.resize( new_stack_size );

	llvm::GenericValue val;
	val.IntVal= llvm::APInt( pointer_size_in_bits_, uint64_t(stack_offset) );
	instructions_map_[ instruction ]= val;
}

void Interpreter::ProcessLoad( const llvm::Instruction* const instruction )
{
	const llvm::GenericValue address_val= GetVal( instruction->getOperand(0u) );

	const size_t offset= size_t(address_val.IntVal.getLimitedValue());
	const std::byte* data_ptr= nullptr;
	if( offset >= g_constants_segment_offset )
		data_ptr= globals_stack_.data() + ( offset - g_constants_segment_offset );
	else
		data_ptr= stack_.data() + offset;

	instructions_map_[ instruction ]= DoLoad( data_ptr, instruction->getType() );
}

llvm::GenericValue Interpreter::DoLoad( const std::byte* ptr, llvm::Type* const t )
{
	llvm::GenericValue val;
	if( t->isIntegerTy() )
	{
		uint64_t buff[4];
		std::memcpy( buff, ptr, size_t(data_layout_.getTypeStoreSize( t )) );

		val.IntVal= llvm::APInt( t->getIntegerBitWidth() , buff );
	}
	else if( t->isFloatTy() )
		std::memcpy( &val.FloatVal, ptr, sizeof(float) );
	else if( t->isDoubleTy() )
		std::memcpy( &val.DoubleVal, ptr, sizeof(double) );
	else if( t->isPointerTy() )
	{
		uint64_t int_ptr;
		std::memcpy( &int_ptr, ptr, size_t(data_layout_.getTypeAllocSize( t )) );
		val.IntVal= llvm::APInt( 64u, int_ptr );
	}
	else if( t->isStructTy() )
	{
		const auto struct_type= llvm::dyn_cast<llvm::StructType>(t);
		const uint32_t num_elements= struct_type->getNumElements();
		const llvm::StructLayout *const struct_layout= data_layout_.getStructLayout(struct_type);

		val.AggregateVal.resize(num_elements);
		for (uint32_t i= 0; i < num_elements; ++i)
			val.AggregateVal[i]=
				DoLoad(
					ptr + struct_layout->getElementOffset(i),
					struct_type->getElementType(i));

	}
	else if( t->isArrayTy() )
	{
		const auto array_type= llvm::cast<llvm::ArrayType>(t);
		const uint64_t num_elements= array_type->getNumElements();
		const auto element_type= array_type->getElementType();
		const uint64_t element_size= data_layout_.getTypeAllocSize(element_type);

		val.AggregateVal.resize(num_elements);
		for( uint64_t i= 0; i < num_elements; ++i)
			val.AggregateVal[i]=
				DoLoad(
					ptr+ i * element_size,
					element_type );
	}
	else U_ASSERT(false);

	return val;
}

void Interpreter::ProcessStore( const llvm::Instruction* const instruction )
{
	const llvm::GenericValue address_val= GetVal( instruction->getOperand(1u) );

	const size_t offset= size_t(address_val.IntVal.getLimitedValue());
	std::byte* data_ptr= nullptr;
	if( offset >= g_constants_segment_offset )
		data_ptr= globals_stack_.data() + ( offset - g_constants_segment_offset );
	else
		data_ptr= stack_.data() + offset;

	const auto value_operand= instruction->getOperand(0u);
	DoStore( data_ptr, GetVal( value_operand ), value_operand->getType() );
}

void Interpreter::DoStore( std::byte* const ptr, const llvm::GenericValue& val, llvm::Type* const t )
{
	if( t->isIntegerTy() )
	{
		if( t->getIntegerBitWidth() <= 64 )
		{
			const uint64_t limited_value= val.IntVal.getLimitedValue();
			std::memcpy( ptr, &limited_value, size_t(data_layout_.getTypeStoreSize( t )) );
		}
		else if( t->getIntegerBitWidth() % 64u == 0 )
			std::memcpy( ptr, val.IntVal.getRawData(), t->getIntegerBitWidth() / 8u );
		else
		{
			U_ASSERT(false); // Not implemented yet.
		}
	}
	else if( t->isFloatTy() )
		std::memcpy( ptr, &val.FloatVal, size_t(data_layout_.getTypeAllocSize( t )) );
	else if( t->isDoubleTy() )
		std::memcpy( ptr, &val.DoubleVal, size_t(data_layout_.getTypeAllocSize( t )) );
	else if( t->isPointerTy() )
	{
		const uint64_t int_ptr= val.IntVal.getLimitedValue();
		std::memcpy( ptr, &int_ptr, size_t(data_layout_.getTypeAllocSize( t )) );
	}
	else if( t->isStructTy() )
	{
		const auto struct_type= llvm::dyn_cast<llvm::StructType>(t);
		const uint32_t num_elements= struct_type->getNumElements();
		const llvm::StructLayout *const struct_layout= data_layout_.getStructLayout(struct_type);

		U_ASSERT( val.AggregateVal.size() == num_elements );
		for (uint32_t i= 0; i < num_elements; ++i)
			DoStore(
				ptr + struct_layout->getElementOffset(i),
				val.AggregateVal[i],
				struct_type->getElementType(i));

	}
	else if( t->isArrayTy() )
	{
		const auto array_type= llvm::cast<llvm::ArrayType>(t);
		const uint64_t num_elements= array_type->getNumElements();
		const auto element_type= array_type->getElementType();
		const uint64_t element_size= data_layout_.getTypeAllocSize(element_type);

		U_ASSERT( val.AggregateVal.size() == num_elements );
		for( uint64_t i= 0; i < num_elements; ++i)
				DoStore(
					ptr + i * element_size,
					val.AggregateVal[i],
					element_type );
	}
	else U_ASSERT(false);
}

void Interpreter::ProcessGEP( const llvm::Instruction* const instruction )
{
	instructions_map_[instruction]= BuildGEP( instruction );
}

void Interpreter::ProcessCall( const llvm::CallInst* const instruction, const size_t stack_depth )
{
	const llvm::Value* const calle= instruction->getCalledOperand();
	const llvm::Function* function= llvm::dyn_cast<llvm::Function>(calle);
	if( function == nullptr )
		function= reinterpret_cast<const llvm::Function*>( size_t( GetVal( calle ).IntVal.getLimitedValue() ) );

	if( function == nullptr )
	{
		errors_.push_back( "Calling zero functon pointer" );
		return;
	}

	U_ASSERT( function->arg_size() == instruction->getNumOperands() - 1u );

	if( function->isIntrinsic() )
	{
		if( function->getIntrinsicID() == llvm::Intrinsic::dbg_declare ||
			function->getIntrinsicID() == llvm::Intrinsic::lifetime_start ||
			function->getIntrinsicID() == llvm::Intrinsic::lifetime_end)
			return;
		if( function->getIntrinsicID() == llvm::Intrinsic::memcpy || function->getIntrinsicID() == llvm::Intrinsic::memmove )
		{
			ProcessMemmove( instruction );
			return;
		}
	}
	else if( const auto func_it= custom_functions_.find( function->getName() ); func_it != custom_functions_.end() )
	{
		const CustomFunction func= func_it->second;
		llvm::SmallVector<llvm::GenericValue, 8> args;
		args.reserve( function->arg_size() );
		for( size_t i= 0; i < function->arg_size(); ++i )
			args.push_back( GetVal( instruction->getOperand(uint32_t(i)) ) );

		instructions_map_[instruction]= func( function->getFunctionType(), args );
		return;
	}

	InstructionsMap new_instructions_map;

	const size_t prev_stack_size= stack_.size();

	uint32_t i= 0u;
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

void Interpreter::ProcessMemmove( const llvm::Instruction* const instruction )
{
	const size_t dst_offset= size_t( GetVal( instruction->getOperand(0u) ).IntVal.getLimitedValue() );
	const size_t src_offset= size_t( GetVal( instruction->getOperand(1u) ).IntVal.getLimitedValue() );
	const size_t size= size_t( GetVal( instruction->getOperand(2u) ).IntVal.getLimitedValue() );

	std::byte* const dst_ptr=
		( dst_offset >= g_constants_segment_offset )
			? ( globals_stack_.data() + ( dst_offset - g_constants_segment_offset ) )
			: ( stack_.data() + dst_offset );
	const std::byte* const src_ptr=
		( src_offset >= g_constants_segment_offset )
			? ( globals_stack_.data() + ( src_offset - g_constants_segment_offset ) )
			: ( stack_.data() + src_offset );

	std::memmove( dst_ptr, src_ptr, size );
}

void Interpreter::ProcessUnaryArithmeticInstruction( const llvm::Instruction* const instruction )
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
			if( src_type->isFloatTy() ) val.DoubleVal= double(op.FloatVal);
			else U_ASSERT(false);
		}
		else U_ASSERT(false);
		break;

	case llvm::Instruction::FPTrunc:
		if( dst_type->isFloatTy() )
		{
			if( src_type->isFloatTy() ) val.FloatVal= op.FloatVal;
			else if( src_type->isDoubleTy() ) val.FloatVal= float(op.DoubleVal);
			else U_ASSERT(false);
		}
		else if( dst_type->isDoubleTy() )
		{
			if( src_type->isDoubleTy() ) val.DoubleVal= op.DoubleVal;
			else U_ASSERT(false);
		}
		else U_ASSERT(false);
		break;

	case llvm::Instruction::IntToPtr:
		val.IntVal= llvm::APInt( pointer_size_in_bits_, op.IntVal.getLimitedValue() );
		break;

	case llvm::Instruction::PtrToInt:
		val.IntVal= llvm::APInt( pointer_size_in_bits_, op.IntVal.getLimitedValue() );
		break;

	case llvm::Instruction::FNeg:
		if( src_type->isFloatTy() ) val.FloatVal= -op.FloatVal;
		else if( src_type->isDoubleTy() ) val.DoubleVal = -op.DoubleVal;
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
		if( dst_type->isFloatingPointTy() && src_type->isIntegerTy() )
		{
			if( dst_type->isDoubleTy() )
			{
				std::byte bytes[ sizeof(double) ] {};
				const uint64_t v= op.IntVal.getLimitedValue();
				std::memcpy(bytes, &v, sizeof(double));
				std::memcpy(&val.DoubleVal, bytes, sizeof(double));
			}
			else if( dst_type->isFloatTy() )
			{
				std::byte bytes[ sizeof(float) ] {};
				const uint32_t v= uint32_t(op.IntVal.getLimitedValue());
				std::memcpy(bytes, &v, sizeof(float));
				std::memcpy(&val.FloatVal, bytes, sizeof(float));
			}
			else
				errors_.push_back( "Invalid int to float cast" );
		}
		else if( dst_type->isIntegerTy() && src_type->isFloatingPointTy() )
		{
			if( src_type->isDoubleTy() )
			{
				std::byte bytes[ sizeof(double) ] {};
				std::memcpy(bytes, &op.DoubleVal, sizeof(double));
				uint64_t v= 0;
				std::memcpy(&v, bytes, sizeof(double));
				val.IntVal= llvm::APInt( sizeof(double) * 8, v );
			}
			else if( src_type->isFloatTy() )
			{
				std::byte bytes[ sizeof(float) ] {};
				std::memcpy(bytes, &op.FloatVal, sizeof(float));
				uint32_t v= 0;
				std::memcpy(&v, bytes, sizeof(float));
				val.IntVal= llvm::APInt( sizeof(float) * 8, uint64_t(v) );
			}
			else
				errors_.push_back( "Invalid float to int cast" );
		}
		else
		{
			// Cast function pointer or pointer for memcpy
			val.IntVal= llvm::APInt( uint32_t(data_layout_.getTypeAllocSizeInBits( dst_type )), op.IntVal.getLimitedValue() );
		}
		break;

	default:
		errors_.push_back( std::string( "executing unknown unary instruction \"" ) + instruction->getOpcodeName() + "\"" );
		break;
	}

	instructions_map_[instruction]= val;
}

void Interpreter::ProcessBinaryArithmeticInstruction( const llvm::Instruction* const instruction )
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
		U_ASSERT(type->isIntegerTy() || type->isPointerTy());
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

void Interpreter::ReportDataStackOverflow()
{
	errors_.push_back( "Max data stack size (" + std::to_string( g_max_data_stack_size ) + ") reached");
}

void Interpreter::ReportGlobalsStackOverflow()
{
	errors_.push_back( "Max globals stack size (" + std::to_string( g_max_globals_stack_size ) + ") reached" );
}

} // namespace U
