#pragma once
#include <cstdint>

namespace U
{

// Fundamental language types.
enum class U_FundamentalType
{
	InvalidType,

	void_,
	bool_,

	i8_  ,
	u8_  ,
	i16_ ,
	u16_ ,
	i32_ ,
	u32_ ,
	i64_ ,
	u64_ ,
	i128_,
	u128_,

	f32_,
	f64_,

	char8_ ,
	char16_,
	char32_,

	byte8_  ,
	byte16_ ,
	byte32_ ,
	byte64_ ,
	byte128_,

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
