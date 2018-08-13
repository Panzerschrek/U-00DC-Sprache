from py_tests_common import *


def EnableIfDeclaration_Test0():
	c_program_text= """
		fn enable_if( true ) Foo() {}
	"""
	tests_lib.build_program( c_program_text )


def EnableIfDeclaration_Test1():
	c_program_text= """
		class C polymorph
		{
			fn virtual enable_if( true ) Foo( this ) {}
		}
	"""
	tests_lib.build_program( c_program_text )


def EnableIfDeclaration_Test2():
	c_program_text= """
		class I interface
		{
			fn virtual pure enable_if( true ) Foo( this );
		}
	"""
	tests_lib.build_program( c_program_text )
