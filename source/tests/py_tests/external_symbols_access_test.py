from py_tests_common import *


def ExternalFunctionAccessOperator_Test0():
	c_program_text= """
		fn Foo()
		{
			auto f= unsafe( import fn</ fn() />( "some_func" ) );
		}
	"""
	tests_lib.build_program( c_program_text )


def ExternalFunctionAccessOperator_Test1():
	c_program_text= """
		fn Foo()
		{
			auto f= unsafe( import fn</ fn( f32 x, i32 y ) : i32 & />( "__Starship_Flight_7_failed" ) );
		}
	"""
	tests_lib.build_program( c_program_text )


def ExternalFunctionAccessOperator_Test2():
	c_program_text= """
		fn Foo()
		{
			type MyFuncType= fn( $(byte8) ptr, size_type size ) : ssize_type;
			auto f= unsafe( import fn</ MyFuncType />( "read_something" ) );
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingExternalFunctionInGlobalContext_Test0():
	c_program_text= """
		auto f= import fn</ fn() />( "some_func" );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingExternalFunctionInGlobalContext", 2 ) )


def AccessingExternalFunctionInGlobalContext_Test1():
	c_program_text= """
		struct S
		{
			(fn()) ptr= import fn</ fn() />( "some_func" );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingExternalFunctionInGlobalContext", 4 ) )


def AccessingExternalFunctionOutsideUnsafeBlock_Test0():
	c_program_text= """
		fn Foo()
		{
			auto f= import fn</ fn() />( "some_func" );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingExternalFunctionOutsideUnsafeBlock", 4 ) )


def ExternalFunctionSignatureMismatch_Test0():
	c_program_text= """
		fn Foo()
		{
			auto f0= unsafe( import fn</ fn() />( "some_func" ) );
			auto f1= unsafe( import fn</ fn() : f32 />( "some_func" ) ); // Error - using different signature for the same function.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExternalFunctionSignatureMismatch", 5 ) )


def ExternalFunctionSignatureMismatch_Test1():
	c_program_text= """
		fn Foo()
		{
			auto f0= unsafe( import fn</ fn( f32 x ) />( "some_func" ) );
			auto f1= unsafe( import fn</ fn( i32 x ) />( "some_func" ) ); // Error - using different signature for the same function.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExternalFunctionSignatureMismatch", 5 ) )


def ExternalFunctionSignatureMismatch_Test2():
	c_program_text= """
		fn Foo()
		{
			auto f0= unsafe( import fn</ fn( u64 a, u64 b ) />( "some_func" ) );
			auto f1= unsafe( import fn</ fn( u64 a ) />( "some_func" ) ); // Error - using different signature for the same function.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExternalFunctionSignatureMismatch", 5 ) )


def ExternalFunctionSignatureMismatch_Test3():
	c_program_text= """
		fn Foo()
		{
			auto f0= unsafe( import fn</ fn( size_type  s ) />( "some_func" ) );
			auto f1= unsafe( import fn</ fn( size_type& s ) />( "some_func" ) ); // Error - using different signature for the same function.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExternalFunctionSignatureMismatch", 5 ) )


def ExternalVariableAccessOperator_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 &mut x= unsafe( import var</ i32 />( "some_var" ) );
		}
	"""
	tests_lib.build_program( c_program_text )


def ExternalVariableAccessOperator_Test1():
	c_program_text= """
		fn Foo()
		{
			var $(u64) &mut addr= unsafe( import var</ $(u64) />( "__  some_ptr_var" ) );
		}
	"""
	tests_lib.build_program( c_program_text )
