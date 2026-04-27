from py_tests_common import *


def FunctionParameterNameMismatch_Test0():
	c_program_text= """
		fn Foo( i32 y );
		fn Foo( i32 x ){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FunctionParameterNameMismatch", 3 ) )


def FunctionParameterNameMismatch_Test1():
	c_program_text= """
		fn Foo( i32 x ){}
		fn Foo( i32 y );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FunctionParameterNameMismatch", 2 ) )
