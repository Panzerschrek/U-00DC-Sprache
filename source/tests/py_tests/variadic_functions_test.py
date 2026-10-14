from py_tests_common import *


def VariadicFunctionDeclaration_Test0():
	c_program_text= """
		fn Foo( ... ) call_conv( "C" ) : i32; // Only variadic args.
	"""
	tests_lib.build_program( c_program_text )


def VariadicFunctionDeclaration_Test1():
	c_program_text= """
		fn Foo( i32 x, u64 y, f32 z, ... ) call_conv( "C" ) : size_type; // Variadic and non-variadic args.
	"""
	tests_lib.build_program( c_program_text )


def VariadicFunctionDeclaration_Test2():
	c_program_text= """
		type VarFn= ( fn( ... ) call_conv( "C" ) : void ); // Only variadic args.
	"""
	tests_lib.build_program( c_program_text )


def VariadicFunctionDeclaration_Test3():
	c_program_text= """
		type VarFn= ( fn( $(char8) f, ... ) call_conv( "C" ) : i32 ); // Variadic and non-variadic args.
	"""
	tests_lib.build_program( c_program_text )


def VariadicFunctionCall_Test0():
	c_program_text= """
		fn Bar( ... ) call_conv( "C" ) : i32;
		fn Foo()
		{
			Bar(); // Pass zero variadic args.
		}
	"""
	tests_lib.build_program( c_program_text )


def VariadicFunctionCall_Test1():
	c_program_text= """
		fn Bar( ... ) call_conv( "C" ) : i32;
		fn Foo()
		{
			var f32 mut f= 0.25f;
			Bar( 67u, Baz(), f ); // Pass several variadic args.
		}
		fn Baz() : u64;
	"""
	tests_lib.build_program( c_program_text, True )
