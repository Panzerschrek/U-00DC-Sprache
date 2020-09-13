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


def KeepFieldsOrder_Test1():
	# Fields order must be stable even if "ordered" flag is not specified.
	# Fields will be reordered only if it is absolutly neccecary. Otherwise fields order should be preserved

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
	fn constexpr GetFieldOffset( [ char8, name_size ]& name ) : size_type
	{
		for( &list_element : typeinfo</T/>.fields_list )
		{
			if( StringEquals( list_element.name, name ) )
			{
				return list_element.offset;
			}
		}
		halt;
	}

	// No reodrering applied here.
	struct ABC{ i32 a; i32 b; i32 c; }
	struct CBA{ i32 c; i32 b; i32 a; }
	static_assert( typeinfo</ABC/>.size_of == 12s );
	static_assert( GetFieldOffset</ABC/>( "a" ) == 0s );
	static_assert( GetFieldOffset</ABC/>( "b" ) == 4s );
	static_assert( GetFieldOffset</ABC/>( "c" ) == 8s );
	static_assert( typeinfo</CBA/>.size_of == 12s );
	static_assert( GetFieldOffset</CBA/>( "c" ) == 0s );
	static_assert( GetFieldOffset</CBA/>( "b" ) == 4s );
	static_assert( GetFieldOffset</CBA/>( "a" ) == 8s );

	// No reodrering applied here, because fields in intitial roder fits without any padding.
	struct X
	{
		f32 a;
		bool b;
		bool c;
		bool d;
		bool e;
		f32 f;
	}
	static_assert( typeinfo</X/>.size_of == 12s );
	static_assert( GetFieldOffset</X/>( "a" ) == 0s );
	static_assert( GetFieldOffset</X/>( "b" ) == 4s );
	static_assert( GetFieldOffset</X/>( "c" ) == 5s );
	static_assert( GetFieldOffset</X/>( "d" ) == 6s );
	static_assert( GetFieldOffset</X/>( "e" ) == 7s );
	static_assert( GetFieldOffset</X/>( "f" ) == 8s );

	// No reodrering applied here, because fields in intitial roder fits without any padding.
	struct Y
	{
		f32 a;
		[ bool, 4 ] b;
		f32 c;
	}
	static_assert( typeinfo</Y/>.size_of == 12s );
	static_assert( GetFieldOffset</Y/>( "a" ) == 0s );
	static_assert( GetFieldOffset</Y/>( "b" ) == 4s );
	static_assert( GetFieldOffset</Y/>( "c" ) == 8s );

	// No reordering applied here, because padding will be added anyway.
	struct Z
	{
		i32 a;
		i8 b; // + 3 bytes of padding
		f32 c;
	}
	static_assert( typeinfo</Z/>.size_of == 12s );
	static_assert( GetFieldOffset</Z/>( "a" ) == 0s );
	static_assert( GetFieldOffset</Z/>( "b" ) == 4s );
	static_assert( GetFieldOffset</Z/>( "c" ) == 8s );

	// Reordering applied here, but order between "a" and "c", "b" and "d" is saved.
	struct W
	{
		i32 a;
		i16 b;
		i32 c;
		i16 d;
	}
	static_assert( typeinfo</W/>.size_of == 12s );
	static_assert( GetFieldOffset</W/>( "a" ) == 0s );
	static_assert( GetFieldOffset</W/>( "b" ) == 4s );
	static_assert( GetFieldOffset</W/>( "d" ) == 6s );
	static_assert( GetFieldOffset</W/>( "c" ) == 8s );

	struct S
	{
		i64 a;
		bool b;
		i16 c;
		i8 d;
		i32 e;
		f32 f;
		f64 g;
		i32 h;
	} // Rordered to a, b, d, c, e, f, h, g
	static_assert( typeinfo</S/>.size_of == 32s );
	static_assert( GetFieldOffset</S/>( "a" ) ==  0s );
	static_assert( GetFieldOffset</S/>( "b" ) ==  8s );
	static_assert( GetFieldOffset</S/>( "d" ) ==  9s );
	static_assert( GetFieldOffset</S/>( "c" ) == 10s );
	static_assert( GetFieldOffset</S/>( "e" ) == 12s );
	static_assert( GetFieldOffset</S/>( "f" ) == 16s );
	static_assert( GetFieldOffset</S/>( "h" ) == 20s );
	static_assert( GetFieldOffset</S/>( "g" ) == 24s );

	// No reordering, 1 byte of padding added.
	struct T
	{
		i8  a; // + 1 byte of padding
		i16 b;
		i32 c;
		i64 d;
	}
	static_assert( typeinfo</T/>.size_of == 16s );
	static_assert( GetFieldOffset</T/>( "a" ) ==  0s );
	static_assert( GetFieldOffset</T/>( "b" ) ==  2s );
	static_assert( GetFieldOffset</T/>( "c" ) ==  4s );
	static_assert( GetFieldOffset</T/>( "d" ) ==  8s );

	struct U
	{
		i8  a;
		i16 b;
		i32 c;
		i64 d;
		char8 e;
	} // "e" placed after "a"
	static_assert( typeinfo</U/>.size_of == 16s );
	static_assert( GetFieldOffset</U/>( "a" ) ==  0s );
	static_assert( GetFieldOffset</U/>( "e" ) ==  1s );
	static_assert( GetFieldOffset</U/>( "b" ) ==  2s );
	static_assert( GetFieldOffset</U/>( "c" ) ==  4s );
	static_assert( GetFieldOffset</U/>( "d" ) ==  8s );
	"""
	tests_lib.build_program( c_program_text )
