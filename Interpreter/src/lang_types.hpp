#pragma once

namespace Interpreter
{

// C++ representation of language types.
typedef void U_void;
typedef bool U_bool;
typedef std::int8_t   U_i8 ;
typedef std::uint8_t  U_u8 ;
typedef std::int16_t  U_i16;
typedef std::uint16_t U_u16;
typedef std::int32_t  U_i32;
typedef std::uint32_t U_u32;
typedef std::int64_t  U_i64;
typedef std::uint64_t U_u64;
typedef float  U_f32;
typedef double U_f64;

// Fundamental langiage types.
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

} // namespace Interpreter
