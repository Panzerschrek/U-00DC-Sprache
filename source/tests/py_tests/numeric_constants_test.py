from py_tests_common import *


def NumericConstants_DecimalConstants_Test0():
	c_program_text= """
		static_assert( 5 == 5 );
		static_assert( 13 == 10 + 3 );
		static_assert( 279 == 200 + 70 + 9 );
		static_assert( 16.625 == 16.0 + 0.5 + 0.125 );
		static_assert( 354e5 == 35400000.0 );
		static_assert( 25.42e10 == 254200000000.0 );
		static_assert( 17.23e3 == 17.23e+3 );
		static_assert( 256000.0e-3 == 256.0 ); // floating point with negative exponent
		static_assert( 13e2 == 1300.0 ); // floating point with exponent
		static_assert( i32( 13.52e3 )== 13520 ); // fractional part saved for integer constant
		static_assert( 0.3 == 3.0 / 10.0 );
		static_assert( 25.0e-5 == 0.00025 );
		static_assert( 0.00025 == 25.0 / 100000.0 );
		static_assert( 0.32145e5 == 32145.0 );
		static_assert( 5.0e32 == 5.0 * 1.0e16 * 1.0e16 ); // pow( 10, exponent ) is greater, than u64 limit.
		static_assert( 0.00000004e18 == 40000000000.0 ); // Small value with large exponent results into large value.
		static_assert( 0.00000004754248911e18 == 47542489110.0 ); // Small value with large exponent results into large value.
	"""
	tests_lib.build_program( c_program_text )


def NumericConstants_DecimalConstants_Test1():
	c_program_text= """
		// Limits
		static_assert( 2147483647 == ( (1<<30u) | ( (1<<30u) - 1 ) ) ); // max i32
		static_assert( i32(-2147483648) == ( (-1)<<31u ) ); // min i32
		static_assert( 9223372036854775807i64 == ( (1i64<<62u) | ( (1i64<<62u) - 1i64 ) ) ); // max i64
		static_assert( i64( -9223372036854775808u64 ) == ( (-1i64)<<63u ) ); // min i64
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
	"""
	tests_lib.build_program( c_program_text )


def NumericConstants_BinaryConstants_Test1():
	c_program_text= """
		// Limits
		static_assert( 0b01111111111111111111111111111111 == ( (1<<30u) | ( (1<<30u) - 1 ) ) ); // max i32
		static_assert( i32(0b10000000000000000000000000000000) == ( (-1)<<31u ) ); // min i32
		static_assert( 0b0111111111111111111111111111111111111111111111111111111111111111i64 == ( (1i64<<62u) | ( (1i64<<62u) - 1i64 ) ) ); // max i64
		static_assert( i64( 0b1000000000000000000000000000000000000000000000000000000000000000u64 ) == ( (-1i64)<<63u ) ); // min i64
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
	"""
	tests_lib.build_program( c_program_text )


def NumericConstants_OctalConstants_Test1():
	c_program_text= """
		// Limits
		static_assert( 0o17777777777 == ( (1<<30u) | ( (1<<30u) - 1 ) ) ); // max i32
		static_assert( i32(0o20000000000) == ( (-1)<<31u ) ); // min i32
		static_assert( 0o777777777777777777777i64 == ( (1i64<<62u) | ( (1i64<<62u) - 1i64 ) ) ); // max i64
		static_assert( i64( 0o1000000000000000000000u64 ) == ( (-1i64)<<63u ) ); // min i64
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
	"""
	tests_lib.build_program( c_program_text )


def NumericConstants_HexadecimalConstants_Test1():
	c_program_text= """
		// Limits
		static_assert( 0x7fffffff == ( (1<<30u) | ( (1<<30u) - 1 ) ) ); // max i32
		static_assert( i32(0x80000000) == ( (-1)<<31u ) ); // min i32
		static_assert( 0x7fffffffffffffffi64 == ( (1i64<<62u) | ( (1i64<<62u) - 1i64 ) ) ); // max i64
		static_assert( i64( 0x8000000000000000u64 ) == ( (-1i64)<<63u ) ); // min i64
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
			check_type( 0x0DEADC0D, i32(0) );
			check_type( 0b101, i32(0) );
			check_type( 0o52147, i32(0) );

			// No suffix and has fractional part - is 64bit floating point.
			check_type( 0.25, f64(0) );
			check_type( 65354.1, f64(0) );
			check_type( 0.001, f64(0) );
			check_type( 5.34e-5, f64(0) );
			check_type( 7.2e11, f64(0) );

			// No suffix and has exponent - f64 floating point.
			check_type( 1e5, f64(0) );

			// "u" siffix for unsigned 32bit integer.
			check_type( 99u, u32(0) );
			check_type( 953652114u, u32(0) );
			check_type( 0xF41Au, u32(0) );
			check_type( 0b1110u, u32(0) );
			check_type( 0o7u, u32(0) );

			// "s" suffix for size_type.
			check_type( 0s, size_type(0) );
			check_type( 0b000101110s, size_type(0) );
			check_type( 102452752s, size_type(0) );
			check_type( 0xFFs, size_type(0) );

			// "f" for 32-bit floating point.
			check_type( 3.14f, f32(0) );
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
			check_type( 8.1f32, f32(0) );
			check_type( 25.0f64, f64(0) );
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

		// Even larger constants may be extended to i128.
		static_assert( same_type</ typeof(9223512774343262318), i128 /> );
		static_assert( 9223512774343262318 == i128(922351277) * i128(10000000000) + i128(4343262318) );
		static_assert( 9223512774343262318 == i128(9223512774343262318u) );
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


def NumericConstantsExtendedType_Test2():
	c_program_text= """
		// Large constants (over 64-bit range) are parsed as floating point constants.
		static_assert( same_type</ typeof(1234567900681729874025512960), f64 /> );
		static_assert( 1234567900681729874025512960 == 1234567900681729874025512960.0 );
	"""
	tests_lib.build_program( c_program_text )


def UnsupportedIntegerConstantType_Test0():
	c_program_text= """
		auto x= 7876f; // Use floating-point suffix for integer literal.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsupportedIntegerConstantType", 2 ) )


