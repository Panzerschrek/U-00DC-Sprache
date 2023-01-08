#include "tbaa_metadada_builder.hpp"

#include "keywords.hpp"

namespace U
{

TBAAMetadataBuilder::TBAAMetadataBuilder( llvm::LLVMContext& llvm_context )
	: md_builder_(llvm_context)
{
	tbaa_root_= md_builder_.createTBAARoot( "__U_tbaa_root" );

	// byte8 is a base type for all other byte types.
	// byteN type is base for fundamental types with size N.

	funamental_types_descriptors_.byte8_  = md_builder_.createTBAANode( Keyword( Keywords::byte8_   ), tbaa_root_ );
	funamental_types_descriptors_.byte16_ = md_builder_.createTBAANode( Keyword( Keywords::byte16_  ), funamental_types_descriptors_.byte8_   );
	funamental_types_descriptors_.byte32_ = md_builder_.createTBAANode( Keyword( Keywords::byte32_  ), funamental_types_descriptors_.byte16_  );
	funamental_types_descriptors_.byte64_ = md_builder_.createTBAANode( Keyword( Keywords::byte64_  ), funamental_types_descriptors_.byte32_  );
	funamental_types_descriptors_.byte128_= md_builder_.createTBAANode( Keyword( Keywords::byte128_ ), funamental_types_descriptors_.byte64_  );

	funamental_types_descriptors_.void_= md_builder_.createTBAANode( Keyword( Keywords::void_ ), funamental_types_descriptors_.byte8_ );
	funamental_types_descriptors_.bool_= md_builder_.createTBAANode( Keyword( Keywords::bool_ ), funamental_types_descriptors_.byte8_ );

	funamental_types_descriptors_.i8_  = md_builder_.createTBAANode( Keyword( Keywords::i8_   ), funamental_types_descriptors_.byte8_   );
	funamental_types_descriptors_.u8_  = md_builder_.createTBAANode( Keyword( Keywords::u8_   ), funamental_types_descriptors_.byte8_   );
	funamental_types_descriptors_.i16_ = md_builder_.createTBAANode( Keyword( Keywords::i16_  ), funamental_types_descriptors_.byte16_  );
	funamental_types_descriptors_.u16_ = md_builder_.createTBAANode( Keyword( Keywords::u16_  ), funamental_types_descriptors_.byte16_  );
	funamental_types_descriptors_.i32_ = md_builder_.createTBAANode( Keyword( Keywords::i32_  ), funamental_types_descriptors_.byte32_  );
	funamental_types_descriptors_.u32_ = md_builder_.createTBAANode( Keyword( Keywords::u32_  ), funamental_types_descriptors_.byte32_  );
	funamental_types_descriptors_.i64_ = md_builder_.createTBAANode( Keyword( Keywords::i64_  ), funamental_types_descriptors_.byte64_  );
	funamental_types_descriptors_.u64_ = md_builder_.createTBAANode( Keyword( Keywords::u64_  ), funamental_types_descriptors_.byte64_  );
	funamental_types_descriptors_.i128_= md_builder_.createTBAANode( Keyword( Keywords::i128_ ), funamental_types_descriptors_.byte128_ );
	funamental_types_descriptors_.u128_= md_builder_.createTBAANode( Keyword( Keywords::u128_ ), funamental_types_descriptors_.byte128_ );

	funamental_types_descriptors_.char8_ = md_builder_.createTBAANode( Keyword( Keywords::char8_  ), funamental_types_descriptors_.byte8_  );
	funamental_types_descriptors_.char16_= md_builder_.createTBAANode( Keyword( Keywords::char16_ ), funamental_types_descriptors_.byte16_ );
	funamental_types_descriptors_.char32_= md_builder_.createTBAANode( Keyword( Keywords::char32_ ), funamental_types_descriptors_.byte32_ );

	funamental_types_descriptors_.f32_= md_builder_.createTBAANode( Keyword( Keywords::f32_ ), funamental_types_descriptors_.byte32_ );
	funamental_types_descriptors_.f64_= md_builder_.createTBAANode( Keyword( Keywords::f64_ ), funamental_types_descriptors_.byte64_ );
}

} // namespace U
