from py_tests_common import *


def SimpleAsyncFunction_Test0():
	c_program_text= """
		fn async SimpleFunc() : i32
		{
			return 42;
		}
		fn Foo()
		{
			auto mut f= SimpleFunc();
			if_coro_advance( x : f )
			{
				halt if( x != 42 );
			}
			else { halt; }

			if_coro_advance( x : f )
			{
				halt;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
