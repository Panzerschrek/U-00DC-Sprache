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
		struct TwoIntBox{ TwoInt ti; }
		struct TripleBool{ [ bool, 3 ] b; }
		struct OptionalInt{ i32 x; bool y; }
		struct Ref{ i32& r; }
		struct EmptyStruct{}
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

			static_assert( typeinfo</ EmptyStruct />.size_of == size_type(0) );  // Empty struct have zero size.
			static_assert( typeinfo</ [ i32, 0 ] />.size_of == size_type(0) );  // Empty array have zero size.
			static_assert( typeinfo</ [ EmptyStruct, 16 ] />.size_of == size_type(0) );  // Array of empty structs have zero size.

			static_assert( typeinfo</ TwoIntBox />.size_of == typeinfo</ TwoInt />.size_of ); // Struct with single field have size of this field.
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


def ClassTypesInfo_Test2():
	c_program_text= """
		class A polymorph { i32 x; }
		class B : A {}

		template</ type T /> fn constexpr MustBeSame( T& a, T& b ) : bool { return true; }
		fn Foo()
		{
			static_assert( MustBeSame( typeinfo</B/>.fields_list.class_type, typeinfo</A/> ) );
			static_assert( MustBeSame( typeinfo</A/>.fields_list.class_type, typeinfo</A/>.fields_list.class_type ) );
		}
	"""
	tests_lib.build_program( c_program_text )


def ClassTypesInfo_Test3():
	c_program_text= """
	template</ size_type size0, size_type size1 />
	fn constexpr StringEquals( [ char8, size0 ]& s0, [ char8, size1 ]& s1 ) : bool
	{
		if( size0 != size1 ) { return false; }
		var size_type mut i(0);
		while( i < size0 )
		{
			if( s0[i] != s1[i] ) { return false; }
			++i;
		}
		return true;
	}

	template</ type T, size_type name_size />
	fn constexpr ClassFieldsListNodeOffset( T& node, [ char8, name_size ]& name ) : size_type
	{
		static_if( T::is_end )
		{
			halt;
		}
		else
		{
			if( StringEquals( node.name, name ) )
			{
				return node.offset;
			}
			else
			{
				return ::ClassFieldsListNodeOffset( node.next, name );
			}
		}
	}

	struct S
	{
		i32 x;
		i32 y;
	}

	auto constexpr size_of_i32= typeinfo</i32/>.size_of;
	auto constexpr x_offset= ClassFieldsListNodeOffset( typeinfo</S/>.fields_list, "x" );
	auto constexpr y_offset= ClassFieldsListNodeOffset( typeinfo</S/>.fields_list, "y" );
	static_assert( x_offset != y_offset );
	static_assert( x_offset == size_type(0) || x_offset == size_of_i32 );
	static_assert( y_offset == size_type(0) || y_offset == size_of_i32 );
	"""
	tests_lib.build_program( c_program_text )


