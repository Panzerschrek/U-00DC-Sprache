#include "tbaa_metadata_builder.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"

#include "enum.hpp"

namespace U
{

TBAAMetadataBuilder::TBAAMetadataBuilder(
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	std::shared_ptr<IMangler> mangler )
	: mangler_( std::move(mangler) )
	, md_builder_(llvm_context)
{
	llvm::MDNode* const tbaa_root= md_builder_.createTBAARoot( "__U_tbaa_root" );

	// byte8 is a base type for all other byte types.
	// byteN type is base for fundamental types with size N.

	type_descriptors_.byte8_  = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::byte8_   ), tbaa_root );
	type_descriptors_.byte16_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::byte16_  ), type_descriptors_.byte8_  );
	type_descriptors_.byte32_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::byte32_  ), type_descriptors_.byte16_ );
	type_descriptors_.byte64_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::byte64_  ), type_descriptors_.byte32_ );
	type_descriptors_.byte128_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::byte128_ ), type_descriptors_.byte64_ );

	type_descriptors_.void_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::void_ ), type_descriptors_.byte8_ );
	type_descriptors_.bool_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::bool_ ), type_descriptors_.byte8_ );

	type_descriptors_.i8_  = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::i8_   ), type_descriptors_.byte8_   );
	type_descriptors_.u8_  = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::u8_   ), type_descriptors_.byte8_   );
	type_descriptors_.i16_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::i16_  ), type_descriptors_.byte16_  );
	type_descriptors_.u16_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::u16_  ), type_descriptors_.byte16_  );
	type_descriptors_.i32_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::i32_  ), type_descriptors_.byte32_  );
	type_descriptors_.u32_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::u32_  ), type_descriptors_.byte32_  );
	type_descriptors_.i64_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::i64_  ), type_descriptors_.byte64_  );
	type_descriptors_.u64_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::u64_  ), type_descriptors_.byte64_  );
	type_descriptors_.i128_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::i128_ ), type_descriptors_.byte128_ );
	type_descriptors_.u128_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::u128_ ), type_descriptors_.byte128_ );

	type_descriptors_.char8_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::char8_  ), type_descriptors_.byte8_  );
	type_descriptors_.char16_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::char16_ ), type_descriptors_.byte16_ );
	type_descriptors_.char32_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::char32_ ), type_descriptors_.byte32_ );

	type_descriptors_.f32_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::f32_ ), type_descriptors_.byte32_ );
	type_descriptors_.f64_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::f64_ ), type_descriptors_.byte64_ );

	const auto ptr_base=
		data_layout.getIntPtrType(llvm_context)->getIntegerBitWidth() == 32u
			? type_descriptors_.byte32_
			: type_descriptors_.byte64_;

	// Use intermediate type for all pointer type (not just raw byte32 or byte64).
	// Do this in case we add something, like "generic" pointers/references.
	type_descriptors_.ptr_= md_builder_.createTBAAScalarTypeNode( "__U_any_pointer", ptr_base );
}

llvm::MDNode* TBAAMetadataBuilder::CreateAccessTag( const Type& type )
{
	llvm::MDNode* const type_descriptor= GetTypeDescriptor( type );

	// Calng uses this function instead of createTBAAAccessTag. I do not know, how it is correct, but it seems to be working.
	// TODO - set "IsConstant" flag.
	return md_builder_.createTBAAStructTagNode( type_descriptor, type_descriptor, 0 );
}

llvm::MDNode* TBAAMetadataBuilder::CreateReferenceAccessTag( const Type& type )
{
	// Create access tags for references as for pointers.
	RawPointerType raw_pointer_type;
	raw_pointer_type.type= type;

	return CreateAccessTag( raw_pointer_type );
}

llvm::MDNode* TBAAMetadataBuilder::CreateVirtualTablePointerAccessTag()
{
	// Use just base type for all pointers as access tag for virtual table pointers.
	// Do this in order to allow read virtual table pointers properly via any pointer/reference type.
	// This is mostly needed in standard library helpers for polymorph classes.
	const auto type_descriptor= type_descriptors_.ptr_;
	return md_builder_.createTBAAStructTagNode( type_descriptor, type_descriptor, 0 );
}

llvm::MDNode* TBAAMetadataBuilder::CreateVirtualTableFunctionPointerAccessTag()
{
	// Use generic pointer type for virtual table fetches. It's important, becase for simplicity reasons pointers in virtual table are almost untyped.
	const auto type_descriptor= type_descriptors_.ptr_;
	return md_builder_.createTBAAStructTagNode( type_descriptor, type_descriptor, 0 );
}

