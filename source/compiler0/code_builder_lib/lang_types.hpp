#pragma once
#include <cstdint>

namespace U
{

// Fundamental language types.
enum class U_FundamentalType
{
	InvalidType,
	Void,
	Bool,

	i8 ,
	u8 ,
	i16,
	u16,
	i32,
	u32,
	i64,
	u64,
	i128,
	u128,

	f32,
	f64,

	char8,
	char16,
	char32,

	byte8,
	byte16,
	byte32,
	byte64,
	byte128,

	LastType,
};

bool IsUnsignedInteger( U_FundamentalType type );
bool IsSignedInteger( U_FundamentalType type );
bool IsInteger( U_FundamentalType type );
bool IsFloatingPoint( U_FundamentalType type );
bool IsNumericType( U_FundamentalType type );
bool IsChar( U_FundamentalType type );
bool IsByte( U_FundamentalType type );

const std::string& GetFundamentalTypeName( U_FundamentalType type );
U_FundamentalType GetFundamentalTypeByName( const std::string& name );

} // namespace U
