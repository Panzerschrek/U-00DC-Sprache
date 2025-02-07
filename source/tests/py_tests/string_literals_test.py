from py_tests_common import *


def StringLiteral_Test0():
	c_program_text= """
		fn Foo() : char8
		{
			return "eto stroka"[2u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('o') )


def StringLiteralEmpty_Test0():
	c_program_text= """
		auto& s0 = "";
		auto& s1 = ""u8;
		auto& s2 = ""u16;
		auto& s3 = ""u32;
		static_assert( typeinfo</typeof(s0)/>.element_count == 0s );
		static_assert( typeinfo</typeof(s1)/>.element_count == 0s );
		static_assert( typeinfo</typeof(s2)/>.element_count == 0s );
		static_assert( typeinfo</typeof(s3)/>.element_count == 0s );
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralIsArray_Test0():
	c_program_text= """
		template</ type T /> fn AssertArray( T& t )
		{
			halt if( ! typeinfo</ T />.is_array );
		}
		fn Foo()
		{
			AssertArray( "sting literal is array" );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StringLiteralElementIsCharByDefault_Test0():
	c_program_text= """
		fn Foo()
		{
			var char8 c= "abc"[1u];
		}
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralIsConstantExpression_Test0():
	c_program_text= """
		static_assert( "abc"[1u] == "b"[0u] );
		static_assert( "a"[0u] != "b"[0u] );
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralIsConstantExpression_Test1():
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
		fn Foo()
		{
			static_assert( StringEquals( "qwerty", "qwerty" ) );
			static_assert( !StringEquals( "foo", "bar" ) );
			static_assert( !StringEquals( "qwe", "qwert" ) );
		}
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralIsConstantReference_Test0():
	c_program_text= """
		fn Foo()
		{
			"try assign value to string leteral element"[6u]= char8(85);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedReferenceValue" )
	assert( errors_list[0].src_loc.line == 4 )


def StringLiteralIsReferenceToGlobalVariable_Test0():
	c_program_text= """
		fn Foo() : [char8, 4] &
		{
			return "tmHe"; // Ok - return reference to string literal.
		}
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralIsReferenceToGlobalVariable_Test1():
	c_program_text= """
		fn Foo() : char8 &
		{
			return "SomeString"[3]; // Ok - return reference to element of string literal.
		}
	"""
	tests_lib.build_program( c_program_text )



def StringLiteralIsReferenceToGlobalVariable_Test2():
	c_program_text= """
		struct SRef{ [char8, 9] & s; }
		fn Foo() : SRef
		{
			var SRef s_ref{ .s= "eghrhrhrh" };
			return s_ref; // Return reference to string literal inside variable.
		}
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralIsReferenceToGlobalVariable_Test3():
	c_program_text= """
		struct CharRef{ char8 & c; }
		fn Foo() : CharRef
		{
			var CharRef char_ref{ .c= "7777766"[5] };
			return char_ref; // Return reference to element of string literal inside variable.
		}
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralIsReferenceToGlobalVariable_Test4():
	c_program_text= """
		fn Pass( [char8, 5]& s) : [char8, 5]& { return s; }
		fn Foo() : [char8, 5]&
		{
			auto& s= Pass( "FbbnR" ); // return locally-created string literal and create local reference to it.
			return s; // Return reference to string literal.
		}
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralIsReferenceToGlobalVariable_Test5():
	c_program_text= """
		fn GetS() : [char8, 6]& { return "abcdef"; }
		fn Foo() : [char8, 6]&
		{
			auto& s= GetS(); // Obtain string literal and create local reference to it.
			return s; // Return reference to string literal.
		}
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralIsReferenceToGlobalVariable_Test6():
	c_program_text= """
		fn GetChar() : char8& { return "ABC"[1]; }
		fn Foo() : char8&
		{
			auto& c= GetChar(); // Obtain static char reference.
			return c; // Return reference to char.
		}
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralIsReferenceToGlobalVariable_Test7():
	c_program_text= """
		struct CharRef{ char8& c; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn LinkReferences( CharRef &mut cr, [ char8, 4 ] & s ) @(pollution);
		fn Foo() : CharRef
		{
			var CharRef mut char_ref{ .c= "Lol"[1] };
			LinkReferences( char_ref, "SPQR" );
			return char_ref; // Return reference to string literal after references pollution.
		}
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralIsReferenceToGlobalVariable_Test8():
	c_program_text= """
		struct CharRef{ char8& c; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn LinkReferences( CharRef &mut cr, [ char8, 3 ] & s ) @(pollution);
		fn Foo( CharRef &mut cr )
		{
			LinkReferences( cr, "WTF" ); // Add a reference to string literal into function argument inner reference.
		}
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralIsNotNullTerminated_Test0():
	c_program_text= """
		template</ type T, size_type S /> fn constexpr ArraySize( [ T, S ]& arr ) : size_type {  return S;  }
		auto &constexpr str= "abc";
		static_assert( str[ ArraySize(str) - size_type(1) ] == "c"[0u] );
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralSuffix_Test0():
	c_program_text= """
		fn Foo()
		{
			auto& str= "str8"u8;
			var char8 c= str[1u];
		}
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralSuffix_Test1():
	c_program_text= """
		fn Foo()
		{
			auto& str= "str16"u16;
			var char16 c= str[1u];
		}
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralSuffix_Test2():
	c_program_text= """
		fn Foo()
		{
			auto& str= "str32"u32;
			var char32 c= str[1u];
		}
	"""
	tests_lib.build_program( c_program_text )


def StringLiteralSuffix_Test3():
	c_program_text= """
		fn Foo()
		{
			auto& str= "str8";  // Empty suffix means char8 string.
			var char8 c= str[1u];
		}
	"""
	tests_lib.build_program( c_program_text )


def UnknownStringLiteralSuffix_Test0():
	c_program_text= """
		fn Foo()
		{
			"str"fff;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnknownStringLiteralSuffix" )
	assert( errors_list[0].src_loc.line == 4 )


def UnknownStringLiteralSuffix_Test1():
	c_program_text= """
		fn Foo()
		{
			"str"a;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnknownStringLiteralSuffix" )
	assert( errors_list[0].src_loc.line == 4 )


def UnknownStringLiteralSuffix_Test2():
	c_program_text= """
		fn Foo()
		{
			"str"u64;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnknownStringLiteralSuffix" )
	assert( errors_list[0].src_loc.line == 4 )


def StringLiteral_UTF8_Test0():
	c_program_text= """
		template</ type T, size_type S /> fn constexpr ArraySize( [ T, S ]& arr ) : size_type {  return S;  }
		static_assert( ArraySize( "строка" ) == size_type( 6 * 2 ) ); // Each cyrillic letter converted into 2-bytes symbol.
		static_assert( ArraySize( "string" ) == size_type( 6 ) ); // Here all letter are ASCII, so, size of each letter is 1.
		static_assert( ArraySize( "ღთႭა" ) == size_type( 4 * 3 ) ); // Georgian letters have size 3.
		static_assert( ArraySize( "☭" ) == 3s );
		static_assert( ArraySize( "😀" ) == 4s );
	"""
	tests_lib.build_program( c_program_text )


def StringLiteral_UTF16_Test0():
	c_program_text= """
		template</ type T, size_type S /> fn constexpr ArraySize( [ T, S ]& arr ) : size_type {  return S;  }
		static_assert( ArraySize( "строка"u16 ) == size_type( 6 ) );
		static_assert( ArraySize( "string"u16 ) == size_type( 6 ) );
		static_assert( ArraySize( "ღთႭა"u16 ) == size_type( 4 ) );
		static_assert( ArraySize( "😀"u16 ) == 2s ); // This symbol uses surrogate pair in UTF-16 representation.
	"""
	tests_lib.build_program( c_program_text )


def StringLiteral_UTF32_Test0():
	c_program_text= """
		template</ type T, size_type S /> fn constexpr ArraySize( [ T, S ]& arr ) : size_type {  return S;  }
		static_assert( ArraySize( "😀"u32 ) == 1s );
		static_assert( '😀'c32 == char32(0x1F600) );
	"""
	tests_lib.build_program( c_program_text )


def StringLiteral_EscapeSequences_Test0():
	c_program_text= """
		fn Foo() : char8
		{
			return "\\n"[0u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('\n') )


def StringLiteral_EscapeSequences_Test1():
	c_program_text= """
		fn Foo() : char8
		{
			return "\\\\"[0u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('\\') )


def StringLiteral_EscapeSequences_Test2():
	c_program_text= """
		fn Foo() : char8
		{
			return "\\""[0u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('"') )


def StringLiteral_EscapeSequences_Test3():
	c_program_text= """
		fn Foo() : char8
		{
			return "\\0"[0u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0 )


def StringLiteral_EscapeSequences_Test4():
	c_program_text= """
		fn Foo() : char8
		{
			return "\\t"[0u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('\t') )


def StringLiteral_EscapeSequences_Test5():
	c_program_text= """
		fn Foo() : char8
		{
			return "\\r"[0u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('\r') )


def StringLiteral_EscapeSequences_Test6():
	c_program_text= """
		fn Foo() : char8
		{
			return "\\f"[0u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('\f') )


def StringLiteral_EscapeSequences_Test7():
	c_program_text= """
		fn Foo() : char8
		{
			return "\\b"[0u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('\b') )


def StringLiteral_EscapeSequences_Test8():
	c_program_text= """
		fn Foo() : char8
		{
			return "\\'"[0u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('\'') )


def StringLiteral_CharNumber_Test0():
	c_program_text= """
		fn Foo() : char16
		{
			return "\\u00DC"u16[0u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('Ü') )


def StringLiteral_CharNumber_Test1():
	c_program_text= """
		fn Foo() : char16
		{
			return "\\u0565"u16[0u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('ե') )
