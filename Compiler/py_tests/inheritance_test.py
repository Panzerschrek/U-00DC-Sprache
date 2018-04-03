from py_tests_common import *

def InheritanceTes_ClassKindAttribute_Test0():
	c_program_text= """
	class S final {}
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ClassKindAttribute_Test1():
	c_program_text= """
	class S polymorph {}
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ClassKindAttribute_Test2():
	c_program_text= """
	class S interface {}
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ClassKindAttribute_Test3():
	c_program_text= """
	class S abstract {}
	"""
	tests_lib.build_program( c_program_text )
