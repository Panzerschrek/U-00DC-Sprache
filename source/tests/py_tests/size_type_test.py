from py_tests_common import *


def SizeTypeIsDistinct_Test0():
	c_program_text= """
		// size_type isn't equal to any other fundamental type.
		static_assert( !same_type</size_type,   i8/> );
		static_assert( !same_type</size_type,   u8/> );
		static_assert( !same_type</size_type,  i16/> );
		static_assert( !same_type</size_type,  u16/> );
		static_assert( !same_type</size_type,  i32/> );
		static_assert( !same_type</size_type,  u32/> );
		static_assert( !same_type</size_type,  i64/> );
		static_assert( !same_type</size_type,  u64/> );
		static_assert( !same_type</size_type, i128/> );
		static_assert( !same_type</size_type, u128/> );
		static_assert( !same_type</size_type,  char8/> );
		static_assert( !same_type</size_type, char16/> );
		static_assert( !same_type</size_type, char32/> );
		static_assert( !same_type</size_type, f32/> );
		static_assert( !same_type</size_type, f64/> );
		static_assert( !same_type</size_type,   byte8/> );
		static_assert( !same_type</size_type,  byte16/> );
		static_assert( !same_type</size_type,  byte32/> );
		static_assert( !same_type</size_type,  byte64/> );
		static_assert( !same_type</size_type, byte128/> );
		static_assert( !same_type</size_type, void/> );
		static_assert( !same_type</size_type, bool/> );
		// size_type is equal to size_type.
		static_assert( same_type</size_type, size_type/> );
	"""
	tests_lib.build_program( c_program_text )


def SizeTypeIsDistinct_Test1():
	c_program_text= """
		fn Foo( size_type s ){}
		fn Bar()
		{
			var u32 x(0);
			Foo(x); // Can't call a function with "u32" argument where "size_type" is required.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 6 ) )


def SizeTypeIsDistinct_Test2():
	c_program_text= """
		fn Foo( size_type s ){}
		fn Bar()
		{
			var u64 x(0);
			Foo(x); // Can't call a function with "u32" argument where "size_type" is required.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 6 ) )


def SizeTypeIsDistinct_Test3():
	c_program_text= """
		fn Foo( u32 x ){}
		fn Bar()
		{
			var size_type s(0);
			Foo(s); // Can't call a function with "size_type" argument where "u32" is required.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 6 ) )


def SizeTypeIsDistinct_Test4():
	c_program_text= """
		fn Foo( u64 x ){}
		fn Bar()
		{
			var size_type s(0);
			Foo(s); // Can't call a function with "size_type" argument where "u32" is required.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 6 ) )
