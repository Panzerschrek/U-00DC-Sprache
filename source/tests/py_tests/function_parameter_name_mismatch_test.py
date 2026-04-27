from py_tests_common import *


def FunctionParameterNameMismatch_Test0():
	c_program_text= """
		fn Foo( i32 y );
		fn Foo( i32 x ){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FunctionParameterNameMismatch", 3 ) )


def FunctionParameterNameMismatch_Test1():
	c_program_text= """
		fn Foo( i32 x ){}
		fn Foo( i32 y );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FunctionParameterNameMismatch", 2 ) )


def FunctionParameterNameMismatch_Test2():
	c_program_text= """
		fn Foo(
			i32 x,
			i32 y,
			i32 z );
		fn Foo(
			i32 x,
			i32 yyy, // Should report an error here.
			i32 z ){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FunctionParameterNameMismatch", 8 ) )


def FunctionParameterNameMismatch_Test3():
	c_program_text= """
		fn Foo(
			i32 x,
			i32 y,
			i32 z ) // Should report an error here.
		{}
		fn Foo(
			i32 x,
			i32 y,
			i32 zzzz );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FunctionParameterNameMismatch", 5 ) )


def FunctionParameterNameMismatch_Test4():
	c_program_text= """
		fn Foo( u32& x );
		fn Foo( u32& X ) {} // Case mismatch - still an error.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FunctionParameterNameMismatch", 3 ) )


def FunctionParameterNameMismatch_Test5():
	c_program_text= """
		fn Foo( [ u64, 4 ] SOME );
		fn Foo( [ u64, 4 ] some ) {} // Case mismatch - still an error.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FunctionParameterNameMismatch", 3 ) )


def FunctionParameterNameMismatch_Test6():
	c_program_text= """
		fn Foo( bool name_ ) {} // Extra "_" - still an error.
		fn Foo( bool name );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FunctionParameterNameMismatch", 2 ) )


def FunctionParameterNameMismatch_Test7():
	c_program_text= """
		fn Foo( f64 name_ );
		fn Foo( f64 name ) {} // Missing "_" - still an error.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FunctionParameterNameMismatch", 3 ) )


def FunctionParameterNameMismatch_Test8():
	c_program_text= """
		namespace Abc
		{
			namespace Def
			{
				fn Foo( f32 yyy );
			}
		}
		fn Abc::Def::Foo( f32 y ) {} // Mismatch for out of line function.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FunctionParameterNameMismatch", 9 ) )
