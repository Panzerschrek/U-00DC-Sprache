from py_tests_common import *


def SwitchOperatorDeclaration_Test0():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch( x ){} // Empty switch
		}
	"""
	tests_lib.build_program( c_program_text )


def SwitchOperatorDeclaration_Test1():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch( x ) // Switch with one value.
			{
				0 -> {},
			}
			switch( x )
			{
				1 -> {} // Last comma is unnecessary.
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def SwitchOperatorDeclaration_Test2():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch( x ) // Switch with two values.
			{
				0 -> {},
				1 -> { return; },
			}
			switch( x )
			{
				2 -> {},
				3 -> {} // Last comma is unnecessary.
			}
		}
	"""
	tests_lib.build_program( c_program_text )
