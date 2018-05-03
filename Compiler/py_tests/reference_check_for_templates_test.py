from py_tests_common import *


def ContinuousInnerReferenceTagDeclaration_Test0():
	c_program_text= """
		struct S{}
		fn Foo( S& s' x... ' ){}
	"""
	tests_lib.build_program( c_program_text )
