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
