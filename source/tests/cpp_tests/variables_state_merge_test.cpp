#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( IfMergeTest0_PollutionAllowedInAllBranches )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b '
		{}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			if( true )
			{
				Link( s, y );
			}
			else
			{
				Link( s, y ); // Ok, pollution does not occurs here, other pollution occurs in different if-else bracnh.
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( IfMergeTest1_PollutionInOneBranchDoesNotAffectOtherBranch )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b '
		{}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			if( true )
			{
				Link( s, y );
			}
			else
			{
				++y; // ok, in this branch 'y' still have no references
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( IfMergeTest2_MutablePollutionSelectedAsResult )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn LinkMut ( S &mut s'a', i32&'b  mut x ) ' a <- b ' {}
		fn LinkImut( S &mut s'a', i32&'b imut x ) ' a <- b ' {}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			if( true ){ LinkMut( s, y ); }
			else if( false ){}
			else { LinkImut( s, y ); }
			// In different branches 'y' links with 's' both as mutable and immutable. Result will be mutable.
			auto &imut r= y; // error, 'y' have mutable reference
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 14u ) );
}

U_TEST( IfMergeTest3_ResultPollutionOccursIfPollutionOccursNotInAllBranches )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b '
		{}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			if( true ){}
			else if( false )
			{
				Link( s, y );
			}
			else {}
			y= 42; // Error, 's' have reference to 'y'
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 16u );
}

