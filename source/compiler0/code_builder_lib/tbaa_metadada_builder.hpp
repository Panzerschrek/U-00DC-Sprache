#pragma once

#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/MDBuilder.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "mangling.hpp"
#include "type.hpp"

namespace U
{

class TBAAMetadataBuilder
{
public:
	explicit TBAAMetadataBuilder(
		llvm::LLVMContext& llvm_context,
		const llvm::DataLayout& data_layout,
		std::shared_ptr<IMangler> mangler );

	llvm::MDNode* CreateAccessTag( const Type& type );
	llvm::MDNode* CreateReferenceAccessTag( const Type& type );

	llvm::MDNode* CreateVirtualTablePointerAccessTag();

private:
	llvm::MDNode* GetTypeDescriptor( const Type& type );
	llvm::MDNode* CreateTypeDescriptor( const Type& type );

	llvm::MDNode* GetTypeDescriptorForFundamentalType( U_FundamentalType fundamental_type );
	llvm::MDNode* GetEnumTypeBaseTypeDescriptor( EnumPtr enum_type );

private:
	const llvm::DataLayout data_layout_;
	const std::shared_ptr<IMangler> mangler_;
	llvm::MDBuilder md_builder_;

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

		// Base for all pointers.
		llvm::MDNode* ptr_= nullptr;

	} fundamental_types_descriptors_;

	std::unordered_map< Type, llvm::MDNode*, TypeHasher > types_dscriptors_cache_;
};

} // namespace U
