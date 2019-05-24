from py_tests_common import *


def GlobalsLoop_ForFunctionWithAutoReturnType_Test0():
	c_program_text= """
		fn Foo() : auto
		{
			Foo(); // Error, acces to "Foo" forbiden, while compiling "Foo"
			return 0;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "GlobalsLoopDetected" )
	assert( errors_list[0].file_pos.line == 2 )



def GlobalsLoop_ForFunctionWithAutoReturnType_Test1():
	c_program_text= """
		fn Foo( i32 x ) : i32 { return x; }
		fn Foo() : auto
		{
			Foo( 0 ); // Error, acces to "Foo" forbiden, while compiling "Foo". We access all functions set, not single function Foo(i32).
			return 0;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "GlobalsLoopDetected" )
	assert( errors_list[0].file_pos.line == 2 )
