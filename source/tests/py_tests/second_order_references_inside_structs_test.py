from py_tests_common import *


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
