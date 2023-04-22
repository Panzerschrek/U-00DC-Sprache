from py_tests_common import *


def LoopLabelDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			while( true ) label something
			{
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def LoopLabelDeclaration_Test1():
	c_program_text= """
		fn Foo( tup[f32, i32]& t )
		{
			for( el : t ) label something
			{
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def LoopLabelDeclaration_Test2():
	c_program_text= """
		fn Foo()
		{
			for( auto mut i= 0; i < 16; ++i ) label something
			{
			}
		}
	"""
	tests_lib.build_program( c_program_text )
