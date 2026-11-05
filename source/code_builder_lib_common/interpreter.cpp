#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/DebugInfoMetadata.h>
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

constexpr size_t g_globals_segment_offset= g_max_data_stack_size + 16u;
constexpr size_t g_max_globals_stack_size =1024u * 1024u * 64u; // 64 Megabytes will be enough for constants.

constexpr size_t g_heap_segment_offset= g_globals_segment_offset + g_max_globals_stack_size + 16u;
constexpr size_t g_max_heap_segment_size= 1024u * 1024u * 64u - 16u; // 64 Megabytes will be enough for heap.

}

Interpreter::Interpreter( const llvm::DataLayout& data_layout, InterpreterOptions options )
	: data_layout_(data_layout)
	, pointer_size_in_bits_( data_layout_.getPointerSizeInBits() )
	, options_( std::move(options) )
{}

Interpreter::ResultConstexpr Interpreter::EvaluateConstexpr(
	llvm::Function* const llvm_function,
	const llvm::ArrayRef<const llvm::Constant*> args,
	llvm::Type& return_type )
{
	instructions_executed_= 0;

	stack_.resize(16u); // reserve null pointer

	if( args.size() < llvm_function->arg_size() )
	{
		ResultConstexpr result;
		result.errors.push_back( "Too few arguments for function" );
		return result;
	}

	if( return_type.isPointerTy() )
	{
		ResultConstexpr result;
		result.errors.push_back( "Evaluation result of pointer type - it's not supported" );
		return result;
	}

	size_t s_ret_ptr= 0u;

	// Fill arguments
	size_t i= 0u;
	for( const llvm::Argument& param : llvm_function->args() )
	{
		if( param.getType()->isPointerTy() )
		{
			if( param.hasStructRetAttr() )
			{
				if( i != 0 )
				{
					ResultConstexpr result;
					result.errors.push_back( "\"sret\" attribute for non-first argument" );
					return result;
				}

				s_ret_ptr= stack_.size();
				const size_t new_stack_size= stack_.size() + size_t( data_layout_.getTypeAllocSize( &return_type ) );
				if( new_stack_size >= g_max_data_stack_size )
				{
					ReportDataStackOverflow();
					continue;
				}
				stack_.resize( new_stack_size );

				llvm::GenericValue val;
				val.IntVal= llvm::APInt( pointer_size_in_bits_, uint64_t(s_ret_ptr) );
				current_function_frame_.instructions_map[ &param ]= std::move(val);
			}
			else
			{
				// Assume this is reference param.
				llvm::GenericValue val;
				val.IntVal= llvm::APInt( pointer_size_in_bits_, uint64_t( MoveConstantToStack( *args[i] ) ) );
				current_function_frame_.instructions_map[ &param ]= std::move(val);
			}
		}
		else if( param.getType() == args[i]->getType() )
			current_function_frame_.instructions_map[ &param ]= GetVal( args[i] );
		else
		{
			// Assume we have a constant of type compatible with parameter type.
			// Convert it into expected type, using load-store, which is effectively equivalent to bitcast.
			const size_t offset= MoveConstantToStack( *args[i] );
			const MemoryRegionView memory_region= GetMemoryForVirtualAddress( offset );
			current_function_frame_.instructions_map[ &param ]= DoLoad( memory_region, param.getType() );
		}

		++i;
	}

	const llvm::GenericValue res= CallFunction( *llvm_function );

	ResultConstexpr result;
	result.errors= std::move(errors_);
	errors_= {};

	if( return_type.isIntegerTy() )
		result.result_constant= llvm::Constant::getIntegerValue( &return_type, res.IntVal );
	else if( return_type.isFloatTy() )
		result.result_constant= llvm::ConstantFP::get( &return_type, double(res.FloatVal) );
	else if( return_type.isDoubleTy() )
		result.result_constant= llvm::ConstantFP::get( &return_type, res.DoubleVal );
	else if( return_type.isVoidTy() )
		result.result_constant= llvm::UndefValue::get( &return_type ); // TODO - set correct value
	else if( return_type.isArrayTy() || return_type.isStructTy() )
	{
		if( s_ret_ptr != 0 )
			result.result_constant= ReadConstantFromStack( &return_type, s_ret_ptr );
		else
		{
			// It's an immediate value, which should be converted into a constant of given composite type.
			// Convert it into expected type, using load-store, which is effectively equivalent to bitcast.
			const size_t address= stack_.size();
			const size_t new_stack_size= stack_.size() + size_t( data_layout_.getTypeAllocSize( &return_type ) );
			if( new_stack_size >= g_max_data_stack_size )
			{
				ReportDataStackOverflow();
				return result;
			}
			stack_.resize( new_stack_size );

			DoStore( GetMemoryForVirtualAddress( address ), res, llvm_function->getReturnType() );
			result.result_constant= ReadConstantFromStack( &return_type, address );
		}
	}
	else if( return_type.isPointerTy() )
		ReportError( "returning pointer in constexpr function" );
	else
		ReportError( "returning unsupported type" );

	current_function_frame_.instructions_map.clear();
	stack_.clear();
	globals_stack_.clear();
	heap_.clear();
	coroutines_data_.clear();
	external_constant_mapping_.clear();
	function_pointers_map_.clear();
	function_pointers_mapping_vec_.clear();

	return result;
}

Interpreter::ResultGeneric Interpreter::EvaluateGeneric(
	llvm::Function* const llvm_function,
	const llvm::ArrayRef<llvm::GenericValue> args )
{
	instructions_executed_= 0;

	stack_.resize(16u); // reserve null pointer

	if( args.size() < llvm_function->arg_size() )
	{
		ResultGeneric result;
		result.errors.push_back( "Too few arguments for function" );
		return result;
	}

	// Fill arguments
	size_t i= 0u;
	for( const llvm::Argument& param : llvm_function->args() )
	{
		if( param.getType()->isPointerTy() )
		{
			ResultGeneric result;
			result.errors.push_back( "Evaluating a function with pointer-typed param" );
			return result;
		}

		current_function_frame_.instructions_map[ &param ]= args[i];

		++i;
	}

	ResultGeneric res;
	res.result= CallFunction( *llvm_function );

	res.errors= std::move(errors_);
	errors_= {};

	current_function_frame_.instructions_map.clear();
	stack_.clear();

	// Preserve globals, heap and external constants here.

	return res;
}

void Interpreter::RegisterCustomFunction( const llvm::StringRef name, const CustomFunction function )
{
	custom_functions_.insert_or_assign( name, function );
}

void Interpreter::ReadExecutinEngineData( void* const dst, const uint64_t address, const size_t size ) const
{
	const MemoryRegionView memory_region= const_cast<Interpreter*>(this)->GetMemoryForVirtualAddress( size_t(address ) );
	std::memcpy( dst, memory_region.ptr, std::min( size, memory_region.size ) );
}

llvm::GenericValue Interpreter::CallFunction( const llvm::Function& llvm_function )
{
	// Check for special marker, generated by the code builder for functions which building is not finished yet.
	if( llvm_function.hasMetadata( "__U_incomplete_function_marker" ) )
	{
		ReportError( "executing incomplete function \"" + std::string(llvm_function.getName()) + "\"" );
		return llvm::GenericValue();
	}
	if( llvm_function.empty() )
	{
		ReportError( "executing function \"" + std::string(llvm_function.getName()) + "\" with no body" );
		return llvm::GenericValue();
	}
	if( call_stack_.size() >= options_.max_call_stack_depth )
	{
		ReportError( "Max call stack depth (" + std::to_string( options_.max_call_stack_depth ) + ") reached" );
		return llvm::GenericValue();
	}
	if( llvm_function.empty() )
	{
		ReportError( "executing function \"" + std::string(llvm_function.getName()) + "\" with empty body" );
		return llvm::GenericValue();
	}

	const llvm::BasicBlock& bb= llvm_function.front();
	if( bb.empty() )
	{
		ReportError( "executing function \"" + std::string(llvm_function.getName()) + "\" with empty body" );
		return llvm::GenericValue();
	}

	return ExecuteFunctionBody( &*bb.begin() );
}

