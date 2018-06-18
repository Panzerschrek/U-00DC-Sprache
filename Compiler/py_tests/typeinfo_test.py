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


def FundamentalTypesInfo_Test0():
	c_program_text= """
		fn Foo()
		{
			static_assert( typeinfo</i32/>.is_signed_integer );
			static_assert( typeinfo</u32/>.is_unsigned_integer );
			static_assert( !typeinfo</i16/>.is_float );
			static_assert( typeinfo</f64/>.is_float );
			static_assert( typeinfo</bool/>.is_bool );
			static_assert( !typeinfo</u64/>.is_bool );
		}
	"""
	tests_lib.build_program( c_program_text )


def EnumTypesInfo_Test0():
	c_program_text= """
		enum E : u16 { A, B, C }
		fn Foo()
		{
			static_assert( typeinfo</E/>.is_enum );
			static_assert( typeinfo</E/>.underlaying_type.is_unsigned_integer );
			static_assert( typeinfo</E/>.element_count == u64(3));
		}
	"""
	tests_lib.build_program( c_program_text, True )


def TypeinfoCalssIsSameForSameTypes_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut t= typeinfo</i32/>;
			t= typeinfo</i32/>;  // Must asign here, because types of both typeinfo expressions is same.
			halt if( ! t.is_fundamental );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
