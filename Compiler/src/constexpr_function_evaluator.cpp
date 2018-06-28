#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/InstrTypes.h>
#include "pop_llvm_warnings.hpp"

#include "assert.hpp"
#include "constexpr_function_evaluator.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

ConstexprFunctionEvaluator::ConstexprFunctionEvaluator( const llvm::DataLayout& data_layout )
	: data_layout_(data_layout)
{
}

ConstexprFunctionEvaluator::Result ConstexprFunctionEvaluator::Evaluate(
	const Function& function_type,
	llvm::Function* const llvm_function,
	const std::vector<llvm::Constant*>& args )
{
	U_ASSERT( function_type.args.size() == args.size() );

	// Fill arguments
	size_t i= 0u;
	for( const llvm::Argument& arg : llvm_function->args() )
	{
		if( arg.hasStructRetAttr() )
		{
			++i;
			continue;
		}

		if( arg.getType()->isPointerTy() )
		{
			size_t ptr= MoveConstantToStack( *args[i] );
			llvm::GenericValue val;
			val.IntVal= llvm::APInt( 64u, uint64_t(ptr) );

			instructions_map_[ &arg ]= val;
		}
		else if( arg.getType()->isIntegerTy() )
		{
			llvm::GenericValue val;
			val.IntVal= args[i]->getUniqueInteger();
			instructions_map_[ &arg ]= val;
		}
		else if( arg.getType()->isFloatTy() )
		{
		}
		else if( arg.getType()->isDoubleTy() )
		{
		}
		else
			U_ASSERT(false);

		++i;
	}

	llvm::GenericValue res= CallFunction( *llvm_function );
	Result result;

	if( function_type.return_value_is_reference )
	{
		U_ASSERT(false);
	}
	else
	{
		if( const FundamentalType* const fundamental= function_type.return_type.GetFundamentalType() )
		{
			if( IsInteger( fundamental->fundamental_type ) )
				result.result_constant= llvm::Constant::getIntegerValue( function_type.return_type.GetLLVMType(), res.IntVal );
			else if( IsFloatingPoint( fundamental->fundamental_type ) )
				result.result_constant= llvm::ConstantFP::get( function_type.return_type.GetLLVMType(), res.DoubleVal );
			else
				U_ASSERT(false);
		}
		else
		{
			U_ASSERT(false);
		}
	}

	instructions_map_.clear();
	stack_.clear();

	return result;
}

