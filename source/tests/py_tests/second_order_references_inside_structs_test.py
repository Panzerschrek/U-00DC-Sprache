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
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )
