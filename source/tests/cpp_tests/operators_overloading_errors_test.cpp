#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( OperatorDeclarationOutsideClass_Test )
{
	static const char c_program_text[]=
	R"(
		struct S{}
		op+( S s, f32 f ) : S {}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::OperatorDeclarationOutsideClass );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( OperatorDoesNotHaveParentClassArguments_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			op+( i32 i, f32 f ) : i32 {}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::OperatorDoesNotHaveParentClassArguments );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST( OperatorDoesNotHaveParentClassArguments_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			op+( i32 i ) : bool {}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::OperatorDoesNotHaveParentClassArguments );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST( OperatorDoesNotHaveParentClassArguments_Test2 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct S
		{
			type this_type= S</T/>;
			op+( this_type a, this_type b ) : bool { return false; }   // Should be ok, if args are template-dependent
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( OperatorDoesNotHaveParentClassArguments_Test3 )
{
	// First argument of () operator should have class type.
	static const char c_program_text[]=
	R"(
		struct S
		{
			op()( i32 x, S& s ){}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::OperatorDoesNotHaveParentClassArguments );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST( InvalidArgumentCountForOperator_Test )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			op*( i32 x, S &imut s, f32 f ) : bool { return true; } // 3 arguments, expected 2
			op=( S &mut s ) {} // 1 argument, expected 1
			op=( i32 x, S &imut s, f32 f ) {} // 2 arguments,e xpected 1
			op++( i32 x, S &imut s ){} // 2 arguments, expected 1
			op--( i32 x, S &imut s, f32 f ) {}
			op!( S &imut s, f32 x ) : bool { return false; } //  2 arguments, expected 1
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidArgumentCountForOperator, 4u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidArgumentCountForOperator, 5u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidArgumentCountForOperator, 6u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidArgumentCountForOperator, 7u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidArgumentCountForOperator, 8u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidArgumentCountForOperator, 9u ) );
}

U_TEST( InvalidFirstParamValueTypeForAssignmentLikeOperator_Test )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			op=( S &imut dst, S &imut src );
			op+=( S dst, i32 x );
			op<<=( i32 x, S& s );
			op/=( f32 &imut x, S s );
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidFirstParamValueTypeForAssignmentLikeOperator, 4u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidFirstParamValueTypeForAssignmentLikeOperator, 5u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidFirstParamValueTypeForAssignmentLikeOperator, 6u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidFirstParamValueTypeForAssignmentLikeOperator, 7u ) );
}

U_TEST( InvalidReturnTypeForOperator_Test )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			op=( S &mut dst, S &imut src ) : bool {} // Expected void
			op++( S &mut a ) : i32 {} // Expected void
			op--( S &mut a ) : void& {} // Expected non-reference
			op/=( S &mut dst, S &imut src ) : S &mut {} // Expected void
			op+=( S &mut dst, S &imut src ) : void & {} // Expected non-reference

			// Expected bool
			op==( S &mut a, S &mut b );
			// Expected i32
			op<=>( S& a, S& b ) : i64;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidReturnTypeForOperator, 4u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidReturnTypeForOperator, 5u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidReturnTypeForOperator, 6u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidReturnTypeForOperator, 7u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidReturnTypeForOperator, 8u ) );

	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidReturnTypeForOperator, 11u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InvalidReturnTypeForOperator, 13u ) );
}

U_TEST( IndexationOperatorHaveFirstArgumentOfNonparentClass )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			op[]( i32 i, S &imut s ){}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::OperatorDoesNotHaveParentClassArguments ); // TODO - use separate error code for this case
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST( CanNotSelectOverloadedOperator )
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			op+(A a, B b) : i32;
		}
		struct B
		{
			op+(A a, B b) : i32;
		}
		fn Foo()
		{
			var A a;
			var B b;
			auto x = a + b; // Error - both classes contain overloaded operator with same signature. Can't select one of them.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors,  CodeBuilderErrorCode::TooManySuitableOverloadedFunctions, 14 ) );
}

} // namespace

} // namespace U
