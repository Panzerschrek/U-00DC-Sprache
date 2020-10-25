#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "lang_types.hpp"

namespace U
{

bool IsUnsignedInteger( const U_FundamentalType type )
{
	return
		type == U_FundamentalType::u8  ||
		type == U_FundamentalType::u16 ||
		type == U_FundamentalType::u32 ||
		type == U_FundamentalType::u64 ||
		type == U_FundamentalType::u128;
}

bool IsSignedInteger( const U_FundamentalType type )
{
	return
		type == U_FundamentalType::i8  ||
		type == U_FundamentalType::i16 ||
		type == U_FundamentalType::i32 ||
		type == U_FundamentalType::i64 ||
		type == U_FundamentalType::i128;
}

bool IsInteger( const U_FundamentalType type )
{
	return IsSignedInteger( type ) || IsUnsignedInteger( type );
}

bool IsFloatingPoint( U_FundamentalType type )
{
	return
		type == U_FundamentalType::f32 ||
		type == U_FundamentalType::f64;
}

bool IsNumericType( const U_FundamentalType type )
{
	return IsInteger( type ) || IsFloatingPoint( type );
}

bool IsChar( const U_FundamentalType type )
{
	return
		type == U_FundamentalType::char8  ||
		type == U_FundamentalType::char16 ||
		type == U_FundamentalType::char32;
}

const std::string g_invalid_type_name= "InvalidType";

const std::string& GetFundamentalTypeName( const U_FundamentalType type )
{
	switch(type)
	{
	case U_FundamentalType::InvalidType: return g_invalid_type_name;
	case U_FundamentalType::Void: return Keyword( Keywords::void_ );
	case U_FundamentalType::Bool: return Keyword( Keywords::bool_ );
	case U_FundamentalType::i8  : return Keyword( Keywords::i8_ );
	case U_FundamentalType::u8  : return Keyword( Keywords::u8_ );
	case U_FundamentalType::i16 : return Keyword( Keywords::i16_ );
	case U_FundamentalType::u16 : return Keyword( Keywords::u16_ );
	case U_FundamentalType::i32 : return Keyword( Keywords::i32_ );
	case U_FundamentalType::u32 : return Keyword( Keywords::u32_ );
	case U_FundamentalType::i64 : return Keyword( Keywords::i64_ );
	case U_FundamentalType::u64 : return Keyword( Keywords::u64_ );
	case U_FundamentalType::i128: return Keyword( Keywords::i128_ );
	case U_FundamentalType::u128: return Keyword( Keywords::u128_ );
	case U_FundamentalType::f32: return Keyword( Keywords::f32_ );
	case U_FundamentalType::f64: return Keyword( Keywords::f64_ );
	case U_FundamentalType::char8 : return Keyword( Keywords::char8_  );
	case U_FundamentalType::char16: return Keyword( Keywords::char16_ );
	case U_FundamentalType::char32: return Keyword( Keywords::char32_ );
	case U_FundamentalType::LastType: break;
	};

	U_ASSERT( false );
	return g_invalid_type_name;
}

U_FundamentalType GetFundamentalTypeByName( const std::string& name )
{
	// Use here a lot of compare operations, instead of std::unordered_map, because
	// unordered maps needs be constructed after construction of keywords list.
	// But for garantee of construction order we needs use static function variable, that requires mutex, which is slow.
	if( name ==  Keywords::void_ ) return U_FundamentalType::Void;
	if( name == Keywords::bool_ ) return U_FundamentalType::Bool;
	if( name == Keywords::i8_   ) return U_FundamentalType::i8;
	if( name == Keywords::u8_   ) return U_FundamentalType::u8;
	if( name == Keywords::i16_  ) return U_FundamentalType::i16;
	if( name == Keywords::u16_  ) return U_FundamentalType::u16;
	if( name == Keywords::i32_  ) return U_FundamentalType::i32;
	if( name == Keywords::u32_  ) return U_FundamentalType::u32;
	if( name == Keywords::i64_  ) return U_FundamentalType::i64;
	if( name == Keywords::u64_  ) return U_FundamentalType::u64;
	if( name == Keywords::i128_ ) return U_FundamentalType::i128;
	if( name == Keywords::u128_ ) return U_FundamentalType::u128;
	if( name == Keywords::f32_ ) return U_FundamentalType::f32;
	if( name == Keywords::f64_ ) return U_FundamentalType::f64;
	if( name == Keywords::char8_  ) return U_FundamentalType::char8;
	if( name == Keywords::char16_ ) return U_FundamentalType::char16;
	if( name == Keywords::char32_ ) return U_FundamentalType::char32;

	return U_FundamentalType::InvalidType;
}

} // namespace U
