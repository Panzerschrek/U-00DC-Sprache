from py_tests_common import *

def FieldInitializerDeclaration_Test0():
	c_program_text= """
		struct S
		{
			i32 x= 0; // Expression initializer
		}
	"""
	tests_lib.build_program( c_program_text )


def FieldInitializerDeclaration_Test1():
	c_program_text= """
		struct S
		{
			i32 x(0); // Constructor initializer
		}
	"""
	tests_lib.build_program( c_program_text )


def FieldInitializerDeclaration_Test2():
	c_program_text= """
		struct S
		{
			i32 x= zero_init; // Zero initializer
		}
	"""
	tests_lib.build_program( c_program_text )


def FieldInitializerDeclaration_Test2():
	c_program_text= """
		struct S
		{
			[ i32, 2 ] arr[ 55, 66 ]; // Array initializer
		}
	"""
	tests_lib.build_program( c_program_text )


def FieldInitializerDeclaration_Test3():
	c_program_text= """
		struct S
		{
			i32 x;
		}
		struct T
		{
			S s{ .x= 0 }; // Struct named initializerg
		}
	"""
	tests_lib.build_program( c_program_text )