llvm::GenericValue Interpreter::ExecuteFunctionBody( const llvm::Instruction* instruction )
{
	const size_t prev_stack_size= stack_.size();

	const llvm::BasicBlock* prev_basic_block= nullptr;
	const llvm::BasicBlock* current_basic_block= instruction->getParent();

	if( current_basic_block == nullptr )
	{
		ReportError( "Current basic block is empty!" );
		return llvm::GenericValue();
	}

	while( errors_.empty() )
	{
		if( instruction == nullptr )
		{
			ReportError( "Reached null instruction!" );
			return llvm::GenericValue();
		}

		++instructions_executed_;
		if( instructions_executed_ >= options_.max_instructions_executed )
		{
			ReportError( "Interpreter instructions limit (" + std::to_string( instructions_executed_ ) + ") reached", *instruction );
			return llvm::GenericValue();
		}

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
			{
				const auto call_instruction= llvm::dyn_cast<llvm::CallInst>(instruction);
				call_stack_.push_back( call_instruction );
				ProcessCall( call_instruction );
				call_stack_.pop_back();

				// If there are some errors, abort execution.
				if( !errors_.empty() )
					return llvm::GenericValue();
			}
			break;

		case llvm::Instruction::Br:
			prev_basic_block= current_basic_block;
			if( instruction->getNumOperands() == 1u ) // Unconditional
			{
				current_basic_block= llvm::dyn_cast<llvm::BasicBlock>(instruction->getOperand(0u));

				if( current_basic_block == nullptr )
				{
					ReportError( "Incorrect basic block in \"br\" instruction!" );
					return llvm::GenericValue();
				}
			}
			else if( instruction->getNumOperands() == 3u ) // Conditional
			{
				const llvm::GenericValue val= GetVal(instruction->getOperand(0u));
				current_basic_block= llvm::dyn_cast<llvm::BasicBlock>(instruction->getOperand( val.IntVal.getBoolValue() ? 2u : 1u ));

				if( current_basic_block == nullptr )
				{
					ReportError( "Incorrect basic block in \"br\" instruction!" );
					return llvm::GenericValue();
				}
			}
			else
			{
				ReportError( "Incorrect number of operands in \"br\" instruction!" );
				return llvm::GenericValue();
			}

			instruction= current_basic_block->empty() ? nullptr : &current_basic_block->front();
			continue; // Continue loop without advancing instruction.

		case llvm::Instruction::Switch:
			{
				const uint32_t num_operands= instruction->getNumOperands();
				if( num_operands < 2 )
				{
					ReportError( "Incorrect number of operands in \"switch\" instruction!" );
					return llvm::GenericValue();
				}

				prev_basic_block= current_basic_block;
				const uint64_t index= GetVal(instruction->getOperand(0u)).IntVal.getLimitedValue();

				current_basic_block= llvm::dyn_cast<llvm::BasicBlock>(instruction->getOperand(1u));

				if( current_basic_block == nullptr )
				{
					ReportError( "Incorrect basic block in \"switch\" instruction!" );
					return llvm::GenericValue();
				}

				for( uint32_t i = 2; i < num_operands; i+= 2 )
				{
					if( GetVal( instruction->getOperand(i) ).IntVal.getLimitedValue() == index )
					{
						current_basic_block= llvm::dyn_cast<llvm::BasicBlock>( instruction->getOperand( i + 1 ) );

						if( current_basic_block == nullptr )
						{
							ReportError( "Incorrect basic block in \"switch\" instruction!" );
							return llvm::GenericValue();
						}

						break;
					}
				}
				instruction= current_basic_block->empty() ? nullptr : &current_basic_block->front();
			}
			continue; // Continue loop without advancing instruction.

		case llvm::Instruction::PHI:
			{
				const auto phi_node= llvm::dyn_cast<llvm::PHINode>(instruction);

				for (uint32_t i= 0u; i < phi_node->getNumIncomingValues(); ++i )
				{
					if( phi_node->getIncomingBlock(i) == prev_basic_block )
					{
						current_function_frame_.instructions_map[instruction]= GetVal( phi_node->getIncomingValue(i) );
						break;
					}
					if( i + 1u == phi_node->getNumIncomingValues() )
					{
						ReportError( "Previous basic block isn't found in \"phi\" instruction!" );
						return llvm::GenericValue();
					}
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
				if( instruction->getNumOperands() != 3 )
				{
					ReportError( "Incorrect number of operands in \"select\" instruction!" );
					return llvm::GenericValue();
				}

				const llvm::GenericValue bool_val= GetVal(instruction->getOperand(0u));
				current_function_frame_.instructions_map[instruction]= GetVal( instruction->getOperand( bool_val.IntVal.getBoolValue() ? 1u : 2u ) );
			}
			break;

		case llvm::Instruction::AtomicCmpXchg:
			{
				if( instruction->getNumOperands() != 3 )
				{
					ReportError( "Incorrect number of operands in \"atomiccmpxchg\" instruction!" );
					return llvm::GenericValue();
				}

				const llvm::GenericValue op0= GetVal(instruction->getOperand(0u));
				const llvm::GenericValue op1= GetVal(instruction->getOperand(1u));
				const llvm::GenericValue op2= GetVal(instruction->getOperand(2u));
				llvm::Type* const read_type= instruction->getOperand(1u)->getType();

				const MemoryRegionView memory_region= GetMemoryForVirtualAddress( size_t(op0.IntVal.getLimitedValue()) );
				const llvm::GenericValue load_result= DoLoad( memory_region, read_type );

				bool success= false;
				if( load_result.IntVal == op1.IntVal )
				{
					DoStore( memory_region, op2, read_type );
					success= true;
				}

				llvm::GenericValue success_val;
				success_val.IntVal= llvm::APInt( 1, success ? 1 : 0 );

				llvm::GenericValue val;
				val.AggregateVal= { load_result, success_val };
				current_function_frame_.instructions_map[ instruction ]= val;
			}
			break;

		case llvm::Instruction::Fence:
			// Interpreter is single-threaded, memory fences aren't necessary.
			break;

		case llvm::Instruction::Unreachable:
			ReportError( "executing Unreachable instruction", *instruction );
			return llvm::GenericValue();

		default:
			if( instruction->getNumOperands() == 1u )
				ProcessUnaryArithmeticInstruction(instruction);
			else if( instruction->getNumOperands() == 2u )
				ProcessBinaryArithmeticInstruction(instruction);
			else
			{
				ReportError( std::string( "executing unknown instruction \"" ) + instruction->getOpcodeName() + "\"", *instruction );
				return llvm::GenericValue();
			}
		};

		// If this is not a terminal instruction, just advance to next instruction in block.
		instruction= instruction->getNextNode();
	}

	// Return a dummy in case of errors.
	return llvm::GenericValue();
}

size_t Interpreter::MoveConstantToStack( const llvm::Constant& constant )
{
	const auto prev_it= external_constant_mapping_.find( &constant );
	if( prev_it != external_constant_mapping_.end() )
		return prev_it->second;

	llvm::Type* const constant_type= constant.getType();

	if( !constant_type->isSized() )
		return 0u; // Constant has incomplete type.

	// Use separate stack for constants, because we can push constants to it in any time.

	const size_t stack_offset= globals_stack_.size();
	const size_t new_stack_size= globals_stack_.size() + size_t( data_layout_.getTypeAllocSize( constant_type ) );
	if( new_stack_size >= g_max_globals_stack_size )
	{
		ReportGlobalsStackOverflow();
		return 0u;
	}
	globals_stack_.resize( new_stack_size );

	external_constant_mapping_[&constant]= stack_offset + g_globals_segment_offset;

	CopyConstantToStack( constant, stack_offset );

	return stack_offset + g_globals_segment_offset;
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
					else
						ReportError( "storing unsupported constant expression" );
				}
				else if( const auto global_variable= llvm::dyn_cast<llvm::GlobalVariable>(element) )
					element_ptr= MoveConstantToStack( *global_variable->getInitializer() );
				else if( element->isNullValue() )
					element_ptr= 0;
				else if( const auto function= llvm::dyn_cast<llvm::Function>(element) )
					element_ptr= GetFunctionIndex( function );
				else
					ReportError( "storing constant pointer of unsupported kind" );

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
		else
			ReportError( "stroring integer constant of unsupported size" );
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
			const uint64_t val= GetFunctionIndex( function );
			std::memcpy( globals_stack_.data() + stack_offset, &val, pointer_size_in_bits_ / 8 );
		}
		else
			std::memset( globals_stack_.data() + stack_offset, 0, size_t( data_layout_.getTypeAllocSize( constant_type ) ) );
	}
	else
		ReportError( "storing constant of unsupported type" );
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
		else
		{
			ReportError( "loading integer constant of unsupported size" );
			return llvm::UndefValue::get( type );
		}
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

		llvm::SmallVector<llvm::Constant*, 32> initializers( size_t(array_type->getNumElements()), nullptr );
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
		ReportError( "loading constant pointer" );
		return llvm::UndefValue::get( type );
	}
	else
	{
		ReportError( "loading constant of unsupported type" );
		return llvm::UndefValue::get( type );
	}
}

llvm::GenericValue Interpreter::BuildGEP( const llvm::User* const instruction )
{
	if( instruction->getNumOperands() < 2u )
	{
		ReportError( "Invalid \"getelementptr\" instruction - too few operands!" );
		return llvm::GenericValue();
	}

	llvm::User::const_op_iterator op= instruction->op_begin();
	const llvm::User::const_op_iterator op_end= instruction->op_end();

	const llvm::GenericValue ptr= GetVal( op->get() );
	++op;

	const llvm::GenericValue first_index= GetVal( op->get() );
	++op;

	// LLVM uses some magic with "GEPOperator", so that casting both "GetElementPtrInst" and "ConstantExpr" work fine.
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
		else
		{
			ReportError( "Unsupported type kind in \"getelementptr\" instruction!" );
			return llvm::GenericValue();
		}
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
		else
			ReportError( "Accessing floating-point constant of unsupported type" );
	}
	else if( const auto constant_int= llvm::dyn_cast<llvm::ConstantInt>(val) )
		res.IntVal= constant_int->getValue();
	else if( llvm::dyn_cast<llvm::ConstantPointerNull>(val) != nullptr )
		res.IntVal= llvm::APInt( pointer_size_in_bits_, uint64_t(0) );
	else if( const auto global_variable= llvm::dyn_cast<llvm::GlobalVariable>( val ) )
		res.IntVal= llvm::APInt( pointer_size_in_bits_, MoveConstantToStack( *global_variable->getInitializer() ) );
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
		else
			ReportError( "Accessing unsupported undef value" );
	}
	else if( const auto function= llvm::dyn_cast<llvm::Function>(val) )
		res.IntVal= llvm::APInt( pointer_size_in_bits_, GetFunctionIndex( function ) );
	else if( const auto constant_expression= llvm::dyn_cast<llvm::ConstantExpr>( val ) )
	{
		if( constant_expression->getOpcode() == llvm::Instruction::GetElementPtr )
			res= BuildGEP( constant_expression );
		else
			ReportError( "Accessing unsupported constant expression" );
	}
	else
	{
		const auto it= current_function_frame_.instructions_map.find( val );
		if( it != current_function_frame_.instructions_map.end() )
			res= it->second;
		else
			ReportError( "Accessing unknown value not present in instructions map" );
	}
	return res;
}

