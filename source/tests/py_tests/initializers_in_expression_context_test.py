from py_tests_common import *

def StructInitializationExpression_Test0():
	c_program_text= """
		struct S{ i32 x; }
		fn Foo()
		{
			auto s= S{ .x= 56 };
			halt if( s.x != 56 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
