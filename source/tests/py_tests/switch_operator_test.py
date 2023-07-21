from py_tests_common import *


def SwitchOperatorDeclaration_Test0():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch( x ){} // Empty switch
		}
	"""
	tests_lib.build_program( c_program_text )


def SwitchOperatorDeclaration_Test1():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch( x ) // Switch with one value.
			{
				0 -> {},
			}
			switch( x )
			{
				1 -> {} // Last comma is unnecessary.
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def SwitchOperatorDeclaration_Test2():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch( x ) // Switch with two values.
			{
				0 -> {},
				1 -> { return; },
			}
			switch( x )
			{
				2 -> {},
				3 -> {} // Last comma is unnecessary.
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def SwitchOperator_Test0():
	# Switch for signed integer with terminal blocks.
	c_program_text= """
		fn Foo( i32 x ) : i32
		{
			switch( x )
			{
				1 -> { return 1; },
				2 -> { return 22; },
				3 -> { return 333; },
				4 -> { return 4444; },
				5 -> { return 55555; },
				6 -> { return 666666; },
			}

			return 0;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Fooi", 0 ) == 0 )
	assert( tests_lib.run_function( "_Z3Fooi", 1 ) == 1 )
	assert( tests_lib.run_function( "_Z3Fooi", 2 ) == 22 )
	assert( tests_lib.run_function( "_Z3Fooi", 3 ) == 333 )
	assert( tests_lib.run_function( "_Z3Fooi", 4 ) == 4444 )
	assert( tests_lib.run_function( "_Z3Fooi", 5 ) == 55555 )
	assert( tests_lib.run_function( "_Z3Fooi", 6 ) == 666666 )
	assert( tests_lib.run_function( "_Z3Fooi", 7 ) == 0 )
	assert( tests_lib.run_function( "_Z3Fooi", -25 ) == 0 )


def SwitchOperator_Test1():
	# Switch for signed integer with non-terminal blocks.
	c_program_text= """
		fn Foo( i32 x ) : u32
		{
			var u32 mut res= 0u;
			switch( x )
			{
				// Order of cases is practically insignificant.
				6 -> { res= 666666u; },
				5 -> { res= 55555u; },
				4 -> { res= 4444u; },
				3 -> { res= 333u; },
				2 -> { res= 22u; },
				1 -> { res= 1u; },
			}

			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Fooi", 0 ) == 0 )
	assert( tests_lib.run_function( "_Z3Fooi", 1 ) == 1 )
	assert( tests_lib.run_function( "_Z3Fooi", 2 ) == 22 )
	assert( tests_lib.run_function( "_Z3Fooi", 3 ) == 333 )
	assert( tests_lib.run_function( "_Z3Fooi", 4 ) == 4444 )
	assert( tests_lib.run_function( "_Z3Fooi", 5 ) == 55555 )
	assert( tests_lib.run_function( "_Z3Fooi", 6 ) == 666666 )
	assert( tests_lib.run_function( "_Z3Fooi", 7 ) == 0 )
	assert( tests_lib.run_function( "_Z3Fooi", -25 ) == 0 )


def SwitchOperator_Test2():
	# Switch for char.
	c_program_text= """
		fn Foo()
		{
			halt if( Capitalize( "d"c8 ) != "D"c8 );
			halt if( Capitalize( "c"c8 ) != "C"c8 );
			halt if( Capitalize( "b"c8 ) != "B"c8 );
			halt if( Capitalize( "a"c8 ) != "A"c8 );
		}
		fn Capitalize(char8 c ) : char8
		{
			switch( c )
			{
				"a"c8 -> { return "A"c8; },
				"b"c8 -> { return "B"c8; },
				"c"c8 -> { return "C"c8; },
				"d"c8 -> { return "D"c8; },
			}
			halt;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SwitchOperator_Test3():
	# Switch for enum.
	c_program_text= """
		fn Foo()
		{
			halt if( Reverse( E::A ) != E::C );
			halt if( Reverse( E::B ) != E::B );
			halt if( Reverse( E::C ) != E::A );
		}
		enum E{ A, B, C }
		fn Reverse( E e ) : E
		{
			switch( e )
			{
				E::A -> { return E::C; },
				E::B -> { return E::B; },
				E::C -> { return E::A; },
			}
			halt;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SwitchOperator_MultipleCaseValues_Test0():
	# A couple of cases with multiple values.
	c_program_text= """
		fn Even( i32 x ) : bool
		{
			switch( x )
			{
				0, 2, 4, 6, 8, 10, 12, 14 -> { return  true; },
				1, 3, 5, 7, 9, 11, 13, 15 -> { return false; },
			}

			halt;
		}
	"""
	tests_lib.build_program( c_program_text )
	for i in range(0, 16):
		assert( tests_lib.run_function( "_Z4Eveni", i ) == (i % 2 == 0) )


def SwitchOperator_MultipleCaseValues_Test1():
	# Single case with multiple values and one case with single value.
	c_program_text= """
		fn IsFive( i32 x ) : bool
		{
			switch( x )
			{
				0, 1, 2, 3, 4, 6, 7, 8, 9 -> {  return false; },
				5 -> { return true; }
			}

			halt;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z6IsFivei", 0 ) == False )
	assert( tests_lib.run_function( "_Z6IsFivei", 1 ) == False )
	assert( tests_lib.run_function( "_Z6IsFivei", 2 ) == False )
	assert( tests_lib.run_function( "_Z6IsFivei", 3 ) == False )
	assert( tests_lib.run_function( "_Z6IsFivei", 4 ) == False )
	assert( tests_lib.run_function( "_Z6IsFivei", 5 ) == True )
	assert( tests_lib.run_function( "_Z6IsFivei", 6 ) == False )
	assert( tests_lib.run_function( "_Z6IsFivei", 7 ) == False )
	assert( tests_lib.run_function( "_Z6IsFivei", 8 ) == False )
	assert( tests_lib.run_function( "_Z6IsFivei", 9 ) == False )


def SwithOperator_DefaultBranch_Test0():
	c_program_text= """
		fn IsTen( i32 x ) : bool
		{
			switch( x )
			{
				10 -> { return true; },
				default -> { return false; }
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z5IsTeni",  0 ) == False )
	assert( tests_lib.run_function( "_Z5IsTeni",  7 ) == False )
	assert( tests_lib.run_function( "_Z5IsTeni", 10 ) == True )
	assert( tests_lib.run_function( "_Z5IsTeni", 16 ) == False )


def SwithOperator_DefaultBranch_Test1():
	c_program_text= """
		fn IsPowerOfTwo( i32 x ) : bool
		{
			switch( x )
			{
				default -> { return false; }, // It is possible to use default branch as first branch.
				1, 2, 4, 8, 16, 32, 64 -> { return true; },
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z12IsPowerOfTwoi",  0 ) == False )
	assert( tests_lib.run_function( "_Z12IsPowerOfTwoi",  1 ) == True )
	assert( tests_lib.run_function( "_Z12IsPowerOfTwoi",  2 ) == True )
	assert( tests_lib.run_function( "_Z12IsPowerOfTwoi",  3 ) == False )
	assert( tests_lib.run_function( "_Z12IsPowerOfTwoi",  4 ) == True )
	assert( tests_lib.run_function( "_Z12IsPowerOfTwoi",  5 ) == False )
	assert( tests_lib.run_function( "_Z12IsPowerOfTwoi",  6 ) == False )
	assert( tests_lib.run_function( "_Z12IsPowerOfTwoi",  7 ) == False )
	assert( tests_lib.run_function( "_Z12IsPowerOfTwoi",  8 ) == True )
	assert( tests_lib.run_function( "_Z12IsPowerOfTwoi",  9 ) == False )
