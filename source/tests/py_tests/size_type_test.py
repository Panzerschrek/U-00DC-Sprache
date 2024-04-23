from py_tests_common import *

def SizeTypeInfo_Test0():
	c_program_text= """
		auto& ti= typeinfo</size_type/>;

		static_assert( ti.is_fundamental );
		static_assert( !ti.is_enum );
		static_assert( !ti.is_array );
		static_assert( !ti.is_tuple );
		static_assert( !ti.is_class );
		static_assert( !ti.is_raw_pointer );
		static_assert( !ti.is_function_pointer );

		static_assert( ti.is_numeric );
		static_assert( ti.is_integer );
		static_assert( ti.is_unsigned_integer );
		static_assert( !ti.is_signed_integer );
		static_assert( !ti.is_float );
		static_assert( !ti.is_char );
		static_assert( !ti.is_byte );
		static_assert( !ti.is_bool );
		static_assert( !ti.is_void );

		static_assert( !ti.is_default_constructible );
		static_assert( ti.is_copy_constructible );
		static_assert( ti.is_copy_assignable );
		static_assert( ti.is_equality_comparable );

		static_assert( ti.size_of == 4s || ti.size_of == 8s );
		static_assert( ti.size_of == typeinfo</ $(byte8) />.size_of ); // size_type always has pointer size.
	"""
	tests_lib.build_program( c_program_text )


def SizeTypeInfo_Test1():
	c_program_text= """
		auto& ti= typeinfo</ssize_type/>;

		static_assert( ti.is_fundamental );
		static_assert( !ti.is_enum );
		static_assert( !ti.is_array );
		static_assert( !ti.is_tuple );
		static_assert( !ti.is_class );
		static_assert( !ti.is_raw_pointer );
		static_assert( !ti.is_function_pointer );

		static_assert( ti.is_numeric );
		static_assert( ti.is_integer );
		static_assert( !ti.is_unsigned_integer );
		static_assert( ti.is_signed_integer );
		static_assert( !ti.is_float );
		static_assert( !ti.is_char );
		static_assert( !ti.is_byte );
		static_assert( !ti.is_bool );
		static_assert( !ti.is_void );

		static_assert( !ti.is_default_constructible );
		static_assert( ti.is_copy_constructible );
		static_assert( ti.is_copy_assignable );
		static_assert( ti.is_equality_comparable );

		static_assert( ti.size_of == 4s || ti.size_of == 8s );
		static_assert( ti.size_of == typeinfo</ $(byte8) />.size_of ); // ssize_type always has pointer size.
	"""
	tests_lib.build_program( c_program_text )


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
		// size_type and ssize_type are distinct.
		static_assert( !same_type</size_type, ssize_type/> );
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


def SsizeTypeIsDistinct_Test0():
	c_program_text= """
		// ssize_type isn't equal to any other fundamental type.
		static_assert( !same_type</ssize_type,   i8/> );
		static_assert( !same_type</ssize_type,   u8/> );
		static_assert( !same_type</ssize_type,  i16/> );
		static_assert( !same_type</ssize_type,  u16/> );
		static_assert( !same_type</ssize_type,  i32/> );
		static_assert( !same_type</ssize_type,  u32/> );
		static_assert( !same_type</ssize_type,  i64/> );
		static_assert( !same_type</ssize_type,  u64/> );
		static_assert( !same_type</ssize_type, i128/> );
		static_assert( !same_type</ssize_type, u128/> );
		static_assert( !same_type</ssize_type,  char8/> );
		static_assert( !same_type</ssize_type, char16/> );
		static_assert( !same_type</ssize_type, char32/> );
		static_assert( !same_type</ssize_type, f32/> );
		static_assert( !same_type</ssize_type, f64/> );
		static_assert( !same_type</ssize_type,   byte8/> );
		static_assert( !same_type</ssize_type,  byte16/> );
		static_assert( !same_type</ssize_type,  byte32/> );
		static_assert( !same_type</ssize_type,  byte64/> );
		static_assert( !same_type</ssize_type, byte128/> );
		static_assert( !same_type</ssize_type, void/> );
		static_assert( !same_type</ssize_type, bool/> );
		// ssize_type is equal to ssize_type.
		static_assert( same_type</ssize_type, ssize_type/> );
		// ssize_type and size_type are distinct.
		static_assert( !same_type</ssize_type, size_type/> );
	"""
	tests_lib.build_program( c_program_text )


def SsizeTypeIsDistinct_Test1():
	c_program_text= """
		fn Foo( ssize_type s ){}
		fn Bar()
		{
			var i32 x(0);
			Foo(x); // Can't call a function with "i32" argument where "ssize_type" is required.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 6 ) )


def SsizeTypeIsDistinct_Test2():
	c_program_text= """
		fn Foo( ssize_type s ){}
		fn Bar()
		{
			var i64 x(0);
			Foo(x); // Can't call a function with "i32" argument where "ssize_type" is required.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 6 ) )


def SsizeTypeIsDistinct_Test3():
	c_program_text= """
		fn Foo( i32 x ){}
		fn Bar()
		{
			var ssize_type s(0);
			Foo(s); // Can't call a function with "ssize_type" argument where "i32" is required.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 6 ) )


def SsizeTypeIsDistinct_Test4():
	c_program_text= """
		fn Foo( u64 x ){}
		fn Bar()
		{
			var ssize_type s(0);
			Foo(s); // Can't call a function with "ssize_type" argument where "i32" is required.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 6 ) )


def SizeTypesConversions_Test0():
	c_program_text= """
		// Unsigned integer to size_type.
		var u32 x(67), y(3249720402);
		var size_type xs(x), ys(y);
		static_assert(xs == 67s);
		static_assert(ys == 3249720402s);
	"""
	tests_lib.build_program( c_program_text )


def SizeTypesConversions_Test1():
	c_program_text= """
		// Signed integer to size_type.
		var i32 x(67), y(-33);
		var size_type xs(x), ys(y);
		static_assert(xs == 67s);
		// Should overflow in case of negative value.
		static_assert(ys == size_type(0u32 - 33u32) || ys == size_type(0u64 - 33u64));
	"""
	tests_lib.build_program( c_program_text )


def SizeTypesConversions_Test2():
	c_program_text= """
		// Unsigned integer to ssize_type.
		var u32 x(67), y(3249720402);
		var ssize_type xs(x), ys(y);
		static_assert(xs == ssize_type(67));
		// May overflow in case if ssize_type is 32 bit.
		static_assert(ys == ssize_type(i32(3249720402)) || ys == ssize_type(i64(3249720402)));
	"""
	tests_lib.build_program( c_program_text )


def SizeTypesConversions_Test3():
	c_program_text= """
		// Signed integer to ssize_type.
		var i32 x(67), y(-33);
		var ssize_type xs(x), ys(y);
		static_assert(xs == ssize_type(67));
		// Should not overflow.
		static_assert(ys == ssize_type(-33));
	"""
	tests_lib.build_program( c_program_text )


def SizeTypesConversions_Test4():
	c_program_text= """
		// size_type to ssize_type and backwards.
		auto a= 765745s;
		var ssize_type b(a);
		var size_type c(b);
		static_assert(c == a);
	"""
	tests_lib.build_program( c_program_text )
