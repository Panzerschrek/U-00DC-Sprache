#pragma once

#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/MDBuilder.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

namespace U
{

class TBAAMetadataBuilder
{
public:
	explicit TBAAMetadataBuilder( llvm::LLVMContext& llvm_context );

private:
	llvm::MDBuilder md_builder_;

	llvm::MDNode* tbaa_root_= nullptr;

	struct
	{
		llvm::MDNode* byte8_  = nullptr;
		llvm::MDNode* byte16_ = nullptr;
		llvm::MDNode* byte32_ = nullptr;
		llvm::MDNode* byte64_ = nullptr;
		llvm::MDNode* byte128_= nullptr;

		llvm::MDNode* void_= nullptr;
		llvm::MDNode* bool_= nullptr;

		llvm::MDNode* i8_  = nullptr;
		llvm::MDNode* u8_  = nullptr;
		llvm::MDNode* i16_ = nullptr;
		llvm::MDNode* u16_ = nullptr;
		llvm::MDNode* i32_ = nullptr;
		llvm::MDNode* u32_ = nullptr;
		llvm::MDNode* i64_ = nullptr;
		llvm::MDNode* u64_ = nullptr;
		llvm::MDNode* i128_= nullptr;
		llvm::MDNode* u128_= nullptr;

		llvm::MDNode* char8_ = nullptr;
		llvm::MDNode* char16_= nullptr;
		llvm::MDNode* char32_= nullptr;

		llvm::MDNode* f32_= nullptr;
		llvm::MDNode* f64_= nullptr;

	} funamental_types_descriptors_;
};

} // namespace U
