from py_tests_common import *


def AllocaDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			// Size is static.
			alloca i32 arr[ 16s ];
			static_assert( same_type</ arr, $(i32) /> );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AllocaOperator_Test0():
	c_program_text= """
		fn Foo()
		{
			unsafe
			{
				// Size is static.
				var $(f64) mem= alloca</f64/>(32s);
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AllocaOperator_Test1():
	c_program_text= """
		fn Foo(u32 size)
		{
			unsafe
			{
				// Size is dynamic.
				var $(u16) mem= alloca</u16/>( size_type(size) );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Fooj", 128 )


def AllocaOperator_Test2():
	c_program_text= """
		fn Foo(u32 size)
		{
			// Conditional "alloca"
			if( size % 2u == 0u )
			{
				unsafe
				{
					var $(char32) mem= alloca</ char32 />( size_type(size) );
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Fooj", 33 )
	tests_lib.run_function( "_Z3Fooj", 34 )


def AllocaOperator_Test3():
	c_program_text= """
		// Class with no constructor and destructor which halts.
		class C
		{
			i32 field;

			fn destructor()
			{
				halt;
			}
		}
		fn Foo()
		{
			unsafe
			{
				// Memory is allocated, but no constructors/destructors are called.
				var $(C) mem= alloca</ C />( 20s );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AllocaOperator_Test4():
	c_program_text= """
		fn Foo(u32 size)
		{
			// Potentially use heap fallback.
			unsafe
			{
				var $(byte8) mem= alloca</ byte8 />( size_type(size) );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Fooj", 80 )
	tests_lib.run_function( "_Z3Fooj", 800 )
	tests_lib.run_function( "_Z3Fooj", 8000 )
	tests_lib.run_function( "_Z3Fooj", 80000 )


def AllocaOutsideUnsafeBlock_Test0():
	c_program_text= """
		fn Foo()
		{
			auto ptr= alloca</i32/>(4s);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AllocaOutsideUnsafeBlock", 4 ) )


def AllocaOutsideUnsafeBlock_Test1():
	c_program_text= """
		fn Foo()
		{
			unsafe
			{
				safe
				{
					auto ptr= alloca</i32/>(4s);
				}
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AllocaOutsideUnsafeBlock", 8 ) )


def AllocaOutsideUnsafeBlock_Test2():
	c_program_text= """
		fn Foo()
		{
			unsafe
			{
				auto ptr= safe( alloca</i32/>(4s) );
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AllocaOutsideUnsafeBlock", 6 ) )


def AllocaOutsideUnsafeBlock_Test3():
	c_program_text= """
		fn Foo()
		{
			// Ok - use unsafe expression.
			auto ptr= unsafe( alloca</i32/>(4s) );
		}
	"""
	tests_lib.build_program( c_program_text )


def AllocaInsideLoop_Test0():
	c_program_text= """
		fn Foo()
		{
			loop
			{
				auto ptr= unsafe( alloca</i32/>(4s) );
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AllocaInsideLoop", 6 ) )


def AllocaInsideLoop_Test1():
	c_program_text= """
		fn Foo()
		{
			for( auto mut i= 0; i < 10; ++i )
			{
				if( i % 2 != 0 )
				{
					auto ptr= unsafe( alloca</i32/>(4s) );
				}
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AllocaInsideLoop", 8 ) )


def AllocaInsideLoop_Test2():
	c_program_text= """
		fn Foo()
		{
			while( Bar() )
			{
				auto ptr= unsafe( alloca</f32/>(16s) );
			}
		}
		fn Bar() : bool;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AllocaInsideLoop", 6 ) )


def AllocaInsideCorouine_Test0():
	c_program_text= """
		fn async Foo()
		{
			auto ptr= unsafe( alloca</f32/>(16s) );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AllocaInsideCorouine", 4 ) )


def AllocaInsideCorouine_Test1():
	c_program_text= """
		fn generator Foo()
		{
			auto ptr= unsafe( alloca</u32/>(8s) );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AllocaInsideCorouine", 4 ) )
