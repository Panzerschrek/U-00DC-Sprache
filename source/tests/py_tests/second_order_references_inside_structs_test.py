from py_tests_common import *


def SecondOrderReferenceInsideStructUsage_Test0():
	c_program_text= """
		struct A{ i32 &mut x; }
		// Reference field of type with a reference inside.
		struct B{ A &imut a; }
		fn Foo()
		{
			var i32 mut x= 17;
			{
				var A a{ .x= x };
				var B b{ .a= a };

				b.a.x -= 5;
				halt if( a.x != 12 );
			}
			halt if( x != 12 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SecondOrderReferenceInsideStructUsage_Test1():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Foo()
		{
			var i32 mut x= 987;
			{
				var A a{ .x= x };
				var A& a_ref= a;
				var B b{ .a= a_ref };
				var B& b_ref= b;

				b_ref.a.x /= 3;
				halt if( a_ref.x != 987 / 3 );
			}
			halt if( x != 987 / 3 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SecondOrderReferenceInsideStructUsage_Test2():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Bar( B& b )
		{
			// Access second order reference of an argument to modify referenced value.
			b.a.x *= 5;
		}
		fn Foo()
		{
			var i32 mut x= 13;
			{
				var A a{ .x= x };
				var B b{ .a= a };

				Bar( b );
				halt if( a.x != 13 * 5 );
			}
			halt if( x != 13 * 5 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SecondOrderReferenceInsideStructUsage_Test4():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }

		// Return a reference to a variable with second order references inside.
		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0a" ] ];
		fn Pass( B& b ) : B @(return_inner_references) & @(return_references)
		{
			return b;
		}

		fn Foo()
		{
			var i32 mut x= 1765;
			{
				var A a{ .x= x };
				var B b{ .a= a };

				Pass(b).a.x += 15;
				halt if( a.x != 1765 + 15 );
			}
			halt if( x != 1765 + 15 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SecondOrderReferenceInsideStructUsage_Test5():
	# Second order references inside global variables.
	c_program_text= """
		struct A{ i32 &imut x; }
		struct B{ A &imut a; }

		var i32 x= 78;
		var A a{ .x= x };
		static_assert( a.x == 78 );
		var B b{ .a= a };
		static_assert( b.a.x == 78 );
	"""
	tests_lib.build_program( c_program_text )


def SecondOrderReferenceInsideStructUsage_Test6():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }

		// Return a reference to contents of an argument.
		var [ [ char8, 2 ], 1 ] return_references[ "0a" ];
		fn Bar( B& b ) : A & @(return_references)
		{
			return b.a;
		}

		fn Foo()
		{
			var i32 mut x= 87;
			{
				var A a{ .x= x };
				var B b{ .a= a };

				Bar(b).x -= 10;
				halt if( a.x != 77 );
			}
			halt if( x != 77 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SecondOrderReferenceInsideStructUsage_Test7():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		struct C{ i32 & @("a"c8) i; B @("b") b; }

		// Return a reference to contents of an argument.
		var [ [ char8, 2 ], 1 ] return_references[ "1b" ];
		fn Bar( i32& y, C& c ) : A & @(return_references)
		{
			return c.b.a;
		}

		fn Foo()
		{
			var i32 mut x= 765;
			{
				var A a{ .x= x };
				var i32 i= 88;
				var C c{ .i= i, .b{ .a= a } };

				Bar( 7, c ).x /= 5;
				halt if( a.x != 765 / 5 );
			}
			halt if( x != 765 / 5 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SecondOrderReferenceInsideStructUsage_Test8():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }

		// Return a reference to type with references inside inside a struct.
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0a" ] ];
		fn Bar( B& b ) : B @(return_inner_references)
		{
			return b;
		}

		fn Foo()
		{
			var i32 mut x= 15;
			{
				var A a{ .x= x };
				var B b{ .a= a };

				var B b_copy= Bar(b);
				b_copy.a.x -= 76;
				halt if( b.a.x != 15 - 76 );
			}
			halt if( x != 15 - 76 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SecondOrderReferenceInsideStructUsage_Test9():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		struct BWrapper{ B b; }

		// Return a reference to type with references inside inside a struct.
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0a" ] ];
		fn Bar( BWrapper b_wrapper ) : B @(return_inner_references)
		{
			return b_wrapper.b;
		}

		fn Foo()
		{
			var i32 mut x= 159972;
			{
				var A a{ .x= x };
				var BWrapper b_wrapper{ .b{ .a= a } };

				var B b_copy= Bar(b_wrapper);
				b_copy.a.x /= 31;
				halt if( b_wrapper.b.a.x != 159972 / 31 );
			}
			halt if( x != 159972 / 31 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SecondOrderReferenceInsideStructUsage_Test10():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }

		// Return a reference to contents of an argument.
		fn Bar( B& b ) : auto &
		{
			return b.a;
		}

		var [ [ char8, 2 ], 1 ] expected_return_references[ "0a" ];
		var tup[ [ [ char8, 2 ], 0 ] ] expected_return_inner_references[ [] ];
		static_assert( typeinfo</ typeof(Bar) />.return_references == expected_return_references );
		// Inner references list is empty, since it's impossible to list in it second order reference.
		static_assert( typeinfo</ typeof(Bar) />.return_inner_references == expected_return_inner_references );

		fn Foo()
		{
			var i32 mut x= 987;
			{
				var A a{ .x= x };
				var B b{ .a= a };

				Bar(b).x -= 10;
				halt if( a.x != 977 );
			}
			halt if( x != 977 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SecondOrderReference_InLambdaCapture_Test0():
	c_program_text= """
		struct A{ i32 &mut x; }

		fn Foo()
		{
			var i32 mut x= 7867;
			var A a{ .x= x };
			auto f = lambda[&]() : i32
			{
				// Implicitly capture here "a" by reference.
				auto prev= a.x;
				a.x/= 3;
				return prev;
			};
			halt if( f() != 7867 );
			halt if( a.x != 7867 / 3 );
		}
	"""
	tests_lib.build_program( c_program_text )


def SecondOrderReference_InLambdaCapture_Test1():
	c_program_text= """
		struct A{ i32 &mut x; }

		fn Foo()
		{
			var i32 mut x= 165;
			var A a{ .x= x };
			auto f = lambda[&a]() : i32 // Explicitly capture here "a" by reference.
			{
				auto prev= a.x;
				a.x/= 5;
				return prev;
			};
			halt if( f() != 165 );
			halt if( a.x != 165 / 5 );
		}
	"""
	tests_lib.build_program( c_program_text )


def SecondOrderReference_InLambdaCapture_Test2():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A& a; }
		fn Foo()
		{
			var i32 mut x= 897;
			var A a{ .x= x };
			var B b{ .a= a };
			auto f = lambda[=]() : i32
			{
				// Implicitly capture here "b" by value.
				auto prev= b.a.x;
				b.a.x/= 17;
				return prev;
			};
			halt if( f() != 897 );
			halt if( a.x != 897 / 17 );
		}
	"""
	tests_lib.build_program( c_program_text )


