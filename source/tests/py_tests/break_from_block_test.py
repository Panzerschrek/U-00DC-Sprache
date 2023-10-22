from py_tests_common import *


def BlockLabelDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			{
			} label some
		}
	"""
	tests_lib.build_program( c_program_text )


def BlockLabelDeclaration_Test1():
	c_program_text= """
		fn Foo()
		{
			unsafe
			{
			} label some
		}
	"""
	tests_lib.build_program( c_program_text )


def BlockLabelDeclaration_Test2():
	c_program_text= """
		fn Foo()
		{
			safe
			{
			} label some
		}
	"""
	tests_lib.build_program( c_program_text )


def BlockLabelDeclaration_Test3():
	c_program_text= """
		fn Foo()
		{
			if( true )
			{
				while( false )
				{
					{
					} label lll
				}
			}
			else
			{
				{
				} label some
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def BreakFromBlock_Test0():
	c_program_text= """
		fn Foo()
		{
			{
				var bool cond= true;
				if( cond )
				{
					break label block_end;
				}
				halt; // Is unreachable
			} label block_end
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def BreakFromBlock_Test1():
	c_program_text= """
		fn Foo()
		{
			{
				auto mut breaked= false;
				for( auto mut i= 0; i < 100; ++i ) label outer_loop
				{
					while(true)
					{
						{
							if( i == 13 )
							{
								breaked= true;
								break label outer_block;
							}
							else
							{
								break; // Break from loop, not from labeled block.
							}
						} label inner_block
					}
					halt if( i > 66 );
				}
				halt if( !breaked );
			} label outer_block
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def BreakFromBlock_Test2():
	c_program_text= """
		fn Foo()
		{
			{
				return;
			} label block_end // This label is unreachable
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def BreakFromBlock_Test3():
	c_program_text= """
		fn Foo()
		{
			{
			} label block_end // This label is reachable only via normal control flow - without any "break".
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def BreakFromBlock_Test4():
	c_program_text= """
		fn Foo()
		{
			auto mut iterations= 0s;
			for( auto mut i= 0; i < 10; ++i )
			{
				++iterations;
				{
					if( i == 3 )
					{
						break; // break outside loop, not outside labeled block.
					}
				} label some
				halt if( i >= 3 );
			}
			halt if( iterations != 4s );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def BreakFromBlock_Test5():
	c_program_text= """
		fn Foo()
		{
			auto mut iterations= 0s;
			for( auto mut i= 0; i < 10; ++i )
			{
				++iterations;
				{
					if( i == 3 )
					{
						continue; // continue to loop, not to labeled block.
					}
				} label some
				halt if( i == 3 );
			}
			halt if( iterations != 10s );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def BreakFromBlock_Test6():
	c_program_text= """
		fn Foo( bool c0, bool c1 ) : i32
		{
			var i32 mut i= 0;
			{
				if( c0 )
				{
					if( c1 )
					{
						i= 66;
						// Break and skip the rest of the block.
						// Doing this we can emulate multiple-condition "if" with single "else" branch.
						break label some;
					}
				}

				// Alternative code for cases where at least one of conditions is false.
				i = 44;
			} label some
			return i;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foobb", False, False ) == 44 )
	assert( tests_lib.run_function( "_Z3Foobb", False, True ) == 44 )
	assert( tests_lib.run_function( "_Z3Foobb", True, False ) == 44 )
	assert( tests_lib.run_function( "_Z3Foobb", True, True ) == 66 )


def BreakOutiseLoop_FroBlock_Test0():
	c_program_text= """
		fn Foo()
		{
			{
				break; // break without any frame
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "BreakOutsideLoop", 5 ) )


def BreakOutiseLoop_FroBlock_Test1():
	c_program_text= """
		fn Foo()
		{
			{
				break; // break without label is processed as break outside loop, not block. And here there is no any loop.
			} label some
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "BreakOutsideLoop", 5 ) )


def ContinueOutiseLoop_FroBlock_Test0():
	c_program_text= """
		fn Foo()
		{
			{
				continue;
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ContinueOutsideLoop", 5 ) )


def ContinueOutiseLoop_FroBlock_Test1():
	c_program_text= """
		fn Foo()
		{
			{
				continue; // "continue" works only for loops, not blocks.
			} label some
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ContinueOutsideLoop", 5 ) )


def ContinueForBlock_Test0():
	c_program_text= """
		fn Foo()
		{
			{
				continue label some; // Can't continue to block label.
			} label some
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ContinueForBlock", 5 ) )


def UnreachableCode_ForBreakFromBlock_Test0():
	c_program_text= """
		fn Foo()
		{
			{
				break label some;
				auto x= 0;
			} label some
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnreachableCode", 6 ) )


def UnreachableCode_ForBreakFromBlock_Test1():
	c_program_text= """
		fn Foo()
		{
			{
				{
					break label outer_block;
				} label inner_block
				auto x= 0;
			} label outer_block
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnreachableCode", 8 ) )


def UnreachableCode_ForBreakFromBlock_Test2():
	c_program_text= """
		fn Foo()
		{
			{
				return;
			} label some
			return; // This "return" is unreachable, since previos block cotnains terminal operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnreachableCode", 7 ) )


def BreakFromBlock_VariablesStateMerge_Test0():
	c_program_text= """
		fn Foo(bool cond )
		{
			var i32 mut x= 0;
			{
				if( cond )
				{
					move(x);
					break label some;
				}
			} label some
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ConditionalMove", 11 ) )


def BreakFromBlock_VariablesStateMerge_Test1():
	c_program_text= """
		fn Foo(bool cond )
		{
			var i32 mut x= 0;
			{
				if( cond )
				{
					break label some;
				}
				move(x);
			} label some
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ConditionalMove", 11 ) )


def BreakFromBlock_VariablesStateMerge_Test2():
	c_program_text= """
		fn Foo(bool cond )
		{
			var i32 mut x= 0;
			{
				if( cond )
				{
					move(x);
					break label some;
				}
				// "x" in this branch is not moved yet.
				++x;
				move(x);
			} label some // Ok - move "x" in all branches.
		}
	"""
	tests_lib.build_program( c_program_text )


def BreakFromBlock_VariablesStateMerge_Test3():
	c_program_text= """
		fn Foo(bool cond )
		{
			var i32 mut x= 0;
			{
				if( cond )
				{
					move(x);
					break label some;
				}
				// "x" in this branch is not moved yet.
				++x;
				move(x);
				return;
			} label some // Ok - move "x" in all branches. Branch with "return" is ignored.
		}
	"""
	tests_lib.build_program( c_program_text )


def BreakFromBlock_VariablesStateMerge_Test4():
	c_program_text= """
		fn Foo(bool cond )
		{
			var i32 mut x= 0;
			{
				if( cond )
				{
					move(x);
					break label some;
				}
				++x; // Ok - can access "x", it is still not moved.
				move(x);
			} label some // "x" is moved after this block.
			++x; // Error, "x" is moved.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HaveError( errors_list, "AccessingMovedVariable", 11 ) )
	assert( HaveError( errors_list, "AccessingMovedVariable", 14 ) )


def BreakFromBlock_VariablesStateMerge_Test5():
	c_program_text= """
		struct S{ i32& x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollution( S &mut s'a', i32 &'b x ) @(pollution);
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };
			{
				{
					if( Bar() )
					{
						DoPollution( s, y );
						break label outer;
					}
				} label inner
				++y; // Here there is still no reference to "y" inside "s".
			} label outer
			++y; // Error - "y" has reference inside "s"
		}
		fn Bar() : bool;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HaveError( errors_list, "ReferenceProtectionError", 17 ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 19 ) )
