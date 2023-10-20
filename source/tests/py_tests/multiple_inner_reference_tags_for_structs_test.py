from py_tests_common import *


def ClassFieldReferenceNotation_Test0():
	c_program_text= """
		struct S
		{
			i32 & @("a"c8) ref;
		}
	"""
	tests_lib.build_program( c_program_text )


def ClassFieldReferenceNotation_Test1():
	c_program_text= """
		struct S
		{
			i32 &mut @("a"c8) ref;
		}
	"""
	tests_lib.build_program( c_program_text )


def ClassFieldReferenceNotation_Test2():
	c_program_text= """
		struct S
		{
			i32 &imut @("a"c8) ref;
		}
	"""
	tests_lib.build_program( c_program_text )



def ClassFieldReferenceNotation_Test3():
	c_program_text= """
		struct S{ i32& x; }
		struct T
		{
			S @("a"c8) s;
		}
	"""
	tests_lib.build_program( c_program_text )


def ClassFieldReferenceNotation_Test4():
	c_program_text= """
		struct S{ i32& x; }
		struct T
		{
			S @("a"c8) imut s;
		}
	"""
	tests_lib.build_program( c_program_text )
