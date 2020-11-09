from py_tests_common import *


def ArraysAreCopyConstructible_Test0():
	c_program_text= """
		fn Foo() : f32
		{
			var [ f32, 2 ] mut a0[ 45.0f, 5.0f ];
			var[ f32, 2 ] mut a1(a0);

			return a1[0] / a1[1];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 45.0 / 5.0 )
