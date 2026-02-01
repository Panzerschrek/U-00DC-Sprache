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


def ArrayFillerInitializer_Test1():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 5 ] arr[ 2, 45, 78 ... ];
			halt if( arr[0] != 2 );
			halt if( arr[1] != 45 );
			halt if( arr[2] != 78 );
			halt if( arr[3] != 78 );
			halt if( arr[4] != 78 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
