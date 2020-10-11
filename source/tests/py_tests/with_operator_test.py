from py_tests_common import *

def WithOperatorDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			with( x : 0 ){}
		}
	"""
	tests_lib.build_program( c_program_text )


def WithOperatorDeclaration_Test1():
	c_program_text= """
		fn Foo()
		{
			with( & y : 0 )
			{
				return;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def WithOperatorDeclaration_Test2():
	c_program_text= """
		fn Foo( i32 mut a )
		{
			with( &mut z : a )
			{
				z*= 2;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
