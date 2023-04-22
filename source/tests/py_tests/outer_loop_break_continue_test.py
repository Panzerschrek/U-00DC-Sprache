from py_tests_common import *


def LoopLabelDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			while( true ) label something
			{
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def LoopLabelDeclaration_Test1():
	c_program_text= """
		fn Foo( tup[f32, i32]& t )
		{
			for( el : t ) label something
			{
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def LoopLabelDeclaration_Test2():
	c_program_text= """
		fn Foo()
		{
			for( auto mut i= 0; i < 16; ++i ) label something
			{
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def BreakOperatorWithLabel_Test0():
	c_program_text= """
		fn Foo()
		{
			while( true ) label some
			{
				break label some;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def BreakOperatorWithLabel_Test1():
	c_program_text= """
		fn Foo()
		{
			while( true ) label some
			{
				for( auto mut i= 0; i < 16; ++i ) label another
				{
					break label another;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def ContinueOperatorWithLabel_Test0():
	c_program_text= """
		fn Foo()
		{
			while( true ) label some
			{
				continue label some;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def ContinueOperatorWithLabel_Test1():
	c_program_text= """
		fn Foo()
		{
			while( true ) label some
			{
				for( auto mut i= 0; i < 16; ++i ) label another
				{
					continue label another;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def OuterLoopBreak_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 mut x= 0;
			while( true ) label outer
			{
				while( true )
				{
					x+= 34;
					break label outer;
				}
			}
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 34 )


def OuterLoopBreak_Test1():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 mut x= 0;
			for( auto mut i= 0; i < 3; ++i )
			{
				while( true ) label outer
				{
					while( true )
					{
						x+= 17;
						break label outer;
					}
				}
				x *= 2;
			}
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 2 * ( 17 + ( 2 * ( 17 + ( 2 * 17 ) ) ) ) )


def OuterLoopContinue_Test0():
	c_program_text= """
		fn GetNumPrimes(u32 up_to) : u32
		{
			auto mut num_primes= 0u;
			for( auto mut i= 1u; i < up_to; ++i ) label primes_search
			{
				for( auto mut j= 2u; j < i; ++j ) label check_div
				{
					if( i % j == 0u )
					{
						continue label primes_search;
					}
				}
				++num_primes;
			}
			return num_primes;
		}
	"""
	tests_lib.build_program( c_program_text )
	# There are 26 prime numbers less than 100 (including 1 ).
	assert( tests_lib.run_function( "_Z12GetNumPrimesj", 100 ) == 26 )
	# There are 169 prime numbers less than 1000 (including 1 ).
	assert( tests_lib.run_function( "_Z12GetNumPrimesj", 1000 ) == 169 )
