from py_tests_common import *


def VariadicFunctionDeclaration_Test0():
	c_program_text= """
		fn Foo( ... ) unsafe call_conv( "C" ) : i32; // Only variadic args.
	"""
	tests_lib.build_program( c_program_text )


def VariadicFunctionDeclaration_Test1():
	c_program_text= """
		fn Foo( i32 x, u64 y, f32 z, ... ) unsafe call_conv( "C" ) : size_type; // Variadic and non-variadic args.
	"""
	tests_lib.build_program( c_program_text )


def VariadicFunctionDeclaration_Test2():
	c_program_text= """
		type VarFn= ( fn( ... ) unsafe call_conv( "C" ) : void ); // Only variadic args.
	"""
	tests_lib.build_program( c_program_text )


def VariadicFunctionDeclaration_Test3():
	c_program_text= """
		type VarFn= ( fn( $(char8) f, ... ) unsafe call_conv( "C" ) : i32 ); // Variadic and non-variadic args.
	"""
	tests_lib.build_program( c_program_text )


def VariadicFunctionCall_Test0():
	c_program_text= """
		fn Bar( ... ) unsafe call_conv( "C" ) : i32;
		fn Foo()
		{
			unsafe( Bar() ); // Pass zero variadic args.
		}
	"""
	tests_lib.build_program( c_program_text )


def VariadicFunctionCall_Test1():
	c_program_text= """
		fn Bar( ... ) unsafe call_conv( "C" ) : i32;
		fn Foo()
		{
			var f32 mut f= 0.25f;
			unsafe( Bar( 67u, Baz(), f ) ); // Pass several variadic args.
		}
		fn Baz() : u64;
	"""
	tests_lib.build_program( c_program_text )


def InvalidCallingConventionForVariadicFunction_Test0():
	c_program_text= """
		fn Foo( ... ) unsafe; // No calling convention specified - default is used, which is distinct from "C".
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidCallingConventionForVariadicFunction", 2 ) )


def InvalidCallingConventionForVariadicFunction_Test1():
	c_program_text= """
		fn Foo( ... ) unsafe call_conv( "fast" );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidCallingConventionForVariadicFunction", 2 ) )


def InvalidCallingConventionForVariadicFunction_Test2():
	c_program_text= """
		fn Foo( ... ) unsafe call_conv( "cold" );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidCallingConventionForVariadicFunction", 2 ) )


def InvalidCallingConventionForVariadicFunction_Test3():
	c_program_text= """
		fn Foo( ... ) unsafe call_conv( "default" );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidCallingConventionForVariadicFunction", 2 ) )


def InvalidCallingConventionForVariadicFunction_Test4():
	c_program_text= """
		fn Foo( ... ) unsafe call_conv( "system" ); // Even "system" convention can't for now be used for variadic functions.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidCallingConventionForVariadicFunction", 2 ) )


def InvalidCallingConventionForVariadicFunction_Test5():
	c_program_text= """
		type VarFn= ( fn( ... ) unsafe ); // No calling convention specified - default is used, which is distinct from "C".
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidCallingConventionForVariadicFunction", 2 ) )


def InvalidCallingConventionForVariadicFunction_Test6():
	c_program_text= """
		type VarFn= ( fn( ... ) unsafe call_conv( "default" ) );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidCallingConventionForVariadicFunction", 2 ) )


def InvalidCallingConventionForVariadicFunction_Test7():
	c_program_text= """
		type VarFn= ( fn( ... ) unsafe call_conv( "fast" ) );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidCallingConventionForVariadicFunction", 2 ) )


def InvalidSafetyForVariadicFunction_Test0():
	c_program_text= """
		fn Foo( ... ) call_conv( "C" );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidSafetyForVariadicFunction", 2 ) )


def InvalidSafetyForVariadicFunction_Test1():
	c_program_text= """
		type VarFn= ( fn( ... ) call_conv( "C" ) );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidSafetyForVariadicFunction", 2 ) )
