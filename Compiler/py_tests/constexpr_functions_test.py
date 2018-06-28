from py_tests_common import *


def ConstexprCall_Test0():
	c_program_text= """
		fn constexpr Pass( i32 x ) : i32
		{
			return x;
		}

		fn Foo()
		{
			auto x= Pass(42);
		}
	"""
	tests_lib.build_program( c_program_text )
