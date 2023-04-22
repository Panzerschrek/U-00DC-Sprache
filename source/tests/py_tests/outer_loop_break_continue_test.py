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


def BreakOperatorWithLabel_Test0():
	c_program_text= """
		fn Foo()
		{
			while( true ) label some
			{
				break label some;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def BreakOperatorWithLabel_Test1():
	c_program_text= """
		fn Foo()
		{
			while( true ) label some
			{
				for( auto mut i= 0; i < 16; ++i ) label another
				{
					break label another;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def ContinueOperatorWithLabel_Test0():
	c_program_text= """
		fn Foo()
		{
			while( true ) label some
			{
				continue label some;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def ContinueOperatorWithLabel_Test1():
	c_program_text= """
		fn Foo()
		{
			while( true ) label some
			{
				for( auto mut i= 0; i < 16; ++i ) label another
				{
					continue label another;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
