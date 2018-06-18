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


def TypeAdditionalCommonFields_Test0():
	c_program_text= """
		struct S{}
		class C{}
		fn Foo()
		{
			static_assert( !typeinfo</i32/>.is_default_constructible );
			static_assert(  typeinfo</i32/>.is_copy_constructible );
			static_assert(  typeinfo</i32/>.is_copy_assignable );
			static_assert( typeinfo</S/>.is_default_constructible );
			static_assert( typeinfo</S/>.is_copy_constructible );
			static_assert( typeinfo</S/>.is_copy_assignable );
			static_assert( !typeinfo</C/>.is_copy_constructible );
			static_assert( !typeinfo</C/>.is_copy_assignable );
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
			static_assert( !typeinfo</bool/>.is_numeric );
			static_assert( !typeinfo</f32/>.is_integer );
			static_assert( typeinfo</f32/>.is_numeric );
			static_assert( typeinfo</u8/>.is_integer );
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
	tests_lib.build_program( c_program_text )


def ArrayTypesInfo_Test0():
	c_program_text= """
		type Arr0= [ i32, 7 ];
		type Arr1= [ Arr0, 4 ];
		fn Foo()
		{
			static_assert( typeinfo</Arr0/>.is_array );
			static_assert( typeinfo</Arr1/>.is_array );
			static_assert( typeinfo</Arr0/>.element_count == u64(7) );
			static_assert( typeinfo</Arr1/>.element_type.is_array );
			static_assert( typeinfo</Arr1/>.element_type.element_count == u64(7) );
		}
	"""
	tests_lib.build_program( c_program_text )


def ClassTypesInfo_Test0():
	c_program_text= """
		struct S{}
		class C{ i32 x; }
		fn Foo()
		{
			static_assert( typeinfo</S/>.field_count == u64(0) );
			static_assert( typeinfo</C/>.field_count == u64(1) );
		}
	"""
	tests_lib.build_program( c_program_text )


def ClassTypesInfo_Test1():
	c_program_text= """
		struct S{}
		class I interface {}
		class A abstract {}
		class NP {}
		class PNF : I {}
		class PF final : I {}
		fn Foo()
		{
			static_assert(  typeinfo</S/>.is_struct );
			static_assert( !typeinfo</S/>.is_polymorph );
			static_assert(  typeinfo</S/>.is_final );
			static_assert( !typeinfo</S/>.is_abstract );
			static_assert( !typeinfo</S/>.is_interface );

			static_assert( !typeinfo</I/>.is_struct );
			static_assert(  typeinfo</I/>.is_polymorph );
			static_assert( !typeinfo</I/>.is_final );
			static_assert(  typeinfo</I/>.is_abstract );
			static_assert(  typeinfo</I/>.is_interface );

			static_assert( !typeinfo</A/>.is_struct );
			static_assert(  typeinfo</A/>.is_polymorph );
			static_assert( !typeinfo</A/>.is_final );
			static_assert(  typeinfo</A/>.is_abstract );
			static_assert( !typeinfo</A/>.is_interface );

			static_assert( !typeinfo</NP/>.is_struct );
			static_assert( !typeinfo</NP/>.is_polymorph );
			static_assert(  typeinfo</NP/>.is_final );
			static_assert( !typeinfo</NP/>.is_abstract );
			static_assert( !typeinfo</NP/>.is_interface );

			static_assert( !typeinfo</PNF/>.is_struct );
			static_assert(  typeinfo</PNF/>.is_polymorph );
			static_assert( !typeinfo</PNF/>.is_final );
			static_assert( !typeinfo</PNF/>.is_abstract );
			static_assert( !typeinfo</PNF/>.is_interface );

			static_assert( !typeinfo</PF/>.is_struct );
			static_assert(  typeinfo</PF/>.is_polymorph );
			static_assert(  typeinfo</PF/>.is_final );
			static_assert( !typeinfo</PF/>.is_abstract );
			static_assert( !typeinfo</PF/>.is_interface );
		}
	"""
	tests_lib.build_program( c_program_text )


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
