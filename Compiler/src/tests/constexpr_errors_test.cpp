#include "tests.hpp"

namespace U
{

U_TEST(ExpectedConstantExpressionTest0)
{
	// Constructed struct is not contant expression.
	static const char c_program_text[]=
	R"(
		struct S{}
		fn Foo()
		{
			var [ i32, S() ] x;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedConstantExpression );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(ExpectedConstantExpressionTest1)
{
	// Sum of non-constexpr variables is not constexpr.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 42, y= 34;
			var [ i32, x + y ] x;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedConstantExpression );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( ViariableInitializerIsNotConstantExpressionTest0 )
{
	// = initializer for constexpr variable using non-constexpr variable.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 42;
			var i32 constexpr y= x;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::VariableInitializerIsNotConstantExpression );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( ViariableInitializerIsNotConstantExpressionTest1 )
{
	// Constructor initializer for constexpr variable using non-constexpr variable.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 42;
			var i32 constexpr y(x);
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::VariableInitializerIsNotConstantExpression );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( ViariableInitializerIsNotConstantExpressionTest2 )
{
	// Auto constexpr variable using non-constexpr variable for initialization.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 42;
			auto constexpr y= x * 2;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::VariableInitializerIsNotConstantExpression );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( InvalidTypeForConstantExpressionVariableTest0 )
{
	// Constexpr variable of struct type.
	static const char c_program_text[]=
	R"(
		struct S{}
		fn Foo()
		{
			var S constexpr s{};
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InvalidTypeForConstantExpressionVariable );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

} // namespace U
