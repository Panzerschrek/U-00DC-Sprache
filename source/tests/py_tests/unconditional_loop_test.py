from py_tests_common import *


def UnconditionalLoop_Test0():
	c_program_text= """
		fn Foo()
		{
			loop{}
		}
	"""
	tests_lib.build_program( c_program_text )


def UnconditionalLoop_Test1():
	c_program_text= """
		fn Foo()
		{
			loop
			{
				break;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def UnconditionalLoop_Test2():
	c_program_text= """
		fn Foo()
		{
			loop
			{
				continue;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def UnconditionalLoop_Test3():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 mut x= 0;
			loop
			{
				++x;
				if( x >= 10 )
				{
					break;
				}
			}
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 10 )


def UnconditionalLoop_Test4():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 mut x= 0;
			loop
			{
				++x;
				if( x >= 10 )
				{
					return x;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 10 )


def UnconditionalLoop_Test5():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 mut x= 0;
			loop label some
			{
				++x;
				if( x >= 33 )
				{
					break label some;
				}
			}
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 33 )


def UnconditionalLoop_Test6():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 mut x= 0;
			loop label some
			{
				for( auto mut i= 0s; i < 10s; ++i )
				{
					++x;
					if( x >= 47 )
					{
						break label some;
					}
				}
			}
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 47 )


def UnconditionalLoop_Test7():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 mut x= 0;
			loop label some
			{
				if( x >= 24 )
				{
					break;
				}
				for( auto mut i= 0s; i < 7s; ++i )
				{
					++x;
					if( x >= 26 )
					{
						continue label some;
					}
				}
			}
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 26 )


def UnreachableCode_ForUnconditionalLoop_Test0():
	c_program_text= """
		fn Foo()
		{
			loop{} // This loop never breaks.
			var i32 x= 0;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnreachableCode", 5 ) )


def UnreachableCode_ForUnconditionalLoop_Test1():
	c_program_text= """
		fn Foo()
		{
			loop // This loop never breaks, only returns.
			{
				if( Bar() ) { return; }
			}
			var i32 x= 0;
		}
		fn Bar() : bool;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnreachableCode", 8 ) )


def UnreachableCode_ForUnconditionalLoop_Test2():
	c_program_text= """
		fn Foo()
		{
			loop label outer
			{
				loop label inner
				{
					if( Bar() ){ break label outer; }
				}
				var i32 x = 0; // This code is unreachable, because inner loop contains only break to outer loop.
			}
		}
		fn Bar() : bool;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnreachableCode", 10 ) )


def VariablesStateMerge_ForUnconditionalLoop_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			loop
			{
				if( Bar() )
				{
					move(x);
					break;
				}
				else
				{
					break;
				}
			} // Variable "x" moved not in all "break" branches.
		}
		fn Bar() : bool;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ConditionalMove", 16 ) )


def VariablesStateMerge_ForUnconditionalLoop_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			loop
			{
				if( Bar() )
				{
					move(x);
					break;
				}
			}
			++x; // Error, "x" here is moved.
		}
		fn Bar() : bool;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HaveError( errors_list, "ConditionalMove", 12 ) )
	assert( HaveError( errors_list, "AccessingMovedVariable", 13 ) )


def VariablesStateMerge_ForUnconditionalLoop_Test2():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			loop
			{
				if( Bar() )
				{
					move(x);
					return;
				}
				if( Baz() )
				{
					break;
				}
			}
			++x; // Ok, "x" was moved only in "return" branch, but not in any "break" branch.
		}
		fn Bar() : bool;
		fn Baz() : bool;
	"""
	tests_lib.build_program( c_program_text )


def VariablesStateMerge_ForUnconditionalLoop_Test3():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			loop
			{
				if( Bar() )
				{
					move(x);
					break;
				}
				else if( Baz() )
				{
					move(x);
					break;
				}
				else
				{
					return;
				}
			} // Ok - variable "x" moved an all (2) "break" branches.
		}
		fn Bar() : bool;
		fn Baz() : bool;
	"""
	tests_lib.build_program( c_program_text )


def VariablesStateMerge_ForUnconditionalLoop_Test4():
	c_program_text= """
		struct S{ i32& x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( S& mut s, i32 & x ) @(pollution);
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };
			loop
			{
				if( Bar() )
				{
					MakePollution( s, y );
					break;
				}
				else
				{
					break;
				}
			}
			++y; // Error, "y" has references inside "s".
		}
		fn Bar() : bool;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HaveError( errors_list, "ReferenceProtectionError", 20 ) )


def OuterVariableMoveInsideLoop_ForUnconditionalLoop_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			loop
			{
				move(x);
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "OuterVariableMoveInsideLoop", 8 ) )


def OuterVariableMoveInsideLoop_ForUnconditionalLoop_Test1():
	c_program_text= """
		fn Foo(i32 mut x)
		{
			loop
			{
				if( Bar() )
				{
					move(x);
				}
				else
				{
					break;
				}
			}
		}
		fn Bar() : bool;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "OuterVariableMoveInsideLoop", 14 ) )


def OuterVariableMoveInsideLoop_ForUnconditionalLoop_Test2():
	c_program_text= """
		fn Foo(i32 mut x)
		{
			loop
			{
				if( Bar() )
				{
					move(x);
					continue;
				}
				else
				{
					break;
				}
			}
		}
		fn Bar() : bool;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "OuterVariableMoveInsideLoop", 15 ) )


def OuterVariableMoveInsideLoop_ForUnconditionalLoop_Test3():
	c_program_text= """
		fn Foo(i32 mut x)
		{
			loop
			{
				if( Bar() )
				{
					move(x); // Ok - outer variable is moved only in "break" branch.
					break;
				}
			}
		}
		fn Bar() : bool;
	"""
	tests_lib.build_program( c_program_text )
