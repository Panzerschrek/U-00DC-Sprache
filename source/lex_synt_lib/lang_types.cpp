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

} // namespace U