U_TEST( IfMergeTest4_BreakingReferenceProtectionInMergeResult0 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b '
		{}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0, mut z= 0;
			var S mut s0{ .x= x }, mut s1{ .x= y };

			if( true )
			{
				Link( s0, z );
			}
			else
			{
				Link( s1, z );
			} // Ok - variable 'z' linked mutable both with 's0' and 's1'.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( IfMergeTest5_BreakingReferenceProtectionInMergeResult1 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 & mut x; }
		struct T { i32 &imut x; }
		fn LinkMut ( S &mut s'a', i32&'b  mut x ) ' a <- b ' {}
		fn LinkImut( T &mut t'a', i32&'b imut x ) ' a <- b ' {}
		fn Foo()
		{
			var i32 mut x= 0, imut y= 0, mut z= 0;
			var S mut s{ .x= x };
			var T mut t{ .x= y };

			if( true )
			{
				LinkMut( s, z );
			}
			else
			{
				LinkImut( t, z );
			} // Ok - variable 'z' have both mutable and immutable references inside 's0' and 's1'.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( IfMergeTest6_ConditionAffectsLowerBranches0 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			if( Link( s, y ) )
			{
				++y; // error, 'y' have reference inside 's'.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 11u );
}

U_TEST( IfMergeTest7_ConditionAffectsLowerBranches1 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			if( Link( s, y ) ) {}
			else
			{
				++y; // error, 'y' have reference inside 's'.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 12u );
}

U_TEST( IfMergeTest8_ConditionAffectsLowerBranches2 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			if( Link( s, y ) ) {}
			++y; // error, 'y' have reference inside 's'.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 10u );
}

U_TEST( IfMergeTest9_ConditionAffectsLowerBranches3 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			if( Link( s, y ) ) {}
			else if( Link( s, y ) ) {}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 10u ) );
}

U_TEST( IfMergeTest10_TerminalBranchesAreIgnored0 )
{
	static const char c_program_text[]=
	R"(
		fn Cond() : bool;
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b '
		{}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			if( Cond() )
			{
				Link( s, y );
				return;
			}
			++y; // Ok, pollution happens only in 'return' branch, so, 'y' have no references
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( IfMergeTest11_TerminalBranchesAreIgnored1 )
{
	static const char c_program_text[]=
	R"(
		fn Cond() : bool;
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b '
		{}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			if( Cond() )
			{
			}
			else
			{
				Link( s, y );
				return;
			}
			++y; // Ok, pollution happens only in 'return' branch, so, 'y' have no references
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( IfMergeTest12_TerminalBranchesAreIgnored2 )
{
	static const char c_program_text[]=
	R"(
		fn Cond() : bool;
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b '
		{}
		fn Foo()
		{
			while( Cond() )
			{
				var i32 mut x= 0, mut y= 0;
				var S mut s{ .x= x };

				if( Cond() )
				{
					Link( s, y );
					break;
				}
				++y; // Ok, pollution happens only in 'break' branch, so, 'y' have no references
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( WhileMergeTest_PollutionInsideLoop0 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' {}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			while(true)
			{
				Link( s, y ); // error, mutable pollution for outer variables does not allowed.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferencePollutionOfOuterLoopVariable, 12u ) );
}

U_TEST( WhileMergeTest_PollutionInsideLoop1 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &imut x; }
		fn Link( S &mut s'a', i32&'b imut x ) ' a <- b ' {}
		fn Foo()
		{
			var i32 imut x= 0, imut y= 0;
			var S mut s{ .x= x };

			while(true)
			{
				Link( s, y ); // error, immutable pollution for outer variables does not allowed.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferencePollutionOfOuterLoopVariable, 12u ) );
}

U_TEST( WhileMergeTest_PollutionInsideLoop2 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Foo()
		{
			var i32 mut x= 0;

			while(true)
			{
				var S mut s{ .x= x }; // ok, pollution of inner loop variable by outer loop variable.
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( WhileMergeTest_PollutionInsideLoop3 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			while( Link( s, y ) ) // error, mutable pollution for outer variables does not allowed.
			{}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferencePollutionOfOuterLoopVariable, 10u ) );
}

U_TEST( WhileMergeTest_PollutionInsideLoop4 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &imut x; }
		fn Link( S &mut s'a', i32&'b imut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 imut x= 0, imut y= 0;
			var S mut s{ .x= x };

			while( Link( s, y ) ) // error, immutable pollution for outer variables does not allowed.
			{}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferencePollutionOfOuterLoopVariable, 10u ) );
}

U_TEST( WhileMergeTest_PollutionInsideLoop5 )
{
	static const char c_program_text[]=
	R"(
		fn Cond() : bool;
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			while( Cond() )
			{
				if( Cond() )
				{
					Link( s, y ); // Ok, do mutable pollution in branch with 'break'
					break;
				}
				if( Cond() )
				{
					Link( s, y ); // Ok, do mutable pollution in branch with 'break'
					return;
				}
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( WhileMergeTest_PollutionInsideLoop6 )
{
	static const char c_program_text[]=
	R"(
		fn Cond() : bool;
		struct S { i32 &imut x; }
		fn Link( S &mut s'a', i32&'b imut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 imut x= 0, imut y= 0;
			var S mut s{ .x= x };

			while( Cond() )
			{
				if( Cond() )
				{
					Link( s, y ); // Ok, do immutable pollution in branch with 'break'
					break;
				}
				if( Cond() )
				{
					Link( s, y ); // Ok, do immutable pollution in branch with 'return'
					return;
				}
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( WhileMergeTest_PollutionInsideLoop7 )
{
	static const char c_program_text[]=
	R"(
		fn Cond() : bool;
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			while( Cond() )
			{
				if( Cond() )
				{
					Link( s, y );
					continue; // error, immutable pollution for outer variables does not allowed in 'continue' branch.
				}
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferencePollutionOfOuterLoopVariable, 17u ) );
}

U_TEST( WhileMergeTest_PollutionInsideLoop8 )
{
	static const char c_program_text[]=
	R"(
		fn Cond() : bool;
		struct S { i32 &imut x; }
		fn Link( S &mut s'a', i32&'b imut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 imut x= 0, imut y= 0;
			var S mut s{ .x= x };

			while( Cond() )
			{
				if( Cond() )
				{
					Link( s, y );
					continue; // error, immutable pollution for outer variables does not allowed in 'continue' branch.
				}
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferencePollutionOfOuterLoopVariable, 17u ) );
}

U_TEST( WhileMergeTest_PollutionInsideLoop9 )
{
	static const char c_program_text[]=
	R"(
		fn Cond() : bool;
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			while( Cond() )
			{
				if( Cond() )
				{
					Link( s, y ); // Ok, do mutable pollution in branch with 'break'
					break;
				}
			}
			++y; // error, 's' have reference to 'y' inside.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 18u ) );
}

U_TEST( WhileMergeTest_PollutionInsideLoop10 )
{
	static const char c_program_text[]=
	R"(
		fn Cond() : bool;
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			while( Cond() )
			{
				if( Cond() )
				{
					Link( s, y ); // Ok, do mutable pollution in branch with 'return'
					return;
				}
			}
			++y; // Pollution in branch with "return" have no effect, so, 'y' have no alive references.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( WhileMergeTest_ReturningUnallowedReference )
{
	// Reference pollution inside loop should affect loop body itself.
	static const char c_program_text[]=
	R"(
		struct S { i32 &imut x; }
		fn Link( S &mut s'a', i32&'b imut x ) ' a <- b ';

		fn Foo( i32&'aa a, i32&'bb b ) : i32&'aa
		{
			var S mut s{ .x=a };

			auto mut i= 0u;
			while( i < 10u )
			{
				if( i == 5u ){ return s.x; } // returning 'b', which is not allowed.
				Link( s, b ); // 's' now contains reference to 'b'
				++i;
			}
			return a;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT(
		HaveError( build_result.errors, CodeBuilderErrorCode::ReturningUnallowedReference, 12u ) ||
		HaveError( build_result.errors, CodeBuilderErrorCode::ReferencePollutionOfOuterLoopVariable, 15u ) );
}

U_TEST( WhileMergeTest_TryMutateVariable )
{
	// Reference pollution inside loop should affect loop body itself.
	static const char c_program_text[]=
	R"(
		struct S { i32 &imut x; }
		fn Link( S &mut s'a', i32&'b imut x ) ' a <- b ';

		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };
			auto mut i= 0u;
			while( i < 100u )
			{
				++y; // Mutate variable. In second iteration of loop we should get error.
				Link( s, y ); // Create imut link to 'y' inside 's'.
				++i;
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT(
		HaveError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 12u ) ||
		HaveError( build_result.errors, CodeBuilderErrorCode::ReferencePollutionOfOuterLoopVariable, 15u ));
}

U_TEST( CStyleForMergeTest_PollutionInsideLoop0 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' {}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			for(;;)
			{
				Link( s, y ); // error, mutable pollution for outer variables does not allowed.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferencePollutionOfOuterLoopVariable, 12u ) );
}

U_TEST( CStyleForMergeTest_PollutionInsideLoop1 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &imut x; }
		fn Link( S &mut s'a', i32&'b imut x ) ' a <- b ' {}
		fn Foo()
		{
			var i32 imut x= 0, imut y= 0;
			var S mut s{ .x= x };

			for(;;)
			{
				Link( s, y ); // error, immutable pollution for outer variables does not allowed.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferencePollutionOfOuterLoopVariable, 12u ) );
}

U_TEST( CStyleForMergeTest_PollutionInsideLoop2 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Foo()
		{
			var i32 mut x= 0;

			for(;;)
			{
				var S mut s{ .x= x }; // ok, pollution of inner loop variable by outer loop variable.
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( CStyleForMergeTest_PollutionInsideLoop3 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' {}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			for(;; Link( s, y ) ) // error, mutable pollution for outer variables does not allowed.
			{
				break;
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferencePollutionOfOuterLoopVariable, 12u ) );
}

U_TEST( CStyleForMergeTest_PollutionInsideLoop4 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &imut x; }
		fn Link( S &mut s'a', i32&'b imut x ) ' a <- b ' {}
		fn Foo()
		{
			var i32 imut x= 0, imut y= 0;
			var S mut s{ .x= x };

			for(;; Link( s, y ) ) // error, immutable pollution for outer variables does not allowed.
			{
				break;
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferencePollutionOfOuterLoopVariable, 12u ) );
}

U_TEST( CStyleForMergeTest_PollutionInsideLoop5 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			for( ; Link( s, y ) ; ) // error, mutable pollution for outer variables does not allowed.
			{
				break;
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferencePollutionOfOuterLoopVariable, 12u ) );
}

U_TEST( CStyleForMergeTest_PollutionInsideLoop6 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &imut x; }
		fn Link( S &mut s'a', i32&'b imut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 imut x= 0, imut y= 0;
			var S mut s{ .x= x };

			for( ; Link( s, y ) ; ) // error, immutable pollution for outer variables does not allowed.
			{
				break;
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferencePollutionOfOuterLoopVariable, 12u ) );
}

U_TEST( CStyleForMergeTest_PollutionInsideLoop7 )
{
	static const char c_program_text[]=
	R"(
		fn Cond() : bool;
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' {}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			for( ;Cond(); )
			{
				if( Cond() )
				{
					Link( s, y ); // Ok, do mutable pollution in branch with 'break'
					break;
				}
				if( Cond() )
				{
					Link( s, y ); // Ok, do mutable pollution in branch with 'return'
					return;
				}
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( CStyleForMergeTest_PollutionInsideLoop8 )
{
	static const char c_program_text[]=
	R"(
		fn Cond() : bool;
		struct S { i32 &imut x; }
		fn Link( S &mut s'a', i32&'b imut x ) ' a <- b ' {}
		fn Foo()
		{
			var i32 imut x= 0, imut y= 0;
			var S mut s{ .x= x };

			for( ;Cond(); )
			{
				if( Cond() )
				{
					Link( s, y ); // Ok, do immutable pollution in branch with 'break'
					break;
				}
				if( Cond() )
				{
					Link( s, y ); // Ok, do immutable pollution in branch with 'return'
					return;
				}
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( CStyleForMergeTest_PollutionInsideLoop9 )
{
	static const char c_program_text[]=
	R"(
		fn Cond() : bool;
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			for( ; Cond() ; )
			{
				if( Cond() )
				{
					Link( s, y ); // Ok, do mutable pollution in branch with 'break'
					break;
				}
			}
			++y; // error, 's' have reference to 'y' inside.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 18u ) );
}

U_TEST( CStyleForMergeTest_PollutionInsideLoop10 )
{
	static const char c_program_text[]=
	R"(
		fn Cond() : bool;
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ' : bool { return false; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			for( ; Cond() ; )
			{
				if( Cond() )
				{
					Link( s, y ); // Ok, do mutable pollution in branch with 'return'
					return;
				}
			}
			++y; // Pollution in branch with "return" have no effect, so, 'y' have no alive references.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( TupleForMegeTest0 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ';
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s {.x= x };
			var tup[ i32, f32 ] t= zero_init;
			for( el : t )
			{
				++y; // Error, 'y' linked to 's' in previous iteration of loop.
				Link( s, y );
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 11u ) );
}

U_TEST( TupleForMegeTest1 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ';
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s {.x= x };
			var tup[ f64 ] t= zero_init;
			for( el : t )
			{
				++y; // Ok, loop have only one iteration, so, 'y' have no references.
				Link( s, y );
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( TupleForMegeTest2 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ';
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s {.x= x };
			var tup[ i32, f32 ] t= zero_init;
			for( el : t )
			{
				++y; // Ok, 'y' have no references, because pollution hapens only in 'break' branch.
				if( u64(el) == 0u64 )
				{
					Link( s, y );
					break;
				}
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( TupleForMegeTest3 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ';
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s {.x= x };
			var tup[ i32, f32 ] t= zero_init;
			for( el : t )
			{
				Link( s, y ); // Accessing variable 'y' which have references since first iteration in second iteration.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 11u ) );
}

U_TEST( TupleForMegeTest4 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ';
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s {.x= x };
			var tup[ i32 ] t= zero_init;
			for( el : t )
			{
				Link( s, y ); // Ok, pollution happens only in single loop iteration.
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( TupleForMegeTest5 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- b ';
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s {.x= x };
			var tup[ ] t= zero_init;
			for( el : t ) // Loop body evaluated 0 times.
			{
				Link( s, y );
			}
			++y; // Ok, loop have no iterations and no pollution happens.
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace

} // namespace U