void Interpreter::ProcessAlloca( const llvm::Instruction* const instruction )
{
	const auto alloca_instruction= llvm::dyn_cast<llvm::AllocaInst>(instruction);
	llvm::Type* const element_type= alloca_instruction->getAllocatedType();

	const size_t element_size= size_t(data_layout_.getTypeAllocSize( element_type ));

	size_t address= 0u;
	if( current_function_frame_.is_coroutine )
	{
		// Allocate from heap if this is a coroutine.
		// This is needed, because we need stable addresses during coroutine resume.
		address= heap_.size() + g_heap_segment_offset;
		const size_t new_heap_size= heap_.size() + element_size;
		if( new_heap_size >= g_max_heap_segment_size )
		{
			ReportError( "Max heap size (" + std::to_string( g_max_heap_segment_size ) + ") reached", *instruction );
			return;
		}
		heap_.resize( new_heap_size );
	}
	else
	{
		address= stack_.size();
		const size_t new_stack_size= stack_.size() + element_size;
		if( new_stack_size >= g_max_data_stack_size )
		{
			ReportDataStackOverflow();
			return;
		}
		stack_.resize( new_stack_size );
	}

	llvm::GenericValue val;
	val.IntVal= llvm::APInt( pointer_size_in_bits_, uint64_t(address) );
	current_function_frame_.instructions_map[ instruction ]= val;
}

Interpreter::MemoryRegionView Interpreter::GetMemoryForVirtualAddress( const size_t offset )
{
	// Map virtual memory addresses to one of existing segments.
	// TODO - produce error if address is wrong.
	if( offset >= g_heap_segment_offset )
	{
		const size_t heap_offset= offset - g_heap_segment_offset;
		if( heap_offset > heap_.size() )
			return MemoryRegionView{ nullptr, 0 };

		return MemoryRegionView{ heap_.data() + heap_offset, heap_.size() - heap_offset };
	}
	if( offset >= g_globals_segment_offset )
	{
		const size_t globals_offset= offset - g_globals_segment_offset;
		if( globals_offset > globals_stack_.size() )
			return MemoryRegionView{ nullptr, 0 };

		return MemoryRegionView{ globals_stack_.data() + globals_offset, globals_stack_.size() - globals_offset };
	}

	if( offset > stack_.size() )
		return MemoryRegionView{ nullptr, 0 };

	return MemoryRegionView{ stack_.data() + offset, stack_.size() - offset };
}

void Interpreter::ProcessLoad( const llvm::Instruction* const instruction )
{
	const llvm::GenericValue address_val= GetVal( instruction->getOperand(0u) );

	const size_t offset= size_t(address_val.IntVal.getLimitedValue());
	const MemoryRegionView memory_region= GetMemoryForVirtualAddress( offset );
	current_function_frame_.instructions_map[ instruction ]= DoLoad( memory_region, instruction->getType() );
}

llvm::GenericValue Interpreter::DoLoad( const MemoryRegionView memory_region, llvm::Type* const t )
{
	llvm::GenericValue val;
	if( t->isIntegerTy() )
	{
		uint64_t buff[4];
		const size_t load_size= size_t(data_layout_.getTypeStoreSize( t ));
		if( load_size > memory_region.size )
		{
			ReportError( "Out of bounds load!" );
			return val;
		}

		std::memcpy( buff, memory_region.ptr, load_size );
		val.IntVal= llvm::APInt( t->getIntegerBitWidth() , buff );
	}
	else if( t->isFloatTy() )
	{
		if( sizeof( val.FloatVal ) > memory_region.size )
		{
			ReportError( "Out of bounds load!" );
			return val;
		}

		std::memcpy( &val.FloatVal, memory_region.ptr, sizeof(float) );
	}
	else if( t->isDoubleTy() )
	{
		if( sizeof( val.DoubleVal ) > memory_region.size )
		{
			ReportError( "Out of bounds load!" );
			return val;
		}

		std::memcpy( &val.DoubleVal, memory_region.ptr, sizeof(double) );
	}
	else if( t->isPointerTy() )
	{
		uint64_t int_ptr;

		const size_t load_size= size_t(data_layout_.getTypeAllocSize( t ) );
		if( load_size > memory_region.size )
		{
			ReportError( "Out of bounds load!" );
			return val;
		}

		std::memcpy( &int_ptr, memory_region.ptr, load_size );
		val.IntVal= llvm::APInt( pointer_size_in_bits_, int_ptr );
	}
	else if( const auto struct_type= llvm::dyn_cast<llvm::StructType>(t) )
	{
		const uint32_t num_elements= struct_type->getNumElements();
		const llvm::StructLayout *const struct_layout= data_layout_.getStructLayout(struct_type);

		val.AggregateVal.resize(num_elements);
		for (uint32_t i= 0; i < num_elements; ++i)
		{
			const size_t offset= size_t( struct_layout->getElementOffset(i) );
			if( offset > memory_region.size )
			{
				ReportError( "Out of bounds load!" );
				return val;
			}

			val.AggregateVal[i]=
				DoLoad(
					MemoryRegionView{ memory_region.ptr + offset, memory_region.size - offset },
					struct_type->getElementType(i));
		}
	}
	else if( const auto array_type= llvm::cast<llvm::ArrayType>(t) )
	{
		const uint64_t num_elements= array_type->getNumElements();
		const auto element_type= array_type->getElementType();
		const uint64_t element_size= data_layout_.getTypeAllocSize(element_type);

		val.AggregateVal.resize(size_t(num_elements));
		for( uint64_t i= 0; i < num_elements; ++i)
		{
			const size_t offset= size_t(i * element_size);
			if( offset > memory_region.size )
			{
				ReportError( "Out of bounds load!" );
				return val;
			}

			val.AggregateVal[size_t(i)]=
				DoLoad(
					MemoryRegionView{ memory_region.ptr + offset, memory_region.size - offset },
					element_type );
		}
	}
	else
		ReportError( "Loading value of unsupported type!" );

	return val;
}

void Interpreter::ProcessStore( const llvm::Instruction* const instruction )
{
	const llvm::GenericValue address_val= GetVal( instruction->getOperand(1u) );

	const size_t offset= size_t(address_val.IntVal.getLimitedValue());
	const MemoryRegionView memory_region= GetMemoryForVirtualAddress( offset );

	const auto value_operand= instruction->getOperand(0u);
	DoStore( memory_region, GetVal( value_operand ), value_operand->getType() );
}

void Interpreter::DoStore( const MemoryRegionView memory_region, const llvm::GenericValue& val, llvm::Type* const t )
{
	if( t->isIntegerTy() )
	{
		const uint32_t integer_bit_width= t->getIntegerBitWidth();

		if( integer_bit_width / 8u > memory_region.size )
		{
			ReportError( "Out of bounds store!" );
			return;
		}

		if( integer_bit_width <= 64 )
		{
			const uint64_t limited_value= val.IntVal.getLimitedValue();
			std::memcpy( memory_region.ptr, &limited_value, size_t(data_layout_.getTypeStoreSize( t )) );
		}
		else if( integer_bit_width % 64u == 0 )
			std::memcpy( memory_region.ptr, val.IntVal.getRawData(), integer_bit_width / 8u );
		else
		{
			ReportError( "Storing an integer of unsupported size!" );
			return;
		}
	}
	else if( t->isFloatTy() )
	{
		if( sizeof( val.FloatVal ) > memory_region.size )
		{
			ReportError( "Out of bounds store!" );
			return;
		}

		std::memcpy( memory_region.ptr, &val.FloatVal, sizeof( val.FloatVal ) );
	}
	else if( t->isDoubleTy() )
	{
		if( sizeof( val.DoubleVal ) > memory_region.size )
		{
			ReportError( "Out of bounds store!" );
			return;
		}

		std::memcpy( memory_region.ptr, &val.DoubleVal, sizeof( val.DoubleVal ) );
	}
	else if( t->isPointerTy() )
	{
		const size_t store_size= size_t(data_layout_.getTypeAllocSize( t ));
		if( store_size > memory_region.size )
		{
			ReportError( "Out of bounds store!" );
			return;
		}

		const uint64_t int_ptr= val.IntVal.getLimitedValue();
		std::memcpy( memory_region.ptr, &int_ptr, store_size );
	}
	else if( const auto struct_type= llvm::dyn_cast<llvm::StructType>(t) )
	{
		const uint32_t num_elements= struct_type->getNumElements();
		const llvm::StructLayout *const struct_layout= data_layout_.getStructLayout(struct_type);

		if( val.AggregateVal.size() != num_elements )
		{
			ReportError( "Storing aggregate value with unexpected size!" );
			return;
		}

		for (uint32_t i= 0; i < num_elements; ++i)
		{
			const size_t offset= size_t( struct_layout->getElementOffset(i) );
			if( offset > memory_region.size )
			{
				ReportError( "Out of bounds store!" );
				return;
			}

			DoStore(
				MemoryRegionView{ memory_region.ptr + offset, memory_region.size - offset },
				val.AggregateVal[i],
				struct_type->getElementType(i));
		}

	}
	else if( const auto array_type= llvm::cast<llvm::ArrayType>(t) )
	{
		const uint64_t num_elements= array_type->getNumElements();
		const auto element_type= array_type->getElementType();
		const uint64_t element_size= data_layout_.getTypeAllocSize(element_type);

		if( val.AggregateVal.size() != num_elements )
		{
			ReportError( "Storing aggregate value with unexpected size!" );
			return;
		}

		for( uint64_t i= 0; i < num_elements; ++i)
		{
			const size_t offset= size_t(i * element_size);
			if( offset > memory_region.size )
			{
				ReportError( "Out of bounds store!" );
				return;
			}

			DoStore(
				MemoryRegionView{ memory_region.ptr + offset, memory_region.size - offset },
				val.AggregateVal[size_t(i)],
				element_type );
		}
	}
	else
		ReportError( "Loading value of unsupported type!" );
}

