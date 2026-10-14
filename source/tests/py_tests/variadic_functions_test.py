from py_tests_common import *


def VariadicFunctionDeclaration_Test0():
	c_program_text= """
		fn Foo( ... ) : i32; // Only variadic args.
	"""
	tests_lib.build_program( c_program_text )


def VariadicFunctionDeclaration_Test1():
	c_program_text= """
		fn Foo( i32 x, u64 y, f32 z, ... ) : size_type; // Variadic and non-variadic args.
	"""
	tests_lib.build_program( c_program_text )


def VariadicFunctionDeclaration_Test2():
	c_program_text= """
		type VarFn= ( fn( ... ) : void ); // Only variadic args.
	"""
	tests_lib.build_program( c_program_text )


def VariadicFunctionDeclaration_Test3():
	c_program_text= """
		type VarFn= ( fn( $(char8) f, ... ) : i32 ); // Variadic and non-variadic args.
	"""
	tests_lib.build_program( c_program_text )
