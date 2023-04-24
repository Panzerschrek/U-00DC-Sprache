from py_tests_common import *


def BlockLabelDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			{
			} label some
		}
	"""
	tests_lib.build_program( c_program_text )


def BlockLabelDeclaration_Test1():
	c_program_text= """
		fn Foo()
		{
			unsafe
			{
			} label some
		}
	"""
	tests_lib.build_program( c_program_text )


def BlockLabelDeclaration_Test2():
	c_program_text= """
		fn Foo()
		{
			safe
			{
			} label some
		}
	"""
	tests_lib.build_program( c_program_text )


def BlockLabelDeclaration_Test3():
	c_program_text= """
		fn Foo()
		{
			if( true )
			{
				while( false )
				{
					{
					} label lll
				}
			}
			else
			{
				{
				} label some
			}
		}
	"""
	tests_lib.build_program( c_program_text )