llvm::MDNode* TBAAMetadataBuilder::GetTypeDescriptor( const Type& type )
{
	if( const auto it= types_descriptors_cache_.find(type); it != types_descriptors_cache_.end() )
	{
		return it->second;
	}

	llvm::MDNode* const descriptor= CreateTypeDescriptor(type);
	types_descriptors_cache_[type]= descriptor;
	return descriptor;
}

llvm::MDNode* TBAAMetadataBuilder::CreateTypeDescriptor( const Type& type )
{
	const std::string name= mangler_->MangleType(type);

	if( const auto fundamental_type= type.GetFundamentalType() )
		return GetTypeDescriptorForFundamentalType( fundamental_type->fundamental_type );
	if( const auto enum_type= type.GetEnumType() )
		return md_builder_.createTBAAScalarTypeNode( name, GetEnumTypeBaseTypeDescriptor(enum_type) );
	if( type.GetRawPointerType() != nullptr )
		return md_builder_.createTBAAScalarTypeNode( name, type_descriptors_.ptr_ );
	if( type.GetFunctionPointerType() != nullptr )
		return md_builder_.createTBAAScalarTypeNode( name, type_descriptors_.ptr_ );

	// TODO - support another kinds.
	return type_descriptors_.byte8_;
}

llvm::MDNode* TBAAMetadataBuilder::GetTypeDescriptorForFundamentalType( const U_FundamentalType fundamental_type )
{
	switch(fundamental_type)
	{
	case U_FundamentalType::InvalidType: return type_descriptors_.byte8_;
	case U_FundamentalType::Void: return type_descriptors_.void_;
	case U_FundamentalType::Bool: return type_descriptors_.bool_;
	case U_FundamentalType::i8  : return type_descriptors_.i8_  ;
	case U_FundamentalType::u8  : return type_descriptors_.u8_  ;
	case U_FundamentalType::i16 : return type_descriptors_.i16_ ;
	case U_FundamentalType::u16 : return type_descriptors_.u16_ ;
	case U_FundamentalType::i32 : return type_descriptors_.i32_ ;
	case U_FundamentalType::u32 : return type_descriptors_.u32_ ;
	case U_FundamentalType::i64 : return type_descriptors_.i64_ ;
	case U_FundamentalType::u64 : return type_descriptors_.u64_ ;
	case U_FundamentalType::i128: return type_descriptors_.i128_;
	case U_FundamentalType::u128: return type_descriptors_.u128_;
	case U_FundamentalType::f32: return type_descriptors_.f32_;
	case U_FundamentalType::f64: return type_descriptors_.f64_;
	case U_FundamentalType::char8 : return type_descriptors_.char8_ ;
	case U_FundamentalType::char16: return type_descriptors_.char16_;
	case U_FundamentalType::char32: return type_descriptors_.char32_;
	case U_FundamentalType::byte8   : return type_descriptors_.byte8_  ;
	case U_FundamentalType::byte16  : return type_descriptors_.byte16_ ;
	case U_FundamentalType::byte32  : return type_descriptors_.byte32_ ;
	case U_FundamentalType::byte64  : return type_descriptors_.byte64_ ;
	case U_FundamentalType::byte128 : return type_descriptors_.byte128_;
	case U_FundamentalType::LastType: break;
	}

	U_ASSERT(false);
	return type_descriptors_.byte8_;
}

llvm::MDNode* TBAAMetadataBuilder::GetEnumTypeBaseTypeDescriptor( const EnumPtr enum_type )
{
	switch( enum_type->underlaying_type.fundamental_type )
	{
	case U_FundamentalType::i8  :
	case U_FundamentalType::u8  :
		return type_descriptors_.byte8_ ;
	case U_FundamentalType::i16 :
	case U_FundamentalType::u16 :
		return type_descriptors_.byte16_ ;
	case U_FundamentalType::i32 :
	case U_FundamentalType::u32 :
		return type_descriptors_.byte32_ ;
	case U_FundamentalType::i64 :
	case U_FundamentalType::u64 :
		return type_descriptors_.byte64_ ;
	case U_FundamentalType::i128:
	case U_FundamentalType::u128:
		return type_descriptors_.byte128_;
	default:
		break;
	}

	U_ASSERT(false);
	return type_descriptors_.byte8_;
}

} // namespace U
