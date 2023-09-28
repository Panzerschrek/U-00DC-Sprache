from py_tests_common import *


def TypeInfoOperator_Test0():
	c_program_text= """
		fn Foo()
		{
			auto& ti= typeinfo</ i32 />;
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


def EqualityComparable_TypeinfoFiled():
	c_program_text= """
		static_assert( typeinfo</void/>.is_equality_comparable );
		static_assert( typeinfo</i32/>.is_equality_comparable );
		static_assert( typeinfo</f64/>.is_equality_comparable );
		static_assert( typeinfo</u8/>.is_equality_comparable );
		static_assert( typeinfo</char16/>.is_equality_comparable );
		static_assert( typeinfo</bool/>.is_equality_comparable );
		static_assert( typeinfo</ [ char8, 4 ] />.is_equality_comparable );
		static_assert( typeinfo</ tup[ u128, f32, i16, char32, bool ] />.is_equality_comparable );

		struct A{} // Constains generated "==" operator.
		static_assert( typeinfo</A/>.is_equality_comparable );

		struct B{ i32 x; } // Constains generated "==" operator.
		static_assert( typeinfo</B/>.is_equality_comparable );

		struct C{ f32& x; } // Constains no generated "==" operator because of reference field.
		static_assert( !typeinfo</C/>.is_equality_comparable );

		struct D{ f32& x; op==(D& l, D& r) : bool; } // Contains explicit "==" operator.
		static_assert( typeinfo</D/>.is_equality_comparable );

		struct E{ f32 x; i32 y; op==(E& l, E& r) : bool = default; } // Contains explicit generated "==" operator.
		static_assert( typeinfo</E/>.is_equality_comparable );

		class F{} // Contains no generated "==" operator because this is calss.
		static_assert( !typeinfo</F/>.is_equality_comparable );

		class G{ bool x; } // Contains no generated "==" operator because this is calss.
		static_assert( !typeinfo</G/>.is_equality_comparable );

		class H{ bool x; op==(H& l, H& r) : bool = default; } // Contains requested generated "==" operator.
		static_assert( typeinfo</H/>.is_equality_comparable );

		class I{ u64  x; op==(I& l, I& r) : bool; } // Contains explicit  "==" operator.
		static_assert( typeinfo</I/>.is_equality_comparable );

		class J{ u64  x; op==(J& l, A& r) : bool; } // Contains wrong "==" operator - for comparision agains another type. This is not a proper "==" operator for this class.
		static_assert( !typeinfo</J/>.is_equality_comparable );
	"""
	tests_lib.build_program( c_program_text )


def SizeAndAlignmentFields_Test0():
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
			static_assert( typeinfo</char8 />.is_char );
			static_assert( typeinfo</char16/>.is_char );
			static_assert( typeinfo</char32/>.is_char );
			static_assert( !typeinfo</f32/>.is_char );
			static_assert( !typeinfo</bool/>.is_char );
			static_assert( !typeinfo</void/>.is_char );
			static_assert( !typeinfo</u8/>.is_char );
			static_assert( !typeinfo</i8/>.is_char );
			static_assert( !typeinfo</u32/>.is_char );
			static_assert( !typeinfo</i32/>.is_char );
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


def FunctionPointerTypesInfo_Test0():
	c_program_text= """
		type FnPtr = (fn() : f32);
		fn Foo()
		{
			static_assert( typeinfo</FnPtr/>.is_function_pointer );
			static_assert( typeinfo</FnPtr/>.return_type.is_float );
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
	fn constexpr ClassFieldsListNodeOffset( T& list, [ char8, name_size ]& name ) : size_type
	{
		for( & field_info : list )
		{
			if( StringEquals( field_info.name, name ) )
			{
				return field_info.offset;
			}
		}
		halt;
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
	fn constexpr GetVisibility( T& list, [ char8, name_size ]& name ) : PPP
	{
		for( & list_element : list )
		{
			if( StringEquals( list_element.name, name ) )
			{
				var PPP result{ .is_public= list_element.is_public, .is_private= list_element.is_private, .is_protected= list_element.is_protected };
				return result;
			}
		}
		halt;
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
	assert( errors_list[0].src_loc.line == 5 )


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
	assert( errors_list[0].src_loc.line == 4 )


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
	assert( errors_list[0].src_loc.line == 4 )


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
	assert( errors_list[0].src_loc.line == 5 )


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
	assert( errors_list[0].src_loc.line == 4 )


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
	assert( errors_list[0].src_loc.line == 5 )


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
	assert( errors_list[0].src_loc.line == 4 )


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
	assert( errors_list[0].src_loc.line == 5 )


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
	assert( errors_list[0].src_loc.line == 4 )


def TypeinfoList_FunctionTypeParams_Test0():
	c_program_text= """
		fn Foo()
		{
			auto& ti= typeinfo</ ( fn( i32 x, f32 & y, bool &mut z ) : size_type ) />;
			static_assert( ti.is_function_pointer );

			static_assert( ti.arguments_list[0].type.is_signed_integer );
			static_assert( !ti.arguments_list[0].is_mutable );
			static_assert( !ti.arguments_list[0].is_reference );

			static_assert( ti.arguments_list[1].type.is_float );
			static_assert( !ti.arguments_list[1].is_mutable );
			static_assert( ti.arguments_list[1].is_reference );

			static_assert( ti.arguments_list[2].type.is_bool );
			static_assert( ti.arguments_list[2].is_mutable );
			static_assert( ti.arguments_list[2].is_reference );
		}
	"""
	tests_lib.build_program( c_program_text )


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
		fn constexpr ListHaveName( T& list, [ char8, name_size ]& name ) : bool
		{
			for( &list_element : list )
			{
				if( StringEquals( list_element.name, name ) )
				{
					return true;
				}
			}
			return false;
		}

		enum E{ A, B, C, Dee, Frtr }

		static_assert( ListHaveName( typeinfo</E/>.elements_list, "A" ) );
		static_assert( ListHaveName( typeinfo</E/>.elements_list, "B" ) );
		static_assert( ListHaveName( typeinfo</E/>.elements_list, "C" ) );
		static_assert( ListHaveName( typeinfo</E/>.elements_list, "Dee" ) );
		static_assert( ListHaveName( typeinfo</E/>.elements_list, "Frtr" ) );
		static_assert( !ListHaveName( typeinfo</E/>.elements_list, "D" ) );
		static_assert( !ListHaveName( typeinfo</E/>.elements_list, "LOL" ) );
		static_assert( !ListHaveName( typeinfo</E/>.elements_list, "a" ) );
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
		fn constexpr GetEnumNodeValue( T& list, [ char8, name_size ]& name ) : u32
		{
			for( &list_element : list )
			{
				if( StringEquals( list_element.name, name ) )
				{
					return list_element.value;
				}
			}
			halt;
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
		struct S{}
		static_assert( typeinfo</ typeof( typeinfo</S/>.fields_list ) />.element_count == 0s );
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
		fn constexpr NodeListHaveName( T& list, [ char8, name_size ]& name ) : bool
		{
			for( & list_element : list )
			{
				if( StringEquals( list_element.name, name ) )
				{
					return true;
				}
			}
			return false;
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
		fn constexpr NodeListHaveName( T& list, [ char8, name_size ]& name ) : bool
		{
			for( & list_element : list )
			{
				if( StringEquals( list_element.name, name ) )
				{
					return true;
				}
			}
			return false;
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
		fn constexpr NodeListNameCount( T& list, [ char8, name_size ]& name ) : size_type
		{
			var size_type mut count(0);
			for( & list_element : list )
			{
				if( StringEquals( list_element.name, name ) )
				{
					++count;
				}
			}
			return count;
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

		static_assert( NodeListNameCount( typeinfo</S/>.functions_list, "Foo" ) == 1s );
		static_assert( NodeListNameCount( typeinfo</S/>.functions_list, "Bar" ) == 2s );
		static_assert( NodeListNameCount( typeinfo</S/>.functions_list, "constructor" ) == 2s );
		static_assert( NodeListNameCount( typeinfo</S/>.functions_list, "destructor" ) == 1s );
		static_assert( NodeListNameCount( typeinfo</S/>.functions_list, "+" ) == 1s );
		static_assert( NodeListNameCount( typeinfo</S/>.functions_list, "-" ) == 0s );
		static_assert( NodeListNameCount( typeinfo</S/>.functions_list, "UnknownFunc" ) == 0s );
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
		fn constexpr GetFunctionUnsafe( T& list, [ char8, name_size ]& name ) : bool
		{
			for( & list_element : list )
			{
				if( StringEquals( list_element.name, name ) )
				{
					return list_element.type.unsafe;
				}
			}
			halt;
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
		fn constexpr GetParentCount( T& list ) : u32
		{
			return u32( typeinfo</T/>.element_count );
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


def ReferencesTagsField_Test0():
	c_program_text= """
		struct S{}
		enum E{ A }
		type FP= fn();
		type A= [ i32, 2 ];
		struct R{ i32& x; }
		type RA= [ R, 64 ];
		struct RMut{ i32& mut x; }
		type RMutTup= tup[ i32, f32, RMut ];
		fn Foo()
		{
			static_assert( typeinfo</i32/>.references_tags_count == size_type(0) );
			static_assert( !typeinfo</i32/>.contains_mutable_references );
			static_assert( typeinfo</  A/>.references_tags_count == size_type(0) );
			static_assert( !typeinfo</  A/>.contains_mutable_references );
			static_assert( typeinfo</  E/>.references_tags_count == size_type(0) );
			static_assert( !typeinfo</  E/>.contains_mutable_references );
			static_assert( typeinfo</ FP/>.references_tags_count == size_type(0) );
			static_assert( !typeinfo</ FP/>.contains_mutable_references );
			static_assert( typeinfo</  R/>.references_tags_count == size_type(1) );
			static_assert( !typeinfo</  R/>.contains_mutable_references );
			static_assert( typeinfo</ RA/>.references_tags_count == size_type(1) );
			static_assert( !typeinfo</ RA/>.contains_mutable_references );
			static_assert( typeinfo</RMut/>.references_tags_count == size_type(1) );
			static_assert( typeinfo</RMut/>.contains_mutable_references );
			static_assert( typeinfo</RMutTup/>.references_tags_count == size_type(1) );
			static_assert( typeinfo</RMutTup/>.contains_mutable_references );
		}
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoForTypeinfo_Test0():
	c_program_text= """
		static_assert( typeinfo</ typeof( typeinfo</ f32 /> ) />.is_class );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoForTypeinfo_Test1():
	c_program_text= """
		struct S{ i32 x; }
		static_assert( typeinfo</ typeof( typeinfo</ f32 /> ) />.is_typeinfo );
		static_assert( typeinfo</ typeof( typeinfo</ S /> ) />.is_typeinfo );
		static_assert( typeinfo</ typeof( typeinfo</ S />.fields_list[0] ) />.is_typeinfo );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoForTypeinfo_Test2():
	c_program_text= """
		auto& tt= typeinfo</ typeof( typeinfo</i32/> ) />;
		// There are no special methods for typeinfo class except destructor.
		static_assert( !tt.is_default_constructible );
		static_assert( !tt.is_copy_constructible );
		static_assert( !tt.is_copy_assignable );
		static_assert( !tt.is_equality_comparable );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoForTypeinfo_Test3():
	c_program_text= """
		auto& t= typeinfo</i32/>;
		type I32Typeinfo= typeof(t);
		fn Foo()
		{
			var I32Typeinfo default_constructed_typeinfo;
			var I32Typeinfo copy_constructed_typeinfo= t;
			unsafe{ cast_mut(t) = typeinfo</i32/>; } // Copy assignment operator doesn't exists.
			t == t; // Equality compare operator doesn't exists.
			t > t; // Order compare operator doesn't exists.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) >= 5 )
	assert( HaveError( errors_list, "ExpectedInitializer", 6 ) )
	assert( HaveError( errors_list, "CopyConstructValueOfNoncopyableType", 7 ) )
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 8 ) )
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 9 ) )
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 10 ) )


def Typeinfo_SrcType_Test0():
	c_program_text= """
		template</ type T /> struct MustBeSame</ T, T /> {}
		type SS= MustBeSame</ i32, i32 />;

		template</type T/> struct PassType{ type type_passed= T; }

		struct S{}

		type TIF= typeof( typeinfo</f32/> );
		type FS= MustBeSame</ TIF::src_type, f32 />;

		type TA= MustBeSame</ PassType</ typeof(typeinfo</ [u64, 8] /> ) />::type_passed::src_type, [u64, 8] />;
	"""

	tests_lib.build_program( c_program_text )


def TypeId_Test():
	c_program_text= """
		class A polymorph{}
		class B : A{}

		auto& a_id= typeinfo</A/>.type_id;
		auto& b_id= typeinfo</B/>.type_id;
		var size_type a_id_copy= typeinfo</A/>.type_id;
		var size_type b_id_copy= typeinfo</B/>.type_id;

		fn Foo()
		{
			auto& a_id= typeinfo</A/>.type_id;
			auto& b_id= typeinfo</B/>.type_id;
			var size_type a_id_copy= typeinfo</A/>.type_id;
			var size_type b_id_copy= typeinfo</B/>.type_id;
		}
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoListsAreLazy_Test0():
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
		fn constexpr ClassHasField( [ char8, name_size ]& name ) : bool
		{
			for( &field_info : typeinfo</T/>.fields_list )
			{
				if( StringEquals( field_info.name, name ) )
				{
					return true;
				}
			}
			return false;
		}

		namespace ClassCheck
		{
			class SomeClass{}
			auto& some_class_typeinfo= typeinfo</ SomeClass />;
			type SomeTypeinfoClass= typeof( some_class_typeinfo );
			static_assert( typeinfo</ SomeTypeinfoClass />.is_class );
			static_assert( typeinfo</ SomeTypeinfoClass />.is_typeinfo );
			// Regular fields - must be listed.
			static_assert( ClassHasField</ SomeTypeinfoClass />( "size_of" ) );
			static_assert( ClassHasField</ SomeTypeinfoClass />( "is_array" ) );
			static_assert( ClassHasField</ SomeTypeinfoClass />( "is_copy_assignable" ) );
			// List fields - must not be listed.
			static_assert( !ClassHasField</ SomeTypeinfoClass />( "fields_list" ) );
			static_assert( !ClassHasField</ SomeTypeinfoClass />( "types_list" ) );
			static_assert( !ClassHasField</ SomeTypeinfoClass />( "functions_list" ) );
			static_assert( !ClassHasField</ SomeTypeinfoClass />( "parents_list" ) );
			// But list fields must be accessible.
			auto& fields_list= some_class_typeinfo.fields_list;
			auto& types_list= some_class_typeinfo.types_list;
			auto& functions_list= some_class_typeinfo.functions_list;
			auto& parents_list= some_class_typeinfo.parents_list;
		}
		namespace TupleCheck
		{
			auto& some_tuple_typeinfo= typeinfo</ tup[ f32, i32 ] />;
			type SomeTypeinfoClass= typeof(some_tuple_typeinfo);
			static_assert( typeinfo</ SomeTypeinfoClass />.is_class );
			static_assert( typeinfo</ SomeTypeinfoClass />.is_typeinfo );
			// Regular fields - must be listed.
			static_assert( ClassHasField</ SomeTypeinfoClass />( "align_of" ) );
			static_assert( ClassHasField</ SomeTypeinfoClass />( "is_fundamental" ) );
			static_assert( ClassHasField</ SomeTypeinfoClass />( "is_copy_constructible" ) );
			// List fields - must not be listed.
			static_assert( !ClassHasField</ SomeTypeinfoClass />( "elements_list" ) );
			// But list fields must be accessible.
			auto& elements_list= some_tuple_typeinfo.elements_list;
		}
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoListsAreLazy_Test1():
	c_program_text= """
		fn Foo()
		{
			unsafe
			{
				var typeof( typeinfo</ tup[ f32, i32 ] /> ) mut variable_of_typeinfo_type= uninitialized;
				// Even we can create own mutable variable of typeinfo type (via unsafe) elements list is still immutable.
				variable_of_typeinfo_type.elements_list[0].offset= 42s;
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ExpectedReferenceValue", 8 ) )
