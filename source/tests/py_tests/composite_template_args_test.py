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


def ArrayTemplateArg_Test6():
	c_program_text= """
		// Type template overloading.
		// Specialize template for array of zeros.
		template</ [i32, 2 ] arr_arg /> struct S { auto is_zero_arr= false; }
		template<//> struct S</ zero_arr /> { auto is_zero_arr= true; }
		var [ i32, 2 ] zero_arr= zero_init;

		var [ i32, 2 ] a[ 0, 1 ], b[ 1, 0 ], c[ 0, 0 ], d[ 1, 1 ];
		static_assert( !S</a/>::is_zero_arr );
		static_assert( !S</b/>::is_zero_arr );
		static_assert(  S</c/>::is_zero_arr );
		static_assert( !S</d/>::is_zero_arr );
	"""
	tests_lib.build_program( c_program_text )


def ArrayTemplateArg_Test7():
	c_program_text= """
		// Array of tuples.
		template</ [ tup[ char8, u32, bool ], 2 ] arr_arg />
		struct S
		{
			auto val= arr_arg;
		}
		var [ tup[ char8, u32, bool ], 2 ] a[ [ "~"c8, 8756u, false ], [ "\\0"c8, 75427344u, true ] ];
		static_assert( S</ a />::val == a );
	"""
	tests_lib.build_program( c_program_text )


def TupleTemplateArg_Test0():
	c_program_text= """
		type MyTup= tup[ i32, byte32 ];
		template</ MyTup tup_arg />
		struct S
		{
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleTemplateArg_Test1():
	c_program_text= """
		template</ tup[ char16, bool, [ i32, 3 ] ] tuple_arg />
		struct S
		{
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleTemplateArg_Test2():
	c_program_text= """
		template</ tup[ i32, u64, bool ] tup_arg />
		struct S
		{
			static_assert( tup_arg[0] == 427 );
			static_assert( tup_arg[1] == 14u64 );
			static_assert( tup_arg[2] == true );
		}
		var tup[ i32, u64, bool ] t[ 427, 14u64, true ];
		type S_alias= S</ t />;
	"""
	tests_lib.build_program( c_program_text )


def TupleTemplateArg_Test3():
	c_program_text= """
		template</ type A, type B, tup[ A, B ] tup_arg />
		struct S</ tup_arg /> // Deduce "A", "B", "tup_arg" based on "tup_arg".
		{
			auto val= tup_arg;
		}
		var tup[ char8, u32 ] t0[ "T"c8, 8765u ];
		static_assert( S</ t0 />::val == t0 );
		var tup[ i32, i16 ] t1[ 78, -5i16 ];
		static_assert( S</ t1 />::val == t1 );
	"""
	tests_lib.build_program( c_program_text )


def TupleTemplateArg_Test4():
	c_program_text= """
		// Type template overloading.
		// Specialize template for tuple of ones.
		template</ tup[ u32, i32 ] tup_arg /> struct S { auto is_ones_tup= false; }
		template<//> struct S</ ones_tup /> { auto is_ones_tup= true; }
		var tup[ u32, i32 ] ones_tup[ 1u, 1 ];

		var tup[ u32, i32 ] a[ 0u, 1 ], b[ 1u, 0 ], c[ 0u, 0 ], d[ 1u, 1 ];
		static_assert( !S</a/>::is_ones_tup );
		static_assert( !S</b/>::is_ones_tup );
		static_assert( !S</c/>::is_ones_tup );
		static_assert(  S</d/>::is_ones_tup );
	"""
	tests_lib.build_program( c_program_text )


def TupleTemplateArg_Test5():
	c_program_text= """
		// Tuple of arrays.
		template</ tup[ [ i32, 2 ], [ u32, 3 ] ] tup_arg />
		struct S
		{
			auto val= tup_arg;
		}
		var tup[ [ i32, 2 ], [ u32, 3 ] ] t[ [ 48, -3 ], [ 78u, 12467u, 1u  ] ];
		static_assert( S</ t />::val == t );
	"""
	tests_lib.build_program( c_program_text )


def FunctionTemplateCompositeArg_Test0():
	c_program_text= """
		template</ [ char8, 1s ] field_name, type T />
		fn AccessField( T& t ) : auto
		{
			return mixin( "t." + field_name );
		}
		struct S
		{
			i32 a;
			f32 b;
		}
		var S constexpr s{ .a= 87, .b= 0.25f };
		static_assert( AccessField</ "a" />(s) == 87 );
		static_assert( AccessField</ "b" />(s) == 0.25f );
	"""
	tests_lib.build_program( c_program_text )


def FunctionTemplateCompositeArg_Test1():
	c_program_text= """
		template</ [ char8, S ] field_name, size_type S, type T />
		fn AccessField( T& t ) : auto
		{
			return mixin( "t." + field_name );
		}
		struct S
		{
			i32 first;
			f32 second;
		}
		var S constexpr s{ .first= 87, .second= 0.25f };
		static_assert( AccessField</ "first", 5s, S />(s) == 87 );
		static_assert( AccessField</ "second", 6s, S />(s) == 0.25f );
	"""
	tests_lib.build_program( c_program_text )