llvm::GenericValue ConstexprFunctionEvaluator::CallFunction( const llvm::Function& llvm_function )
{
	const llvm::Instruction* instruction= llvm_function.getBasicBlockList().front().begin();
	while(true)
	{
		switch( instruction->getOpcode() )
		{
		case llvm::Instruction::Alloca:
			{
				//llvm::Type* const pointer_type= instruction->getOperand(0u)->getType();
				//llvm::Type* const element_type= llvm::dyn_cast<llvm::PointerType>(pointer_type)->getElementType();
				llvm::Type* const element_type= instruction->getOperand(0u)->getType();

				const size_t size= data_layout_.getTypeAllocSize( element_type );

				const size_t stack_offset= stack_.size();
				stack_.resize( size );

				llvm::GenericValue val;
				val.IntVal= llvm::APInt( 64u, uint64_t(stack_offset) );
				instructions_map_[ instruction ]= val;
			}
			instruction= instruction->getNextNode();
			break;

		case llvm::Instruction::Load:
			{
				const llvm::Value* const address= instruction->getOperand(0u);
				U_ASSERT( instructions_map_.find( address ) != instructions_map_.end() );
				const llvm::GenericValue& address_val= instructions_map_[address];

				const size_t offset= address_val.IntVal.getLimitedValue();
				U_ASSERT( offset < stack_.size() );

				llvm::Type* const element_type= llvm::dyn_cast<llvm::PointerType>(address->getType())->getElementType();
				if( element_type->isIntegerTy() )
				{
					uint64_t buff[4];
					std::memcpy( buff, stack_.data() + offset, element_type->getIntegerBitWidth() / 8u );

					llvm::GenericValue val;
					val.IntVal= llvm::APInt( element_type->getIntegerBitWidth() , buff );
					instructions_map_[ instruction ]= val;
				}
				else
				{
					U_ASSERT(false);
					// TODO
				}
			}
			instruction= instruction->getNextNode();
			break;

		case llvm::Instruction::Store:
			{
				const llvm::Value* const address= instruction->getOperand(1u);
				U_ASSERT( instructions_map_.find( address ) != instructions_map_.end() );
				const llvm::GenericValue& address_val= instructions_map_[address];

				const size_t offset= address_val.IntVal.getLimitedValue();
				U_ASSERT( offset < stack_.size() );

				const llvm::Value* const value= instruction->getOperand(0u);
				U_ASSERT( instructions_map_.find( value ) != instructions_map_.end() );
				const llvm::GenericValue& val= instructions_map_[value];

				llvm::Type* const element_type= llvm::dyn_cast<llvm::PointerType>(address->getType())->getElementType();
				if( element_type->isIntegerTy() )
				{
					uint64_t limited_value= val.IntVal.getLimitedValue();
					std::memcpy( stack_.data() + offset, &limited_value, element_type->getIntegerBitWidth() / 8u );
				}
				else
				{
					U_ASSERT(false);
					// TODO
				}
			}
			instruction= instruction->getNextNode();
			break;


		case llvm::Instruction::Br:
			{
				const auto basic_block= llvm::dyn_cast<llvm::BasicBlock>(instruction->getOperand(0u));
				instruction= basic_block->getInstList().begin();
			}
			break;

		case llvm::Instruction::Ret:
			{
				if( instruction->getNumOperands() == 0u )
					return llvm::GenericValue();

				const llvm::Value* const value= instruction->getOperand(0u);
				U_ASSERT( instructions_map_.find( value ) != instructions_map_.end() );
				return instructions_map_[value];
			}

		default:
			U_ASSERT(false);
			break;
		};
	}
}

size_t ConstexprFunctionEvaluator::MoveConstantToStack( const llvm::Constant& constant )
{
	const size_t stack_offset= stack_.size();
	stack_.resize( stack_.size() + data_layout_.getTypeAllocSize( constant.getType() ) );

	if( const auto struct_type= llvm::dyn_cast<llvm::StructType>( constant.getType() ) )
	{
		const llvm::StructLayout& struct_layout= *data_layout_.getStructLayout( struct_type );

		size_t i= 0u;
		for( llvm::Type* const element_type : struct_type->elements() )
		{
			llvm::Constant* const element= constant.getAggregateElement(i);
			const size_t element_size= data_layout_.getTypeAllocSize( element_type );
			const size_t element_offset= struct_layout.getElementOffset(i);

			if( element_type->isPointerTy() ) // TODO - what if it is function pointer ?
			{
				const size_t element_ptr= MoveConstantToStack( *element );
				std::memcpy( stack_.data() + stack_offset + element_offset, stack_.data() + element_ptr, element_size );
			}
			else if( element_type->isStructTy() )
			{
				U_ASSERT(false); // TODO
			}
			// TODO - checl big/little-endian correctness.
			else if( element_type->isIntegerTy() )
			{
				const uint64_t val= element->getUniqueInteger().getLimitedValue();
				std::memcpy( stack_.data() + stack_offset + element_offset, &val, element_size );
			}
			else if( element_type->isFloatTy() )
			{
				const float val= llvm::dyn_cast<llvm::ConstantFP>( element )->getValueAPF().convertToFloat();
				std::memcpy( stack_.data() + stack_offset + element_offset, &val, element_size );
			}
			else if( element_type->isDoubleTy() )
			{
				const double val= llvm::dyn_cast<llvm::ConstantFP>( element )->getValueAPF().convertToDouble();
				std::memcpy( stack_.data() + stack_offset + element_offset, &val, element_size );
			}
			else if( element_type->isArrayTy() )
			{
				U_ASSERT(false); // TODO
			}
			else
				U_ASSERT(false);

			++i;
		}
	}

	return stack_offset;
}

} // namespace CodeBuilderPrivate

} // namespace U
