from py_tests_common import *


def TypeInfoOperator_Test0():
	c_program_text= """
		fn Foo()
		{
			typeinfo</ i32 />;
		}
	"""
	tests_lib.build_program( c_program_text )


def TypeKindFields_Test0():
	c_program_text= """
		struct S{}
		enum E{ A }
		type FP= fn();
		type A= [ i32, 2 ];
		fn Foo()
		{
			static_assert( typeinfo</i32/>.is_fundamental );
			static_assert( typeinfo</A/>.is_array );
			static_assert( typeinfo</E/>.is_enum );
			static_assert( typeinfo</FP/>.is_function_pointer );

			static_assert( ! typeinfo</S/>.is_fundamental );
			static_assert( ! typeinfo</i32/>.is_enum );
			static_assert( ! typeinfo</A/>.is_function_pointer );
			static_assert( ! typeinfo</E/>.is_array );
		}
	"""
	tests_lib.build_program( c_program_text )
