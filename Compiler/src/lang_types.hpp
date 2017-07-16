#pragma once

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

	f32,
	f64,

	LastType,
};

bool IsUnsignedInteger( U_FundamentalType type );
bool IsSignedInteger( U_FundamentalType type );
bool IsInteger( U_FundamentalType type );
bool IsFloatingPoint( U_FundamentalType type );
bool IsNumericType( U_FundamentalType type );

} // namespace U
