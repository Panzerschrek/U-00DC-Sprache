from py_tests_common import *


def TernaryOperatorParsing_Test0():
	c_program_text= """
		fn Foo()
		{
			select( true ? 0 : 1 );
		}
	"""
	tests_lib.build_program( c_program_text )


def TernaryOperatorParsing_Test1():
	c_program_text= """
		fn Foo( i32 x )
		{
			2 + select( x > 0 ? x : -x ) * 2;
		}
	"""
	tests_lib.build_program( c_program_text )
