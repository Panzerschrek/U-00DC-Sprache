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


def TypeTemplatesOvelroading_Specialization_Test0():
	c_program_text= """
		template</ type T />
		struct S</ T />
		{
			auto constexpr x= 555;
		}

		template</ type T, type SizeType, SizeType s />
		struct S</ [ T, s ] />  // Array is more specialized.
		{
			auto constexpr x= 999;
		}

		static_assert( S</ [ i32, 4 ] />::x == 999 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_Specialization_Test1():
	c_program_text= """
		template</ type T /> struct Box{}

		template</ type T />
		struct S</ Box</ T /> />  // template is more specialized.
		{
			auto constexpr x= 22258;
		}

		template</ type T />
		struct S</ T />
		{
			auto constexpr x= 88852;
		}

		static_assert( S</ Box</ f32 /> />::x == 22258 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_Specialization_Test2():
	c_program_text= """
		template</  />
		struct S</ bool />  // concrete type is more specialized.
		{
			auto constexpr x= 44456;
		}

		template</ type T />
		struct S</ T />
		{
			auto constexpr x= 66654;
		}

		static_assert( S</ bool />::x == 44456 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_Specialization_Test3():
	c_program_text= """
		template</ type T />
		struct S</ T />
		{
			auto constexpr x= 11111;
		}

		template</ type T />
		struct S</ fn() : T />  // function type is more specialized.
		{
			auto constexpr x= 55555;
		}


		static_assert( S</ fn() : i16 />::x == 55555 );
	"""
	tests_lib.build_program( c_program_text )
