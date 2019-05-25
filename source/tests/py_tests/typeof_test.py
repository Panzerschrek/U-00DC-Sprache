from py_tests_common import *

def TypeofOperatorDeclaration_Test0():
	c_program_text= """
		type T= typeof(0);
	"""
	tests_lib.build_program( c_program_text )


def TypeofOperatorDeclaration_Test1():
	c_program_text= """
		type T= typeof( 55 * 88 );
	"""
	tests_lib.build_program( c_program_text )


def TypeofOperatorDeclaration_Test2():
	c_program_text= """
		type T= [ typeof( 0.25 ), 64 ];
	"""
	tests_lib.build_program( c_program_text )


def TypeofOperatorDeclaration_Test3():
	c_program_text= """
		type T= typeof( "str" );
	"""
	tests_lib.build_program( c_program_text )


def TypeofOperatorDeclaration_Test5():
	c_program_text= """
		fn Foo() : i32;
		type T= typeof( Foo() );
	"""
	tests_lib.build_program( c_program_text )
