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


def ExternalFunctionAccessOperator_Test3():
	c_program_text= """
		fn Foo()
		{
			// Call result of the external function access operator directly.
			var u32 res= unsafe( import fn</ fn( i32 x ) : u32 />( "~~Get@Abs~~" ) )( 77 );
		}
	"""
	tests_lib.build_program( c_program_text )


def TypesMismatch_InExternalFunctionAccess_Test0():
	c_program_text= """
		fn Foo()
		{
			// Error - given type is not function type.
			auto x= unsafe( import fn</ i32 />( "some_func" ) );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 5 ) )


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
			unsafe
			{
				var i32 &mut x= import var</ i32 />( "some_var" );
				x= x * 3 + 1;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def ExternalVariableAccessOperator_Test1():
	c_program_text= """
		fn Foo()
		{
			unsafe
			{
				var $(u64) &mut addr= import var</ $(u64) />( "__  some_ptr_var" );
				++addr;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingExternalVariableInGlobalContext_Test0():
	c_program_text= """
		auto c= import var</ char8 />( "some_var" );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingExternalVariableInGlobalContext", 2 ) )


def AccessingExternalVariableInGlobalContext_Test1():
	c_program_text= """
		struct S
		{
			[ i32, 16 ] arr = import var</ [ i32, 16 ] />( "   var   " );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingExternalVariableInGlobalContext", 4 ) )


def AccessingExternalVariableOutsideUnsafeBlock_Test0():
	c_program_text= """
		fn Foo()
		{
			auto x= import var</ u16 />( "_X_X_X_" );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingExternalVariableOutsideUnsafeBlock", 4 ) )


def ExternalVariableTypeMismatch_Test0():
	c_program_text= """
		fn Foo()
		{
			auto a= unsafe( import var</ f32 />( "some_func" ) );
			auto b= unsafe( import var</ i32 />( "some_func" ) ); // Error - using different type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExternalVariableTypeMismatch", 5 ) )


def ExternalVariableTypeMismatch_Test1():
	c_program_text= """
		fn Foo()
		{
			auto a= unsafe( import var</ size_type />( "some_var" ) );
			auto b= unsafe( import var</ $(size_type) />( "some_var" ) ); // Error - using different type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExternalVariableTypeMismatch", 5 ) )


def ExternalVariableTypeMismatch_Test2():
	c_program_text= """
		fn Foo()
		{
			auto a= unsafe( import var</ u16 />( "some_var" ) );
			auto b= unsafe( import var</ u32 />( "some_var" ) ); // Error - using different type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExternalVariableTypeMismatch", 5 ) )