def SecondOrderReference_InLambdaCapture_Test3():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A& a; }
		fn Foo()
		{
			var i32 mut x= -865;
			var A a{ .x= x };
			var B b{ .a= a };
			auto f = lambda[b]() : i32 // Explicitly capture here "b" by value.
			{
				auto prev= b.a.x;
				b.a.x/= 12;
				return prev;
			};
			halt if( f() != -865 );
			halt if( a.x != -865 / 12 );
		}
	"""
	tests_lib.build_program( c_program_text )


def SecondOrderReference_InLambdaCapture_Test4():
	c_program_text= """
		struct A{ i32 &mut x; }

		fn Foo()
		{
			var i32 mut x= 87;
			var A a{ .x= x };
			auto f = lambda[&]() : A&
			{
				// Return reference to captured reference to type with a reference inside.
				return a;
			};
			halt if( f().x != 87 );
		}
	"""
	tests_lib.build_program( c_program_text )


def SecondOrderReference_InLambdaCapture_Test5():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A& a; }
		fn Foo()
		{
			var i32 mut x= 7891;
			var A a{ .x= x };
			var B b{ .a= a };
			auto f = lambda[b]() : A&
			{
				// Return inner reference of captured variable, which has second order reference inside.
				return b.a;
			};
			halt if( f().x != 7891 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test0():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A   &imut a; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			var A& a0= b.a; // "a0.x" points to "a.x".
			var A& a1= b.a; // Error - creating second node pointing to "a.x" inside "a1".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 10 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test1():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A   &imut a; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			var A& a0= b.a; // "a0.x" points to "a.x".
			auto& x_ref= a.x; // Error - creating second node pointing to "a.x".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 10 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test2():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A   &imut a; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			auto &x_ref= a.x; // "x_ref" points to "a.x" inner reference node.
			var A &a_ref= b.a; // Error - creating second node pointing to "a.x" inside "a_ref".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 10 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test3():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A   &imut a; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };
			var B& b_ref= b;

			var A& a0= b_ref.a; // "a0.x" points to "a.x".
			var A& a1= b_ref.a; // Error - creating second node pointing to "a.x" inside "a1".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 9 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 11 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test4():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A   &imut a; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };
			var B& b_ref= b;

			var A& a0= b_ref.a; // "a0.x" points to "a.x".
			auto& x_ref= a.x; // Error - creating second node pointing to "a.x".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 9 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 11 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test5():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A   &imut a; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };
			var B& b_ref= b;

			auto &x_ref= a.x; // "x_ref" points to "a.x" inner reference node.
			var A &a_ref= b_ref.a; // Error - creating second node pointing to "a.x" inside "a_ref".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 9 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 11 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test6():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A   &imut a; }
		struct BWrapper{ B b; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var BWrapper b_wrapper{ .b{ .a= a } };

			var A& a0= b_wrapper.b.a; // "a0.x" points to "a.x".
			var A& a1= b_wrapper.b.a; // Error - creating second node pointing to "a.x" inside "a1".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 11 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test7():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A   &imut a; }
		struct BWrapper{ B b; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var BWrapper b_wrapper{ .b{ .a= a } };

			auto &x_ref= a.x; // "x_ref" points to "a.x" inner reference node.
			var A &a_ref= b_wrapper.b.a; // Error - creating second node pointing to "a.x" inside "a_ref".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 11 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test8():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A   &imut a; }
		struct BWrapper{ B b; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };
			var BWrapper b_wrapper{ .b{ .a= a } };
			var BWrapper& b_wrapper_ref= b_wrapper;
			var B& b_ref= b_wrapper_ref.b;

			var A& a0= b_ref.a; // "a0.x" points to "a.x".
			var A& a1= b_ref.a; // Error - creating second node pointing to "a.x" inside "a1".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 11 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 12 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 14 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 15 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test9():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut @("b"c8) a; i32 &mut @("a"c8) y; i32 &imut @("c"c8) z; }
		fn Foo()
		{
			var i32 mut x= 0, mut y=0 , imut z= 0;
			var A a{ .x= x };
			var B b{ .a= a, .y= y, .z= z };

			var A& a0= b.a; // "a0.x" points to "a.x".
			var A& a1= b.a; // Error - creating second node pointing to "a.x" inside "a1".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 10 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test10():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a;}
		struct BWrapper{ B @("c") b; i32 &mut @("a"c8) y; i32 &imut @("b"c8) z; }
		fn Foo()
		{
			var i32 mut x= 0, mut y=0 , imut z= 0;
			var A a{ .x= x };
			var BWrapper b_wrapper{ .b{ .a= a }, .y= y, .z= z };
			var BWrapper& b_wrapper_ref= b_wrapper;
			var B& b_ref= b_wrapper.b;

			var A& a0= b_ref.a; // "a0.x" points to "a.x".
			var A& a1= b_ref.a; // Error - creating second node pointing to "a.x" inside "a1".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 10 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 11 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 13 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 14 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test11():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct Q{ A a; f32 &mut y; }
		struct B{ A   &imut a; }
		fn Foo()
		{
			var i32 mut x= 0;
			var f32 mut y= 0.0f;
			var Q q{ .a{ .x= x }, .y= y };
			var B b{ .a= q.a };

			var A& a0= b.a; // "a0.x" points to "q.a.x".
			var A& a1= b.a; // Error - creating second node pointing to "q.a.x" inside "a1".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 12 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 13 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test12():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct Q{ A a; f32 &mut y; }
		struct B{ A   &imut a; }
		fn Foo()
		{
			var i32 mut x= 0;
			var f32 mut y= 0.0f;
			var Q q{ .a{ .x= x }, .y= y };
			var Q& q_ref= q;
			var B b{ .a= q_ref.a };

			var A& a0= b.a; // "a0.x" points to "q.a.x".
			var A& a1= b.a; // Error - creating second node pointing to "q.a.x" inside "a1".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 10 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 11 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 13 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 14 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test13():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct Q{ A a; f32 &mut y; }
		struct B{ A   &imut a; }
		fn Foo()
		{
			var i32 mut x= 0;
			var f32 mut y= 0.0f;
			var Q q{ .a{ .x= x }, .y= y };
			var Q& q_ref= q;
			var A& a_ref= q_ref.a;
			var B b{ .a= a_ref };

			var A& a0= b.a; // "a0.x" points to "q.a.x".
			var A& a1= b.a; // Error - creating second node pointing to "q.a.x" inside "a1".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 10 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 11 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 12 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 14 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 15 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test14():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }

		// Return a reference to a variable with second order references inside.
		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0a" ] ];
		fn Pass( B& b ) : B @(return_inner_references) & @(return_references)
		{
			return b;
		}

		fn Foo()
		{
			var i32 mut x= 1765;
			var A a{ .x= x };
			var B b{ .a= a };

			var i32 &mut x_ref0= Pass(b).a.x;
			var i32 &mut x_ref1= a.x; // Error - creating second mutable reference to "a.x".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 19 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 20 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test15():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }

		// Return a reference to a variable with second order references inside.
		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0a" ] ];
		fn Pass( B& b ) : B @(return_inner_references) & @(return_references)
		{
			return b;
		}

		fn Foo()
		{
			var i32 mut x= 1765;
			var A a{ .x= x };
			var B b{ .a= a };

			var i32 &mut x_ref= a.x;
			Pass(b); // Error - creating second mutable reference to "a.x".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 19 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 20 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test16():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }

		type ATup3= tup[ A, A, A ];

		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		var tup[ [ [ char8, 2 ], 3 ] ] return_inner_references[ [ "0a", "0b", "0c" ] ];
		fn Bar( ATup3& a_tup ) : A @(return_inner_references) & @(return_references );

		fn Foo()
		{
			var i32 mut x= 0, mut y= 0, mut z= 0;
			// This variable has 3 inner reference tags inside.
			var ATup3 a_tup[ { .x= x }, { .x= y }, { .x= z } ];
			// "b.a" has a derived from "a_tup" reference, but because of function indirection not from a "a_tup" element.
			var B b{ .a= Bar( a_tup ) };

			var A& a0= b.a; // "a0.x" points to "a_tup.x".
			var A& a1= b.a; // Error - creating second node pointing to "a_tup.x" inside "a1".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 15 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 17 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 19 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 20 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test17():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }

		type ATup3= tup[ A, A, A ];

		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		var tup[ [ [ char8, 2 ], 3 ] ] return_inner_references[ [ "0a", "0b", "0c" ] ];
		fn Bar( ATup3& a_tup ) : A @(return_inner_references) & @(return_references );

		fn Baz( B& b0, B& b1 );

		fn Foo()
		{
			var i32 mut x= 0, mut y= 0, mut z= 0;
			// This variable has 3 inner reference tags inside.
			var ATup3 a_tup[ { .x= x }, { .x= y }, { .x= z } ];
			// "b.a" has a derived from "a_tup" reference, but because of function indirection not from a "a_tup" element.
			var B b{ .a= Bar( a_tup ) };

			// Error here. Passing "b" as immutable reference twice is ok.
			// Passing it twice is also ok, since it contains only an immutable reference inside.
			// But it's not ok, since second order reference is mutable.
			// So, passing "b" twice may lead to mutable sharid "b.a.x".
			Baz( b, b );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 17 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 19 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 25 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test18():
	c_program_text= """
		struct A{ i32 &mut x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			auto f= lambda[&a]() : A& // Explicitly capture "a" by reference. "f" holds a second order reference to "x" via "a".
			{
				return a;
			};

			auto& a0= f(); // Creating a reference to "a" with inner reference pointing to "x".
			f(); // This call requires creating second mutable reference to "x".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 7 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 9 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 12 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 13 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test19():
	c_program_text= """
		struct A{ i32 &mut x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			auto f= lambda[&]()
			{
				// Implicitly capture "a" by reference. "f" holds a second order reference to "x" via "a".
				auto& a_ref= a;
			};

			auto& x_ref= a.x; // Fine, take reference to "x".
			f(); // This call requires creating second mutable reference to "x".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 7 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 10 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 13 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 14 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test20():
	c_program_text= """
		struct A{ i32 &mut x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			auto f= lambda[&a]() : A& // Explicitly capture "a" by reference. "f" holds a second order reference to "x" via "a".
			{
				return a;
			};

			auto& a0= f(); // Creating a reference to "a" with inner reference pointing to "x".
			auto &mut x_ref= a.x; // Creating second mutable reference to "x".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 7 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 9 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 12 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 13 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test21():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A& a; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };
			auto f= lambda[=]() : A& // Explicitly capture "b" by copy. "f" holds a second order reference to "x" via "b.a".
			{
				return b.a;
			};

			auto& a0= f(); // Creating a reference to "a" with inner reference pointing to "x".
			auto &mut x_ref= a.x; // Creating second mutable reference to "x".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 9 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 11 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 14 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 15 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test22():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A& a; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			auto f= lambda[ b= B{ .a= a } ]() // Create capture variable which captures reference to "a".
			{
				auto& b_ref= b;
			};

			auto &mut x_ref= a.x; // Creating a mutable reference to "x".
			f(); // Error - this call will create internally second mutable reference to "x".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 8 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 13 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 14 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test23():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Foo( B& b )
		{
			auto& x= b.a.x; // Create a mutable reference to second order reference of an argument.
			b.a; // Error here - accessing reference field "a" creates for it a mutable inner reference pointing to second order reference of this argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 6 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test24():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Foo( B& b )
		{
			auto& x= b.a.x; // Create a mutable reference to second order reference of an argument.
			Bar(b); // Error here - calling a function creates a second mutable reference to second order reference of an argument.
		}
		fn Bar( B& b );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 6 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test25():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B
		{
			A& a;

			fn constructor( mut this, A& in_a ) @(reference_pollution)
				( a= in_a )
			{}

			var [ [ [ char8, 2 ], 2 ], 1 ] reference_pollution[ [ "0a", "1_" ] ];
		}
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b(a); // Create pollution via a constructor.

			var A& a0= b.a; // "a0.x" points to "a.x".
			var A& a1= b.a; // Error - creating second node pointing to "a.x" inside "a1".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 19 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 20 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test26():
	c_program_text= """
		struct A{ i32 &mut x; }
		fn Foo( A& a )
		{
			auto f= lambda[&]()
			{
				auto& x= a.x; // Create a mutable reference to second order reference of lambda "this".
				a.x; // Error here - accessing reference field "a" of lambda creates for it a mutable inner reference pointing to second order reference of this argument.
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 7 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test27():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Foo(B& b)
		{
			auto f= lambda [b]()
			{
				auto& x= b.a.x; // Create a reference to second order reference of a captured by value variable.
				b.a; // Error, create inner reference for "b.a" pointing to "x".
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 6 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 8 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 9 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test28():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Foo(B& b)
		{
			auto f= lambda [b] byval ()
			{
				auto& x= b.a.x; // Create a reference to second order reference of a captured by value variable.
				b.a; // Error, create inner reference for "b.a" pointing to "x".
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 6 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 8 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 9 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test29():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Foo(B& b)
		{
			auto f= lambda [b] byval mut ()
			{
				auto& x= b.a.x; // Create a reference to second order reference of a captured by value variable.
				b.a; // Error, create inner reference for "b.a" pointing to "x".
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 6 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 8 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 9 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test30():
	c_program_text= """
		struct A{ i32 &mut x; }
		fn Foo(A& a)
		{
			auto f= lambda [&]()
			{
				auto& x_ref= a.x; // Create a reference to second order reference of a captured by reference variable.
				auto &mut x_ref1= a.x; // Error, create a mutable reference to "x", when an immutable reference already exists.
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 5 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 7 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test31():
	c_program_text= """
		struct A{ i32 &mut x; }
		fn Foo(A& a)
		{
			auto f= lambda [&] byval ()
			{
				auto& x_ref= a.x; // Create a reference to second order reference of a captured by reference variable.
				auto &mut x_ref1= a.x; // Error, create a mutable reference to "x", when an immutable reference already exists.
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 5 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 7 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test32():
	c_program_text= """
		struct A{ i32 &mut x; }
		fn Foo(A& a)
		{
			auto f= lambda [&] byval mut ()
			{
				auto& x_ref= a.x; // Create a reference to second order reference of a captured by reference variable.
				auto &mut x_ref1= a.x; // Error, create a mutable reference to "x", when an immutable reference already exists.
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 5 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 7 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test33():
	c_program_text= """
		struct A{ i32 &mut x; }

		class B polymorph
		{
			A &imut a;

			fn constructor( mut this, A& in_a ) @(reference_pollution)
				( a= in_a )
			{}

			var [ [ [ char8, 2 ], 2 ], 1 ] reference_pollution[ [ "0a", "1_" ] ];
		}

		class BWrapper : B
		{
			fn constructor( mut this, A& in_a ) @(reference_pollution)
				( base( in_a ) )
			{}
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var BWrapper b_wrapper( a );

			var A& a0= b_wrapper.a; // "a0.x" points to "a.x".
			var A& a1= b_wrapper.a; // Error - creating second node pointing to "a.x" inside "a1".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 28 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 29 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test34():
	c_program_text= """
		struct A{ i32 &mut x; }

		class B polymorph
		{
			A &imut a;

			fn constructor( mut this, A& in_a ) @(reference_pollution)
				( a= in_a )
			{}

			var [ [ [ char8, 2 ], 2 ], 1 ] reference_pollution[ [ "0a", "1_" ] ];
		}

		class BWrapper : B
		{
			fn constructor( mut this, A& in_a ) @(reference_pollution)
				( base( in_a ) )
			{}

			fn Bar( this );
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var BWrapper b_wrapper( a );

			var i32& x_ref = b_wrapper.a.x; // "x_ref" points to "x".
			b_wrapper.Bar(); // Method call of a class with references inside causes creation of mutable lock node for second order reference to "x".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 30 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 31 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test35():
	c_program_text= """
		struct A
		{
			i32 &mut x;
			fn constructor();
		}
		struct B
		{
			A &imut a;
			fn constructor();
		}
		fn Foo( bool cond )
		{
			var i32 mut x= 0;
			var A mut a;
			var B mut b;
			// Test here effects of variables state merging after branching for second order references.
			if( cond )
			{
				MakePollution( a, x );
				auto& x_ref= b.a.x; // "x_ref" points here to nothing, since inner reference of "b" points to nothing.
				auto& a_ref= b.a; // Inner reference of "a" points here to nothing.
			}
			else
			{
				MakePollution( b, a );
			}
			// Here it's assumed that "b" points to "a" and "a" points to "x".
			{
				auto& x_ref= x; // Error - can't create a reference to "x", since a mutable reference to it exists inside "a".
			}
			{
				auto& x_ref= b.a.x; // Fine - access "x".
				auto& a_ref= b.a; // Error - creating mutable inner reference node pointing to "x".
			}
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] reference_pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A& mut a, i32 &mut x ) @(reference_pollution);
		fn MakePollution( B& mut b, A &imut a ) @(reference_pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 21 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 22 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 30 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 33 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 34 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test36():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B
		{
			A &imut a;

			fn Foo( this )
			{
				auto& x_ref= a.x; // Fine - access reference field and its inner reference.
				auto& a_ref= a; // Error - create mutable inner reference node to "x", when a reference to it exists.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 9 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 10 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test37():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B
		{
			A &imut a;

			fn Foo( this )
			{
				auto& a_ref0= a; // Fine - create mutable inner reference node to "x".
				auto& a_ref1= a; // Error - create second mutable inner reference node to "x".
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 9 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 10 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_Test38():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B
		{
			A &imut a;

			fn Foo( this )
			{
				auto& x_ref= a.x; // Fine - access reference field and its inner reference.
				auto& this_ref= this; // Fine - create a reference to "this".
				this_ref.a; // Error - create mutable inner reference node to "x", when a reference to it exists.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 9 ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 10 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test0():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Bar( B& b0, B& b1 );
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			// Error here. Passing "b" as immutable reference twice is ok.
			// Passing it twice is also ok, since it contains only an immutable reference inside.
			// But it's not ok, since second order reference is mutable.
			// So, passing "b" twice may lead to mutable sharid "b.a.x".
			Bar( b, b );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 15 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test1():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Bar( B& b, A& a );
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			// Error - passing "a" and "b" will lead to sharing inner mutable reference of "a" twice.
			Bar( b, a );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test2():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Bar( A& a, B& b );
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			// Error - passing "a" and "b" will lead to sharing inner mutable reference of "a" twice.
			Bar( a, b );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test3():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Bar( B b0, B b1 );
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			// Error - passing "b" twice will lead to sharing inner mutable reference of "a" twice.
			Bar( b, b );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test4():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Bar( B b, A a );
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			// Error - passing "a" and "b" will lead to sharing inner mutable reference of "a" twice.
			Bar( b, a );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test5():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Bar( A a, B b );
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			// Error - passing "a" and "b" will lead to sharing inner mutable reference of "a" twice.
			Bar( a, b );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test6():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Bar( B& b, i32 &mut x );
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			// Error - passing "b" will lead to sharing inner reference of "a" and "x".
			Bar( b, x );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test7():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Bar( i32 &mut x, B b );
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			// Error - passing "b" will lead to sharing inner reference of "a" and "x".
			Bar( x, b );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test8():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Bar( B b, i32& x );
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			// Error - passing "b" will lead to sharing inner reference of "a" and "x".
			Bar( b, x );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test9():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Bar( i32& x, B& b );
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			// Error - passing "b" will lead to sharing inner reference of "a" and "x".
			Bar( x, b );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test10():
	c_program_text= """
		struct A{ i32 &imut x; }
		struct B{ A &imut a; }
		fn Bar( B b, i32 &mut x );
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			// Error - passing "b" will lead to sharing inner reference of "a" and "x".
			Bar( b, x );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test11():
	c_program_text= """
		struct A{ i32 &imut x; }
		struct B{ A &imut a; }
		fn Bar( i32 &mut x, B& b );
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			// Error - passing "b" will lead to sharing inner reference of "a" and "x".
			Bar( x, b );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test12():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Bar( B b, i32 &mut x );
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			// Error - passing "b" will lead to sharing inner reference of "a" and "x".
			Bar( b, a.x );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test13():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		fn Bar( i32 &mut x, B& b );
		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .a= a };

			// Error - passing "b" will lead to sharing inner reference of "a" and "x".
			Bar( a.x, b );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test14():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		struct C{ i32 & @("a"c8) i; B @("b") b; }

		// Return a reference to contents of an argument.
		var [ [ char8, 2 ], 1 ] return_references[ "1b" ];
		fn Bar( i32& y, C& c ) : A & @(return_references)
		{
			return c.b.a;
		}

		fn Foo()
		{
			var i32 mut x= 765;
			var A a{ .x= x };
			var i32 i= 88;
			var C c{ .i= i, .b{ .a= a } };

			var i32 &mut x_ref0= Bar( 7, c ).x; // "x" points to "a.x"
			var i32 &mut x_ref1= a.x; // Error - creating second mutable reference to "a.x".

		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 20 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 21 ) )


def ReferenceProtectionError_ForSecondOrderInnerReference_InCall_Test15():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut a; }
		struct C{ i32 & @("a"c8) i; B @("b") b; }

		// Return a reference to contents of an argument.
		var [ [ char8, 2 ], 1 ] return_references[ "0b" ];
		fn Bar( C& c ) : A & @(return_references)
		{
			return c.b.a;
		}

		fn Foo()
		{
			var i32 mut x= 765;
			var A a{ .x= x };
			var C c{ .i= i, .b{ .a= a } };

			var i32& x_ref0= Bar( c ).x; // "x" points to "a.x"
			var i32 &mut x_ref1= a.x; // Error - creating second mutable reference to "a.x".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceProtectionError", 19 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 20 ) )


def DestroyedVariableStillHasReferences_ForSecondOrderInner_Test0():
	c_program_text= """
		struct A{ i32 &imut x; }
		struct DoubleA{ A @("a") first; A @("b") second; }

		struct C{ DoubleA @("aa") double_a; }
		struct B{ C& c; }

		var [ [ char8, 2 ], 1 ] return_references[ "0a" ];
		fn Bar( B& b ) : DoubleA & @(return_references)
		{
			return b.c.double_a;
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] reference_pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A& mut a, i32& x ) @(reference_pollution)
		{
		}

		fn Foo()
		{
			var i32 f= 0;
			var A mut outer_a{ .x= f };
			{
				var i32 x= 0, y= 0;
				var C c{ .double_a{ .first{ .x= x }, .second{ .x= y } } };
				var B b{ .c= c };

				var DoubleA& double_a_ref= Bar(b); // Inner reference of "double_a_ref" points to "x" and "y".

				MakePollution( outer_a, double_a_ref.first.x ); // Save references to "x" and "y" inside "outer_a".
			} // Error - destroyed variables "x" and "y" have reference inside "outer_a".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 31 ) )


def DestroyedVariableStillHasReferences_ForSecondOrderInner_Test1():
	c_program_text= """
		struct A{ i32 &imut x; }
		struct B{ A& a; }

		var [ [ [ char8, 2 ], 2 ], 1 ] reference_pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A& mut a, i32& x ) @(reference_pollution);

		fn Foo()
		{
			var i32 x= 0;
			var A mut outer_a{ .x= x };
			{
				var i32 y= 0;
				var A inner_a{ .x= y };
				var B b{ .a= inner_a };

				// "b.a.x" points to "y". A reference to it is saved inside "outer_a".
				MakePollution( outer_a, b.a.x );
			} // Error here - destroyed variable "y" still has a reference (inside "outer_a").
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 19 ) )


def DestroyedVariableStillHasReferences_ForSecondOrderInner_Test2():
	c_program_text= """
		struct A{ i32 &imut x; }
		struct B{ A& a; }

		var [ [ [ char8, 2 ], 2 ], 1 ] reference_pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A& mut a, i32& x ) @(reference_pollution);

		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0a" ] ];
		fn Pass( B& b ) : B @(return_inner_references) & @(return_references );

		fn Foo()
		{
			var i32 x= 0;
			var A mut outer_a{ .x= x };
			{
				var i32 y= 0;
				var A inner_a{ .x= y };
				var B b{ .a= inner_a };

				// "Pass(b).a.x" points to "y". A reference to it is saved inside "outer_a".
				MakePollution( outer_a, Pass(b).a.x );
			} // Error here - destroyed variable "y" still has a reference (inside "outer_a").
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 23 ) )


def DestroyedVariableStillHasReferences_ForSecondOrderInner_Test3():
	c_program_text= """
		struct A{ i32 &imut x; }
		struct B{ A& a; }

		var [ [ [ char8, 2 ], 2 ], 1 ] reference_pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A& mut a, i32& x ) @(reference_pollution);

		var [ [ char8, 2 ], 1 ] return_references[ "0a" ];
		fn GetA( B& b ) : A & @(return_references )
		{
			return b.a;
		}

		fn Foo()
		{
			var i32 x= 0;
			var A mut outer_a{ .x= x };
			{
				var i32 y= 0;
				var A inner_a{ .x= y };
				var B b{ .a= inner_a };

				// "GetA(b).x" points to "y". A reference to it is saved inside "outer_a".
				MakePollution( outer_a, GetA(b).x );
			} // Error here - destroyed variable "y" still has a reference (inside "outer_a").
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 25 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test0():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		fn Foo( B& b ) : i32 &
		{
			return b.a.x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 6 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test1():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		fn Foo( B b ) : i32 &
		{
			return b.a.x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 6 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test2():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		fn Foo( B& b ) : A // Return second order reference inside "A".
		{
			return b.a;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 6 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test3():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		fn Foo( B b ) : A // Return second order reference inside "A".
		{
			return b.a;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 6 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test4():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		fn Foo( B& b ) : A // Return second order reference inside "A".
		{
			return A{ .x= b.a.x };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 6 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test5():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		fn Foo( B b ) : A // Return second order reference inside "A".
		{
			return A{ .x= b.a.x };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 6 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test6():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		struct BWrapper{ B b; }
		fn Foo( BWrapper& b_wrapper ) : i32 &
		{
			return b_wrapper.b.a.x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 7 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test7():
	c_program_text= """
		struct A{ i32 & x; }
		struct AWrapper{ A a; }
		struct B{ AWrapper & a_wrapper; }
		fn Foo( B& b ) : i32 &
		{
			return b.a_wrapper.a.x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 7 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test8():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		// Even with auto-return it's not possible for now to return second order reference.
		fn Foo( B& b ) : auto&
		{
			return b.a.x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 7 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test9():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		// Even with auto-return it's not possible for now to return second order reference.
		fn Foo( B b ) : auto &
		{
			return b.a.x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 7 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test10():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		// Even with auto-return it's not possible for now to return second order reference.
		fn Foo( B& b ) : auto // Return second order reference inside "A".
		{
			return b.a;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 7 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test11():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		// Even with auto-return it's not possible for now to return second order reference.
		fn Foo( B b ) : auto // Return second order reference inside "A".
		{
			return b.a;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 7 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test12():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		// Even with auto-return it's not possible for now to return second order reference.
		fn Foo( B& b ) : auto // Return second order reference inside "A".
		{
			return A{ .x= b.a.x };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 7 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test13():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		// Even with auto-return it's not possible for now to return second order reference.
		fn Foo( B b ) : auto // Return second order reference inside "A".
		{
			return A{ .x= b.a.x };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 7 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test14():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		struct BWrapper{ B b; }
		// Even with auto-return it's not possible for now to return second order reference.
		fn Foo( BWrapper& b_wrapper ) : auto &
		{
			return b_wrapper.b.a.x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 8 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test15():
	c_program_text= """
		struct A{ i32 & x; }
		struct AWrapper{ A a; }
		struct B{ AWrapper & a_wrapper; }
		// Even with auto-return it's not possible for now to return second order reference.
		fn Foo( B& b ) : auto &
		{
			return b.a_wrapper.a.x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 8 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test16():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		fn Foo()
		{
			// Even with return references deduction for lambdas it's impossible to return second order reference.
			auto f= lambda[]( B& b ) : i32 &
			{
				return b.a.x;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 9 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test17():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		fn Foo()
		{
			// Even with return references deduction for lambdas it's impossible to return second order reference.
			auto f= lambda[]( B b ) : i32 &
			{
				return b.a.x;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 9 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test18():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		fn Foo()
		{
			// Even with return references deduction for lambdas it's impossible to return second order reference.
			auto f= lambda[]( B& b ) : A // Return second order reference inside "A".
			{
				return b.a;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 9 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test19():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		fn Foo()
		{
			// Even with return references deduction for lambdas it's impossible to return second order reference.
			auto f= lambda[]( B b ) : A // Return second order reference inside "A".
			{
				return b.a;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 9 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test20():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		fn Foo()
		{
			// Even with return references deduction for lambdas it's impossible to return second order reference.
			auto f= lambda[]( B& b ) : A // Return second order reference inside "A".
			{
				return A{ .x= b.a.x };
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 9 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test21():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		fn Foo()
		{
			// Even with return references deduction for lambdas it's impossible to return second order reference.
			auto f= lambda[]( B b ) : A // Return second order reference inside "A".
			{
				return A{ .x= b.a.x };
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 9 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test22():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &a; }
		struct BWrapper{ B b; }
		fn Foo()
		{
			// Even with return references deduction for lambdas it's impossible to return second order reference.
			auto f= lambda[]( BWrapper& b_wrapper ) : i32 &
			{
				return b_wrapper.b.a.x;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 10 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test23():
	c_program_text= """
		struct A{ i32 & x; }
		struct AWrapper{ A a; }
		struct B{ AWrapper & a_wrapper; }
		fn Foo()
		{
			// Even with return references deduction for lambdas it's impossible to return second order reference.
			auto f= lambda[]( B& b ) : i32 &
			{
				return b.a_wrapper.a.x;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 10 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test24():
	c_program_text= """
		struct A{ i32 & x; }
		fn Foo( A& a )
		{
			auto f= lambda[&a]() : i32& // Explicitly capture "a" by reference.
			{
				// Inner reference of a captured by reference variable is second order reference.
				// It's impossible to return it.
				return a.x;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 9 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test25():
	c_program_text= """
		struct A{ i32 & x; }
		fn Foo( A& a )
		{
			auto f= lambda[&]() : i32&
			{
				// Implicitle capture "a" by reference.
				// Inner reference of a captured by reference variable is second order reference.
				// It's impossible to return it.
				return a.x;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 10 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test26():
	c_program_text= """
		struct A{ i32& x; }
		struct B{ A& a; }
		fn Foo()
		{
			var i32 x= 0;
			var A a{ .x= x };
			var B b{ .a= a };
			auto f= lambda[b]() : i32& // Explicitly capture "b" by copy.
			{
				// Can't return second order reference.
				return b.a.x;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 12 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test27():
	c_program_text= """
		struct A{ i32& x; }
		struct B{ A& a; }
		fn Foo()
		{
			var i32 x= 0;
			var A a{ .x= x };
			var B b{ .a= a };
			auto f= lambda[=]() : i32&
			{
				// Implicitly capture "b" by value.
				// Can't return second order reference.
				return b.a.x;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 13 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test28():
	c_program_text= """
		struct A{ i32& x; }
		fn Foo()
		{
			var i32 x= 0;
			var A a{ .x= x };
			auto f= lambda[&a]() : A // Explicitly capture "a" by reference.
			{
				// Can't return a second order reference inside return value.
				return a;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 10 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test29():
	c_program_text= """
		struct A{ i32& x; }
		struct B{ A& a; }
		fn Foo( B& b )
		{
			auto f= lambda[b]() : A // Explicitly capture "b" by value.
			{
				// Can't return a second order reference inside return value.
				return b.a;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 9 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test30():
	c_program_text= """
		struct A{ i32& x; }
		struct B{ A& a; }
		fn Foo()
		{
			var i32 x= 0;
			var A a{ .x= x };
			var B b{ .a= a };
			auto f= lambda[=] byval mut () : i32&
			{
				// Implicitly capture "b" by value.
				// Can't return second order reference.
				return b.a.x;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 13 ) )


def ReturningUnallowedReference_ForSecondOrderReference_Test31():
	c_program_text= """
		struct A{ i32& x; }
		struct B{ A& a; }
		fn Foo()
		{
			var i32 x= 0;
			var A a{ .x= x };
			var B b{ .a= a };
			auto f= lambda[=] byval mut () : A
			{
				// Implicitly capture "b" by value.
				// Can't return second order reference inside "A".
				return b.a;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 13 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test0():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &mut a; }
		fn Foo( B& b, i32& y )
		{
			// Perform pollution for indirectly accessible variable, which isn't allowed.
			MakePollution( b.a, y );
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 8 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test1():
	c_program_text= """
		struct A{ i32 & x; }
		struct AWrapper{ A a; }
		struct B{ AWrapper &mut a_wrapper; }
		fn Foo( B& b, i32& y )
		{
			// Perform pollution for indirectly accessible variable, which isn't allowed.
			MakePollution( b.a_wrapper.a, y );
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 9 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test2():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &mut a; }
		struct BWrapper{ B b; }
		fn Foo( BWrapper& b_wrapper, i32& y )
		{
			// Perform pollution for indirectly accessible variable, which isn't allowed.
			MakePollution( b_wrapper.b.a, y );
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 9 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test3():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &mut a; }
		fn Foo( B b, i32& y )
		{
			// Perform pollution for indirectly accessible variable, which isn't allowed.
			MakePollution( b.a, y );
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 8 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test4():
	c_program_text= """
		struct A{ i32 & x; }
		struct AWrapper{ A a; }
		struct B{ AWrapper &mut a_wrapper; }
		fn Foo( B b, i32& y )
		{
			// Perform pollution for indirectly accessible variable, which isn't allowed.
			MakePollution( b.a_wrapper.a, y );
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 9 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test5():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &mut a; }
		struct BWrapper{ B b; }
		fn Foo( BWrapper b_wrapper, i32& y )
		{
			// Perform pollution for indirectly accessible variable, which isn't allowed.
			MakePollution( b_wrapper.b.a, y );
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 9 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test6():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &mut a; }
		fn Foo( B& b, i32& y ) : auto // Enable reference notation deduction via "auto".
		{
			// Perform pollution for indirectly accessible variable, which isn't allowed.
			MakePollution( b.a, y );
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 8 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test7():
	c_program_text= """
		struct A{ i32 & x; }
		struct AWrapper{ A a; }
		struct B{ AWrapper &mut a_wrapper; }
		fn Foo( B& b, i32& y ) : auto // Enable reference notation deduction via "auto".
		{
			// Perform pollution for indirectly accessible variable, which isn't allowed.
			MakePollution( b.a_wrapper.a, y );
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 9 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test8():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &mut a; }
		struct BWrapper{ B b; }
		fn Foo( BWrapper& b_wrapper, i32& y ) : auto // Enable reference notation deduction via "auto".
		{
			// Perform pollution for indirectly accessible variable, which isn't allowed.
			MakePollution( b_wrapper.b.a, y );
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 9 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test9():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &mut a; }
		fn Foo( B b, i32& y ) : auto // Enable reference notation deduction via "auto".
		{
			// Perform pollution for indirectly accessible variable, which isn't allowed.
			MakePollution( b.a, y );
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 8 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test10():
	c_program_text= """
		struct A{ i32 & x; }
		struct AWrapper{ A a; }
		struct B{ AWrapper &mut a_wrapper; }
		fn Foo( B b, i32& y ) : auto // Enable reference notation deduction via "auto".
		{
			// Perform pollution for indirectly accessible variable, which isn't allowed.
			MakePollution( b.a_wrapper.a, y );
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 9 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test11():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &mut a; }
		struct BWrapper{ B b; }
		fn Foo( BWrapper b_wrapper, i32& y ) : auto // Enable reference notation deduction via "auto".
		{
			// Perform pollution for indirectly accessible variable, which isn't allowed.
			MakePollution( b_wrapper.b.a, y );
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 9 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test12():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &mut a; }
		fn Foo()
		{
			auto f= lambda[]( B& b, i32& y ) // Reference notation deduction is enabled for lambdas.
			{
				// Perform pollution for indirectly accessible variable, which isn't allowed.
				MakePollution( b.a, y );
			};
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 10 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test13():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &imut a; }
		fn Foo( A& mut a, B& b )
		{
			// Perform pollution with indirectly-accessible source, which for now isn't possible.
			MakePollution( a, b.a.x );
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 8 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test14():
	c_program_text= """
		struct A{ i32 & x; }
		struct AWrapper{ A a; }
		struct B{ A &imut a; }
		fn Foo( AWrapper& mut a_wrapper, B& b )
		{
			// Perform pollution with indirectly-accessible source, which for now isn't possible.
			MakePollution( a_wrapper.a, b.a.x );
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 9 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test15():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &imut a; }
		fn Foo( A& mut a, B& b ) : auto // Enable reference notation deduction via "auto".
		{
			// Perform pollution with indirectly-accessible source, which for now isn't possible.
			MakePollution( a, b.a.x );
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 8 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test16():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &imut a; }
		fn Foo()
		{
			auto f= lambda[]( A& mut a, B& b ) // Reference notation deduction is enabled for lambdas.
			{
				// Perform pollution with indirectly-accessible source, which for now isn't possible.
				MakePollution( a, b.a.x );
			};
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 10 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test17():
	c_program_text= """
		struct A{ i32 & x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A mut a{ .x= x };
			auto f= lambda[&]( i32& y )
			{
				// Implicitly capture "a" and create pollution for it.
				// It's impossible, because captured in lambda reference creates second order pollution.
				MakePollution( a, y );
			};
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 12 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test18():
	c_program_text= """
		struct A{ i32 & x; }
		struct B{ A &mut a; }
		fn Foo()
		{
			var i32 mut x= 0;
			var A mut a{ .x= x };
			auto f= lambda[ b= B{ .a= a } ]( i32& y ) // "a" is captured inside inside "b".
			{
				// It's impossible to perform pollution, because captured in lambda reference creates second order pollution.
				MakePollution( b.a, y );
			};
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 12 ) )


def UnallowedReferencePollution_ForSecondOrderReference_Test19():
	c_program_text= """
		struct A{ i32 & x; }
		fn Foo()
		{
			var i32 x= 0;
			var A outer_a{ .x= x };
			auto f= lambda[&]( A& mut dst_a )
			{
				// Implicitly capture "outer_a" by reference.
				// Pollution source is second order reference, so, it's impossible.
				MakePollution( dst_a, outer_a.x );
			};
		}

		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( A &mut a, i32& x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 12 ) )


def ReferenceIndirectionDepthExceeded_Test0():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut x; }
		struct C{ B &imut x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceIndirectionDepthExceeded", 4 ) )


def ReferenceIndirectionDepthExceeded_Test1():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ [ A, 2 ] &imut x; }
		struct C{ tup[ B, i32, f32 ] &imut x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceIndirectionDepthExceeded", 4 ) )


def ReferenceIndirectionDepthExceeded_Test2():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A &imut x; }
		fn Foo( B& b )
		{
			auto f= lambda[&]() // Error - capturing reference of type with second order references inside.
			{
				auto& b_ref= b;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceIndirectionDepthExceeded", 6 ) )


def MoreThanOneInnerReferenceTagForSecondOrderReferenceField_Test0():
	c_program_text= """
		struct A
		{
			i32 & @("a"c8) x;
			f32 & @("b"c8) y;
		}
		struct B
		{
			A &imut x; // Error - "A" contains more than one inner reference tag inside.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MoreThanOneInnerReferenceTagForSecondOrderReferenceField", 9 ) )


def MoreThanOneInnerReferenceTagForSecondOrderReferenceField_Test1():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B
		{
			tup[ A, A ] &imut x; // A tuple contains number of inner references equal to sum of its elements inner references.
		}

	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MoreThanOneInnerReferenceTagForSecondOrderReferenceField", 5 ) )


def MoreThanOneInnerReferenceTagForSecondOrderReferenceField_Test2():
	c_program_text= """
		struct A
		{
			i32 & @("a"c8) x;
			f32 & @("b"c8) y;
		}
		fn Foo(A& a)
		{
			// Error - capturing "a" by reference creates a reference field in the lambda class, which isn't possible for types with more than one inner reference tag inside.
			auto f= lambda[&]()
			{
				auto& a_ref= a;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MoreThanOneInnerReferenceTagForSecondOrderReferenceField", 10 ) )


def MoreThanOneInnerReferenceTagForSecondOrderReferenceField_Test3():
	c_program_text= """
		struct A // This struct have two references inside, but they use the same inner reference tag.
		{
			i32 & @("a"c8) x;
			f32 & @("a"c8) y;
		}
		struct B
		{
			A &imut x; // Fine - "A" contains only one inner reference tag inside.
		}
	"""
	tests_lib.build_program( c_program_text )


def MixingMutableAndImmutableSecondOrderReferencesInSameReferenceTag_Test0():
	c_program_text= """
		struct A { i32 &imut r; }
		struct B { i32 &mut  r; }
		struct C
		{
			A& @("a"c8) a;
			B& @("a"c8) b;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MixingMutableAndImmutableSecondOrderReferencesInSameReferenceTag", 4 ) )


def MixingMutableAndImmutableSecondOrderReferencesInSameReferenceTag_Test1():
	c_program_text= """
		struct A { i32 &imut r; }
		struct B { i32 &imut r; }
		struct C // Fine - second order inner references are both immutable.
		{
			A& @("a"c8) a;
			B& @("a"c8) b;
		}
	"""
	tests_lib.build_program( c_program_text )


def MixingMutableAndImmutableSecondOrderReferencesInSameReferenceTag_Test2():
	c_program_text= """
		struct A { i32 &mut r; }
		struct B { i32 &mut r; }
		struct C // Fine - second order inner references are both mutable.
		{
			A& @("a"c8) a;
			B& @("a"c8) b;
		}
	"""
	tests_lib.build_program( c_program_text )


def MixingMutableAndImmutableSecondOrderReferencesInSameReferenceTag_Test3():
	c_program_text= """
		struct A { i32 &imut r; }
		struct B { i32 &mut  r; }
		struct C { A& a; }
		struct D { B& b; }
		struct E
		{
			C @("a") c;
			D @("a") d;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MixingMutableAndImmutableSecondOrderReferencesInSameReferenceTag", 6 ) )


def MixingMutableAndImmutableSecondOrderReferencesInSameReferenceTag_Test4():
	c_program_text= """
		struct A { i32 &imut r; }
		struct B { i32 &imut r; }
		struct C { A& a; }
		struct D { B& b; }
		struct E // Fine - second order inner references are both immutable.
		{
			C @("a") c;
			D @("a") d;
		}
	"""
	tests_lib.build_program( c_program_text )


def MixingMutableAndImmutableSecondOrderReferencesInSameReferenceTag_Test5():
	c_program_text= """
		struct A { i32 &mut r; }
		struct B { i32 &mut r; }
		struct C { A& a; }
		struct D { B& b; }
		struct E // Fine - second order inner references are both mutable.
		{
			C @("a") c;
			D @("a") d;
		}
	"""
	tests_lib.build_program( c_program_text )


def MixingMutableAndImmutableSecondOrderReferencesInSameReferenceTag_Test6():
	c_program_text= """
		struct A { i32 &imut r; }
		struct B { i32 &mut  r; }
		struct C { A& a; }
		struct E
		{
			C @("a") c;
			B & @("a"c8) b;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MixingMutableAndImmutableSecondOrderReferencesInSameReferenceTag", 5 ) )


def MixingMutableAndImmutableSecondOrderReferencesInSameReferenceTag_Test7():
	c_program_text= """
		struct A { i32 &imut r; }
		struct B { i32 &mut  r; }
		class C polymorph
		{
			A & @("a"c8) a;
		}
		class D : C // Inherit field with tag "a" with immutable second order inner reference.
		{
			B & @("a"c8) b; // Add field with tag "a" with mutable second order inner reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MixingMutableAndImmutableSecondOrderReferencesInSameReferenceTag", 8 ) )


def PreventNonOwningMutationInDestructor_ForfStructWithSecondOrderReferenceInside_Test0():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B
		{
			A &imut a;

			fn destructor()
			{
				auto& t= this; // Error here. Since a second order mutable reference is stored inside this struct, whole "this" in destructor isn't available.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ThisUnavailable", 9 ) )


def PreventNonOwningMutationInDestructor_ForfStructWithSecondOrderReferenceInside_Test1():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B
		{
			A &imut a;

			fn destructor()
			{
				auto& a_ref= a; // Error here. Prevent accessing second order mutable reference in field "a".
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MutableReferenceFieldAccessInDestructor", 9 ) )


def PreventNonOwningMutationInDestructor_ForfStructWithSecondOrderReferenceInside_Test2():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B{ A& a; }
		struct BWrapper
		{
			B b;

			fn destructor()
			{
				auto& b_ref= b; // Error here. Prevent accessing second order mutable reference in field "b".
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingFieldWithMutableReferencesInsideInDestructor", 10 ) )


def InnerReferenceTagsForReferenceField_Test0():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B
		{
			A @("a") &  a_ref;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InnerReferenceTagsForReferenceField", 5 ) )


def InnerReferenceTagsForReferenceField_Test1():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B
		{
			A @("a") & @("a"c8) a_ref;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InnerReferenceTagsForReferenceField", 5 ) )


def InnerReferenceTagsForReferenceField_Test2():
	c_program_text= """
		struct A{ i32 &mut x; }
		struct B
		{
			A @("b") & @("a"c8) a_ref;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InnerReferenceTagsForReferenceField", 5 ) )