void Interpreter::ProcessGEP( const llvm::Instruction* const instruction )
{
	current_function_frame_.instructions_map[instruction]= BuildGEP( instruction );
}

void Interpreter::ProcessCall( const llvm::CallInst* const instruction )
{
	const llvm::Value* const calle= instruction->getCalledOperand();

	const llvm::Function* function= llvm::dyn_cast<llvm::Function>(calle);
	if( function == nullptr )
	{
		const size_t index= size_t( GetVal( calle ).IntVal.getLimitedValue() );
		if( index >= function_pointers_mapping_vec_.size() )
		{
			ReportError( "Calling unknown function pointer", *instruction );
			return;
		}

		function= function_pointers_mapping_vec_[ index ];
	}

	if( function == nullptr )
	{
		ReportError( "Calling zero function pointer", *instruction );
		return;
	}


	if( function->arg_size() > instruction->arg_size() )
	{
		ReportError( "Not enough arguments are provided for function call", *instruction );
		return;
	}

	// Allow providing more than needed arguments. It's necessary for variadic functions and some other tricky cases.

	const llvm::StringRef function_name= function->getName();

	if( function->isIntrinsic() )
	{
		switch( function->getIntrinsicID() )
		{
		case llvm::Intrinsic::dbg_declare:
		case llvm::Intrinsic::lifetime_start:
		case llvm::Intrinsic::lifetime_end:
			return;
		case llvm::Intrinsic::memcpy:
		case llvm::Intrinsic::memmove:
			ProcessMemmove( instruction );
			return;
		case llvm::Intrinsic::coro_id:
			ProcessCoroId( instruction );
			return;
		case llvm::Intrinsic::coro_alloc:
			ProcessCoroAlloc( instruction );
			return;
		case llvm::Intrinsic::coro_free:
			ProcessCoroFree( instruction );
			return;
		case llvm::Intrinsic::coro_size:
			ProcessCoroSize( instruction );
			return;
		case llvm::Intrinsic::coro_begin:
			ProcessCoroBegin( instruction );
			return;
		case llvm::Intrinsic::coro_end:
			ProcessCoroEnd( instruction );
			return;
		case llvm::Intrinsic::coro_suspend:
			ProcessCoroSuspend( instruction );
			return;
		case llvm::Intrinsic::coro_resume:
			ProcessCoroResume( instruction );
			return;
		case llvm::Intrinsic::coro_destroy:
			ProcessCoroDestroy( instruction );
			return;
		case llvm::Intrinsic::coro_done:
			ProcessCoroDone( instruction );
			return;
		case llvm::Intrinsic::coro_promise:
			ProcessCoroPromise( instruction );
			return;
		case llvm::Intrinsic::sadd_with_overflow:
			ProcessSAddWithOverflow( instruction );
			return;
		case llvm::Intrinsic::uadd_with_overflow:
			ProcessUAddWithOverflow( instruction );
			return;
		case llvm::Intrinsic::ssub_with_overflow:
			ProcessSSubWithOverflow( instruction );
			return;
		case llvm::Intrinsic::usub_with_overflow:
			ProcessUSubWithOverflow( instruction );
			return;
		case llvm::Intrinsic::smul_with_overflow:
			ProcessSMulWithOverflow( instruction );
			return;
		case llvm::Intrinsic::umul_with_overflow:
			ProcessUMulWithOverflow( instruction );
			return;
		case llvm::Intrinsic::stacksave:
			ProcessStacksave( instruction );
			return;
		case llvm::Intrinsic::stackrestore:
			ProcessStackrestore( instruction );
			return;
		case llvm::Intrinsic::threadlocal_address:
			ProcessThreadLocalAddress( instruction );
			return;
		default:
			break;
		};
	}
	else if( function_name == "malloc" || function_name == "ust_memory_allocate_impl" || function_name == "__U_ust_memory_allocate_impl" )
	{
		ProcessMalloc( instruction );
		return;
	}
	else if( function_name == "realloc" || function_name == "ust_memory_reallocate_impl" )
	{
		ProcessRealloc( instruction );
		return;
	}
	else if( function_name == "free" || function_name == "ust_memory_free_impl" || function_name == "__U_ust_memory_free_impl" )
	{
		ProcessFree( instruction );
		return;
	}
	else if( const auto func_it= custom_functions_.find( function_name ); func_it != custom_functions_.end() )
	{
		const CustomFunction func= func_it->second;
		llvm::SmallVector<llvm::GenericValue, 8> args;
		args.reserve( function->arg_size() );
		for( size_t i= 0; i < function->arg_size(); ++i )
			args.push_back( GetVal( instruction->getOperand(uint32_t(i)) ) );

		current_function_frame_.instructions_map[instruction]= func( function->getFunctionType(), args );
		return;
	}

	CallFrame call_frame;
	call_frame.is_coroutine= function->hasFnAttribute( llvm::Attribute::PresplitCoroutine );

	const size_t prev_stack_size= stack_.size();

	uint32_t i= 0u;
	for( const llvm::Argument& arg : function->args() )
	{
		llvm::GenericValue val= GetVal( instruction->getOperand(i) );

		if( arg.hasByValAttr() )
		{
			// Create copy of the memory block representing by a "byval" pointer argument.
			// It's necessary in order to avoid changes for an argument made in calle to be visible in caller.

			const size_t object_size= size_t( data_layout_.getTypeAllocSize( arg.getParamByValType() ) );

			const size_t new_address_value= stack_.size();
			const size_t new_stack_size= stack_.size() + object_size;
			if( new_stack_size >= g_max_data_stack_size )
			{
				ReportDataStackOverflow();
				return;
			}
			stack_.resize( new_stack_size );

			std::memcpy(
				stack_.data() + new_address_value,
				stack_.data() + size_t( val.IntVal.getLimitedValue() ),
				object_size );

			val.IntVal= llvm::APInt( pointer_size_in_bits_, uint64_t(new_address_value) );
		}

		call_frame.instructions_map[ &arg ]= std::move(val);

		++i;
	}

	std::swap( call_frame, current_function_frame_ );
	llvm::GenericValue result_val= CallFunction( *function );
	std::swap( call_frame, current_function_frame_ );

	if( !function->getReturnType()->isVoidTy() )
		current_function_frame_.instructions_map[instruction]= result_val;

	stack_.resize( prev_stack_size ); // Drop temporary byval arguments.
}

void Interpreter::ProcessMemmove( const llvm::Instruction* const instruction )
{
	if( instruction->getNumOperands() < 3 )
	{
		ReportError( "Incorrect number of operands for \"memcpy\"/\"memmove\"!" );
		return;
	}

	const size_t dst_offset= size_t( GetVal( instruction->getOperand(0u) ).IntVal.getLimitedValue() );
	const size_t src_offset= size_t( GetVal( instruction->getOperand(1u) ).IntVal.getLimitedValue() );
	const size_t size= size_t( GetVal( instruction->getOperand(2u) ).IntVal.getLimitedValue() );

	const MemoryRegionView dst_memory_region= GetMemoryForVirtualAddress( dst_offset );
	const MemoryRegionView src_memory_region= GetMemoryForVirtualAddress( src_offset );

	if( size > std::min( dst_memory_region.size, src_memory_region.size ) )
	{
		ReportError( "Out of bounds memmove!" );
		return;
	}

	std::memmove( dst_memory_region.ptr, src_memory_region.ptr, size );
}

constexpr size_t g_malloc_header_size= sizeof(size_t);

void Interpreter::ProcessMalloc( const llvm::CallInst* const instruction )
{
	if( instruction->getNumOperands() < 1 )
	{
		ReportError( "Incorrect number of operands for \"malloc\"!" );
		return;
	}

	const size_t size= size_t( GetVal( instruction->getOperand(0u) ).IntVal.getLimitedValue() );

	const size_t offset= heap_.size();

	const size_t new_size= offset + size + g_malloc_header_size;
	if( new_size >= g_max_heap_segment_size )
	{
		ReportError( "Max heap size (" + std::to_string( g_max_heap_segment_size ) + ") reached", *instruction );
		return;
	}
	heap_.resize( new_size );

	// Save size of allocated block.
	std::memcpy( heap_.data() + offset, &size, g_malloc_header_size );

	llvm::GenericValue val;
	val.IntVal= llvm::APInt( pointer_size_in_bits_, uint64_t(offset + g_malloc_header_size + g_heap_segment_offset) );
	current_function_frame_.instructions_map[ instruction ]= val;
}

