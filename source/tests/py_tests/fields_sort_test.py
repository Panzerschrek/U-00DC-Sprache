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
