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
	"""
	tests_lib.build_program( c_program_text )


def NumericConstants_DecimalConstants_Test1():
	c_program_text= """
		// Limits
		static_assert( 2147483647 == ( (1<<30u) | ( (1<<30u) - 1 ) ) ); // max i32
		static_assert( -2147483648 == ( (-1)<<31u ) ); // min i32
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
		static_assert( 0b10000000000000000000000000000000 == ( (-1)<<31u ) ); // min i32
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
		static_assert( 0o20000000000 == ( (-1)<<31u ) ); // min i32
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
		static_assert( 0x80000000 == ( (-1)<<31u ) ); // min i32
		static_assert( 0x7fffffffffffffffi64 == ( (1i64<<62u) | ( (1i64<<62u) - 1i64 ) ) ); // max i64
		static_assert( 0x8000000000000000i64 == ( (-1i64)<<63u ) ); // min i64
		static_assert( 0xffffffffu == ~0u ); // max u32
		static_assert( 0xffffffffffffffffu64 == ~0u64 ); // max u64
	"""
	tests_lib.build_program( c_program_text )
