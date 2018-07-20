#include "tests.hpp"

namespace U
{

U_TEST( IfMergeTest0_PollutionAllowedInAllBranches )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- mut b '
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
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- mut b '
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
		fn LinkMut ( S &mut s'a', i32&'b  mut x ) ' a <-  mut b ' {}
		fn LinkImut( S &mut s'a', i32&'b imut x ) ' a <- imut b ' {}
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference, 14u ) );
}

U_TEST( IfMergeTest3_ResultPollutionOccursIfPollutionOccursNotInAllBranches )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- mut b '
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 16u );
}

U_TEST( IfMergeTest4_BreakingReferenceProtectionInMergeResult0 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- mut b '
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
			} // Error here. In result variable 'z' linked  mutable both with 's0' and 's1'.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 17u );
}

U_TEST( IfMergeTest5_BreakingReferenceProtectionInMergeResult1 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn LinkMut ( S &mut s'a', i32&'b  mut x ) ' a <-  mut b ' {}
		fn LinkImut( S &mut s'a', i32&'b imut x ) ' a <- imut b ' {}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0, mut z= 0;
			var S mut s0{ .x= x }, mut s1{ .x= y };

			if( true )
			{
				LinkMut( s0, z );
			}
			else
			{
				LinkImut( s1, z );
			} // Error here. In result variable 'z' have both mutable and immutable references inside 's0' and 's1'.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 17u );
}

U_TEST( IfMergeTest6_ConditionAffectsLowerBranches0 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- mut b ' : bool { return false; }
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 11u );
}

U_TEST( IfMergeTest7_ConditionAffectsLowerBranches1 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- mut b ' : bool { return false; }
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 12u );
}

U_TEST( IfMergeTest8_ConditionAffectsLowerBranches2 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- mut b ' : bool { return false; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			if( Link( s, y ) ) {}
			++y; // error, 'y' have reference inside 's'.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 10u );
}

U_TEST( IfMergeTest9_ConditionAffectsLowerBranches3 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- mut b ' : bool { return false; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			if( Link( s, y ) ) {}
			else if( Link( s, y ) ) {}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference, 10u ) );
}

U_TEST( WhileMergeTest0_MutablePollutionInsideLoop0 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Link( S &mut s'a', i32&'b mut x ) ' a <- mut b ' {}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };

			while(true)
			{
				Link( s, y ); // error, pollution for outer variables does not allowed.
			}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::MutableReferencePollutionOfOuterLoopVariable );
	U_TEST_ASSERT( error.file_pos.line == 12u );
}

U_TEST( WhileMergeTest1_MutablePollutionInsideLoop1 )
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

U_TEST( WhileMergeTest2_MutablePollutionInsideLoop2 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &imut x; }
		fn Link( S &mut s'a', i32&'b imut x ) ' a <- imut b ' {}
		fn Foo()
		{
			var i32 imut x= 0, imut y= 0;
			var S mut s{ .x= x };

			while(true)
			{
				Link( s, y ); // ok, immutable pollution for oter loop variables.
			}
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace U
