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
	assert( errors_list[0].file_pos.line == 4 )
