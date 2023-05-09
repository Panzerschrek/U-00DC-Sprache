from py_tests_common import *


def UnconditionalLoop_Test0():
	c_program_text= """
		fn Foo()
		{
			loop{}
		}
	"""
	tests_lib.build_program( c_program_text )


def UnconditionalLoop_Test1():
	c_program_text= """
		fn Foo()
		{
			loop
			{
				break;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def UnconditionalLoop_Test2():
	c_program_text= """
		fn Foo()
		{
			loop
			{
				continue;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
