from py_tests_common import *


def FieldsSort_Test0():
	c_program_text= """
		struct S
		{
			bool a;
			i32 b;
			bool c;
		}
		static_assert( typeinfo</S/>.size_of <= size_type(8) );
	"""
	tests_lib.build_program( c_program_text )


def FieldsSort_Test1():
	c_program_text= """
		struct S
		{
			f32 x;
			f64 y;
			f32 z;
		}
		static_assert( typeinfo</S/>.size_of == size_type(16) );
	"""
	tests_lib.build_program( c_program_text )


def FieldsSort_Test2():
	c_program_text= """
		struct S
		{
			f64 a;
			bool b;
			i64 c;
			char8 d;
			i16 e;
			f32 f;
		}
		static_assert( typeinfo</S/>.size_of == size_type(24) );
	"""
	tests_lib.build_program( c_program_text )


def FieldsSort_Test3():
	c_program_text= """
		struct S
		{
			i32 a;
			i16 b;
			[ i32, 8 ] c;
			i16 d;
		}
		static_assert( typeinfo</S/>.size_of == size_type(40) );
	"""
	tests_lib.build_program( c_program_text )


def FieldsSort_Test4():
	c_program_text= """
		struct T{ i64 x; f64 y; }
		struct S
		{
			i32 a;
			char16 b;
			T c;
			char16 d;
		}
		static_assert( typeinfo</S/>.size_of == size_type(24) );
	"""
	tests_lib.build_program( c_program_text )


def KeepFieldsOrder_Test0():
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
	fn constexpr GetFieldOffset( T& list, [ char8, name_size ]& name ) : size_type
	{
		for( &list_element : list )
		{
			if( StringEquals( list_element.name, name ) )
			{
				return list_element.offset;
			}
		}
		halt;
	}

	struct A ordered
	{
		bool x;
		i32 y;
		bool z;
	}
	static_assert( GetFieldOffset( typeinfo</A/>.fields_list, "x" ) == size_type(0) );
	static_assert( GetFieldOffset( typeinfo</A/>.fields_list, "y" ) > GetFieldOffset( typeinfo</A/>.fields_list, "x" ) );
	static_assert( GetFieldOffset( typeinfo</A/>.fields_list, "z" ) > GetFieldOffset( typeinfo</A/>.fields_list, "y" ) );

	struct B ordered
	{
		bool z;
		f64 y;
		bool x;
	}
	static_assert( GetFieldOffset( typeinfo</B/>.fields_list, "z" ) == size_type(0) );
	static_assert( GetFieldOffset( typeinfo</B/>.fields_list, "y" ) > GetFieldOffset( typeinfo</B/>.fields_list, "z" ) );
	static_assert( GetFieldOffset( typeinfo</B/>.fields_list, "x" ) > GetFieldOffset( typeinfo</B/>.fields_list, "y" ) );

	struct C ordered
	{
		bool a;
		f32 b;
		bool c;
		f64 d;
		f32 e;
		bool f;
		bool g;
		i16 h;
		i32 i;
		bool j;
	}
	static_assert( GetFieldOffset( typeinfo</C/>.fields_list, "a" ) == size_type(0) );
	static_assert( GetFieldOffset( typeinfo</C/>.fields_list, "b" ) > GetFieldOffset( typeinfo</C/>.fields_list, "a" ) );
	static_assert( GetFieldOffset( typeinfo</C/>.fields_list, "c" ) > GetFieldOffset( typeinfo</C/>.fields_list, "b" ) );
	static_assert( GetFieldOffset( typeinfo</C/>.fields_list, "d" ) > GetFieldOffset( typeinfo</C/>.fields_list, "c" ) );
	static_assert( GetFieldOffset( typeinfo</C/>.fields_list, "e" ) > GetFieldOffset( typeinfo</C/>.fields_list, "d" ) );
	static_assert( GetFieldOffset( typeinfo</C/>.fields_list, "f" ) > GetFieldOffset( typeinfo</C/>.fields_list, "e" ) );
	static_assert( GetFieldOffset( typeinfo</C/>.fields_list, "g" ) > GetFieldOffset( typeinfo</C/>.fields_list, "f" ) );
	static_assert( GetFieldOffset( typeinfo</C/>.fields_list, "h" ) > GetFieldOffset( typeinfo</C/>.fields_list, "g" ) );
	static_assert( GetFieldOffset( typeinfo</C/>.fields_list, "i" ) > GetFieldOffset( typeinfo</C/>.fields_list, "h" ) );
	static_assert( GetFieldOffset( typeinfo</C/>.fields_list, "j" ) > GetFieldOffset( typeinfo</C/>.fields_list, "i" ) );
	"""
	tests_lib.build_program( c_program_text )
