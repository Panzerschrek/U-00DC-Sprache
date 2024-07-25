#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/ConvertUTF.h>
#include "../../code_builder_lib_common/return_value_optimization.hpp"
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

extern "C"
{

LLVMValueRef U1_ConstantTokenNone( const LLVMContextRef C )
{
	return llvm::wrap( llvm::ConstantTokenNone::get( *llvm::unwrap(C) ) );
}

LLVMTypeRef U1_GetFunctionType(const LLVMValueRef f)
{
	return llvm::wrap(llvm::dyn_cast<llvm::Function>(llvm::unwrap(f))->getFunctionType());
}

void U1_SetStructName(const LLVMTypeRef t, const char* const name)
{
	llvm::dyn_cast<llvm::StructType>(llvm::unwrap(t))->setName(name);
}

bool U1_BasicBlockHasPredecessors(const LLVMBasicBlockRef basic_block)
{
	return llvm::unwrap(basic_block)->hasNPredecessorsOrMore(1);
}

LLVMValueRef U1_CreateOrphanInBoundsGEP( const LLVMTypeRef t, const LLVMValueRef poiter, LLVMValueRef* const indices, const uint32_t num_indices )
{
	return
		llvm::wrap(
			llvm::GetElementPtrInst::CreateInBounds(
				llvm::unwrap(t),
				llvm::unwrap(poiter),
				llvm::ArrayRef<llvm::Value*>( reinterpret_cast<llvm::Value**>(indices), num_indices ) ) );
}

void U1_InsertInstructionAfterAnother( const LLVMValueRef src_instructuon, const LLVMValueRef instruction_for_insert )
{
	llvm::dyn_cast<llvm::Instruction>( llvm::unwrap(instruction_for_insert) )->insertAfter( llvm::dyn_cast<llvm::Instruction>( llvm::unwrap( src_instructuon ) ) );
}

void U1_FunctionAddDereferenceableAttr(const LLVMValueRef function, const uint32_t index, const uint64_t bytes)
{
	const auto f= llvm::dyn_cast<llvm::Function>(llvm::unwrap(function));

	llvm::AttrBuilder builder( f->getContext() );
	builder.addDereferenceableAttr( bytes );

	if( index == llvm::AttributeList::ReturnIndex )
		f->addRetAttrs(builder);
	else
		f->addParamAttrs(index - llvm::AttributeList::FirstArgIndex, builder);
}

size_t U1_ConvertUTF8ToUTF16(
	const char* const src_buff, const size_t src_buff_size,
	llvm::UTF16* const dst_buff, const size_t dst_buff_size )
{
	llvm::SmallVector<llvm::UTF16, 32> str;
	llvm::convertUTF8ToUTF16String( llvm::StringRef(src_buff, src_buff_size), str );

	std::memcpy( dst_buff, str.data(), std::min( dst_buff_size, str.size() ) * sizeof(llvm::UTF16) );
	return str.size();
}

size_t U1_ConvertUTF8ToUTF32(
	const llvm::UTF8* src_buff, const size_t src_buff_size,
	llvm::UTF32* dst_buff, const size_t dst_buff_size )
{
	const auto src_buff_end= src_buff + src_buff_size;
	const auto dst_buff_start= dst_buff;
	const auto dst_buff_end= dst_buff + dst_buff_size;

	const llvm::ConversionResult res= llvm::ConvertUTF8toUTF32(
		&src_buff, src_buff_end,
		&dst_buff, dst_buff_end,
		llvm::ConversionFlags() );

	if( res == llvm::conversionOK && src_buff == src_buff_end )
		return size_t(dst_buff - dst_buff_start);
	else
		return dst_buff_size + 1; // Size is unknown, but greater than expected.
}

bool U1_IsLegalUTF8Sequence( const char* const start, const size_t length )
{
	return llvm::isLegalUTF8Sequence(
		reinterpret_cast<const llvm::UTF8*>(start),
		reinterpret_cast<const llvm::UTF8*>(start) + length ) != 0;
}

LLVMValueRef U1_ConstDataArray(LLVMTypeRef t, const char* const data, const size_t size, const size_t element_count)
{
	return llvm::wrap(
		llvm::ConstantDataArray::getRaw(
			llvm::StringRef(data, size),
			element_count,
			llvm::unwrap(t)));
}

void U1_TryToPerformReturnValueAllocationOptimization( const LLVMValueRef function )
{
	const auto function_really= llvm::dyn_cast<llvm::Function>( llvm::unwrap(function) );
	U::TryToPerformReturnValueAllocationOptimization( *function_really );
}

} // extern "C"
