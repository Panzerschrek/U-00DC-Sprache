from py_tests_common import *


def AutoForReturnType_Test0():
	c_program_text= """
		fn Div( i32 x, i32 y ) : auto
		{
			return x / y; // Deduced to "i32"
		}
		fn Foo( i32 x, i32 y ) : i32
		{
			return Div( x, y );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Fooii", 854, 44 )
	assert( call_result == int(854 / 44) )


def AutoForReturnType_Test1():
	c_program_text= """
		fn Abs( f32 x ) : auto
		{
			// Deduced same type - "f32" in all branches.
			if( x >= 0.0f ) { return x; }
			return -x;
		}
		fn Foo()
		{
			halt if( Abs( 0.25f ) != 0.25f );
			halt if( Abs( 1536.0f ) != 1536.0f );
			halt if( Abs( -55.1f ) != 55.1f );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def AutoForReturnType_Test2():
	c_program_text= """
		struct S{ i32 x; }
		fn GetS( i32 x ) : auto
		{
			var S s{ .x= x };
			return s; // "S" deduced.
		}

		fn Foo()
		{
			halt if( GetS( 42 ).x != 42 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def AutoForReturnType_Test3():
	c_program_text= """
		// Auto for reference value.
		fn Max( u64& x, u64& y ) : auto&
		{
			if( x > y ) { return x; }
			return y;
		}
		fn Foo()
		{
			halt if( Max( 45u64, 11u64 ) != 45u64 );
			halt if( Max( 45u64, 174278u64 ) != 174278u64 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def AutoForReturnType_Test4():
	c_program_text= """
		fn Foo() : auto
		{
			// If no "return" inside function - function return type deduced to "void"
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def AutoForReturnType_Test5():
	c_program_text= """
		fn Foo() : auto
		{
			return; // Return type deduced to "void".
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
