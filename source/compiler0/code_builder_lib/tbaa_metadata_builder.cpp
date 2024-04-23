#include "tbaa_metadata_builder.hpp"

#include "../../code_builder_lib_common/string_ref.hpp"
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
	const bool is_32_bit= data_layout.getIntPtrType(llvm_context)->getIntegerBitWidth() == 32u;

	llvm::MDNode* const tbaa_root= md_builder_.createTBAARoot( "__U_tbaa_root" );

	// byte8 is a base type for all other byte types.
	// byteN type is base for fundamental types with size N.

	type_descriptors_.byte8_  = md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::byte8_   ) ), tbaa_root );
	type_descriptors_.byte16_ = md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::byte16_  ) ), type_descriptors_.byte8_  );
	type_descriptors_.byte32_ = md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::byte32_  ) ), type_descriptors_.byte16_ );
	type_descriptors_.byte64_ = md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::byte64_  ) ), type_descriptors_.byte32_ );
	type_descriptors_.byte128_= md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::byte128_ ) ), type_descriptors_.byte64_ );

	type_descriptors_.void_= md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::void_ ) ), type_descriptors_.byte8_ );
	type_descriptors_.bool_= md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::bool_ ) ), type_descriptors_.byte8_ );

	type_descriptors_.i8_  = md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::i8_   ) ), type_descriptors_.byte8_   );
	type_descriptors_.u8_  = md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::u8_   ) ), type_descriptors_.byte8_   );
	type_descriptors_.i16_ = md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::i16_  ) ), type_descriptors_.byte16_  );
	type_descriptors_.u16_ = md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::u16_  ) ), type_descriptors_.byte16_  );
	type_descriptors_.i32_ = md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::i32_  ) ), type_descriptors_.byte32_  );
	type_descriptors_.u32_ = md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::u32_  ) ), type_descriptors_.byte32_  );
	type_descriptors_.i64_ = md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::i64_  ) ), type_descriptors_.byte64_  );
	type_descriptors_.u64_ = md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::u64_  ) ), type_descriptors_.byte64_  );
	type_descriptors_.i128_= md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::i128_ ) ), type_descriptors_.byte128_ );
	type_descriptors_.u128_= md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::u128_ ) ), type_descriptors_.byte128_ );

	type_descriptors_.ssize_type_=
		md_builder_.createTBAAScalarTypeNode(
			StringViewToStringRef( Keyword( Keywords::ssize_type_ ) ),
			is_32_bit ? type_descriptors_.byte32_ : type_descriptors_.byte64_ );

	type_descriptors_.size_type_=
		md_builder_.createTBAAScalarTypeNode(
			StringViewToStringRef( Keyword( Keywords::size_type_ ) ),
			is_32_bit ? type_descriptors_.byte32_ : type_descriptors_.byte64_ );

	type_descriptors_.char8_ = md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::char8_  ) ), type_descriptors_.byte8_  );
	type_descriptors_.char16_= md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::char16_ ) ), type_descriptors_.byte16_ );
	type_descriptors_.char32_= md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::char32_ ) ), type_descriptors_.byte32_ );

	type_descriptors_.f32_= md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::f32_ ) ), type_descriptors_.byte32_ );
	type_descriptors_.f64_= md_builder_.createTBAAScalarTypeNode( StringViewToStringRef( Keyword( Keywords::f64_ ) ), type_descriptors_.byte64_ );

	const auto ptr_base= is_32_bit ? type_descriptors_.byte32_ : type_descriptors_.byte64_;

	// Use intermediate type for all pointer type (not just raw byte32 or byte64).
	// Do this in case we add something, like "generic" pointers/references.
	type_descriptors_.ptr= md_builder_.createTBAAScalarTypeNode( "__U_any_pointer", ptr_base );

	type_descriptors_.size_type_enum_base= is_32_bit ? type_descriptors_.byte32_ : type_descriptors_.byte64_;
}

llvm::MDNode* TBAAMetadataBuilder::CreateAccessTag( const Type& type )
{
	llvm::MDNode* const type_descriptor= GetTypeDescriptor( type );

	// Clang uses this function instead of createTBAAAccessTag. I do not know, is this correct or not, but it seems to be working.
	// TODO - set "IsConstant" flag.
	return md_builder_.createTBAAStructTagNode( type_descriptor, type_descriptor, 0 );
}

llvm::MDNode* TBAAMetadataBuilder::CreateReferenceAccessTag( const Type& type )
{
	// Create access tags for references as for pointers.
	RawPointerType raw_pointer_type;
	raw_pointer_type.element_type= type;

	return CreateAccessTag( std::move(raw_pointer_type) );
}