void Interpreter::ProcessRealloc( const llvm::CallInst* const instruction )
{
	if( instruction->getNumOperands() < 2 )
	{
		ReportError( "Incorrect number of operands for \"realloc\"!" );
		return;
	}

	// Just allocate new memory block and copy contents of old one into new.

	const llvm::GenericValue op0= GetVal( instruction->getOperand(0u) );
	const llvm::GenericValue op1= GetVal( instruction->getOperand(1u) );

	const size_t prev_offset= size_t( op0.IntVal.getLimitedValue() ) - g_heap_segment_offset;

	size_t prev_size= 0;
	std::memcpy( &prev_size, heap_.data() + ( prev_offset - g_malloc_header_size ), g_malloc_header_size );

	const size_t size= size_t( op1.IntVal.getLimitedValue() );

	if( size <= prev_size )
	{
		// If resizing down - just reuse previous block.
		current_function_frame_.instructions_map[ instruction ]= op0;
		return;
	}

	const size_t offset= heap_.size();

	const size_t new_size= offset + size + g_malloc_header_size;
	if( new_size >= g_max_heap_segment_size )
	{
		ReportError( "Max heap size (" + std::to_string( g_max_heap_segment_size ) + ") reached", *instruction );
		return;
	}
	heap_.resize( new_size );

	// Save size of newly allocated block.
	std::memcpy( heap_.data() + offset, &size, g_malloc_header_size );

	// Copy data from previous block.
	std::memcpy(
		heap_.data() + ( offset + g_malloc_header_size ),
		heap_.data() + prev_offset,
		prev_size );

	llvm::GenericValue val;
	val.IntVal= llvm::APInt( pointer_size_in_bits_, uint64_t(offset + g_malloc_header_size + g_heap_segment_offset) );
	current_function_frame_.instructions_map[ instruction ]= val;
}

void Interpreter::ProcessFree( const llvm::CallInst* const instruction )
{
	(void) instruction;
	// Now we manage heap as stack and can't properly free memory from it.
}

void Interpreter::ProcessCoroId( const llvm::CallInst* const instruction )
{
	if( instruction->getNumOperands() < 4 )
	{
		ReportError( "Incorrect number of operands for \"coro.id\"!" );
		return;
	}

	const llvm::GenericValue alignment= GetVal( instruction->getOperand(0u) );
	const llvm::GenericValue promise= GetVal( instruction->getOperand(1u) );
	const llvm::GenericValue coroaddr= GetVal( instruction->getOperand(2u) );
	const llvm::GenericValue fnaddrs= GetVal( instruction->getOperand(3u) );

	(void)alignment;
	(void)coroaddr;
	(void)fnaddrs;

	CoroutineData coroutine_data;
	coroutine_data.promise= promise;

	const auto coroutine_id= next_coroutine_id_;
	++next_coroutine_id_;
	coroutines_data_[coroutine_id]= std::move(coroutine_data);

	current_function_frame_.coroutine_data= &coroutines_data_[coroutine_id];

	llvm::GenericValue val;
	val.IntVal= llvm::APInt( pointer_size_in_bits_, uint64_t( coroutine_id ) );
	current_function_frame_.instructions_map[ instruction ]= val;
}

void Interpreter::ProcessCoroAlloc( const llvm::CallInst* const instruction )
{
	// Do not allocate memory for coroutine - return false.
	llvm::GenericValue val;
	val.IntVal= llvm::APInt( 1u, uint64_t(0u) );
	current_function_frame_.instructions_map[ instruction ]= val;
}

void Interpreter::ProcessCoroFree( const llvm::CallInst* const instruction )
{
	if( instruction->getNumOperands() < 1 )
	{
		ReportError( "Incorrect number of operands for \"coro.free\"!" );
		return;
	}

	const llvm::GenericValue token= GetVal( instruction->getOperand(0u) );
	const uint32_t coroutine_id= uint32_t(token.IntVal.getLimitedValue());
	coroutines_data_.erase( coroutine_id );

	// Return "false" in order to signal, that there is no need to call "free".
	llvm::GenericValue val;
	val.IntVal= llvm::APInt( pointer_size_in_bits_, uint64_t(0u) );
	current_function_frame_.instructions_map[ instruction ]= val;
}

void Interpreter::ProcessCoroSize( const llvm::CallInst* const instruction )
{
	// Do not allocate memory for coroutine - return zero size.
	llvm::GenericValue val;
	val.IntVal= llvm::APInt( pointer_size_in_bits_, uint64_t(0) );
	current_function_frame_.instructions_map[ instruction ]= val;
}

void Interpreter::ProcessCoroBegin( const llvm::CallInst* const instruction )
{
	if( instruction->getNumOperands() < 2 )
	{
		ReportError( "Incorrect number of operands for \"coro.begin\"!" );
		return;
	}

	const llvm::GenericValue token= GetVal( instruction->getOperand(0u) );
	const llvm::GenericValue memory= GetVal( instruction->getOperand(1u) );

	(void)memory;

	// Reuse token also as coroutine handle.
	current_function_frame_.instructions_map[ instruction ]= token;
}

void Interpreter::ProcessCoroEnd( const llvm::CallInst* const instruction )
{
	// Do nothing here
	(void) instruction;
}

void Interpreter::ProcessCoroSuspend( const llvm::CallInst* const instruction )
{
	if( instruction->getNumOperands() < 2 )
	{
		ReportError( "Incorrect number of operands for \"coro.suspend\"!" );
		return;
	}

	U_ASSERT( current_function_frame_.coroutine_data != nullptr );

	// const llvm::GenericValue token_save= GetVal( instruction->getOperand(0u) );
	const llvm::GenericValue is_final= GetVal( instruction->getOperand(1u) );

	current_function_frame_.coroutine_data->instructions_map= current_function_frame_.instructions_map;
	current_function_frame_.coroutine_data->suspend_instruction= instruction;
	current_function_frame_.coroutine_data->done= is_final.IntVal.isAllOnes();

	llvm::GenericValue val;
	val.IntVal= llvm::APInt( 8u, uint64_t(255) ); // return -1 as result for suspension.
	current_function_frame_.instructions_map[ instruction ]= val;
}

void Interpreter::ProcessCoroResume( const llvm::CallInst* const instruction )
{
	ResumeCoroutine( instruction, false );
}

void Interpreter::ProcessCoroDestroy( const llvm::CallInst* const instruction )
{
	ResumeCoroutine( instruction, true );
}

void Interpreter::ProcessCoroDone( const llvm::CallInst* const instruction )
{
	if( instruction->getNumOperands() < 1 )
	{
		ReportError( "Incorrect number of operands for \"coro.done\"!" );
		return;
	}

	const llvm::GenericValue handle= GetVal( instruction->getOperand(0u) );
	const uint32_t coroutine_id= uint32_t(handle.IntVal.getLimitedValue());
	const CoroutineData& coroutine_data= coroutines_data_[coroutine_id];

	llvm::GenericValue val;
	val.IntVal= llvm::APInt( 1u, coroutine_data.done ? uint64_t(1) : uint64_t(0) );
	current_function_frame_.instructions_map[ instruction ]= val;
}

void Interpreter::ProcessCoroPromise( const llvm::CallInst* const instruction )
{
	if( instruction->getNumOperands() < 1 )
	{
		ReportError( "Incorrect number of operands for \"coro.promise\"!" );
		return;
	}

	const llvm::GenericValue handle= GetVal( instruction->getOperand(0u) );
	const uint32_t coroutine_id= uint32_t(handle.IntVal.getLimitedValue());
	const CoroutineData& coroutine_data= coroutines_data_[coroutine_id];

	current_function_frame_.instructions_map[ instruction ]= coroutine_data.promise;
}

void Interpreter::ProcessSAddWithOverflow( const llvm::CallInst* const instruction )
{
	if( instruction->getNumOperands() < 2 )
	{
		ReportError( "Incorrect number of operands for \"sadd.with.overflow\"!" );
		return;
	}

	const llvm::APInt a= GetVal( instruction->getOperand(0u) ).IntVal;
	const llvm::APInt b= GetVal( instruction->getOperand(1u) ).IntVal;

	bool overflow= false;
	const llvm::APInt result= a.sadd_ov( b, overflow );

	llvm::GenericValue result_val;
	result_val.IntVal= result;

	llvm::GenericValue overflow_val;
	overflow_val.IntVal= llvm::APInt( 1, overflow ? 1 : 0 );

	llvm::GenericValue val;
	val.AggregateVal= { result_val, overflow_val };
	current_function_frame_.instructions_map[ instruction ]= val;
}

void Interpreter::ProcessUAddWithOverflow( const llvm::CallInst* const instruction )
{
	if( instruction->getNumOperands() < 2 )
	{
		ReportError( "Incorrect number of operands for \"uadd.with.overflow\"!" );
		return;
	}

	const llvm::APInt a= GetVal( instruction->getOperand(0u) ).IntVal;
	const llvm::APInt b= GetVal( instruction->getOperand(1u) ).IntVal;

	bool overflow= false;
	const llvm::APInt result= a.uadd_ov( b, overflow );

	llvm::GenericValue result_val;
	result_val.IntVal= result;

	llvm::GenericValue overflow_val;
	overflow_val.IntVal= llvm::APInt( 1, overflow ? 1 : 0 );

	llvm::GenericValue val;
	val.AggregateVal= { result_val, overflow_val };
	current_function_frame_.instructions_map[ instruction ]= val;
}

