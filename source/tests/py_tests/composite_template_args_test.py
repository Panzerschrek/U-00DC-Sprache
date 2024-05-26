from py_tests_common import *


def ArrayTemplateArg_Test0():
	c_program_text= """
		type IntVec2= [ i32, 2 ];
		template</ IntVec2 arr_arg />
		struct S
		{
		}
	"""
	tests_lib.build_program( c_program_text )


def ArrayTemplateArg_Test1():
	c_program_text= """
		template</ [ u32, 3] arr_arg />
		struct S
		{
		}
	"""
	tests_lib.build_program( c_program_text )


def ArrayTemplateArg_Test2():
	c_program_text= """
		template</ [ i32, 2 ] arr_arg />
		struct S
		{
			static_assert( arr_arg[0] == 7 );
			static_assert( arr_arg[1] == 897 );
		}
		var [ i32, 2 ] arr[ 7, 897 ];
		type S_7_897= S</ arr />;
	"""
	tests_lib.build_program( c_program_text )


def ArrayTemplateArg_Test3():
	c_program_text= """
		template</ size_type S, [ char8, S ] str_arg />
		struct S</ str_arg /> // Deduce both size argument and value argument.
		{
			static_assert( str_arg == "Schadenfreude" );
		}
		type S_str= S</ "Schadenfreude" />;
	"""
	tests_lib.build_program( c_program_text )


def ArrayTemplateArg_Test4():
	c_program_text= """
		template</ size_type S, [ char16, S ] str_arg />
		struct S</ str_arg /> // Deduce both size argument and value argument.
		{
			auto& str_val= str_arg;
		}
		static_assert( S</ "Foo"u16 />::str_val == "Foo"u16 );
		static_assert( S</ "Qwerty"u16 />::str_val == "Qwerty"u16 );
	"""
	tests_lib.build_program( c_program_text )


def ArrayTemplateArg_Test5():
	c_program_text= """
		template</ type char_type, size_type S, [ char_type, S ] str_arg />
		struct S</ str_arg /> // Deduce type argument, size argument and value argument.
		{
			auto& str_val= str_arg;
		}
		static_assert( S</ "Khe-Khe" />::str_val == "Khe-Khe" );
		static_assert( S</ "Foo"u16 />::str_val == "Foo"u16 );
		static_assert( S</ "Qwerty"u32 />::str_val == "Qwerty"u32 );
	"""
	tests_lib.build_program( c_program_text )
