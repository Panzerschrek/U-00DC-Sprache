from py_tests_common import *


def SwitchOperatorDeclaration_Test0():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch( x ){} // Empty switch
		}
	"""
	# Build with errors, since empty switch is now useless, because it does not handles all possible values.
	tests_lib.build_program_with_errors( c_program_text )


def SwitchOperatorDeclaration_Test1():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch( x ) // Switch with one value.
			{
				0 -> {},
				default -> {},
			}
			switch( x )
			{
				default -> {},
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
				default -> {},
				0 -> {},
				1 -> { return; },
			}
			switch( x )
			{
				default -> {},
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
				default -> {},
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
				default -> {},
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
				default -> { halt; }
			}
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
				default -> { halt; }
			}
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
				5 -> { return true; },
				default -> { halt; }
			}
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


def SwitchOperatorRange_Test0():
	# Ranges for unsigned integer.
	c_program_text= """
		fn Foo( u32 x ) : u32
		{
			switch( x )
			{
				... 33u -> { return 123u; }, // Range from begin to specific value
				34u -> { return 456u; },
				35u ... 40u -> { return 789u; }, // Full range
				41u ... -> { return 1000u; }, // Range from specific value to end
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Fooj",  0 ) == 123 )
	assert( tests_lib.run_function( "_Z3Fooj",  1 ) == 123 )
	assert( tests_lib.run_function( "_Z3Fooj",  2 ) == 123 )
	assert( tests_lib.run_function( "_Z3Fooj", 10 ) == 123 )
	assert( tests_lib.run_function( "_Z3Fooj", 32 ) == 123 )
	assert( tests_lib.run_function( "_Z3Fooj", 33 ) == 123 )
	assert( tests_lib.run_function( "_Z3Fooj", 34 ) == 456 )
	assert( tests_lib.run_function( "_Z3Fooj", 35 ) == 789 )
	assert( tests_lib.run_function( "_Z3Fooj", 36 ) == 789 )
	assert( tests_lib.run_function( "_Z3Fooj", 37 ) == 789 )
	assert( tests_lib.run_function( "_Z3Fooj", 38 ) == 789 )
	assert( tests_lib.run_function( "_Z3Fooj", 39 ) == 789 )
	assert( tests_lib.run_function( "_Z3Fooj", 40 ) == 789 )
	assert( tests_lib.run_function( "_Z3Fooj", 41 ) == 1000 )
	assert( tests_lib.run_function( "_Z3Fooj", 42 ) == 1000 )
	assert( tests_lib.run_function( "_Z3Fooj", 100 ) == 1000 )
	assert( tests_lib.run_function( "_Z3Fooj", 100500 ) == 1000 )


def SwitchOperatorRange_Test1():
	# Ranges for signed integer.
	c_program_text= """
		fn Foo( i32 x ) : i32
		{
			switch( x )
			{
				... -45 -> { return 1; },
				-44 -> { return 2; },
				-43 ... -2 -> { return 3; },
				-1 ... 5 -> { return 4; },
				6 -> { return 5; },
				7 ... -> { return 6; },
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Fooi", -2147483648 ) == 1 )
	assert( tests_lib.run_function( "_Z3Fooi", -100500 ) == 1 )
	assert( tests_lib.run_function( "_Z3Fooi", -46 ) == 1 )
	assert( tests_lib.run_function( "_Z3Fooi", -45 ) == 1 )
	assert( tests_lib.run_function( "_Z3Fooi", -44 ) == 2 )
	assert( tests_lib.run_function( "_Z3Fooi", -43 ) == 3 )
	assert( tests_lib.run_function( "_Z3Fooi", -42 ) == 3 )
	assert( tests_lib.run_function( "_Z3Fooi", -16 ) == 3 )
	assert( tests_lib.run_function( "_Z3Fooi", -3 ) == 3 )
	assert( tests_lib.run_function( "_Z3Fooi", -2 ) == 3 )
	assert( tests_lib.run_function( "_Z3Fooi", -1 ) == 4 )
	assert( tests_lib.run_function( "_Z3Fooi", 0 ) == 4 )
	assert( tests_lib.run_function( "_Z3Fooi", 1 ) == 4 )
	assert( tests_lib.run_function( "_Z3Fooi", 4 ) == 4 )
	assert( tests_lib.run_function( "_Z3Fooi", 5 ) == 4 )
	assert( tests_lib.run_function( "_Z3Fooi", 6 ) == 5 )
	assert( tests_lib.run_function( "_Z3Fooi", 7 ) == 6 )
	assert( tests_lib.run_function( "_Z3Fooi", 8 ) == 6 )
	assert( tests_lib.run_function( "_Z3Fooi", 100 ) == 6 )
	assert( tests_lib.run_function( "_Z3Fooi", 2147483647 ) == 6 )


def SwitchOperatorRange_Test2():
	# Ranges for char
	c_program_text= """
		fn Foo( char8 x ) : i32
		{
			switch( x )
			{
				"a"c8 ... "z"c8 -> { return 1; },
				"A"c8 ... "Z"c8 -> { return 2; },
				"0"c8 ... "9"c8 -> { return 3; },
				"$"c8 -> { return 4; },
				default -> { return 5; },
				128c8 ... -> { return 6; },
				... 31c8 -> { return 7; },
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Fooc", ord('a') ) == 1 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('b') ) == 1 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('c') ) == 1 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('q') ) == 1 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('x') ) == 1 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('y') ) == 1 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('z') ) == 1 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('A') ) == 2 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('B') ) == 2 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('C') ) == 2 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('S') ) == 2 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('X') ) == 2 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('Y') ) == 2 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('Z') ) == 2 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('0') ) == 3 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('1') ) == 3 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('2') ) == 3 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('3') ) == 3 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('4') ) == 3 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('5') ) == 3 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('6') ) == 3 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('7') ) == 3 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('8') ) == 3 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('9') ) == 3 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('$') ) == 4 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('_') ) == 5 )
	assert( tests_lib.run_function( "_Z3Fooc", ord('*') ) == 5 )
	assert( tests_lib.run_function( "_Z3Fooc", 127 ) == 5 )
	assert( tests_lib.run_function( "_Z3Fooc", 128 ) == 6 )
	assert( tests_lib.run_function( "_Z3Fooc", 129 ) == 6 )
	assert( tests_lib.run_function( "_Z3Fooc", 147 ) == 6 )
	assert( tests_lib.run_function( "_Z3Fooc", 255 ) == 6 )
	assert( tests_lib.run_function( "_Z3Fooc", 0 ) == 7 )
	assert( tests_lib.run_function( "_Z3Fooc", 1 ) == 7 )
	assert( tests_lib.run_function( "_Z3Fooc", 18 ) == 7 )
	assert( tests_lib.run_function( "_Z3Fooc", 30 ) == 7 )
	assert( tests_lib.run_function( "_Z3Fooc", 31 ) == 7 )
	assert( tests_lib.run_function( "_Z3Fooc", 32 ) == 5 )


def SwitchOperatorRange_Test3():
	# Ranges for enum
	c_program_text= """
		fn Foo()
		{
			halt if( ProcessE( E::A ) != 1 );
			halt if( ProcessE( E::B ) != 1 );
			halt if( ProcessE( E::C ) != 1 );
			halt if( ProcessE( E::D ) != 2 );
			halt if( ProcessE( E::E ) != 3 );
			halt if( ProcessE( E::F ) != 3 );
			halt if( ProcessE( E::G ) != 3 );
			halt if( ProcessE( E::H ) != 4 );
			halt if( ProcessE( E::I ) != 5 );
			halt if( ProcessE( E::J ) != 5 );
		}
		fn ProcessE( E e ) : i32
		{
			switch( e )
			{
				... E::C -> { return 1; },
				E::D -> { return 2; },
				E::E ... E::G -> { return 3; },
				E::H -> { return 4; },
				E::I ... -> { return 5; },
			}
		}
		enum E{ A, B, C, D, E, F, G, H, I, J }
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SwitchOperatorRange_Test4():
	# Ranges together with other elements,.
	c_program_text= """
		fn Foo( i32 x ) : i32
		{
			switch( x )
			{
				33, ... -7, 66 ... 78, 999 ... -> { return 777; },
				96 ... 108, 80 -> { return 888; },
				82, 200 ... 300 -> { return 999; },
				default -> { return 1000; },
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Fooi", 33 ) == 777 )
	assert( tests_lib.run_function( "_Z3Fooi", -100 ) == 777 )
	assert( tests_lib.run_function( "_Z3Fooi", -8 ) == 777 )
	assert( tests_lib.run_function( "_Z3Fooi", -7 ) == 777 )
	assert( tests_lib.run_function( "_Z3Fooi", 65 ) == 1000 )
	assert( tests_lib.run_function( "_Z3Fooi", 66 ) == 777 )
	assert( tests_lib.run_function( "_Z3Fooi", 67 ) == 777 )
	assert( tests_lib.run_function( "_Z3Fooi", 70 ) == 777 )
	assert( tests_lib.run_function( "_Z3Fooi", 77 ) == 777 )
	assert( tests_lib.run_function( "_Z3Fooi", 78 ) == 777 )
	assert( tests_lib.run_function( "_Z3Fooi", 79 ) == 1000 )
	assert( tests_lib.run_function( "_Z3Fooi", 999 ) == 777 )
	assert( tests_lib.run_function( "_Z3Fooi", 1000 ) == 777 )
	assert( tests_lib.run_function( "_Z3Fooi", 100500 ) == 777 )
	assert( tests_lib.run_function( "_Z3Fooi", 95 ) == 1000 )
	assert( tests_lib.run_function( "_Z3Fooi", 96 ) == 888 )
	assert( tests_lib.run_function( "_Z3Fooi", 97 ) == 888 )
	assert( tests_lib.run_function( "_Z3Fooi", 101 ) == 888 )
	assert( tests_lib.run_function( "_Z3Fooi", 107 ) == 888 )
	assert( tests_lib.run_function( "_Z3Fooi", 108 ) == 888 )
	assert( tests_lib.run_function( "_Z3Fooi", 109 ) == 1000 )
	assert( tests_lib.run_function( "_Z3Fooi", 82 ) == 999 )
	assert( tests_lib.run_function( "_Z3Fooi", 199 ) == 1000 )
	assert( tests_lib.run_function( "_Z3Fooi", 200 ) == 999 )
	assert( tests_lib.run_function( "_Z3Fooi", 201 ) == 999 )
	assert( tests_lib.run_function( "_Z3Fooi", 234 ) == 999 )
	assert( tests_lib.run_function( "_Z3Fooi", 299 ) == 999 )
	assert( tests_lib.run_function( "_Z3Fooi", 300 ) == 999 )
	assert( tests_lib.run_function( "_Z3Fooi", 301 ) == 1000 )


def VariablesStateMerge_ForSwitchOperator_Test0():
	c_program_text= """
		fn Foo( i32 x )
		{
			var i32 mut a(0);
			switch(x)
			{
				1 -> { move(a); },
				default -> {},
			} // Variable "a" moved not in all branches
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ConditionalMove", 9 ) )


def VariablesStateMerge_ForSwitchOperator_Test1():
	c_program_text= """
		fn Foo( u32 x )
		{
			var i32 mut a(0);
			switch(x)
			{
				0u -> {},
				1u ... -> { move(a); },
			} // Variable "a" moved not in all branches
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ConditionalMove", 9 ) )



def VariablesStateMerge_ForSwitchOperator_Test2():
	c_program_text= """
		fn Foo( u32 x )
		{
			var i32 mut a(0);
			switch(x)
			{
				0u -> { move(a); },
				1u ... -> { move(a); },
			} // Ok - move "a" in all branches.
		}
	"""
	tests_lib.build_program( c_program_text )


def VariablesStateMerge_ForSwitchOperator_Test3():
	c_program_text= """
		fn Foo( u32 x )
		{
			var i32 mut a(0);
			switch(x)
			{
				0u -> { move(a); },
				1u -> { move(a); },
				default -> { move(a); },
			} // Ok - move "a" in all branches, including default branch.
		}
	"""
	tests_lib.build_program( c_program_text )


def VariablesStateMerge_ForSwitchOperator_Test4():
	c_program_text= """
		fn Foo( u32 x )
		{
			var i32 mut a(0);
			switch(x)
			{
				0u -> { move(a); },
				default -> { return; },
			} // Ok - move "a" in all branches, terminal branches are ignored.
		}
	"""
	tests_lib.build_program( c_program_text )


def VariablesStateMerge_ForSwitchOperator_Test5():
	c_program_text= """
		struct S{ i32 &mut x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollution( S &mut s'a', i32 &'b mut x ) @(pollution);
		fn Foo( i32 a )
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };
			switch(a)
			{
				0 -> {  DoPollution( s, y ); },
				default -> {},
			} // Merge here variables state. In one of the branches "y" was written into "s".
			++y; // Error, "y" has reference inside "s".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 14 ) )