void Interpreter::ProcessSSubWithOverflow( const llvm::CallInst* const instruction )
{
	if( instruction->getNumOperands() < 2 )
	{
		ReportError( "Incorrect number of operands for \"ssub.with.overflow\"!" );
		return;
	}

	const llvm::APInt a= GetVal( instruction->getOperand(0u) ).IntVal;
	const llvm::APInt b= GetVal( instruction->getOperand(1u) ).IntVal;

	bool overflow= false;
	const llvm::APInt result= a.ssub_ov( b, overflow );

	llvm::GenericValue result_val;
	result_val.IntVal= result;

	llvm::GenericValue overflow_val;
	overflow_val.IntVal= llvm::APInt( 1, overflow ? 1 : 0 );

	llvm::GenericValue val;
	val.AggregateVal= { result_val, overflow_val };
	current_function_frame_.instructions_map[ instruction ]= val;
}

void Interpreter::ProcessUSubWithOverflow( const llvm::CallInst* const instruction )
{
	if( instruction->getNumOperands() < 2 )
	{
		ReportError( "Incorrect number of operands for \"usub.with.overflow\"!" );
		return;
	}
	const llvm::APInt a= GetVal( instruction->getOperand(0u) ).IntVal;
	const llvm::APInt b= GetVal( instruction->getOperand(1u) ).IntVal;

	bool overflow= false;
	const llvm::APInt result= a.usub_ov( b, overflow );

	llvm::GenericValue result_val;
	result_val.IntVal= result;

	llvm::GenericValue overflow_val;
	overflow_val.IntVal= llvm::APInt( 1, overflow ? 1 : 0 );

	llvm::GenericValue val;
	val.AggregateVal= { result_val, overflow_val };
	current_function_frame_.instructions_map[ instruction ]= val;
}

void Interpreter::ProcessSMulWithOverflow( const llvm::CallInst* const instruction )
{
	if( instruction->getNumOperands() < 2 )
	{
		ReportError( "Incorrect number of operands for \"smul.with.overflow\"!" );
		return;
	}

	const llvm::APInt a= GetVal( instruction->getOperand(0u) ).IntVal;
	const llvm::APInt b= GetVal( instruction->getOperand(1u) ).IntVal;

	bool overflow= false;
	const llvm::APInt result= a.smul_ov( b, overflow );

	llvm::GenericValue result_val;
	result_val.IntVal= result;

	llvm::GenericValue overflow_val;
	overflow_val.IntVal= llvm::APInt( 1, overflow ? 1 : 0 );

	llvm::GenericValue val;
	val.AggregateVal= { result_val, overflow_val };
	current_function_frame_.instructions_map[ instruction ]= val;
}

void Interpreter::ProcessUMulWithOverflow( const llvm::CallInst* const instruction )
{
	if( instruction->getNumOperands() < 2 )
	{
		ReportError( "Incorrect number of operands for \"umul.with.overflow\"!" );
		return;
	}

	const llvm::APInt a= GetVal( instruction->getOperand(0u) ).IntVal;
	const llvm::APInt b= GetVal( instruction->getOperand(1u) ).IntVal;

	bool overflow= false;
	const llvm::APInt result= a.umul_ov( b, overflow );

	llvm::GenericValue result_val;
	result_val.IntVal= result;

	llvm::GenericValue overflow_val;
	overflow_val.IntVal= llvm::APInt( 1, overflow ? 1 : 0 );

	llvm::GenericValue val;
	val.AggregateVal= { result_val, overflow_val };
	current_function_frame_.instructions_map[ instruction ]= val;
}

void Interpreter::ProcessStacksave( const llvm::CallInst* const instruction )
{
	// For now just do not bother with it - produce nullptr.
	llvm::GenericValue val;
	val.IntVal= llvm::APInt( pointer_size_in_bits_, uint64_t(0) );
	current_function_frame_.instructions_map[ instruction ]= val;
}

void Interpreter::ProcessStackrestore( const llvm::CallInst* const instruction )
{
	// For now ignore this instruction.
	U_UNUSED(instruction);
}

void Interpreter::ProcessThreadLocalAddress( const llvm::CallInst* const instruction )
{
	if( instruction->getNumOperands() < 1 )
	{
		ReportError( "Incorrect number of operands for \"threadlocal.address\"!" );
		return;
	}

	// Just return variable itself. For now we don't support handling thread-local variables properly (with TLS creation).
	llvm::GenericValue val= GetVal( instruction->getArgOperand(0) );
	current_function_frame_.instructions_map[ instruction ]= std::move(val);
}

void Interpreter::ResumeCoroutine( const llvm::CallInst* instruction, const bool destroy )
{
	if( instruction->getNumOperands() < 1 )
	{
		ReportError( "Incorrect number of operands for coroutine resume!" );
		return;
	}

	const llvm::GenericValue handle= GetVal( instruction->getOperand(0u) );
	const uint32_t coroutine_id= uint32_t(handle.IntVal.getLimitedValue());
	CoroutineData& coroutine_data= coroutines_data_[coroutine_id];

	CallFrame call_frame;
	call_frame.instructions_map= coroutine_data.instructions_map;
	call_frame.coroutine_data= &coroutine_data;
	call_frame.is_coroutine= true;

	const size_t prev_stack_size= stack_.size();

	std::swap( call_frame, current_function_frame_ );

	{ // Set result of "suspend" instriction.
		llvm::GenericValue val;
		val.IntVal= llvm::APInt( 8u, destroy ? uint64_t(1) : uint64_t(0) );
		current_function_frame_.instructions_map[ coroutine_data.suspend_instruction ]= val;
	}
	// Continue execution starting with next instruction.
	ExecuteFunctionBody( coroutine_data.suspend_instruction->getNextNode() );

	std::swap( call_frame, current_function_frame_ );

	// Coroutine should not allocate anything on stack.
	U_ASSERT( stack_.size() == prev_stack_size );
}

