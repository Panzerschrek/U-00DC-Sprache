from py_tests_common import *


def TupleTypeParsing_Test0():
	c_program_text= """
		fn Foo( tup(i32, f32) & arg0 );
	"""
	tests_lib.build_program( c_program_text )


def TupleTypeParsing_Test1():
	c_program_text= """
		type EmptyTuple= tup();
		type LooooongTuple= tup( i32, f32, f32, bool, [ i32, 2 ], char8, fn(), [ char16, 8 ] );
	"""
	tests_lib.build_program( c_program_text )
