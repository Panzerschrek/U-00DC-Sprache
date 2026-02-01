from py_tests_common import *


def ArrayFillerInitializer_Test0():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 3 ] arr[ 55, 777, 9999 ... ];
			halt if( arr[0] != 55 );
			halt if( arr[1] != 777 );
			halt if( arr[2] != 9999 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