void Interpreter::ProcessUnaryArithmeticInstruction( const llvm::Instruction* const instruction )
{
	llvm::Value* const operand0= instruction->getOperand(0u);
	const llvm::GenericValue op= GetVal( operand0 );

	llvm::Type* const dst_type= instruction->getType();
	llvm::Type* const src_type= operand0->getType();
	llvm::GenericValue val;
	switch(instruction->getOpcode())
	{
	case llvm::Instruction::ExtractValue:
		val= op;
		for( const auto index : llvm::dyn_cast<llvm::ExtractValueInst>(instruction)->indices() )
		{
			if( index >= val.AggregateVal.size() )
			{
				ReportError( "Extracting value with incorrect index!" );
				return;
			}

			val= val.AggregateVal[index];
		}
		break;

	case llvm::Instruction::SExt:
		if( !dst_type->isIntegerTy() )
		{
			ReportError( "Unsupported type for \"sext\" instruction", *instruction );
			return;
		}
		val.IntVal= op.IntVal.sext(dst_type->getIntegerBitWidth());
		break;

	case llvm::Instruction::ZExt:
		if( !dst_type->isIntegerTy() )
		{
			ReportError( "Unsupported type for \"zext\" instruction", *instruction );
			return;
		}
		val.IntVal= op.IntVal.zext(dst_type->getIntegerBitWidth());
		break;

	case llvm::Instruction::Trunc:
		if( !dst_type->isIntegerTy() )
		{
			ReportError( "Unsupported type for \"trunc\" instruction", *instruction );
			return;
		}
		val.IntVal= op.IntVal.trunc(dst_type->getIntegerBitWidth());
		break;

	case llvm::Instruction::FPExt:
		if( dst_type->isFloatTy() )
		{
			if( src_type->isFloatTy() )
				val.FloatVal= op.FloatVal;
			else
				ReportError( "unsupported src type for \"fpext\" instruction", *instruction );
		}
		else if( dst_type->isDoubleTy() )
		{
			if( src_type->isFloatTy() )
				val.DoubleVal= double(op.FloatVal);
			else
				ReportError( "unsupported src type for \"fpext\" instruction", *instruction );
		}
		else
			ReportError( "unsupported dst type for \"fpext\" instruction", *instruction );
		break;

	case llvm::Instruction::FPTrunc:
		if( dst_type->isFloatTy() )
		{
			if( src_type->isFloatTy() )
				val.FloatVal= op.FloatVal;
			else if( src_type->isDoubleTy() )
				val.FloatVal= float(op.DoubleVal);
			else
				ReportError( "unsupported src type for \"fptrunc\" instruction", *instruction );
		}
		else if( dst_type->isDoubleTy() )
		{
			if( src_type->isDoubleTy() )
				val.DoubleVal= op.DoubleVal;
			else
				ReportError( "unsupported src type for \"fptrunc\" instruction", *instruction );
		}
		else
			ReportError( "unsupported dst type for \"fptrunc\" instruction", *instruction );
		break;

	case llvm::Instruction::IntToPtr:
		if( !dst_type->isPointerTy() )
		{
			ReportError( "Unsupported type for \"inttoptr\" instruction", *instruction );
			return;
		}
		val.IntVal= llvm::APInt( pointer_size_in_bits_, op.IntVal.getLimitedValue() );
		break;

	case llvm::Instruction::PtrToInt:
		if( !dst_type->isIntegerTy() )
		{
			ReportError( "Unsupported type for \"ptrtoint\" instruction", *instruction );
			return;
		}
		val.IntVal= llvm::APInt( pointer_size_in_bits_, op.IntVal.getLimitedValue() );
		break;

	case llvm::Instruction::FNeg:
		if( src_type->isFloatTy() )
			val.FloatVal= -op.FloatVal;
		else if( src_type->isDoubleTy() )
			val.DoubleVal = -op.DoubleVal;
		else
			ReportError( "unsupported type for \"fneg\" instruction", *instruction );
		break;

	case llvm::Instruction::SIToFP:
		if( !src_type->isIntegerTy() )
		{
			ReportError( "Unsupported type for \"sitofp\" instruction", *instruction );
			return;
		}
		if( dst_type->isFloatTy() )
			val.FloatVal= llvm::APIntOps::RoundSignedAPIntToFloat(op.IntVal);
		else if( dst_type->isDoubleTy() )
			val.DoubleVal= llvm::APIntOps::RoundSignedAPIntToDouble(op.IntVal);
		else
			ReportError( "unsupported type for \"sitofp\" instruction", *instruction );
		break;

	case llvm::Instruction::UIToFP:
		if( !src_type->isIntegerTy() )
		{
			ReportError( "Unsupported type for \"uitofp\" instruction", *instruction );
			return;
		}
		if( dst_type->isFloatTy() )
			val.FloatVal= llvm::APIntOps::RoundAPIntToFloat(op.IntVal);
		else if( dst_type->isDoubleTy() )
			val.DoubleVal= llvm::APIntOps::RoundAPIntToDouble(op.IntVal);
		else
			ReportError( "unsupported type for \"uitofp\" instruction", *instruction );
		break;

	// TODO - is this correct way to convert floats to ints?
	case llvm::Instruction::FPToSI:
		if( !dst_type->isIntegerTy() )
		{
			ReportError( "Unsupported type for \"fptosi\" instruction", *instruction );
			return;
		}
		if( src_type->isFloatTy() )
			val.IntVal= llvm::APIntOps::RoundFloatToAPInt( op.FloatVal, dst_type->getIntegerBitWidth() );
		else if( src_type->isDoubleTy() )
			val.IntVal= llvm::APIntOps::RoundDoubleToAPInt( op.DoubleVal, dst_type->getIntegerBitWidth() );
		else
			ReportError( "unsupported type for \"fptosi\" instruction", *instruction );
		break;

	case llvm::Instruction::FPToUI:
		if( !dst_type->isIntegerTy() )
		{
			ReportError( "Unsupported type for \"fptoui\" instruction", *instruction );
			return;
		}
		if( src_type->isFloatTy() )
			val.IntVal= llvm::APIntOps::RoundFloatToAPInt( op.FloatVal, dst_type->getIntegerBitWidth() );
		else if( src_type->isDoubleTy() )
			val.IntVal= llvm::APIntOps::RoundDoubleToAPInt( op.DoubleVal, dst_type->getIntegerBitWidth() );
		else
			ReportError( "unsupported type for \"fptoui\" instruction", *instruction );
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
				ReportError( "Invalid int to float cast", *instruction );
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
				ReportError( "Invalid float to int cast", *instruction );
		}
		else
		{
			// Cast function pointer or pointer for memcpy
			val.IntVal= llvm::APInt( uint32_t(data_layout_.getTypeAllocSizeInBits( dst_type )), op.IntVal.getLimitedValue() );
		}
		break;

	default:
		ReportError( std::string( "executing unknown unary instruction \"" ) + instruction->getOpcodeName() + "\"", *instruction );
		break;
	}

	current_function_frame_.instructions_map[instruction]= val;
}

void Interpreter::ProcessBinaryArithmeticInstruction( const llvm::Instruction* const instruction )
{
	const llvm::GenericValue op0= GetVal( instruction->getOperand(0u) );
	const llvm::GenericValue op1= GetVal( instruction->getOperand(1u) );

	llvm::Type* const type0= instruction->getOperand(0u)->getType();
	llvm::Type* const type1= instruction->getOperand(1u)->getType();
	llvm::GenericValue val;
	switch(instruction->getOpcode())
	{
	case llvm::Instruction::Add:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"add\" instruction", *instruction );
			return;
		}
		if( !type0->isIntegerTy() )
		{
			ReportError( "unsupported type for \"add\" instruction", *instruction );
			return;
		}
		val.IntVal= op0.IntVal + op1.IntVal;
		break;

	case llvm::Instruction::Sub:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"sub\" instruction", *instruction );
			return;
		}
		if( !type0->isIntegerTy() )
		{
			ReportError( "unsupported type for \"sub\" instruction", *instruction );
			return;
		}
		val.IntVal= op0.IntVal - op1.IntVal;
		break;

	case llvm::Instruction::Mul:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"mul\" instruction", *instruction );
			return;
		}
		if( !type0->isIntegerTy() )
		{
			ReportError( "unsupported type for \"mul\" instruction", *instruction );
			return;
		}
		val.IntVal= op0.IntVal * op1.IntVal;
		break;

	case llvm::Instruction::SDiv:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"sdiv\" instruction", *instruction );
			return;
		}
		if( !type0->isIntegerTy() )
		{
			ReportError( "unsupported type for \"sdiv\" instruction", *instruction );
			return;
		}
		if( op1.IntVal.getBoolValue() )
			val.IntVal= op0.IntVal.sdiv(op1.IntVal);
		else
			ReportError( "constexpr division by zero", *instruction );
		break;

	case llvm::Instruction::UDiv:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"udiv\" instruction", *instruction );
			return;
		}
		if( !type0->isIntegerTy())
		{
			ReportError( "unsupported type for \"lshr\" instruction", *instruction );
			return;
		}

		if( op1.IntVal.getBoolValue() )
			val.IntVal= op0.IntVal.udiv(op1.IntVal);
		else
			ReportError( "constexpr division by zero", *instruction );
		break;

	case llvm::Instruction::SRem:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"srem\" instruction", *instruction );
			return;
		}
		if( !type0->isIntegerTy() )
		{
			ReportError( "unsupported type for \"srem\" instruction", *instruction );
			return;
		}
		if( op1.IntVal.getBoolValue() )
			val.IntVal= op0.IntVal.srem(op1.IntVal);
		else
			ReportError( "constexpr division by zero", *instruction );
		break;

	case llvm::Instruction::URem:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"urem\" instruction", *instruction );
			return;
		}
		if( !type0->isIntegerTy() )
		{
			ReportError( "unsupported type for \"urem\" instruction", *instruction );
			return;
		}
		if( op1.IntVal.getBoolValue() )
			val.IntVal= op0.IntVal.urem(op1.IntVal);
		else
			ReportError( "constexpr division by zero", *instruction );
		break;

	case llvm::Instruction::And:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"and\" instruction", *instruction );
			return;
		}
		if( !type0->isIntegerTy() )
		{
			ReportError( "unsupported type for \"and\" instruction", *instruction );
			return;
		}
		val.IntVal= op0.IntVal & op1.IntVal;
		break;

	case llvm::Instruction::Or:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"or\" instruction", *instruction );
			return;
		}
		if( !type0->isIntegerTy() )
		{
			ReportError( "unsupported type for \"or\" instruction", *instruction );
			return;
		}
		val.IntVal= op0.IntVal | op1.IntVal;
		break;

	case llvm::Instruction::Xor:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"xor\" instruction", *instruction );
			return;
		}
		if( !type0->isIntegerTy())
		{
			ReportError( "unsupported type for \"xor\" instruction", *instruction );
			return;
		}
		val.IntVal= op0.IntVal ^ op1.IntVal;
		break;

	case llvm::Instruction::Shl:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"shl\" instruction", *instruction );
			return;
		}
		if( !type0->isIntegerTy() )
		{
			ReportError( "unsupported type for \"shl\" instruction", *instruction );
			return;
		}
		val.IntVal= op0.IntVal.shl(op1.IntVal);
		break;

	case llvm::Instruction::AShr:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"ashr\" instruction", *instruction );
			return;
		}
		if( !type0->isIntegerTy() )
		{
			ReportError( "unsupported type for \"ashr\" instruction", *instruction );
			return;
		}
		val.IntVal= op0.IntVal.ashr(op1.IntVal);
		break;

	case llvm::Instruction::LShr:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"lshr\" instruction", *instruction );
			return;
		}
		if( !type0->isIntegerTy() )
		{
			ReportError( "unsupported type for \"lshr\" instruction", *instruction );
			return;
		}
		val.IntVal= op0.IntVal.lshr(op1.IntVal);
		break;

	case llvm::Instruction::FAdd:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"fadd\" instruction", *instruction );
			return;
		}
		if( type0->isFloatTy() )
			val.FloatVal= ( llvm::APFloat(op0.FloatVal) + llvm::APFloat(op1.FloatVal) ).convertToFloat();
		else if( type0->isDoubleTy() )
			val.DoubleVal= ( llvm::APFloat(op0.DoubleVal) + llvm::APFloat(op1.DoubleVal) ).convertToDouble();
		else
			ReportError( "unsupported type for \"fadd\" instruction", *instruction );
		break;

	case llvm::Instruction::FSub:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"fsub\" instruction", *instruction );
			return;
		}
		if( type0->isFloatTy() )
			val.FloatVal= ( llvm::APFloat(op0.FloatVal) - llvm::APFloat(op1.FloatVal) ).convertToFloat();
		else if( type0->isDoubleTy() )
			val.DoubleVal= ( llvm::APFloat(op0.DoubleVal) - llvm::APFloat(op1.DoubleVal) ).convertToDouble();
		else
			ReportError( "unsupported type for \"fsub\" instruction", *instruction );
		break;

	case llvm::Instruction::FMul:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"fmul\" instruction", *instruction );
			return;
		}
		if( type0->isFloatTy() )
			val.FloatVal= ( llvm::APFloat(op0.FloatVal) * llvm::APFloat(op1.FloatVal) ).convertToFloat();
		else if( type0->isDoubleTy() )
			val.DoubleVal= ( llvm::APFloat(op0.DoubleVal) * llvm::APFloat(op1.DoubleVal) ).convertToDouble();
		else
			ReportError( "unsupported type for \"fmul\" instruction", *instruction );
		break;

	case llvm::Instruction::FDiv:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"fdiv\" instruction", *instruction );
			return;
		}
		if( type0->isFloatTy() )
			val.FloatVal= ( llvm::APFloat(op0.FloatVal) / llvm::APFloat(op1.FloatVal) ).convertToFloat();
		else if( type0->isDoubleTy() )
			val.DoubleVal= ( llvm::APFloat(op0.DoubleVal) / llvm::APFloat(op1.DoubleVal) ).convertToDouble();
		else
			ReportError( "unsupported type for \"fdiv\" instruction", *instruction );
		break;

	case llvm::Instruction::FRem:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"frem\" instruction", *instruction );
			return;
		}
		if( type0->isFloatTy() )
		{
			llvm::APFloat result_val(op0.FloatVal);
			result_val.mod( llvm::APFloat(op1.FloatVal));
			val.FloatVal= result_val.convertToFloat();
		}
		else if( type0->isDoubleTy() )
		{
			llvm::APFloat result_val(op0.DoubleVal);
			result_val.mod( llvm::APFloat(op1.DoubleVal) );
			val.DoubleVal= result_val.convertToDouble();
		}
		else
			ReportError( "unsupported type for \"frem\" instruction", *instruction );
		break;

	case llvm::Instruction::ICmp:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"icmp\" instruction", *instruction );
			return;
		}
		if( !( type0->isIntegerTy() || type0->isPointerTy() ) )
		{
			ReportError( "unsupported type for \"icmp\" instruction", *instruction );
			return;
		}
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
		default:
			ReportError( "unhandled compare predicate", *instruction );
			return;
		};
		break;

	case llvm::Instruction::FCmp:
		if( type0 != type1 )
		{
			ReportError( "types mismatch for \"fcmp\" instruction", *instruction );
			return;
		}
		if( !type0->isFloatingPointTy() )
		{
			ReportError( "unsupported type for \"fcmp\" instruction", *instruction );
			return;
		}
		{
			llvm::APFloat::cmpResult cmp_result;
			if( type0->isFloatTy() )
				cmp_result= llvm::APFloat(op0.FloatVal).compare(llvm::APFloat(op1.FloatVal));
			else
				cmp_result= llvm::APFloat(op0.DoubleVal).compare(llvm::APFloat(op1.DoubleVal));
			switch(llvm::dyn_cast<llvm::CmpInst>(instruction)->getPredicate())
			{
			// see llvm-3.7.1.src/lib/IR/ConstantFold.cpp:1752
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
			default:
				ReportError( "unhandled compare predicate", *instruction );
				return;
			}
		}
		break;

	case llvm::Instruction::AtomicRMW:
		{
			const MemoryRegionView memory_region= GetMemoryForVirtualAddress( size_t(op0.IntVal.getLimitedValue()) );
			const llvm::GenericValue load_result= DoLoad( memory_region, instruction->getType() );
			val= load_result;

			llvm::GenericValue new_value;
			const auto operation= llvm::dyn_cast<llvm::AtomicRMWInst>(instruction)->getOperation();
			switch(operation)
			{
			case llvm::AtomicRMWInst::Xchg:
				new_value= op1;
				break;
			case llvm::AtomicRMWInst::Add:
				new_value.IntVal= load_result.IntVal + op1.IntVal;
				break;
			case llvm::AtomicRMWInst::Sub:
				new_value.IntVal= load_result.IntVal - op1.IntVal;
				break;
			case llvm::AtomicRMWInst::And:
				new_value.IntVal= load_result.IntVal & op1.IntVal;
				break;
			case llvm::AtomicRMWInst::Nand:
				new_value.IntVal= ~(load_result.IntVal & op1.IntVal);
				break;
			case llvm::AtomicRMWInst::Or:
				new_value.IntVal= load_result.IntVal | op1.IntVal;
				break;
			case llvm::AtomicRMWInst::Xor:
				new_value.IntVal= load_result.IntVal ^ op1.IntVal;
				break;
			default:
				ReportError( ( std::string("Unsupported atomic operation \"") + llvm::AtomicRMWInst::getOperationName(operation) + "\"" ).str(), *instruction );
				new_value= op1;
				break;
			}
			DoStore( memory_region, new_value, instruction->getType() );
		}
		break;

	default:
		ReportError( std::string("executing unknown binary instruction \"") + instruction->getOpcodeName() + "\"", *instruction );
		break;
	};

	current_function_frame_.instructions_map[instruction]= val;
}

