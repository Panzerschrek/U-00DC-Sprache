from py_tests_common import *


def NodiscardClassDeclaration_Test0():
	c_program_text= """
		struct SomeStruct nodiscard
		{}
	"""
	tests_lib.build_program( c_program_text )


def NodiscardClassDeclaration_Test1():
	c_program_text= """
		struct SomeStruct ordered nodiscard // "nodiscard" should be specified after "ordered".
		{
			i32 x;
			i32 y;
		}
	"""
	tests_lib.build_program( c_program_text )


def NodiscardClassDeclaration_Test2():
	c_program_text= """
		class SomeClass nodiscard // "nodiscard" for a class.
		{}
	"""
	tests_lib.build_program( c_program_text )


def NodiscardClassDeclaration_Test3():
	c_program_text= """
		class SomeClass non_sync nodiscard // "nodiscard" after "non_sync"
		{}
	"""
	tests_lib.build_program( c_program_text )


def NodiscardClassDeclaration_Test4():
	c_program_text= """
		class SomeClass polymorph nodiscard // "nodiscard" after "polymorph"
		{}
	"""
	tests_lib.build_program( c_program_text )


def NodiscardClassDeclaration_Test5():
	c_program_text= """
		class SomeInterface interface {}
		class SomeClass : SomeInterface nodiscard // "nodiscard" after parents list
		{}
	"""
	tests_lib.build_program( c_program_text )
