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
