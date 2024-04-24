#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "lang_types.hpp"

namespace U
{

bool IsUnsignedInteger( const U_FundamentalType type )
{
	return
		type == U_FundamentalType::u8_  ||
		type == U_FundamentalType::u16_ ||
		type == U_FundamentalType::u32_ ||
		type == U_FundamentalType::u64_ ||
		type == U_FundamentalType::u128_ ||
		type == U_FundamentalType::size_type_;
}

bool IsSignedInteger( const U_FundamentalType type )
{
	return
		type == U_FundamentalType::i8_  ||
		type == U_FundamentalType::i16_ ||
		type == U_FundamentalType::i32_ ||
		type == U_FundamentalType::i64_ ||
		type == U_FundamentalType::i128_ ||
		type == U_FundamentalType::ssize_type_;
}

bool IsInteger( const U_FundamentalType type )
{
	return IsSignedInteger( type ) || IsUnsignedInteger( type );
}

bool IsFloatingPoint( U_FundamentalType type )
{
	return
		type == U_FundamentalType::f32_ ||
		type == U_FundamentalType::f64_;
}

bool IsNumericType( const U_FundamentalType type )
{
	return IsInteger( type ) || IsFloatingPoint( type );
}

bool IsChar( const U_FundamentalType type )
{
	return
		type == U_FundamentalType::char8_  ||
		type == U_FundamentalType::char16_ ||
		type == U_FundamentalType::char32_;
}

bool IsByte( const U_FundamentalType type )
{
	return
		type == U_FundamentalType::byte8_  ||
		type == U_FundamentalType::byte16_ ||
		type == U_FundamentalType::byte32_ ||
		type == U_FundamentalType::byte64_ ||
		type == U_FundamentalType::byte128_;
}

const std::string_view g_invalid_type_name= "InvalidType";

std::string_view GetFundamentalTypeName( const U_FundamentalType type )
{
	switch(type)
	{
	case U_FundamentalType::InvalidType: return g_invalid_type_name;
	case U_FundamentalType::void_: return Keyword( Keywords::void_ );
	case U_FundamentalType::bool_: return Keyword( Keywords::bool_ );
	case U_FundamentalType::i8_  : return Keyword( Keywords::i8_   );
	case U_FundamentalType::u8_  : return Keyword( Keywords::u8_   );
	case U_FundamentalType::i16_ : return Keyword( Keywords::i16_  );
	case U_FundamentalType::u16_ : return Keyword( Keywords::u16_  );
	case U_FundamentalType::i32_ : return Keyword( Keywords::i32_  );
	case U_FundamentalType::u32_ : return Keyword( Keywords::u32_  );
	case U_FundamentalType::i64_ : return Keyword( Keywords::i64_  );
	case U_FundamentalType::u64_ : return Keyword( Keywords::u64_  );
	case U_FundamentalType::i128_: return Keyword( Keywords::i128_ );
	case U_FundamentalType::u128_: return Keyword( Keywords::u128_ );
	case U_FundamentalType::ssize_type_: return Keyword( Keywords::ssize_type_ );
	case U_FundamentalType::size_type_ : return Keyword( Keywords::size_type_  );
	case U_FundamentalType::f32_: return Keyword( Keywords::f32_ );
	case U_FundamentalType::f64_: return Keyword( Keywords::f64_ );
	case U_FundamentalType::char8_ : return Keyword( Keywords::char8_  );
	case U_FundamentalType::char16_: return Keyword( Keywords::char16_ );
	case U_FundamentalType::char32_: return Keyword( Keywords::char32_ );
	case U_FundamentalType::byte8_   : return Keyword( Keywords::byte8_   );
	case U_FundamentalType::byte16_  : return Keyword( Keywords::byte16_  );
	case U_FundamentalType::byte32_  : return Keyword( Keywords::byte32_  );
	case U_FundamentalType::byte64_  : return Keyword( Keywords::byte64_  );
	case U_FundamentalType::byte128_ : return Keyword( Keywords::byte128_ );
	case U_FundamentalType::LastType: break;
	};

	U_ASSERT( false );
	return g_invalid_type_name;
}

U_FundamentalType GetFundamentalTypeByName( const std::string_view name )
{
	// Use here a lot of compare operations, instead of std::unordered_map, because
	// unordered maps needs be constructed after construction of keywords list.
	// But for garantee of construction order we needs use static function variable, that requires mutex, which is slow.
	if( name == Keywords::void_ ) return U_FundamentalType::void_;
	if( name == Keywords::bool_ ) return U_FundamentalType::bool_;
	if( name == Keywords::i8_   ) return U_FundamentalType::i8_  ;
	if( name == Keywords::u8_   ) return U_FundamentalType::u8_  ;
	if( name == Keywords::i16_  ) return U_FundamentalType::i16_ ;
	if( name == Keywords::u16_  ) return U_FundamentalType::u16_ ;
	if( name == Keywords::i32_  ) return U_FundamentalType::i32_ ;
	if( name == Keywords::u32_  ) return U_FundamentalType::u32_ ;
	if( name == Keywords::i64_  ) return U_FundamentalType::i64_ ;
	if( name == Keywords::u64_  ) return U_FundamentalType::u64_ ;
	if( name == Keywords::i128_ ) return U_FundamentalType::i128_;
	if( name == Keywords::u128_ ) return U_FundamentalType::u128_;
	if( name == Keywords::ssize_type_ ) return U_FundamentalType::ssize_type_;
	if( name == Keywords::size_type_  ) return U_FundamentalType::size_type_ ;
	if( name == Keywords::f32_ ) return U_FundamentalType::f32_;
	if( name == Keywords::f64_ ) return U_FundamentalType::f64_;
	if( name == Keywords::char8_  ) return U_FundamentalType::char8_;
	if( name == Keywords::char16_ ) return U_FundamentalType::char16_;
	if( name == Keywords::char32_ ) return U_FundamentalType::char32_;
	if( name == Keywords::byte8_   ) return U_FundamentalType::byte8_  ;
	if( name == Keywords::byte16_  ) return U_FundamentalType::byte16_ ;
	if( name == Keywords::byte32_  ) return U_FundamentalType::byte32_ ;
	if( name == Keywords::byte64_  ) return U_FundamentalType::byte64_ ;
	if( name == Keywords::byte128_ ) return U_FundamentalType::byte128_;

	return U_FundamentalType::InvalidType;
}

} // namespace U
