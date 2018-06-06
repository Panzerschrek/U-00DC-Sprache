from py_tests_common import *


def TypeTemplatesOvelroading_MustSelectSpecializedTemplate_Test0():
	c_program_text= """
		template</ /> struct S</ i32 />
		{
			auto constexpr x= 555;
		}

		template</ /> struct S</ f32 />
		{
			auto constexpr x= 999;
		}

		static_assert( S</i32/>::x == 555 );
		static_assert( S</f32/>::x == 999 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_MustSelectSpecializedTemplate_Test1():
	c_program_text= """
		template</ /> struct S</ i32 />
		{
			auto constexpr x= 555;
		}

		template</ /> struct S</ f32 />
		{
			auto constexpr x= 999;
		}

		template</ type T />
		struct F</ S</ T /> />  // Must select here one of overloaded type templates.
		{
			type TT= T;
		}

		fn Foo( F</ S</ i32 /> />::TT arg ) : i32 { return arg; }
		fn Foo( F</ S</ f32 /> />::TT arg ) : f32 { return arg; }
	"""
	tests_lib.build_program( c_program_text )
