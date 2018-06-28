from py_tests_common import *

def ConstexprCall_ResultTypeIs_u16():
	c_program_text= """
		fn constexpr Pass( u16 x ) : u16
		{
			return x;
		}
		fn Foo()
		{
			static_assert( Pass(65521u16) == 65521u16 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprCall_ResultTypeIs_i32():
	c_program_text= """
		fn constexpr Pass( i32 x ) : i32
		{
			return x;
		}
		fn Foo()
		{
			static_assert( Pass(42) == 42 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprCall_ResultTypeIs_i64():
	c_program_text= """
		fn constexpr Pass( i64 x ) : i64
		{
			return x;
		}
		fn Foo()
		{
			static_assert( Pass(80816996193i64) == 80816996193i64 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprCall_ResultTypeIs_f32():
	c_program_text= """
		fn constexpr Pass( f32 x ) : f32
		{
			return x;
		}
		fn Foo()
		{
			static_assert( Pass(3.1415926535f) == 3.1415926535f );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprCall_ResultTypeIs_f64():
	c_program_text= """
		fn constexpr Pass( f64 x ) : f64
		{
			return x;
		}
		fn Foo()
		{
			static_assert( Pass(3.1415926535) == 3.1415926535 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_Add():
	c_program_text= """
		fn constexpr Bar( i32 x, i32 y ) : i32
		{
			return x + y;
		}
		fn Foo()
		{
			static_assert( Bar(85, 14) == 85 + 14 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_Sub():
	c_program_text= """
		fn constexpr Bar( i32 x, i32 y ) : i32
		{
			return x - y;
		}
		fn Foo()
		{
			static_assert( Bar(14, 85) == 14 - 85 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_Mul():
	c_program_text= """
		fn constexpr Bar( i32 x, i32 y ) : i32
		{
			return x * y;
		}
		fn Foo()
		{
			static_assert( Bar(14, 85) == 14 * 85 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_Div():
	c_program_text= """
		fn constexpr Bar( i32 x, i32 y ) : i32
		{
			return x / y;
		}
		fn Foo()
		{
			static_assert( Bar(85, 14) == 85 / 14 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_FAdd():
	c_program_text= """
		fn constexpr Bar( f32 x, f32 y ) : f32
		{
			return x + y;
		}
		fn Foo()
		{
			static_assert( Bar(85.5f, 14.1f) == 85.5f + 14.1f );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_FSub():
	c_program_text= """
		fn constexpr Bar( f32 x, f32 y ) : f32
		{
			return x - y;
		}
		fn Foo()
		{
			static_assert( Bar(85.5f, 14.1f) == 85.5f - 14.1f );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_FMul():
	c_program_text= """
		fn constexpr Bar( f32 x, f32 y ) : f32
		{
			return x * y;
		}
		fn Foo()
		{
			static_assert( Bar(85.5f, 14.1f) == 85.5f * 14.1f );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_FDiv():
	c_program_text= """
		fn constexpr Bar( f32 x, f32 y ) : f32
		{
			return x / y;
		}
		fn Foo()
		{
			static_assert( Bar(85.5f, 14.1f) == 85.5f / 14.1f );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_BitOperators():
	c_program_text= """
		fn constexpr Bar( i32 x, i32 y, i32 z, i32 w ) : i32
		{
			return ( x & y ) | ( z ^ w );
		}
		fn Foo()
		{
			static_assert( Bar( 255, 254, 542, 86254 ) == ( (255&254) | (542^86254) ) );
		}
	"""
	tests_lib.build_program( c_program_text )