size_t Interpreter::GetFunctionIndex( const llvm::Function* const function )
{
	// Maintain bidirectional mapping of function pointers to their indices.

	if( const auto it= function_pointers_map_.find( function ); it != function_pointers_map_.end() )
		return it->second; // This function is already known.

	// Create new entry in both containers.

	const size_t index= function_pointers_mapping_vec_.size();
	function_pointers_mapping_vec_.push_back( function );
	function_pointers_map_.insert( std::make_pair( function, index ) );

	return index;
}

void Interpreter::ReportDataStackOverflow()
{
	ReportError( "Max data stack size (" + std::to_string( g_max_data_stack_size ) + ") reached");
}

void Interpreter::ReportGlobalsStackOverflow()
{
	ReportError( "Max globals stack size (" + std::to_string( g_max_globals_stack_size ) + ") reached" );
}

void Interpreter::ReportError( const std::string_view text, const llvm::Instruction& instruction )
{
	if(const llvm::DILocation* const di_location = instruction.getDebugLoc().get() )
	{
		std::string text_extended( text );
		text_extended+= "\n\tAt ";
		text_extended+= di_location->getFilename();
		text_extended+= ":";
		text_extended+= std::to_string(di_location->getLine());
		text_extended+= ":";
		text_extended+= std::to_string(di_location->getColumn());

		const std::string call_stack= GetCurrentCallStackDescription();

		if( !call_stack.empty() )
		{
			text_extended+= "\n\tCall stack:";
			text_extended+= call_stack;
		}

		errors_.push_back( std::move(text_extended) );
	}
	else
		ReportError( text );
}

void Interpreter::ReportError( const std::string_view text )
{
	const std::string call_stack= GetCurrentCallStackDescription();

	if( !call_stack.empty() )
	{
		std::string text_extended( text );
		text_extended+= "\n\tCall stack:";
		text_extended+= call_stack;
		errors_.push_back( std::move(text_extended) );
	}
	else
		errors_.emplace_back( text );
}

std::string Interpreter::GetCurrentCallStackDescription()
{
	const auto format_index=
		[]( const size_t index )
		{
			std::string index_str= std::to_string(index);
			index_str.insert(index_str.begin(), '#' );
			while(index_str.size() < 6)
				index_str.insert(index_str.begin(), ' ' );
			return index_str;
		};

	std::string call_stack;

	// First print current function, using its own location.
	if( !call_stack_.empty() )
	{
		const llvm::CallInst* const call_instruction= call_stack_.back();

		std::string description;
		description+= "\t";
		description+= format_index( 0 );
		description+= " ";

		if( const llvm::Function* const function= llvm::dyn_cast<llvm::Function>(call_instruction->getCalledOperand()) )
		{
			if(const llvm::DISubprogram * const subprogram= function->getSubprogram() )
			{
				description+= subprogram->getName();
				description+= " ( ";
				description+= subprogram->getLinkageName();
				description+= " ) ";
				description+= subprogram->getFilename();
				description+= ":";
				description+= std::to_string(subprogram->getLine());
			}
			else
				description+= function->getName();
		}
		else
			description+= "<unknown function>";

		call_stack+= "\n";
		call_stack+= description;
	}

	// Then print stack of call sites (in reverse order).
	for(auto it= call_stack_.rbegin(); it != call_stack_.rend(); ++it)
	{
		const llvm::CallInst* const call_instruction= *it;

		std::string description;
		description+= "\t";
		description+= format_index( size_t( 1 + it - call_stack_.rbegin() ) );
		description+= " ";

		if( const llvm::Function* const call_site_function= call_instruction->getParent()->getParent() )
		{
			if(const llvm::DISubprogram * const subprogram= call_site_function->getSubprogram() )
			{
				description+= subprogram->getName();
				description+= " ( ";
				description+= subprogram->getLinkageName();
				description+= " ) ";
			}
			else
				description+= call_site_function->getName();
		}
		else
			description+= "<unknown function> ";

		if( const llvm::DILocation* const di_location = call_instruction->getDebugLoc().get() )
		{
			description+= di_location->getFilename();
			description+= ":";
			description+= std::to_string(di_location->getLine());
			description+= ":";
			description+= std::to_string(di_location->getColumn());
		}

		call_stack+= "\n";
		call_stack+= description;
	}

	return call_stack;
}

} // namespace U
