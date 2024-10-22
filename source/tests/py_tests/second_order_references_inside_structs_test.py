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
	assert( not HasError( errors_list, "ReferenceProtectionError", 11 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )
