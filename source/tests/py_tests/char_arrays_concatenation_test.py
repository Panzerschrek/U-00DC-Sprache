from py_tests_common import *


def CharArrayConcatenation_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut a= "ryR";
			auto mut b= "7766N-Q";
			auto c= a + b;
			halt if( c != "ryR7766N-Q" );
			auto d= b + a;
			halt if( d != "7766N-QryR" );

			static_assert( typeinfo</typeof(c)/>.element_count == 10s );
			static_assert( typeinfo</typeof(d)/>.element_count == 10s );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def CharArrayConcatenation_Test1():
	c_program_text= """
		fn Foo()
		{
			auto mut a= "";
			auto mut b= "66v";
			var [ char8, 3 ] c= a + b;
			halt if( c != "66v" );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def CharArrayConcatenation_Test2():
	c_program_text= """
		fn Foo()
		{
			auto mut a= "8n-- ";
			auto mut b= "";
			var [ char8, 5 ] c= a + b;
			halt if( c != "8n-- " );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def CharArrayConcatenation_Test3():
	c_program_text= """
		fn Foo()
		{
			auto mut a= "aaaa";
			auto mut b= "bbbb";
			auto ab= a + b; // Sizes are equal.
			halt if( ab != "aaaabbbb" );
			auto ba= b + a;
			halt if( ba != "bbbbaaaa" );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def CharArrayConcatenation_Test4():
	c_program_text= """
		fn Foo()
		{
			// Non-ASCII UTF-8
			auto mut a= "Пын";
			auto mut b= "еходы";
			auto ab= a + b;
			halt if( ab != "Пынеходы" );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def CharArrayConcatenation_Test5():
	c_program_text= """
		fn Foo()
		{
			// Non-ASCII UTF-16
			auto mut a= "Begrü"u16;
			auto mut b= "ßunf"u16;
			auto ab= a + b;
			halt if( ab != "Begrüßunf"u16 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def CharArrayConcatenation_Test6():
	c_program_text= """
		// Non-ASCII UTF-32
		auto a= "Begrüßunf"u32;
		auto b= "приветствие"u32;
		auto ab= a + " - "u32 + b;
		static_assert( ab == "Begrüßunf - приветствие"u32 );
	"""
	tests_lib.build_program( c_program_text )


def CharArrayConcatenationConstexpr_Test0():
	c_program_text= """
		auto a= "ryR";
		auto b= "7766N-Q";
		auto c= a + b;
		static_assert( c == "ryR7766N-Q" );
		auto d= b + a;
		static_assert( d == "7766N-QryR" );
		auto e= a + a;
		static_assert( e == "ryRryR" );
		auto f= c + d;
		static_assert( f == "ryR7766N-Q7766N-QryR" );
	"""
	tests_lib.build_program( c_program_text )


def NoConcatenationForNonCharArrays_Test0():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 5 ] x= zero_init;
			var [ i32, 6 ] y= zero_init;
			auto z= x + y; // No concatenation for arrays of ints
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 6 ) )


def NoConcatenationForNonCharArrays_Test1():
	c_program_text= """
		fn Foo()
		{
			var [ byte8, 5 ] x= zero_init;
			var [ byte8, 6 ] y= zero_init;
			auto z= x + y; // No concatenation for arrays of bytes
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 6 ) )


def NoConcatenationForNonCharArrays_Test2():
	c_program_text= """
		fn Foo()
		{
			var [ bool, 5 ] x= zero_init;
			var [ bool, 6 ] y= zero_init;
			auto z= x + y; // No concatenation for arrays of bools
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 6 ) )


def NoConcatenationForNonCharArrays_Test3():
	c_program_text= """
		fn Foo()
		{
			var [ void, 5 ] x= zero_init;
			var [ void, 6 ] y= zero_init;
			auto z= x + y; // No concatenation for arrays of voids
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 6 ) )


def NoConcatenationForNonCharArrays_Test4():
	c_program_text= """
		fn Foo()
		{
			var [ f64, 5 ] x= zero_init;
			var [ f64, 6 ] y= zero_init;
			auto z= x + y; // No concatenation for arrays of floats
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 6 ) )


def NoConcatenationForNonCharArrays_Test5():
	c_program_text= """
		fn Foo()
		{
			var [ [ char8, 2 ], 5 ] x= zero_init;
			var [ [ char8, 2 ], 6 ] y= zero_init;
			auto z= x + y; // No concatenation for arrays of char pairs
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 6 ) )


def NoConcatenationForNonCharArrays_Test6():
	c_program_text= """
		fn Foo()
		{
			var [ S, 5 ] x= zero_init;
			var [ S, 6 ] y= zero_init;
			auto z= x + y; // No concatenation for arrays of structs
		}
		struct S{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 6 ) )


def NoConcatenationForNonCharArrays_Test7():
	c_program_text= """
		fn Foo()
		{
			var [ char16, 5 ] x= zero_init;
			var [ char32, 6 ] y= zero_init;
			auto z= x + y; // Different char element types
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 6 ) )


def NoConcatenationForNonCharArrays_Test8():
	c_program_text= """
		fn Foo()
		{
			var [ char8, 5 ] x= zero_init;
			var [ u8, 6 ] y= zero_init;
			auto z= x + y; // Array element types are different
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 6 ) )


def NoBinaryOperatorsForCharArraysExceptConcatenation_Test0():
	c_program_text= """
		fn Foo()
		{
			auto x= "Foo" - "Bar";
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def NoBinaryOperatorsForCharArraysExceptConcatenation_Test1():
	c_program_text= """
		fn Foo()
		{
			auto x= "Foo" * "1234";
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 4 ) )


def NoBinaryOperatorsForCharArraysExceptConcatenation_Test2():
	c_program_text= """
		fn Foo()
		{
			auto x= "Foo" | "Lol";
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def NoBinaryOperatorsForCharArraysExceptConcatenation_Test3():
	c_program_text= """
		fn Foo()
		{
			auto x= "Foo" && "Lol";
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def CharArrayConcatenationResultIsValue_Test0():
	c_program_text= """
		fn Foo()
		{
			auto& s= "Foo" + "Bar"; // Result of + is value, but it is used to initialize a reference value.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 4 ) )


def CharArrayConcatenationResultIsValue_Test1():
	c_program_text= """
		fn Foo()
		{
			++( "Foo" + "Bar" ); // Can't increment result of array concatenation.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 4 ) )


def CharArrayConcatenationReferenceProtection_Test0():
	c_program_text= """
		fn Foo() : [ char8, 6 ] &
		{
			return Pass( "Foo" + "Bar" ); // Passing and returning reference to temporary value - result of char arrays concatenation.
		}
		fn Pass( [ char8, 6 ]& s ) : [ char8, 6 ] &;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 4 ) )


def CharArrayConcatenationReferenceProtection_Test1():
	c_program_text= """
		fn Foo( [ char8, 3 ]& x, [ char8, 66 ]& y )
		{
			auto& z= Pass( x + y ); // Passing reference to temporary value - result of char arrays concatenation and binding it to a reference.
		}
		fn Pass( [ char8, 69 ]& s ) : [ char8, 6 ] &;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 4 ) )


def CharArrayConcatenationReferenceProtection_Test2():
	c_program_text= """
		fn Foo( [ char8, 3 ] &mut x )
		{
			auto y= x + ModifyAndRet(x); // Modifying "x" during concatenation operator right part evaluation, while "x" is locked for left part.
		}
		fn ModifyAndRet( [ char8, 3 ] &mut s ) : [ char8, 6 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 4 ) )


def CharArrayConcatenationReferenceProtection_Test3():
	c_program_text= """
		fn Foo( [ char8, 3 ] &mut x )
		{
			auto y= ModifyAndRet(x) + x; // Ok, finish modification of "x" in left part of "+", than use it in right part.
		}
		fn ModifyAndRet( [ char8, 3 ] &mut s ) : [ char8, 6 ];
	"""
	tests_lib.build_program( c_program_text )
