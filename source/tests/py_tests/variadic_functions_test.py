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
		fn Bar( ... ) unsafe call_conv( "C" ) : i32 { return 563; }
		fn Foo()
		{
			unsafe( Bar() ); // Pass zero variadic args.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VariadicFunctionCall_Test1():
	c_program_text= """
		fn Bar( ... ) unsafe call_conv( "C" ) : i32 { return 77; }
		fn Foo()
		{
			var f32 mut f= 0.25f;
			unsafe( Bar( 67u, Baz(), f ) ); // Pass several variadic args.
		}
		fn Baz() : u64 { return 56789034567u; }
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VariadicAndNonVariadicFunctionTypesAreNotEqual_Test0():
	c_program_text= """
		fn Foo( ... ) unsafe call_conv( "C" );
		fn Bar() unsafe call_conv( "C" );
		static_assert( !same_type</ typeof( Foo ), typeof( Bar ) /> );
		static_assert( !same_type</ ( fn( i32 x ) unsafe call_conv( "C" ) : i32 ), ( fn( i32 x, ... ) unsafe call_conv( "C" ) : i32 ) /> );
	"""
	tests_lib.build_program( c_program_text )


def VariadicAndNonVariadicFunctionTypesAreNotEqual_Test1():
	c_program_text= """
		fn Foo( ... ) unsafe call_conv( "C" );
		fn Bar() unsafe call_conv( "C" );
		var ( fn() unsafe call_conv( "C" ) ) foo_ptr( Foo );
		var ( fn( ... ) unsafe call_conv( "C" ) ) bar_ptr( Bar );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) >= 2 )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 4 ) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 5 ) )


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


def UnsupportedVariadicArgumentType_Test0():
	c_program_text= """
		fn Bar( ... ) unsafe call_conv( "C" );
		fn Foo()
		{
			unsafe( Bar( void() ) ); // Passing "void" is not supported.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsupportedVariadicArgumentType", 5 ) )


def UnsupportedVariadicArgumentType_Test1():
	c_program_text= """
		fn Bar( ... ) unsafe call_conv( "C" );
		fn Foo()
		{
			var [ i32, 3 ] arr= zero_init;
			unsafe( Bar( arr ) ); // Passing arrays is not supported.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsupportedVariadicArgumentType", 6 ) )


def UnsupportedVariadicArgumentType_Test2():
	c_program_text= """
		fn Bar( ... ) unsafe call_conv( "C" );
		fn Foo()
		{
			var tup[ f32 ] t= zero_init;
			unsafe( Bar( t ) ); // Passing tuples is not supported.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsupportedVariadicArgumentType", 6 ) )


def UnsupportedVariadicArgumentType_Test3():
	c_program_text= """
		fn Bar( ... ) unsafe call_conv( "C" );
		fn Foo()
		{
			var S s= zero_init;
			unsafe( Bar( s ) ); // Passing structs is not supported.
		}
		struct S{ i32 x; f32 y; bool z; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsupportedVariadicArgumentType", 6 ) )


def CouldNotOverloadFunction_ForVariadicFunction_Test0():
	c_program_text= """
		fn Foo( i32 x, ... ) unsafe call_conv( "C" );
		fn Foo( f32 x );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CouldNotOverloadFunction", 2 ) or HasError( errors_list, "CouldNotOverloadFunction", 3 ) )


def CouldNotOverloadFunction_ForVariadicFunction_Test1():
	c_program_text= """
		fn Foo( f32 x, i32 y, f64 z );
		fn Foo( ... ) unsafe call_conv( "C" );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CouldNotOverloadFunction", 2 ) or HasError( errors_list, "CouldNotOverloadFunction", 3 ) )


def CouldNotOverloadFunction_ForVariadicFunction_Test2():
	c_program_text= """
		fn Foo();
		fn Foo( f32 x, i32 y, f64 z, ... ) unsafe call_conv( "C" );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CouldNotOverloadFunction", 2 ) or HasError( errors_list, "CouldNotOverloadFunction", 3 ) )


def VariadicFunctionTemplate_Test0():
	c_program_text= """
		template</ type T />
		fn Foo( T t, ... ) unsafe call_conv( "C" ) {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariadicFunctionTemplate", 3 ) )


def VariadicLambda_Test0():
	c_program_text= """
		fn Foo()
		{
			auto l= lambda( ... ) unsafe call_conv( "C" ) {};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariadicLambda", 4 ) )


def VariadicCoroutine_Test0():
	c_program_text= """
		fn generator Foo( ... ) unsafe call_conv( "C" );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariadicCoroutine", 2 ) )


def VariadicCoroutine_Test1():
	c_program_text= """
		fn async Foo( ... ) unsafe call_conv( "C" );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariadicCoroutine", 2 ) )


def VariadicMemberFunction_Test0():
	c_program_text= """
		struct S
		{
			fn Foo( ... ) unsafe call_conv( "C" ); // Variadic non-this-call function.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariadicMemberFunction", 4 ) )


def VariadicMemberFunction_Test1():
	c_program_text= """
		class S
		{
			fn Foo( this, ... ) unsafe call_conv( "C" ); // Variadic this-call function.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariadicMemberFunction", 4 ) )


def VariadicMemberFunction_Test2():
	c_program_text= """
		class SomeInterface interface
		{
			fn virtual pure Foo( this, ... ) unsafe call_conv( "C" ); // Variadic this-call virtual function.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariadicMemberFunction", 4 ) )


def VariadicMemberFunction_Test3():
	c_program_text= """
		struct S
		{
			op()( mut this,  ... ) unsafe call_conv( "C" ); // Variadic operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariadicMemberFunction", 4 ) )
