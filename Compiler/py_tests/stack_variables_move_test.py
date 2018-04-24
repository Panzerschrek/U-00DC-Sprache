from py_tests_common import *


def MoveOperatorDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			move(x);
		}
	"""
	tests_lib.build_program( c_program_text )



def MoveOperatorDeclaration_Test1():
	c_program_text= """
		fn Foo( i32 x )
		{
			move(x);
		}
	"""
	tests_lib.build_program( c_program_text )


def MoveOperatorDeclaration_Test2():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			-( move(x) + 42 );
		}
	"""
	tests_lib.build_program( c_program_text )
