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


def AutoForReturnType_Test7():
	c_program_text= """
		// auto-return functions are also auto-constexpr.
		fn Foo( u32 x ) : auto
		{
			return x / 3u;
		}
		static_assert( Foo( 16u ) == 5u );
	"""
	tests_lib.build_program( c_program_text )


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


def AutoFunctionInsideClass_Test3():
	c_program_text= """
		struct S
		{
			u32 x;
			// "auto" for return reference of a method.
			fn GetXRef( mut this ) : auto &mut
			{
				return x;
			}
		}
		fn Foo()
		{
			var S mut s{ .x= 989u };
			s.GetXRef()= 42u;
			halt if( s.x != 42u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoFunctionInsideClass_Test4():
	c_program_text= """
		struct S
		{
			i32 x;
			// auto-return method is also constexpr.
			template</type T/>
			fn Foo( this ) : auto
			{
				return T(x);
			}
		}
		fn Foo()
		{
			var S s{ .x= 434 };
			static_assert( s.Foo</f32/>() == 434.0f );
			static_assert( s.Foo</u32/>() == 434u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoFunctionInsideClass_Test5():
	c_program_text= """
		struct S
		{
			i32 x;
			// auto-return template method. Should be also auto-constexpr.
			template</type T/>
			fn Foo( this ) : auto
			{
				return T(x);
			}
		}
		fn Foo()
		{
			var S s{ .x= 6565 };
			static_assert( s.Foo</f32/>() == 6565.0f );
			static_assert( s.Foo</u32/>() == 6565u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoReferenceNotation_Test0():
	c_program_text= """
		fn Bar( i32& x, i32& y ) : auto&
		{
			return x; // return only first arg
		}
		var [ [ char8, 2 ], 1 ] expected_return_references[ "0_" ];
		static_assert( typeinfo</ typeof(Bar) />.return_references == expected_return_references );
		fn Foo()
		{
			var i32 x= 42, y= 34;
			halt if( Bar( x, y ) != 42 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoReferenceNotation_Test1():
	c_program_text= """
		fn Bar( i32& x, i32& y ) : auto&
		{
			return y; // return only second arg
		}
		var [ [ char8, 2 ], 1 ] expected_return_references[ "1_" ];
		static_assert( typeinfo</ typeof(Bar) />.return_references == expected_return_references );
		fn Foo()
		{
			var i32 x= 789, y= 123;
			halt if( Bar( x, y ) != 123 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoReferenceNotation_Test2():
	c_program_text= """
		fn Bar( i32& x, i32& y ) : auto&
		{
			return select( x > y ? y : x ); // return both args
		}
		var [ [ char8, 2 ], 2 ] expected_return_references[ "0_", "1_" ];
		static_assert( typeinfo</ typeof(Bar) />.return_references == expected_return_references );
		fn Foo()
		{
			var i32 x= 67, y= 34;
			halt if( Bar( x, y ) != 34 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoReferenceNotation_Test3():
	c_program_text= """
		struct S{ i32& r; }
		fn Bar( S a, S b ) : auto&
		{
			return a.r; // return inner reference of first arg
		}
		var [ [ char8, 2 ], 1 ] expected_return_references[ "0a" ];
		static_assert( typeinfo</ typeof(Bar) />.return_references == expected_return_references );
		fn Foo()
		{
			var i32 x= 444, y= 555;
			var S a{ .r= x }, b{ .r= y };
			halt if( Bar( a, b ) != 444 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoReferenceNotation_Test4():
	c_program_text= """
		struct S{ i32& r; }
		fn Bar( S& a, S& b ) : auto&
		{
			return b.r; // return inner reference of second arg
		}
		var [ [ char8, 2 ], 1 ] expected_return_references[ "1a" ];
		static_assert( typeinfo</ typeof(Bar) />.return_references == expected_return_references );
		fn Foo()
		{
			var i32 x= 654, y= 987;
			var S a{ .r= x }, b{ .r= y };
			halt if( Bar( a, b ) != 987 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoReferenceNotation_Test5():
	c_program_text= """
		struct S{ i32& r; }
		fn Bar( i32& x, i32& y ) : auto
		{
			var S s{ .r= x };
			return s; // return inside "s" reference to first arg
		}
		var tup[ [ [ char8, 2 ], 1 ] ] expected_return_inner_references[ [ "0_" ] ];
		static_assert( typeinfo</ typeof(Bar) />.return_inner_references == expected_return_inner_references );
		fn Foo()
		{
			var i32 x= 454, y= 787;
			halt if( Bar( x, y ).r != 454 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoReferenceNotation_Test6():
	c_program_text= """
		struct S{ i32& r; }
		fn Bar( i32& x, i32& y ) : auto
		{
			var S s{ .r= select( x > y ? x : y ) };
			return s; // return inside "s" references to first args
		}
		var tup[ [ [ char8, 2 ], 2 ] ] expected_return_inner_references[ [ "0_", "1_" ] ];
		static_assert( typeinfo</ typeof(Bar) />.return_inner_references == expected_return_inner_references );
		fn Foo()
		{
			var i32 x= 77, y= 99;
			halt if( Bar( x, y ).r != 99 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoReferenceNotation_Test7():
	c_program_text= """
		struct S{ i32& r; }
		fn Bar( bool b, S s ) : auto
		{
			return s; // Return just copy of arg (including its internal reference)
		}
		var tup[ [ [ char8, 2 ], 1 ] ] expected_return_inner_references[ [ "1a" ] ];
		static_assert( typeinfo</ typeof(Bar) />.return_inner_references == expected_return_inner_references );
		fn Foo()
		{
			var i32 x= 6789;
			var S s{ .r= x };
			halt if( Bar( false, s ).r != 6789 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoReferenceNotation_Test8():
	c_program_text= """
		struct S{ i32& r; }
		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( S &mut s, i32& x ) @(pollution) {}

		fn Bar( S &mut s, i32& x ) : auto
		{
			// Return nothing, but perform reference pollution.
			MakePollution( s, x );
		}
		var [ [ [ char8, 2 ], 2 ], 1 ] expected_references_pollution[ [ "0a", "1_" ] ];
		static_assert( typeinfo</ typeof(Bar) />.references_pollution == expected_references_pollution );
		fn Foo()
		{
			var i32 x= 6789, y= 765;
			var S mut s{ .r= x };
			Bar( s, y );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoReferenceNotation_Test9():
	c_program_text= """
		struct S{ i32& r; }
		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( S &mut s, i32& x ) @(pollution) {}

		fn Bar( i32& x , S &mut s ) : auto
		{
			// Perform reference pollution.
			MakePollution( s, x );
			// And return u32.
			return u32( s.r );
		}
		var [ [ [ char8, 2 ], 2 ], 1 ] expected_references_pollution[ [ "1a", "0_" ] ];
		static_assert( typeinfo</ typeof(Bar) />.references_pollution == expected_references_pollution );
		fn Foo()
		{
			var i32 x= 333, y= 666;
			var S mut s{ .r= x };
			halt if( Bar( y, s ) != 333u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
