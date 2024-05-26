from py_tests_common import *


def ArrayTemplateArg_Test0():
	c_program_text= """
		type IntVec2= [ i32, 2 ];
		template</ IntVec2 arr_arg />
		struct S
		{
		}
	"""
	tests_lib.build_program( c_program_text )


def ArrayTemplateArg_Test1():
	c_program_text= """
		template</ [ u32, 3] arr_arg />
		struct S
		{
		}
	"""
	tests_lib.build_program( c_program_text )
