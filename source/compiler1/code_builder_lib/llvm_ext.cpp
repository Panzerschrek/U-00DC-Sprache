#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/ConvertUTF.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

extern "C" void U1_SetStructName(const LLVMTypeRef t, const char* const name)
{
	llvm::dyn_cast<llvm::StructType>(llvm::unwrap(t))->setName(name);
}

extern "C" void U1_DropAllBasicBlockUsersReferences(const LLVMBasicBlockRef basic_block)
{
	for( const auto use : llvm::unwrap(basic_block)->users() )
		use->dropAllReferences();
}

extern "C" bool U1_BasicBlockHasPredecessors(const LLVMBasicBlockRef basic_block)
{
	return llvm::unwrap(basic_block)->hasNPredecessorsOrMore(1);
}

extern "C" void U1_FunctionAddDereferenceableAttr(const LLVMValueRef function, const uint32_t index, const uint64_t bytes)
{
	llvm::dyn_cast<llvm::Function>(llvm::unwrap(function))->addDereferenceableAttr(index, bytes);
}

extern "C" size_t U1_ConvertUTF8ToUTF16(
	const char* const src_buff, const size_t src_buff_size,
	llvm::UTF16* const dst_buff, const size_t dst_buff_size )
{
	llvm::SmallVector<llvm::UTF16, 32> str;
	llvm::convertUTF8ToUTF16String( llvm::StringRef(src_buff, src_buff_size), str );

	std::memcpy( dst_buff, str.data(), std::min( dst_buff_size, str.size() ) * sizeof(llvm::UTF16) );
	return str.size();
}

extern "C" size_t U1_ConvertUTF8ToUTF32(
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
		return dst_buff_size + 1; // Size is unknown, but greater then expected.
}

extern "C" LLVMValueRef U1_ConstDataArray(LLVMTypeRef t, const char* const data, const size_t size, const size_t element_count)
{
	return llvm::wrap(
		llvm::ConstantDataArray::getRaw(
			llvm::StringRef(data, size),
			element_count,
			llvm::unwrap(t)));
}
