from py_tests_common import *


def NumericConstants_DecimalConstants_Test0():
	c_program_text= """
		static_assert( 5 == 5 );
		static_assert( 13 == 10 + 3 );
		static_assert( 279 == 200 + 70 + 9 );
		static_assert( 16.625 == 16.0 + 0.5 + 0.125 );
		static_assert( 354e5 == 35400000 );
		static_assert( 25.42e10 == 254200000000.0 );
		static_assert( 17.23e3 == 17.23e+3 );
		static_assert( 256000.0e-3 == 256.0 ); // floating point with negative exponent
		static_assert( 13e2 == 1300 ); // integer with exponent
		static_assert( 13.52e3i32 == 13520i32 ); // fractional part saved for integer constant
		static_assert( 0.3 == 3.0 / 10.0 );
		static_assert( 25.0e-5 == 0.00025 );
		static_assert( 0.00025 == 25.0 / 100000.0 );
		static_assert( 0.32145e5 == 32145.0 );
		static_assert( 5.0e32 == 5.0 * 1.0e8 * 1.0e8 * 1.0e8 * 1.0e8 ); // pow( 10, exponent ) is greater, than u64 limit.
	"""
	tests_lib.build_program( c_program_text )


def NumericConstants_DecimalConstants_Test1():
	c_program_text= """
		// Limits
		static_assert( 2147483647 == ( (1<<30u) | ( (1<<30u) - 1 ) ) ); // max i32
		static_assert( i32(-2147483648) == ( (-1)<<31u ) ); // min i32
		static_assert( 9223372036854775807i64 == ( (1i64<<62u) | ( (1i64<<62u) - 1i64 ) ) ); // max i64
		static_assert( -9223372036854775808i64 == ( (-1i64)<<63u ) ); // min i64
		static_assert( 4294967295u == ~0u ); // max u32
		static_assert( 18446744073709551615u64 == ~0u64 ); // max u64
	"""
	tests_lib.build_program( c_program_text )


def NumericConstants_BinaryConstants_Test0():
	c_program_text= """
		static_assert( 0b0 == 0b0 );
		static_assert( 0b0 == 0b000000 );
		static_assert( 0b1 == 1 );
		static_assert( 0b10 == 2 );
		static_assert( 0b11 == 3 );
		static_assert( 0b0000011 == 3 );
		static_assert( 0b1001 == 9 );
		static_assert( 0b11111001001 == 1993 );
		static_assert( 0b10.11 == 2.75 );
		static_assert( 0b000.0101 == 0.25 + 0.0625 );
		static_assert( 0b1000000.1 == 64.5 );
	"""
	tests_lib.build_program( c_program_text )


def NumericConstants_BinaryConstants_Test1():
	c_program_text= """
		// Limits
		static_assert( 0b01111111111111111111111111111111 == ( (1<<30u) | ( (1<<30u) - 1 ) ) ); // max i32
		static_assert( i32(0b10000000000000000000000000000000) == ( (-1)<<31u ) ); // min i32
		static_assert( 0b0111111111111111111111111111111111111111111111111111111111111111i64 == ( (1i64<<62u) | ( (1i64<<62u) - 1i64 ) ) ); // max i64
		static_assert( 0b1000000000000000000000000000000000000000000000000000000000000000i64 == ( (-1i64)<<63u ) ); // min i64
		static_assert( 0b11111111111111111111111111111111u == ~0u ); // max u32
		static_assert( 0b1111111111111111111111111111111111111111111111111111111111111111u64== ~0u64 ); // max u64
	"""
	tests_lib.build_program( c_program_text )


def NumericConstants_OctalConstants_Test0():
	c_program_text= """
		static_assert( 0o0 == 0 );
		static_assert( 0o1 == 1 );
		static_assert( 0o5 == 5 );
		static_assert( 0o7 == 7 );
		static_assert( 0o10 == 8 );
		static_assert( 0o5413641 == 1447841 );
		static_assert( 0o0.12 == 0.15625 );
		static_assert( 0o0.523 == 0.662109375 );
	"""
	tests_lib.build_program( c_program_text )


def NumericConstants_OctalConstants_Test1():
	c_program_text= """
		// Limits
		static_assert( 0o17777777777 == ( (1<<30u) | ( (1<<30u) - 1 ) ) ); // max i32
		static_assert( i32(0o20000000000) == ( (-1)<<31u ) ); // min i32
		static_assert( 0o777777777777777777777i64 == ( (1i64<<62u) | ( (1i64<<62u) - 1i64 ) ) ); // max i64
		static_assert( 0o1000000000000000000000i64 == ( (-1i64)<<63u ) ); // min i64
		static_assert( 0o37777777777u == ~0u ); // max u32
		static_assert( 0o1777777777777777777777u64== ~0u64 ); // max u64
	"""
	tests_lib.build_program( c_program_text )


def NumericConstants_HexadecimalConstants_Test0():
	c_program_text= """
		static_assert( 0x0 == 0 );
		static_assert( 0x00 == 0 );
		static_assert( 0x8 == 8 );
		static_assert( 0xa == 10 );
		static_assert( 0xA == 10 );
		static_assert( 0xF == 15 );
		static_assert( 0xDEADC0DEu == 3735929054u );
		static_assert( 0xFEDCBA == 0xfedcba );
		static_assert( 0x1.5 == 1.3125 );
		static_assert( 0x52.31 == 82.19140625 );
		static_assert( 0x0.ff == 0.99609375 );
	"""
	tests_lib.build_program( c_program_text )


