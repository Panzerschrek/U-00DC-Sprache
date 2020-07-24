from py_tests_common import *


def CStyleForOperatorParsing_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut a= 0, mut b= 0;

			for( var i32 mut x= 0; x < 10; ++x ){} // variable declaration
			for( auto mut x= 0; x < 10; ++x ){} // Auto variable declaration
			for( ; false; a= 0 ) {} // Empty variables declaration part
			for( auto mut x= 0; ; x+= 2 ) { if(x > 100 ){ break; } } // empty condition
			for( auto mut x= 0; x < 100; ) { break; } //empty iteration part
			for( auto mut x= 0; x < 100; ++x, --x, x+=2 ) {} // multiple iteration parts
			for(;;){ break; } // all elements are empty
		}
	"""
	tests_lib.build_program( c_program_text )


def CStyleForOperator_Test0():
	c_program_text= """
		fn Foo() : u32
		{
			auto mut end= 0u;
			for( auto mut x= 0u; x < 100u; ++x )
			{
				end= x;
			}
			return end;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 99 )


def CStyleForOperator_Test1():
	# No iteration part
	c_program_text= """
		fn Foo() : u32
		{
			auto mut end= 0u;
			for( auto mut x= 0u; x < 55u; )
			{
				++x;
				end= x;
			}
			return end;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 55 )


def CStyleForOperator_Test2():
	# No variables declaration part
	c_program_text= """
		fn Foo() : u32
		{
			auto mut x= 0u;
			for( ; x < 23u; ++x ) {}
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 23 )


def CStyleForOperator_Test3():
	# No condition - should iterate forevere
	c_program_text= """
		fn Foo() : u32
		{
			for( auto mut x= 0u; ; ++x )
			{
				if( x == 75u ) { return x; }
			}
			halt;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 75 )


def CStyleForOperator_Test4():
	# Multiple variables, multiple statements in iteration part.
	c_program_text= """
		fn Foo()
		{
			for( var i32 mut i= 0, mut j= 0; i < 25; ++i, j+= 2 )
			{
				halt if( i * 2 != j );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def CStleForOperator_BreakTest0():
	c_program_text= """
		fn Foo() : u32
		{
			auto mut x= 0u;
			for( ; x < 1000000u ; ++x )
			{
				if( x == 52u ){ break; } // After "break" "++x" should not be executed
			}
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 52 )


def CStleForOperator_BreakTest1():
	c_program_text= """
		fn Foo() : i32
		{
			auto mut x= 0;
			for( ; x < 100 ; ++x )
			{
				x= 952;
				break;
			}
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 952 )


def CStleForOperator_ContinueTest0():
	c_program_text= """
		fn Foo() : u32
		{
			auto mut end= 0u;
			for( auto mut x= 0u; x < 128u; ++x )
			{
				end= x;
				continue; // Continue should pass control flow to "++x"
			}
			return end;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 127 )


def CStleForOperator_ContinueTest1():
	c_program_text= """
		fn Foo() : u32
		{
			auto mut res= 0u;
			for( auto mut x= 0u; x < 16u; ++x )
			{
				if( (x & 1u) != 0u ){ continue; }
				res+= x;
			}
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 2 + 4 + 6 + 8  + 10 + 12 + 14 )
