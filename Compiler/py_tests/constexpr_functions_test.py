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
