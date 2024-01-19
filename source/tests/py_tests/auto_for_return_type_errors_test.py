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
	assert( errors_list[0].src_loc.line == 2 )



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
	assert( errors_list[0].src_loc.line == 2 )


def ExpectedBodyForAutoFunction_Test0():
	c_program_text= """
		fn Foo() : auto;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedBodyForAutoFunction" )
	assert( errors_list[0].src_loc.line == 2 )


def ExpectedBodyForAutoFunction_Test1():
	c_program_text= """
		fn Foo( i32 x ) : auto&;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedBodyForAutoFunction" )
	assert( errors_list[0].src_loc.line == 2 )


def TypesMismtach_ForAutoReturnValue_Test0():
	c_program_text= """
		fn Foo( bool b ) : auto
		{
			if( b )
			{ return 1; } // return type deduced to i32
			return -1.0; // return type deduced to f64
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 6 )


def TypesMismtach_ForAutoReturnValue_Test1():
	c_program_text= """
		fn Foo( bool b ) : auto
		{
			if( b )
			{ return; } // return type deduced to void
			return -1.0; // return type deduced to f64
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 6 )


def ExpectedReferenceValue_ForAutoReturnValue_Test0():
	c_program_text= """
		fn Foo( bool b ) : auto&
		{
			return; // Error, expected reference
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 1 )
	assert( errors_list[1].error_code == "ExpectedReferenceValue" )
	assert( errors_list[1].src_loc.line == 4 )
