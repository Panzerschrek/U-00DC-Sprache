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
