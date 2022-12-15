from py_tests_common import *


def SharedTagExpression_Check0():
	c_program_text= """
		class A
		shared( unknown_name )
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].src_loc.line == 3 )


def SharedTagExpression_Check1():
	c_program_text= """
		class A
		shared( 0 )
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 3 )


def SharedTagExpression_Check2():
	c_program_text= """
		fn Foo() : bool;

		class A
		shared( Foo() )
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedConstantExpression" )
	assert( errors_list[0].src_loc.line == 5 )
