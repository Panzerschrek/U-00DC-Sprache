#include "tbaa_metadada_builder.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"

#include "enum.hpp"

namespace U
{

TBAAMetadataBuilder::TBAAMetadataBuilder(
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	std::shared_ptr<IMangler> mangler )
	: data_layout_(data_layout)
	, mangler_( std::move(mangler) )
	, md_builder_(llvm_context)
{
	llvm::MDNode* const tbaa_root= md_builder_.createTBAARoot( "__U_tbaa_root" );

	// byte8 is a base type for all other byte types.
	// byteN type is base for fundamental types with size N.

	fundamental_types_descriptors_.byte8_  = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::byte8_   ), tbaa_root );
	fundamental_types_descriptors_.byte16_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::byte16_  ), fundamental_types_descriptors_.byte8_   );
	fundamental_types_descriptors_.byte32_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::byte32_  ), fundamental_types_descriptors_.byte16_  );
	fundamental_types_descriptors_.byte64_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::byte64_  ), fundamental_types_descriptors_.byte32_  );
	fundamental_types_descriptors_.byte128_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::byte128_ ), fundamental_types_descriptors_.byte64_  );

	fundamental_types_descriptors_.void_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::void_ ), fundamental_types_descriptors_.byte8_ );
	fundamental_types_descriptors_.bool_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::bool_ ), fundamental_types_descriptors_.byte8_ );

	fundamental_types_descriptors_.i8_  = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::i8_   ), fundamental_types_descriptors_.byte8_   );
	fundamental_types_descriptors_.u8_  = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::u8_   ), fundamental_types_descriptors_.byte8_   );
	fundamental_types_descriptors_.i16_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::i16_  ), fundamental_types_descriptors_.byte16_  );
	fundamental_types_descriptors_.u16_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::u16_  ), fundamental_types_descriptors_.byte16_  );
	fundamental_types_descriptors_.i32_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::i32_  ), fundamental_types_descriptors_.byte32_  );
	fundamental_types_descriptors_.u32_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::u32_  ), fundamental_types_descriptors_.byte32_  );
	fundamental_types_descriptors_.i64_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::i64_  ), fundamental_types_descriptors_.byte64_  );
	fundamental_types_descriptors_.u64_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::u64_  ), fundamental_types_descriptors_.byte64_  );
	fundamental_types_descriptors_.i128_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::i128_ ), fundamental_types_descriptors_.byte128_ );
	fundamental_types_descriptors_.u128_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::u128_ ), fundamental_types_descriptors_.byte128_ );

	fundamental_types_descriptors_.char8_ = md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::char8_  ), fundamental_types_descriptors_.byte8_  );
	fundamental_types_descriptors_.char16_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::char16_ ), fundamental_types_descriptors_.byte16_ );
	fundamental_types_descriptors_.char32_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::char32_ ), fundamental_types_descriptors_.byte32_ );

	fundamental_types_descriptors_.f32_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::f32_ ), fundamental_types_descriptors_.byte32_ );
	fundamental_types_descriptors_.f64_= md_builder_.createTBAAScalarTypeNode( Keyword( Keywords::f64_ ), fundamental_types_descriptors_.byte64_ );

	fundamental_types_descriptors_.ptr_=
		data_layout_.getIntPtrType(llvm_context)->getIntegerBitWidth() == 32u
			? fundamental_types_descriptors_.byte32_
			: fundamental_types_descriptors_.byte64_;
}

llvm::MDNode* TBAAMetadataBuilder::CreateAccessTag( const Type& type )
{
	llvm::MDNode* const type_descriptor= GetTypeDescriptor( type );

	// Calng uses this function instead of createTBAAAccessTag. I do not know, how it is correct, but it seems to be working.
	// TODO - set "IsConstant" flag.
	return md_builder_.createTBAAStructTagNode( type_descriptor, type_descriptor, 0 );
}