def ClassTypesInfo_Test4():
	c_program_text= """
	template</ size_type size0, size_type size1 />
	fn constexpr StringEquals( [ char8, size0 ]& s0, [ char8, size1 ]& s1 ) : bool
	{
		if( size0 != size1 ) { return false; }
		var size_type mut i(0);
		while( i < size0 )
		{
			if( s0[i] != s1[i] ) { return false; }
			++i;
		}
		return true;
	}

	struct PPP
	{
		bool is_public;
		bool is_private;
		bool is_protected;
	}
	template</ type T, size_type name_size />
	fn constexpr GetVisibility( T& node, [ char8, name_size ]& name ) : PPP
	{
		static_if( T::is_end )
		{
			halt;
		}
		else
		{
			if( StringEquals( node.name, name ) )
			{
				var PPP result{ .is_public= node.is_public, .is_private= node.is_private, .is_protected= node.is_protected };
				return result;
			}
			else
			{
				return ::GetVisibility( node.next, name );
			}
		}
	}

	class A polymorph
	{
	public:
		fn Foo();
		type T0= A;
		i32 x;
	protected:
		fn Bar();
		type T1= A;
		i32 y;
	private:
		fn Baz();
		type T2= A;
		i32 z;
	}

	static_assert( GetVisibility( typeinfo</A/>.functions_list, "Foo" ).is_public );
	static_assert( GetVisibility( typeinfo</A/>.types_list, "T0" ).is_public );
	static_assert( GetVisibility( typeinfo</A/>.fields_list, "x" ).is_public );
	static_assert( GetVisibility( typeinfo</A/>.functions_list, "Bar" ).is_protected );
	static_assert( GetVisibility( typeinfo</A/>.types_list, "T1" ).is_protected );
	static_assert( GetVisibility( typeinfo</A/>.fields_list, "y" ).is_protected );
	static_assert( GetVisibility( typeinfo</A/>.functions_list, "Baz" ).is_private );
	static_assert( GetVisibility( typeinfo</A/>.types_list, "T2" ).is_private );
	static_assert( GetVisibility( typeinfo</A/>.fields_list, "z" ).is_private );

	static_assert( !GetVisibility( typeinfo</A/>.fields_list, "z" ).is_public );
	static_assert( !GetVisibility( typeinfo</A/>.types_list, "T1" ).is_public );
	static_assert( !GetVisibility( typeinfo</A/>.functions_list, "Foo" ).is_private );
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
		template</ type T /> fn MustBeSame( T& a, T& b ) : bool { return true; }
		enum E : i32{ zero }
		fn Foo()
		{
			MustBeSame( typeinfo</i32/>, typeinfo</i32/> );
			MustBeSame( typeinfo</i32/>, typeinfo</E/>.underlaying_type );
		}
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoForIncompleteType_Test0():
	c_program_text= """
		struct S;
		fn Foo() { typeinfo</S/>; } // Here typeinfo is incomplete.
		struct S { i32 x; f32 y; bool z; }
		static_assert( typeinfo</S/>.field_count == size_type(3) ); // After completion of type, typeinfo for it becomes complete.
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoForIncompleteType_Test1():
	c_program_text= """
		struct S;
		fn Foo() { typeinfo</S/>; } // Here typeinfo is incomplete.
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoForIncompleteTypeIsIncomplete_Test0():
	c_program_text= """
		struct S;
		fn Foo()
		{
			typeinfo</S/>.is_class;  // typeinfo for incomplete type is incomplete
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingIncompleteType" )
	#assert( errors_list[0].file_pos.line == 5 )


def TypeinfoForIncompleteTypeIsIncomplete_Test1():
	c_program_text= """
		struct S;
		fn Foo()
		{
			typeinfo</ [ S, 2 ] />.is_array;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingIncompleteType" )
	#assert( errors_list[0].file_pos.line == 5 )


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


def TypeinfoList_EnumList_Test0():
	c_program_text= """
		template</ size_type size0, size_type size1 />
		fn constexpr StringEquals( [ char8, size0 ]& s0, [ char8, size1 ]& s1 ) : bool
		{
			if( size0 != size1 ) { return false; }
			var size_type mut i(0);
			while( i < size0 )
			{
				if( s0[i] != s1[i] ) { return false; }
				++i;
			}
			return true;
		}

		template</ type T, size_type name_size />
		fn constexpr NodeListHaveName( T& node, [ char8, name_size ]& name ) : bool
		{
			static_if( T::is_end )
			{
				return false;
			}
			else
			{
				if( StringEquals( node.name, name ) )
				{
					return true;
				}
				else
				{
					return ::NodeListHaveName( node.next, name );
				}
			}
		}

		enum E{ A, B, C, Dee, Frtr }

		static_assert( NodeListHaveName( typeinfo</E/>.elements_list, "A" ) );
		static_assert( NodeListHaveName( typeinfo</E/>.elements_list, "B" ) );
		static_assert( NodeListHaveName( typeinfo</E/>.elements_list, "C" ) );
		static_assert( NodeListHaveName( typeinfo</E/>.elements_list, "Dee" ) );
		static_assert( NodeListHaveName( typeinfo</E/>.elements_list, "Frtr" ) );
		static_assert( !NodeListHaveName( typeinfo</E/>.elements_list, "D" ) );
		static_assert( !NodeListHaveName( typeinfo</E/>.elements_list, "LOL" ) );
		static_assert( !NodeListHaveName( typeinfo</E/>.elements_list, "a" ) );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoList_EnumList_Test1():
	c_program_text= """
		template</ size_type size0, size_type size1 />
		fn constexpr StringEquals( [ char8, size0 ]& s0, [ char8, size1 ]& s1 ) : bool
		{
			if( size0 != size1 ) { return false; }
			var size_type mut i(0);
			while( i < size0 )
			{
				if( s0[i] != s1[i] ) { return false; }
				++i;
			}
			return true;
		}

		template</ type T, size_type name_size />
		fn constexpr GetEnumNodeValue( T& node, [ char8, name_size ]& name ) : u32
		{
			static_if( T::is_end )
			{
				halt;
			}
			else
			{
				if( StringEquals( node.name, name ) )
				{
					return node.value;
				}
				else
				{
					return ::GetEnumNodeValue( node.next, name );
				}
			}
		}

		enum E : u32 { A, B, C, Dee, Frtr }

		static_assert( GetEnumNodeValue( typeinfo</ E />.elements_list, "A" ) == 0u );
		static_assert( GetEnumNodeValue( typeinfo</ E />.elements_list, "B" ) == 1u );
		static_assert( GetEnumNodeValue( typeinfo</ E />.elements_list, "C" ) == 2u );
		static_assert( GetEnumNodeValue( typeinfo</ E />.elements_list, "Dee" ) == 3u );
		static_assert( GetEnumNodeValue( typeinfo</ E />.elements_list, "Frtr" ) == 4u );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoList_ClassFieldsList_Test0():
	c_program_text= """
		template</ type T />
		fn constexpr IsEmptyClassFieldsList( T& node ) : bool { return T::is_end; }

		struct S{}
		static_assert( IsEmptyClassFieldsList( typeinfo</S/>.fields_list ) );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoList_ClassFieldsList_Test1():
	c_program_text= """
		template</ size_type size0, size_type size1 />
		fn constexpr StringEquals( [ char8, size0 ]& s0, [ char8, size1 ]& s1 ) : bool
		{
			if( size0 != size1 ) { return false; }
			var size_type mut i(0);
			while( i < size0 )
			{
				if( s0[i] != s1[i] ) { return false; }
				++i;
			}
			return true;
		}

		template</ type T, size_type name_size />
		fn constexpr NodeListHaveName( T& node, [ char8, name_size ]& name ) : bool
		{
			static_if( T::is_end )
			{
				return false;
			}
			else
			{
				if( StringEquals( node.name, name ) )
				{
					return true;
				}
				else
				{
					return ::NodeListHaveName( node.next, name );
				}
			}
		}

		struct S{ i32 x; f32 y; bool zzz; }

		static_assert( NodeListHaveName( typeinfo</S/>.fields_list, "x" ) );
		static_assert( NodeListHaveName( typeinfo</S/>.fields_list, "y" ) );
		static_assert( NodeListHaveName( typeinfo</S/>.fields_list, "zzz" ) );
		static_assert( !NodeListHaveName( typeinfo</S/>.fields_list, "xx" ) );
		static_assert( !NodeListHaveName( typeinfo</S/>.fields_list, "fff" ) );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoList_ClassTypesList_Test0():
	c_program_text= """
		template</ size_type size0, size_type size1 />
		fn constexpr StringEquals( [ char8, size0 ]& s0, [ char8, size1 ]& s1 ) : bool
		{
			if( size0 != size1 ) { return false; }
			var size_type mut i(0);
			while( i < size0 )
			{
				if( s0[i] != s1[i] ) { return false; }
				++i;
			}
			return true;
		}

		template</ type T, size_type name_size />
		fn constexpr NodeListHaveName( T& node, [ char8, name_size ]& name ) : bool
		{
			static_if( T::is_end )
			{
				return false;
			}
			else
			{
				if( StringEquals( node.name, name ) )
				{
					return true;
				}
				else
				{
					return ::NodeListHaveName( node.next, name );
				}
			}
		}

		struct S
		{
			type Self= S;
			type index= u32;
			enum EEE{ A }
			struct SS{}
		}

		static_assert( NodeListHaveName( typeinfo</S/>.types_list, "Self" ) );
		static_assert( NodeListHaveName( typeinfo</S/>.types_list, "index" ) );
		static_assert( NodeListHaveName( typeinfo</S/>.types_list, "EEE" ) );
		static_assert( NodeListHaveName( typeinfo</S/>.types_list, "SS" ) );
		static_assert( !NodeListHaveName( typeinfo</S/>.types_list, "xx" ) );
		static_assert( !NodeListHaveName( typeinfo</S/>.types_list, "fff" ) );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoList_ClassFunctionsList_Test0():
	c_program_text= """
		template</ size_type size0, size_type size1 />
		fn constexpr StringEquals( [ char8, size0 ]& s0, [ char8, size1 ]& s1 ) : bool
		{
			if( size0 != size1 ) { return false; }
			var size_type mut i(0);
			while( i < size0 )
			{
				if( s0[i] != s1[i] ) { return false; }
				++i;
			}
			return true;
		}

		template</ type T, size_type name_size />
		fn constexpr NodeListNameCount( T& node, [ char8, name_size ]& name ) : u32
		{
			static_if( T::is_end )
			{
				return 0u;
			}
			else
			{
				var u32 mut result(0);
				if( StringEquals( node.name, name ) )
				{
					++result;
				}
				return result + ::NodeListNameCount( node.next, name );
			}
		}

		struct S
		{
			fn Foo(){}
			fn Bar(){}
			fn Bar( i32 x, f32 y ) {}

			fn constructor(this)= default;
			fn constructor(this, S& other)= default;
			op+( S& a, S& b ) {}
		}

		static_assert( NodeListNameCount( typeinfo</S/>.functions_list, "Foo" ) == 1u );
		static_assert( NodeListNameCount( typeinfo</S/>.functions_list, "Bar" ) == 2u );
		static_assert( NodeListNameCount( typeinfo</S/>.functions_list, "constructor" ) == 2u );
		static_assert( NodeListNameCount( typeinfo</S/>.functions_list, "destructor" ) == 1u );
		static_assert( NodeListNameCount( typeinfo</S/>.functions_list, "+" ) == 1u );
		static_assert( NodeListNameCount( typeinfo</S/>.functions_list, "-" ) == 0u );
		static_assert( NodeListNameCount( typeinfo</S/>.functions_list, "UnknownFunc" ) == 0u );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoList_ClassFunctionsList_Test1():
	c_program_text= """
		template</ size_type size0, size_type size1 />
		fn constexpr StringEquals( [ char8, size0 ]& s0, [ char8, size1 ]& s1 ) : bool
		{
			if( size0 != size1 ) { return false; }
			var size_type mut i(0);
			while( i < size0 )
			{
				if( s0[i] != s1[i] ) { return false; }
				++i;
			}
			return true;
		}

		template</ type T, size_type name_size />
		fn constexpr GetFunctionUnsafe( T& node, [ char8, name_size ]& name ) : bool
		{
			static_if( T::is_end )
			{
				halt;
			}
			else
			{
				if( StringEquals( node.name, name ) )
				{
					return node.type.unsafe;
				}
				else
				{
					return ::GetFunctionUnsafe( node.next, name );
				}
			}
		}

		class A
		{
			fn Foo() unsafe;
			fn Bar();
			fn ZFF() unsafe;
			fn TTTR();
			fn constructor()= default;
		}

		static_assert( GetFunctionUnsafe( typeinfo</ A />.functions_list, "Foo" ) == true );
		static_assert( GetFunctionUnsafe( typeinfo</ A />.functions_list, "Bar" ) == false );
		static_assert( GetFunctionUnsafe( typeinfo</ A />.functions_list, "ZFF" ) == true );
		static_assert( GetFunctionUnsafe( typeinfo</ A />.functions_list, "TTTR" ) == false );
		static_assert( GetFunctionUnsafe( typeinfo</ A />.functions_list, "constructor" ) == false );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoList_ClassParentsList_Test0():
	c_program_text= """
		template</ type T />
		fn constexpr GetParentCount( T& node ) : u32
		{
			static_if( T::is_end )
			{
				return 0u;
			}
			else
			{
				return 1u + ::GetParentCount( node.next );
			}
		}

		class A interface{}
		class B interface{}
		class C polymorph{}
		class D : A, B, C {}
		class E : D {}
		class F : C, A {}

		static_assert( GetParentCount( typeinfo</A/>.parents_list ) == 0u );
		static_assert( GetParentCount( typeinfo</B/>.parents_list ) == 0u );
		static_assert( GetParentCount( typeinfo</C/>.parents_list ) == 0u );
		static_assert( GetParentCount( typeinfo</D/>.parents_list ) == 3u );
		static_assert( GetParentCount( typeinfo</E/>.parents_list ) == 1u );
		static_assert( GetParentCount( typeinfo</F/>.parents_list ) == 2u );
		static_assert( typeinfo</A/>.parent_count == size_type(0) );
		static_assert( typeinfo</B/>.parent_count == size_type(0) );
		static_assert( typeinfo</C/>.parent_count == size_type(0) );
		static_assert( typeinfo</D/>.parent_count == size_type(3) );
		static_assert( typeinfo</E/>.parent_count == size_type(1) );
		static_assert( typeinfo</F/>.parent_count == size_type(2) );
	"""
	tests_lib.build_program( c_program_text )
