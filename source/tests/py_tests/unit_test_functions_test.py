from py_tests_common import *


def UnitTestFunctionDefinition_Test0():
	c_program_text= """
		// Test function within root namespace.
		fn_test( SomeTest )
		{
			auto mut x= 10;
			auto mut y= 3;
			halt if( x - y != 7 );
		}
	"""
	tests_lib.build_program( c_program_text )


def UnitTestFunctionDefinition_Test1():
	c_program_text= """
		namespace SomeNamespace
		{
			// Test function within a namespace.
			fn_test( SomeTest )
			{
				auto mut x= 10;
				auto mut y= 3;
				halt if( x * y != 30 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def UnitTestFunctionDefinition_Test2():
	c_program_text= """
		struct SomeStruct
		{
			// Test function within a namespace.
			fn_test( SomeTest )
			{
				auto mut x= 10;
				auto mut y= 3;
				halt if( x / y != 3 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
