from py_tests_common import *


def CharSize_Test0():
	c_program_text= """
		static_assert( typeinfo</char8 />.size_of == size_type(1) );
		static_assert( typeinfo</char16/>.size_of == size_type(2) );
		static_assert( typeinfo</char32/>.size_of == size_type(4) );
	"""
	tests_lib.build_program( c_program_text )


def CharLiteral_Test0():
	c_program_text= """
		static_assert( char8 (     89) ==      89char8 );
		static_assert( char8 (    211) ==     211c8 );
		static_assert( char16(  62547) ==   62547c16 );
		static_assert( char16(     55) ==      55char16 );
		static_assert( char32( 158745) ==  158745c32 );
		static_assert( char32(9587452) == 9587452char32 );
	"""
	tests_lib.build_program( c_program_text )


def CharIsConstructibleFromInt_Test0():
	c_program_text= """
		fn Foo() : char8
		{
			var char8 mut c(95);
			return c;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 95 )


def CharIsConstructibleFromInt_Test1():
	c_program_text= """
		fn Foo() : char16
		{
			var char16 mut c(38565);
			return c;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 38565 )


def CharIsConstructibleFromInt_Test2():
	c_program_text= """
		fn Foo() : char32
		{
			var char32 mut c(67854);
			return c;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 67854 )


def CharIsConstructibleFromChar_Test0():
	c_program_text= """
		fn Foo() : char8
		{
			var char32 mut c(67854);
			return char8(c);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == (67854 & 255) )


def CharIsConstructibleFromChar_Test1():
	c_program_text= """
		fn Foo() : char32
		{
			var char16 mut c(39854);
			return char32(c);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 39854 )


def CharIsConstructibleFromChar_Test2():
	c_program_text= """
		fn Foo() : char8
		{
			var char8 mut c0(185);
			var char8 mut c1(c0);
			return c0;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 185 )


def IntIsConstructibleFromChar_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 c( 'a' );
			return c;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 97 )


def IntIsConstructibleFromChar_Test1():
	c_program_text= """
		fn Foo() : u8
		{
			var u8 c( '⅀'c32 ); // It's truncated.
			return c;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 64 )


def ZeroInitializerForChar_Test0():
	c_program_text= """
		fn Foo()
		{
			var char8 mut c0(0);
			var char8 mut c1= zero_init;
			halt if( c0 != c1 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def CharIsEqualityComparable_Test0():
	c_program_text= """
		static_assert( char8(85) == char8(85) );
		static_assert( char8(85) == char8(85 + 256) );
		static_assert( char8(85) != char8(11) );
		static_assert( char16(856) == char16(856) );
		static_assert( char32(885452) != char32(9856565) );
	"""
	tests_lib.build_program( c_program_text )


def CharIsOrderComparable_Test0():
	c_program_text= """
		static_assert( char8(85) <= char8(85) );
		static_assert( char8(98) > char8(11) );
		static_assert( char16(11542) < char16(38564) );
		static_assert( char32(985568) >= char32(11) );
	"""
	tests_lib.build_program( c_program_text )


def CharIsUnsigned_Test0():
	c_program_text= """
		// must use unsigned compare
		static_assert( char8(120) < char8(130) );
		static_assert( char8(0) < char8(255) );
		static_assert( char16(65500) > char16(30000) );
		static_assert( char32(-1) > char32(0) );
	"""
	tests_lib.build_program( c_program_text )


def CharIsUnsigned_Test1():
	c_program_text= """
		// must use zero extension
		static_assert( char16(char8(240)) == char16(240) );
		static_assert( char32(char16(65535)) == char32(65535) );
	"""
	tests_lib.build_program( c_program_text )


def CharAsTemplateParameter_Test0():
	c_program_text= """
		template</ char8 c /> struct S{ auto constexpr x= 8542 * i32(c); }
		static_assert( S</ char8(85) />::x == 8542 * 85 );
	"""
	tests_lib.build_program( c_program_text )


def CharAsTemplateParameter_Test1():
	c_program_text= """
		template</ char8 c /> struct S{ auto constexpr x= 0; }
		template</ /> class S</ char8(99) /> { auto constexpr x= 666; }
		static_assert( S</ char8(99) />::x == 666 );
	"""
	tests_lib.build_program( c_program_text )


def CharIsNotIntegerType_Test0():
	c_program_text= """
		fn Foo()
		{
			char8(5) + char8(45);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OperationNotSupportedForThisType" )
	assert( errors_list[0].src_loc.line == 4 )


def CharIsNotIntegerType_Test1():
	c_program_text= """
		fn Foo()
		{
			char16(55836) - char16(45);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OperationNotSupportedForThisType" )
	assert( errors_list[0].src_loc.line == 4 )


def CharIsNotIntegerType_Test2():
	c_program_text= """
		fn Foo()
		{
			char32(55836) / char32(154);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OperationNotSupportedForThisType" )
	assert( errors_list[0].src_loc.line == 4 )


def CharIsNotIntegerType_Test3():
	c_program_text= """
		fn Foo()
		{
			char16(35412) & char16(255);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OperationNotSupportedForThisType" )
	assert( errors_list[0].src_loc.line == 4 )


def CharIsNotIntegerType_Test4():
	c_program_text= """
		fn Foo()
		{
			char8(42) ^ char8(~0);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OperationNotSupportedForThisType" )
	assert( errors_list[0].src_loc.line == 4 )