llvm::MDNode* TBAAMetadataBuilder::CreateVirtualTablePointerAccessTag()
{
	// Use just base type for all pointers as access tag for virtual table pointers.
	// Do this in order to allow read virtual table pointers properly via any pointer/reference type.
	// This is mostly needed in standard library helpers for polymorph classes.
	const auto type_descriptor= type_descriptors_.ptr;
	return md_builder_.createTBAAStructTagNode( type_descriptor, type_descriptor, 0 );
}

llvm::MDNode* TBAAMetadataBuilder::CreateVirtualTableFunctionPointerAccessTag()
{
	// Use generic pointer type for virtual table fetches. It's important, becase for simplicity reasons pointers in virtual table are almost untyped.
	const auto type_descriptor= type_descriptors_.ptr;
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
		return md_builder_.createTBAAScalarTypeNode( name, type_descriptors_.ptr );
	if( type.GetFunctionPointerType() != nullptr )
		return md_builder_.createTBAAScalarTypeNode( name, type_descriptors_.ptr );

	// TODO - support another type kinds. Composite types descriptors are needed for struct-path TBAA.
	return type_descriptors_.byte8_;
}

llvm::MDNode* TBAAMetadataBuilder::GetTypeDescriptorForFundamentalType( const U_FundamentalType fundamental_type )
{
	switch(fundamental_type)
	{
	case U_FundamentalType::InvalidType: return type_descriptors_.byte8_;
	case U_FundamentalType::void_: return type_descriptors_.void_;
	case U_FundamentalType::bool_: return type_descriptors_.bool_;
	case U_FundamentalType::i8_  : return type_descriptors_.i8_  ;
	case U_FundamentalType::u8_  : return type_descriptors_.u8_  ;
	case U_FundamentalType::i16_ : return type_descriptors_.i16_ ;
	case U_FundamentalType::u16_ : return type_descriptors_.u16_ ;
	case U_FundamentalType::i32_ : return type_descriptors_.i32_ ;
	case U_FundamentalType::u32_ : return type_descriptors_.u32_ ;
	case U_FundamentalType::i64_ : return type_descriptors_.i64_ ;
	case U_FundamentalType::u64_ : return type_descriptors_.u64_ ;
	case U_FundamentalType::i128_: return type_descriptors_.i128_;
	case U_FundamentalType::u128_: return type_descriptors_.u128_;
	case U_FundamentalType::ssize_type_: return type_descriptors_.ssize_type_;
	case U_FundamentalType::size_type_: return type_descriptors_.size_type_;
	case U_FundamentalType::f32_: return type_descriptors_.f32_;
	case U_FundamentalType::f64_: return type_descriptors_.f64_;
	case U_FundamentalType::char8_ : return type_descriptors_.char8_ ;
	case U_FundamentalType::char16_: return type_descriptors_.char16_;
	case U_FundamentalType::char32_: return type_descriptors_.char32_;
	case U_FundamentalType::byte8_   : return type_descriptors_.byte8_  ;
	case U_FundamentalType::byte16_  : return type_descriptors_.byte16_ ;
	case U_FundamentalType::byte32_  : return type_descriptors_.byte32_ ;
	case U_FundamentalType::byte64_  : return type_descriptors_.byte64_ ;
	case U_FundamentalType::byte128_ : return type_descriptors_.byte128_;
	case U_FundamentalType::LastType: break;
	}

	U_ASSERT(false);
	return type_descriptors_.byte8_;
}

llvm::MDNode* TBAAMetadataBuilder::GetEnumTypeBaseTypeDescriptor( const EnumPtr enum_type )
{
	switch( enum_type->underlying_type.fundamental_type )
	{
	case U_FundamentalType::i8_  :
	case U_FundamentalType::u8_  :
		return type_descriptors_.byte8_ ;
	case U_FundamentalType::i16_ :
	case U_FundamentalType::u16_ :
		return type_descriptors_.byte16_ ;
	case U_FundamentalType::i32_ :
	case U_FundamentalType::u32_ :
		return type_descriptors_.byte32_ ;
	case U_FundamentalType::i64_ :
	case U_FundamentalType::u64_ :
		return type_descriptors_.byte64_ ;
	case U_FundamentalType::i128_:
	case U_FundamentalType::u128_:
		return type_descriptors_.byte128_;
	case U_FundamentalType::ssize_type_:
	case U_FundamentalType::size_type_:
		return type_descriptors_.size_type_enum_base;
	default:
		U_ASSERT(false);
		return type_descriptors_.byte8_;
	}
}

} // namespace U
