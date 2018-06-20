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
			static_assert( !typeinfo</ fn() />.is_default_constructible );
			static_assert(  typeinfo</ fn() />.is_copy_constructible );
			static_assert(  typeinfo</ fn() />.is_copy_assignable );
			static_assert( typeinfo</S/>.is_default_constructible );
			static_assert( typeinfo</S/>.is_copy_constructible );
			static_assert( typeinfo</S/>.is_copy_assignable );
			static_assert( !typeinfo</C/>.is_copy_constructible );
			static_assert( !typeinfo</C/>.is_copy_assignable );
		}
	"""
	tests_lib.build_program( c_program_text )


def SizeAndAlignmentFileds_Test0():
	c_program_text= """
		struct TwoInt{ i32 x; i32 y; }
		struct TripleBool{ [ bool, 3 ] b; }
		struct OptionalInt{ i32 x; bool y; }
		struct Ref{ i32& r; }
		fn Foo()
		{
			static_assert( typeinfo</bool/>. size_of == size_type(1) );
			static_assert( typeinfo</bool/>.align_of == size_type(1) );
			static_assert( typeinfo</i8 />. size_of == size_type(1) );
			static_assert( typeinfo</i8 />.align_of == size_type(1) );
			static_assert( typeinfo</i16/>. size_of == size_type(2) );
			static_assert( typeinfo</i16/>.align_of == size_type(2) );
			static_assert( typeinfo</i32/>. size_of == size_type(4) );
			static_assert( typeinfo</i32/>.align_of == size_type(4) );
			static_assert( typeinfo</i64/>. size_of == size_type(8) );
			static_assert( typeinfo</i64/>.align_of == size_type(8) );
			static_assert( typeinfo</f32/>. size_of == size_type(4) );
			static_assert( typeinfo</f32/>.align_of == size_type(4) );
			static_assert( typeinfo</f64/>. size_of == size_type(8) );
			static_assert( typeinfo</f64/>.align_of == size_type(8) );

			static_assert( typeinfo</Ref/>.size_of <= size_type(8) );

			static_assert( typeinfo</TwoInt/>. size_of == size_type(4*2) );
			static_assert( typeinfo</TwoInt/>.align_of == size_type(4) );

			static_assert( typeinfo</TripleBool/>. size_of == size_type(3) );
			static_assert( typeinfo</TripleBool/>.align_of == size_type(1) );

			static_assert( typeinfo</OptionalInt/>. size_of == size_type(4*2) );
			static_assert( typeinfo</OptionalInt/>.align_of == size_type(4) );

			static_assert( typeinfo</ fn() />.size_of == typeinfo</ fn( i32 x ) : i32 />.size_of ); // All function pointers have same size
			static_assert( typeinfo</ fn() />.size_of == typeinfo</Ref/>.size_of ); // Pointer to function have size of reference
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
			static_assert( typeinfo</void/>.is_void );
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
			static_assert( typeinfo</E/>.element_count == size_type(3));
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
			static_assert( typeinfo</Arr0/>.element_count == size_type(7) );
			static_assert( typeinfo</Arr1/>.element_type.is_array );
			static_assert( typeinfo</Arr1/>.element_type.element_count == size_type(7) );
		}
	"""
	tests_lib.build_program( c_program_text )


def ClassTypesInfo_Test0():
	c_program_text= """
		struct S{}
		class C{ i32 x; }
		fn Foo()
		{
			static_assert( typeinfo</S/>.field_count == size_type(0) );
			static_assert( typeinfo</C/>.field_count == size_type(1) );
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


def TypeinfoForTemplateDependentType_Test0():
	c_program_text= """
		template</ type T />
		fn Foo()
		{
			static_if( typeinfo</ T />.is_fundamental )
			{
			}
			else if( typeinfo</T/>.is_array )
			{
				static_if( typeinfo</ [ T, 2 ] />.is_array )
				{
					auto x= typeinfo</ [ T, 2 ] />.element_count;
					auto y= typeinfo</ [ T, 2 ] />.element_type.is_fundamental;
				}
			}
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


def CompleteTypeRequredForTypeinfo_Test0():
	c_program_text= """
		struct S;
		fn Foo()
		{
			typeinfo</S/>;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingIncompleteType" )
	assert( errors_list[0].file_pos.line == 5 )


def CompleteTypeRequredForTypeinfo_Test1():
	c_program_text= """
		struct S;
		fn Foo()
		{
			typeinfo</ [ S, 2 ] />;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingIncompleteType" )
	assert( errors_list[0].file_pos.line == 5 )


def TypeinfoFieldsDependsOnTypeKind_Test0():
	c_program_text= """
		struct S{}
		fn Foo()
		{
			typeinfo</S/>.is_numeric;  // "is_numeric" exists only in typeinfo for fundamental types.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].file_pos.line == 5 )


def TypeinfoFieldsDependsOnTypeKind_Test1():
	c_program_text= """
		fn Foo()
		{
			typeinfo</ [ i32, 8 ] />.is_bool;  // "is_bool" exists only in typeinfo for fundamental types.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].file_pos.line == 4 )


def TypeinfoFieldsDependsOnTypeKind_Test2():
	c_program_text= """
		fn Foo()
		{
			typeinfo</ f32 />.underlaying_type;  // "underlaying_type" exists only in typeinfo for enum types.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].file_pos.line == 4 )


def TypeinfoFieldsDependsOnTypeKind_Test3():
	c_program_text= """
		struct S{}
		fn Foo()
		{
			typeinfo</ S />.element_count;  // "element_count" exists only in typeinfo for enum and array types.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].file_pos.line == 5 )


def TypeinfoFieldsDependsOnTypeKind_Test4():
	c_program_text= """
		fn Foo()
		{
			typeinfo</ u64 />.element_count;  // "element_count" exists only in typeinfo for enum and array types.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].file_pos.line == 4 )


def TypeinfoFieldsDependsOnTypeKind_Test5():
	c_program_text= """
		enum E{ A }
		fn Foo()
		{
			typeinfo</ E />.element_type;  // "element_type" exists only in typeinfo for array types.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].file_pos.line == 5 )


def TypeinfoFieldsDependsOnTypeKind_Test6():
	c_program_text= """
		fn Foo()
		{
			typeinfo</ u8 />.field_count;  // "field_count" exists only in typeinfo for class types.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].file_pos.line == 4 )


def TypeinfoFieldsDependsOnTypeKind_Test7():
	c_program_text= """
		struct S{}
		fn Foo()
		{
			typeinfo</ [ S, 16 ] />.is_abstract;  // "is_abstract" exists only in typeinfo for class types.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].file_pos.line == 5 )


def TypeinfoFieldsDependsOnTypeKind_Test8():
	c_program_text= """
		fn Foo()
		{
			typeinfo</ fn() />.is_struct;  // "is_struct" exists only in typeinfo for class types.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].file_pos.line == 4 )
