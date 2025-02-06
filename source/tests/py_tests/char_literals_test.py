from py_tests_common import *


def CharLiteral_Test0():
	c_program_text= """
		// short-form literal
		static_assert( "str"[1u] == 't' );
	"""
	tests_lib.build_program( c_program_text )


def CharLiteral_Test1():
	c_program_text= """
		// short-form literal, type is char16
		var char16 constexpr c= 'Ё'c16;
		static_assert( c == 1025c16 );
	"""
	tests_lib.build_program( c_program_text )


def CharLiteral_Test2():
	c_program_text= """
		// long-form literal
		var char32 constexpr c= 'Ⴅ'char32;
		static_assert( c == 4261c32 );
	"""
	tests_lib.build_program( c_program_text )


def CharLiteral_Test3():
	c_program_text= """
		fn constexpr GetCharSize( char8  c ) : i32 { return 1; }
		fn constexpr GetCharSize( char16 c ) : i32 { return 2; }
		fn constexpr GetCharSize( char32 c ) : i32 { return 4; }

		static_assert( GetCharSize( 'R'c8  ) == 1 );
		static_assert( GetCharSize( 'R'char8  ) == 1 );
		static_assert( GetCharSize( 'R'c16 ) == 2 );
		static_assert( GetCharSize( 'R'char16 ) == 2 );
		static_assert( GetCharSize( 'R'c32 ) == 4 );
		static_assert( GetCharSize( 'R'char32 ) == 4 );
	"""
	tests_lib.build_program( c_program_text )


def CharLiteralIsConstantValue_Test0():
	c_program_text= """
		fn Bar( char16 &mut c ) {}
		fn Foo()
		{
			Bar( 'Ö'c16 );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 5 )


def InvalidSizeForCharLiteral_Test0():
	c_program_text= """
		fn Foo()
		{
			"try mupltiple symbols"c8;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidSizeForCharLiteral" )
	assert( errors_list[0].src_loc.line == 4 )


def InvalidSizeForCharLiteral_Test1():
	c_program_text= """
		fn Foo()
		{
			""c16; // zero symbols
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidSizeForCharLiteral" )
	assert( errors_list[0].src_loc.line == 4 )


def InvalidSizeForCharLiteral_Test2():
	c_program_text= """
		fn Foo()
		{
			'Ü'c8; // Error, c8 literals may represent only symbols with codes 0-127.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidSizeForCharLiteral" )
	assert( errors_list[0].src_loc.line == 4 )


def InvalidSizeForCharLiteral_Test3():
	c_program_text= """
		fn Foo()
		{
			'😀'c16; // Symbol does not fit into single utf-16 char.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidSizeForCharLiteral" )
	assert( errors_list[0].src_loc.line == 4 )


def InvalidSizeForCharLiteral_Test4():
	c_program_text= """
		fn Foo()
		{
			"wtf"c32; // Too much symbols
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidSizeForCharLiteral" )
	assert( errors_list[0].src_loc.line == 4 )
