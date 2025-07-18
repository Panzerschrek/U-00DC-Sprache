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
		static_assert( c == char16(1025) );
	"""
	tests_lib.build_program( c_program_text )


def CharLiteral_Test2():
	c_program_text= """
		// long-form literal
		var char32 constexpr c= 'Ⴅ'char32;
		static_assert( c == char32(4261) );
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


def CharLiteral_EscapeSequences_Test0():
	c_program_text= """
		fn Foo() : char8
		{
			return '\\n';
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('\n') )


def CharLiteral_EscapeSequences_Test1():
	c_program_text= """
		fn Foo() : char8
		{
			return '\\\\';
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('\\') )


def CharLiteral_EscapeSequences_Test2():
	c_program_text= """
		fn Foo() : char8
		{
			return '\\"';
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('"') )


def CharLiteral_EscapeSequences_Test3():
	c_program_text= """
		fn Foo() : char8
		{
			return '\\0';
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0 )


def CharLiteral_EscapeSequences_Test4():
	c_program_text= """
		fn Foo() : char8
		{
			return '\\t';
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('\t') )


def CharLiteral_EscapeSequences_Test5():
	c_program_text= """
		fn Foo() : char8
		{
			return '\\r';
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('\r') )


def CharLiteral_EscapeSequences_Test6():
	c_program_text= """
		fn Foo() : char8
		{
			return '\\f';
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('\f') )


def CharLiteral_EscapeSequences_Test7():
	c_program_text= """
		fn Foo() : char8
		{
			return '\\b';
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('\b') )


def CharLiteral_EscapeSequences_Test8():
	c_program_text= """
		fn Foo() : char8
		{
			return '\\'';
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('\'') )


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


def CharLiteralOverflow_Test2():
	c_program_text= """
		fn Foo()
		{
			'Ü'c8; // Error, c8 literals may represent only symbols with codes 0-127.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CharLiteralOverflow" )
	assert( errors_list[0].src_loc.line == 4 )


def CharLiteralOverflow_Test3():
	c_program_text= """
		fn Foo()
		{
			'😀'c16; // Symbol does not fit into single utf-16 char.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CharLiteralOverflow" )
	assert( errors_list[0].src_loc.line == 4 )


def UnknownCharLiteralSuffix_Test0():
	c_program_text= """
		fn Foo()
		{
			's'fff;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnknownCharLiteralSuffix" )
	assert( errors_list[0].src_loc.line == 4 )


def UnknownCharLiteralSuffix_Test1():
	c_program_text= """
		fn Foo()
		{
			's'a;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnknownCharLiteralSuffix" )
	assert( errors_list[0].src_loc.line == 4 )


def UnknownCharLiteralSuffix_Test2():
	c_program_text= """
		fn Foo()
		{
			's'u64;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnknownCharLiteralSuffix" )
	assert( errors_list[0].src_loc.line == 4 )
