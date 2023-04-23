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


def OuterLoopBreak_Test2():
	c_program_text= """
		fn Foo()
		{
			while(true) label outer
			{
				while(true)
				{
					if( true ) { break label outer; }
					halt; // Should not reach this
				}
				halt; // Should not reach this
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


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


def OuterLoopContinue_Test1():
	c_program_text= """
		fn Foo()
		{
			auto mut total_iterations= 0;
			for( auto mut i= 0; i < 10; ++i ) label outer
			{
				for( auto mut j= 0; j < 20; ++j )
				{
					++total_iterations;
					if( j == 3 )
					{
						continue label outer;
					}
				}
				halt; // Should not reach this
			}
			halt if( total_iterations != 40 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def NameNotFound_ForLabel_Test0():
	c_program_text= """
		fn Foo()
		{
			while(true)
			{
				break label some;
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NameNotFound", 6 ) )


def NameNotFound_ForLabel_Test1():
	c_program_text= """
		var f32 some= 1.0f;
		fn Foo()
		{
			var bool some= false;
			while(true)
			{
				var i32 some= 0;
				break label some; // Only labels names are checked.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NameNotFound", 9 ) )


def NameNotFound_ForLabel_Test2():
	c_program_text= """
		fn Foo()
		{
			for( auto mut i= 0; i < 10; ++i ) label some
			{
				auto x= some; // Labels names in normal context are not visible.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NameNotFound", 6 ) )


def Redefinition_ForLabels_Test0():
	c_program_text= """
		fn Foo()
		{
			while(true) label some
			{
				for( auto mut i= 0; i < 10; ++i ) label some
				{}
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "Redefinition", 6 ) )


def Redefinition_ForLabels_Test1():
	c_program_text= """
		fn Foo( tup[f32, i32]& t )
		{
			for( el : t ) label some
			{
				for( auto mut i= 0; i < 10; ++i ) // unnamed
				{
					while(true) label some
					{}
				}
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "Redefinition", 8 ) )


def Redefinition_ForLabels_Test2():
	c_program_text= """
		var f32 some= 1.0f;
		fn Foo()
		{
			var bool some= false;
			while(true) label some // It is ok to define label with name of a variable, since labels have their own names scope.
			{
				var i32 some= 0;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def VariablesStateMerge_ForBreakContinueToOuterLoop_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			for( auto mut i= 0; i < 10; ++i ) label outer
			{
				for( auto mut j= 0; j < 10; ++j )
				{
					if( Bar() )
					{
						move(x);
						break label outer;
					}
				}
			} // Conditional move error here - "x" moved in only in one "break" branch.
		}
		fn Bar() : bool;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ConditionalMove", 15 ) )


def VariablesStateMerge_ForBreakContinueToOuterLoop_Test1():
	c_program_text= """
		struct S{ i32& x; }
		fn DoPollution( S &mut s'a', i32 &'b x ) ' a <- b ';
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };
			for( auto mut i= 0; i < 10; ++i ) label outer
			{
				for( auto mut j= 0; j < 10; ++j )
				{
					if( Bar() )
					{
						DoPollution( s, y );
						break label outer;
					}
				}
				++y; // Here there is still no reference to "y" inside "s".
			}
			++y; // Error - "y" has reference inside "s"
		}
		fn Bar() : bool;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 20 ) )
	assert( not HaveError( errors_list, "ReferenceProtectionError", 18 ) )
	assert( not HaveError( errors_list, "ReferencePollutionOfOuterLoopVariable", 19 ) )


def VariablesStateMerge_ForBreakContinueToOuterLoop_Test2():
	c_program_text= """
		struct S{ i32& x; }
		fn DoPollution( S &mut s'a', i32 &'b x ) ' a <- b ';
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };
			for( auto mut i= 0; i < 10; ++i ) label outer
			{
				for( auto mut j= 0; j < 10; ++j )
				{
					if( Bar() )
					{
						DoPollution( s, y );
						continue label outer;
					}
				}
			} // Error here - pollution for outer loop variable "s"
		}
		fn Bar() : bool;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferencePollutionOfOuterLoopVariable", 18 ) )


def VariablesStateMerge_ForBreakContinueToOuterLoop_Test3():
	c_program_text= """
		struct S{ i32& x; }
		fn DoPollution( S &mut s'a', i32 &'b x ) ' a <- b ';
		fn Foo()
		{
			for( auto mut i= 0; i < 10; ++i ) label outer
			{
				var i32 mut x= 0, mut y= 0;
				var S mut s{ .x= x };
				for( auto mut j= 0; j < 10; ++j )
				{
					if( Bar() )
					{
						DoPollution( s, y );
						continue label outer; // OK - since this is "continue" to outer loop, error "ReferencePollutionOfOuterLoopVariable" is not generated.
					}
				}
			}
		}
		fn Bar() : bool;
	"""
	tests_lib.build_program( c_program_text )
