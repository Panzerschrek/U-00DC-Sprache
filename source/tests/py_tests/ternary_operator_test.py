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


def TernaryOperatorParsing_Test0():
	c_program_text= """
		fn Foo( bool b, i32 x, i32 y ) : i32
		{
			return select( b ? x : y ); // Both branches result is const reference.
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foobii", True , 55, 11 ) == 55 )
	assert( tests_lib.run_function( "_Z3Foobii", False, 55, 11 ) == 11 )


def TernaryOperatorParsing_Test1():
	c_program_text= """
		fn Foo( bool b, i32 x, i32 y ) : i32
		{
			return select( b ? x * 5 : y * 7 ); // Both branches result is value.
		}
	"""
	tests_lib.build_program( c_program_text, True )
	assert( tests_lib.run_function( "_Z3Foobii", True , 1, 1 ) == 5 )
	assert( tests_lib.run_function( "_Z3Foobii", False, 1, 1 ) == 7 )