def NumericConstants_HexadecimalConstants_Test1():
	c_program_text= """
		// Limits
		static_assert( 0x7fffffff == ( (1<<30u) | ( (1<<30u) - 1 ) ) ); // max i32
		static_assert( i32(0x80000000) == ( (-1)<<31u ) ); // min i32
		static_assert( 0x7fffffffffffffffi64 == ( (1i64<<62u) | ( (1i64<<62u) - 1i64 ) ) ); // max i64
		static_assert( 0x8000000000000000i64 == ( (-1i64)<<63u ) ); // min i64
		static_assert( 0xffffffffu == ~0u ); // max u32
		static_assert( 0xffffffffffffffffu64 == ~0u64 ); // max u64
	"""
	tests_lib.build_program( c_program_text )


def NumericConstants_TypeSuffix_Test0():
	c_program_text= """
		template</ type T />
		fn check_type( T& x, T& y ){}

		fn Foo()
		{
			// No suffix and no fractional part - is signed 32bit integer.
			check_type( 23, i32(0) );
			check_type( 9652412, i32(0) );
			check_type( 1e5, i32(0) );
			check_type( 0x0DEADC0D, i32(0) );
			check_type( 0b101, i32(0) );
			check_type( 0o52147, i32(0) );

			// No suffix and has fractional part - is 64bit floating point.
			check_type( 0.25, f64(0) );
			check_type( 65354.1, f64(0) );
			check_type( 0.001, f64(0) );
			check_type( 5.34e-5, f64(0) );
			check_type( 7.2e11, f64(0) );

			// "u" siffix for unsigned 32bit integer.
			check_type( 99u, u32(0) );
			check_type( 953652114u, u32(0) );
			check_type( 0xF41Au, u32(0) );
			check_type( 0b1110u, u32(0) );
			check_type( 0o7u, u32(0) );
			check_type( 1.1u, u32(0) ); // Even if numeric constant has fractional point, type specified by suffix.

			// "s" suffix for size_type.
			check_type( 0s, size_type(0) );
			check_type( 0b000101110s, size_type(0) );
			check_type( 102452752s, size_type(0) );
			check_type( 0xFFs, size_type(0) );

			// "f" for 32-bit floating point.
			check_type( 3.14f, f32(0) );
			check_type( 99f, f32(0) );
			check_type( 0b1.1f, f32(0) );
			check_type( 2e9f, f32(0) );
			check_type( 3.1e2f, f32(0) );
			check_type( 653e-3f, f32(0) );

			// Short char literals
			check_type( 95c8, char8(0) );
			check_type( 32c8, char8(0) );
			check_type( 32564c16, char16(0) );
			check_type( 1235678c32, char32(0) );

			// Using fundamental types names as suffixes.
			check_type( 52i8, i8(0) );
			check_type( 254u8, u8(0) );
			check_type( 1254i16, i16(0) );
			check_type( 45214u16, u16(0) );
			check_type( 32i32, i32(0) );
			check_type( 0u32, u32(0) );
			check_type( 12425635875i64, i64(0) );
			check_type( 653214785365245u64, u64(0) );
			check_type( 100i128, i128(0) );
			check_type( 100u128, u128(0) );
			check_type( 8f32, f32(0) );
			check_type( 25f64, f64(0) );
			check_type( 62char8, char8(0) );
			check_type( 25647char16, char16(0) );
			check_type( 7586954char32, char32(0) );
		}
	"""
	tests_lib.build_program( c_program_text )


def NumericConstantsExtendedType_Test0():
	c_program_text= """
		// Small constants have type i32.
		static_assert( same_type</ typeof(98765), i32 /> );
		static_assert( same_type</ typeof(2147483647), i32 /> );

		// Large constants are extended to i64.
		static_assert( same_type</ typeof(2147483648), i64 /> );
		static_assert( 2147483648 == 1i64 << 31u );
		static_assert( 1234567891011 == 12345i64 * 100000000i64 + 67891011i64 );
		static_assert( same_type</ typeof( 1234567891011 ), i64 /> );
	"""
	tests_lib.build_program( c_program_text )


def NumericConstantsExtendedType_Test1():
	c_program_text= """
		// Small constants with suffix "u" have type u32.
		static_assert( same_type</ typeof(98765u), u32 /> );
		static_assert( same_type</ typeof(2147483647u), u32 /> );
		static_assert( same_type</ typeof(2147483648u), u32 /> );
		static_assert( same_type</ typeof(4294967295u), u32 /> );

		// Large constants with suffix "u" are extended to u64.
		static_assert( same_type</ typeof(4294967296u), u64 /> );
		static_assert( 4294967296u == 1u64 << 32u );
		static_assert( 1234567891011u == 12345u64 * 100000000u64 + 67891011u64 );
		static_assert( same_type</ typeof( 1234567891011u ), u64 /> );

	"""
	tests_lib.build_program( c_program_text )
