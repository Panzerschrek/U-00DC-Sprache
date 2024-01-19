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


def AutoForReturnType_Test6():
	c_program_text= """
		fn nomangle Foo() : auto // auto + nomangle
		{
			return 54;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "Foo" )
	assert( call_result == 54 )


def AutoForFunctionTemplate_Test0():
	c_program_text= """
		template</ type T />
		fn GetX( T& t ) : auto { return t.x; }

		struct A{ i32 x; }
		struct B{ f32 x; }
		struct C{ A x; }
		fn Foo()
		{
			var A a{ .x= 58 };
			var B b{ .x= -95.4f };
			var C c{ .x{ .x= 999 } };
			halt if( GetX(a) != 58 );
			halt if( GetX(b) != -95.4f );
			halt if( GetX(c).x != 999 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def AutoForFunctionTemplate_Test1():
	c_program_text= """
		template</ type T />
		fn Pass( T t ) : auto { return t; }

		// template function with "auto" for return type may be constexpr.
		static_assert( Pass(99954) == 99954 );
		static_assert( Pass(false) == false );
		static_assert( Pass(-85.1) == -85.1 );
	"""
	tests_lib.build_program( c_program_text )


def AutoFunctionInsideClass_Test0():
	c_program_text= """
		struct S
		{
			// auto static method.
			fn Foo() : auto { return 384; }
		}
		fn Foo()
		{
			var i32 expected_result= 384;
			halt if( S::Foo() != expected_result );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoFunctionInsideClass_Test1():
	c_program_text= """
		class S
		{
			i32 x= 0;
			// Auto this-call method.
			fn Foo( mut this, i32 new_x ) : auto
			{
				var f32 res= f32(x);
				x= new_x;
				return res;
			}
		}
		fn Foo()
		{
			var S mut s;
			s.x= 643;
			halt if( s.Foo( 134 ) != 643.0f );
			halt if( s.x != 134 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoFunctionInsideClass_Test2():
	c_program_text= """
		struct S
		{
			f32 x;
			// auto-return for "byval this" method.
			fn Foo( byval this ) : auto
			{
				return u64(x) + 15u64;
			}
		}
		fn Foo()
		{
			var S s{ .x= 887.3f };
			halt if( s.Foo() != u64( 887 + 15 ) );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
