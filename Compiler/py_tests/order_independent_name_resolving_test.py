from py_tests_common import *


def OrederIndependentFunctions_Test0():
	c_program_text= """
		fn Baz() : i32 { return 81; }
		fn Foo() : i32
		{
			return Baz() * Bar();  // Functions "Bar" and "Baz" visible here and can be called.
		}
		fn Bar() : i32 { return 52414; }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 52414 * 81 )