def UnsupportedIntegerConstantType_Test1():
	c_program_text= """
		auto x= 7876f32; // Use floating-point suffix for integer literal.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsupportedIntegerConstantType", 2 ) )

def UnsupportedIntegerConstantType_Test2():
	c_program_text= """
		auto x= 7876f64; // Use floating-point suffix for integer literal.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsupportedIntegerConstantType", 2 ) )


def UnsupportedFloatingPointConstantType_Test0():
	c_program_text= """
		auto x= 7876.0u; // Use integer suffix for floating-point literal.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsupportedFloatingPointConstantType", 2 ) )


def UnsupportedFloatingPointConstantType_Test1():
	c_program_text= """
		auto x= 7876e13s; // Use integer suffix for floating-point literal.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsupportedFloatingPointConstantType", 2 ) )


def UnsupportedFloatingPointConstantType_Test2():
	c_program_text= """
		auto x= 7876.12c8; // Use integer suffix for floating-point literal.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsupportedFloatingPointConstantType", 2 ) )


def UnsupportedFloatingPointConstantType_Test3():
	c_program_text= """
		auto x= 7876.12c32; // Use integer suffix for floating-point literal.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsupportedFloatingPointConstantType", 2 ) )


def UnsupportedFloatingPointConstantType_Test4():
	c_program_text= """
		auto x= 7876.25u64; // Use integer suffix for floating-point literal.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsupportedFloatingPointConstantType", 2 ) )


def UnsupportedFloatingPointConstantType_Test5():
	c_program_text= """
		auto x= 7876.0i16; // Use integer suffix for floating-point literal.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsupportedFloatingPointConstantType", 2 ) )


def IntegerConstantOverflow_Test0():
	c_program_text= """
		auto x= 178i8;
		auto y= 273i8;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "IntegerConstantOverflow", 2 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 3 ) )


def IntegerConstantOverflow_Test1():
	c_program_text= """
		auto x= 256u8;
		auto y= 257u8;
		auto z= 259u8;
		auto w= 2554559u8;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "IntegerConstantOverflow", 2 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 3 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 4 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 5 ) )


def IntegerConstantOverflow_Test2():
	c_program_text= """
		auto x= 32768i16;
		auto y= 32769i16;
		auto z= 34768i16;
		auto w= 65545i16;
		auto t= 65536i16;
		auto u= 254765754i16;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "IntegerConstantOverflow", 2 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 3 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 4 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 5 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 6 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 7 ) )


def IntegerConstantOverflow_Test3():
	c_program_text= """
		auto x= 65536u16;
		auto y= 65537u16;
		auto z= 76852u16;
		auto w= 254765754u16;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "IntegerConstantOverflow", 2 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 3 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 4 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 5 ) )


def IntegerConstantOverflow_Test4():
	c_program_text= """
		auto x= 0x80000000i32;
		auto y= 2147483648i32;
		auto z= 2147483649i32;
		auto w= 3147483648i32;
		auto s= 642345476542345667i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "IntegerConstantOverflow", 2 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 3 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 4 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 5 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 6 ) )


def IntegerConstantOverflow_Test5():
	c_program_text= """
		auto x= 4294967296u32;
		auto y= 4294967297u32;
		auto z= 7294967296u32;
		auto w= 9223372036854775807u32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "IntegerConstantOverflow", 2 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 3 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 4 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 5 ) )


def IntegerConstantOverflow_Test6():
	c_program_text= """
		auto x= 9223372036854775808i64;
		auto y= 9223372036854775809i64;
		auto z= 15223372036854775808i64;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "IntegerConstantOverflow", 2 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 3 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 4 ) )


def IntegerConstantOverflow_Test7():
	c_program_text= """
		auto x= 256c8;
		auto y= 257c8;
		auto z= 259c8;
		auto w= 2554559c8;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "IntegerConstantOverflow", 2 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 3 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 4 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 5 ) )


def IntegerConstantOverflow_Test8():
	c_program_text= """
		auto x= 65536c16;
		auto y= 65537c16;
		auto z= 76852c16;
		auto w= 254765754c16;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "IntegerConstantOverflow", 2 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 3 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 4 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 5 ) )


def IntegerConstantOverflow_Test9():
	c_program_text= """
		auto x= 4294967296c32;
		auto y= 4294967297c32;
		auto z= 7294967296c32;
		auto w= 9223372036854775807c32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "IntegerConstantOverflow", 2 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 3 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 4 ) )
	assert( HasError( errors_list, "IntegerConstantOverflow", 5 ) )
