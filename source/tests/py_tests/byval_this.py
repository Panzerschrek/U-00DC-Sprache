from py_tests_common import *

def ByValThis_Declaeation_Test0():
	c_program_text= """
		struct S
		{
			fn Foo( byval this ){}
		}
	"""
	tests_lib.build_program( c_program_text )


def ByValThis_Declaeation_Test1():
	c_program_text= """
		struct S
		{
			fn Foo( byval imut this ){}
		}
	"""
	tests_lib.build_program( c_program_text )


def ByValThis_Declaeation_Test2():
	c_program_text= """
		struct S
		{
			fn Foo( byval mut this ){}
		}
	"""
	tests_lib.build_program( c_program_text )