llvm::MDNode* TBAAMetadataBuilder::GetTypeDescriptor( const Type& type )
{
	if( const auto it= types_dscriptors_cache_.find(type); it != types_dscriptors_cache_.end() )
	{
		return it->second;
	}

	llvm::MDNode* const descriptor= CreateTypeDescriptor(type);
	types_dscriptors_cache_[type]= descriptor;
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
		return md_builder_.createTBAAScalarTypeNode( name, fundamental_types_descriptors_.ptr_ );
	if( type.GetFunctionPointerType() != nullptr )
		return md_builder_.createTBAAScalarTypeNode( name, fundamental_types_descriptors_.ptr_ );

	// TODO - support another kinds.
	return fundamental_types_descriptors_.byte8_;
}

llvm::MDNode* TBAAMetadataBuilder::GetTypeDescriptorForFundamentalType( const U_FundamentalType fundamental_type )
{
	switch(fundamental_type)
	{
	case U_FundamentalType::InvalidType: return fundamental_types_descriptors_.byte8_;
	case U_FundamentalType::Void: return fundamental_types_descriptors_.void_;
	case U_FundamentalType::Bool: return fundamental_types_descriptors_.bool_;
	case U_FundamentalType::i8  : return fundamental_types_descriptors_.i8_  ;
	case U_FundamentalType::u8  : return fundamental_types_descriptors_.u8_  ;
	case U_FundamentalType::i16 : return fundamental_types_descriptors_.i16_ ;
	case U_FundamentalType::u16 : return fundamental_types_descriptors_.u16_ ;
	case U_FundamentalType::i32 : return fundamental_types_descriptors_.i32_ ;
	case U_FundamentalType::u32 : return fundamental_types_descriptors_.u32_ ;
	case U_FundamentalType::i64 : return fundamental_types_descriptors_.i64_ ;
	case U_FundamentalType::u64 : return fundamental_types_descriptors_.u64_ ;
	case U_FundamentalType::i128: return fundamental_types_descriptors_.i128_;
	case U_FundamentalType::u128: return fundamental_types_descriptors_.u128_;
	case U_FundamentalType::f32: return fundamental_types_descriptors_.f32_;
	case U_FundamentalType::f64: return fundamental_types_descriptors_.f64_;
	case U_FundamentalType::char8 : return fundamental_types_descriptors_.char8_ ;
	case U_FundamentalType::char16: return fundamental_types_descriptors_.char16_;
	case U_FundamentalType::char32: return fundamental_types_descriptors_.char32_;
	case U_FundamentalType::byte8   : return fundamental_types_descriptors_.byte8_  ;
	case U_FundamentalType::byte16  : return fundamental_types_descriptors_.byte16_ ;
	case U_FundamentalType::byte32  : return fundamental_types_descriptors_.byte32_ ;
	case U_FundamentalType::byte64  : return fundamental_types_descriptors_.byte64_ ;
	case U_FundamentalType::byte128 : return fundamental_types_descriptors_.byte128_;
	case U_FundamentalType::LastType: break;
	}

	U_ASSERT(false);
	return fundamental_types_descriptors_.byte8_;
}

llvm::MDNode* TBAAMetadataBuilder::GetEnumTypeBaseTypeDescriptor( const EnumPtr enum_type )
{
	switch( enum_type->underlaying_type.fundamental_type )
	{
	case U_FundamentalType::i8  :
	case U_FundamentalType::u8  :
		return fundamental_types_descriptors_.byte8_ ;
	case U_FundamentalType::i16 :
	case U_FundamentalType::u16 :
		return fundamental_types_descriptors_.byte16_ ;
	case U_FundamentalType::i32 :
	case U_FundamentalType::u32 :
		return fundamental_types_descriptors_.byte32_ ;
	case U_FundamentalType::i64 :
	case U_FundamentalType::u64 :
		return fundamental_types_descriptors_.byte64_ ;
	case U_FundamentalType::i128:
	case U_FundamentalType::u128:
		return fundamental_types_descriptors_.byte128_;
	default:
		break;
	}

	U_ASSERT(false);
	return fundamental_types_descriptors_.byte8_;
}

} // namespace U
