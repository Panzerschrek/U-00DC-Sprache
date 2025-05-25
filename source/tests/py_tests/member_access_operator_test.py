from py_tests_common import *


def MemberAccesOperator_AccessType_Test0():
	c_program_text= """
		struct S
		{
			type T= f64;
		}
		fn Foo()
		{
			var S s;
			auto x= s.T( 13.7 ); // Access a type alias via ".".
			static_assert( same_type</ typeof(x), f64 /> );
			static_assert( x == 13.7 );
		}
	"""
	tests_lib.build_program( c_program_text )


def MemberAccesOperator_AccessType_Test1():
	c_program_text= """
		struct S
		{
			struct Inner
			{
				i32 x; f32 y;
			}
		}
		fn Foo()
		{
			var S s;
			auto inner= s.Inner{ .x= 77, .y= -0.25f }; // Access a nested struct type via ".".
			static_assert( same_type</ typeof(inner), S::Inner /> );
			static_assert( inner.x == 77 );
			static_assert( inner.y == -0.25f );
		}
	"""
	tests_lib.build_program( c_program_text )
