from py_tests_common import *


def StaticIfDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			static_if( true ) {}
		}
	"""
	tests_lib.build_program( c_program_text )


def StaticIfDeclaration_Test1():
	c_program_text= """
		fn Foo()
		{
			static_if( true ) {}
			else {}
		}
	"""
	tests_lib.build_program( c_program_text )


def StaticIfDeclaration_Test2():
	c_program_text= """
		fn Foo()
		{
			static_if( true ) {}
			else if( true ) {}
		}
	"""
	tests_lib.build_program( c_program_text )


def StaticIfDeclaration_Test3():
	c_program_text= """
		fn Foo()
		{
			static_if( true ) {}
			else if( true ) {}
			else {}
		}
	"""
	tests_lib.build_program( c_program_text )
