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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedConstantExpression );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(ExpectedConstantExpressionTest1)
{
	// Sum of mutable variables is not constexpr.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 42, mut y= 34;
			var [ i32, x + y ] z= zero_init;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

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
			var i32 mut x= 42;
			var i32 constexpr y= x;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

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
			var i32 mut x= 42;
			var i32 constexpr y(x);
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

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
			var i32 mut x= 42;
			auto constexpr y= x * 2;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::VariableInitializerIsNotConstantExpression );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( ViariableInitializerIsNotConstantExpressionTest3 )
{
	// Non-constexpr initializer for constexpr array.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 42;
			var [ i32, 3 ] constexpr arr[ 5, x, 854 ];
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::VariableInitializerIsNotConstantExpression );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( InvalidTypeForConstantExpressionVariableTest0 )
{
	// Constexpr variable of class type.
	static const char c_program_text[]=
	R"(
		class S{}
		fn Foo()
		{
			var S constexpr s{};
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InvalidTypeForConstantExpressionVariable );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( ConstantExpressionResultIsUndefinedTest0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			84 / 0; // i32 division by zero
			1u / 0u; // u32 division by zero
			1.0f / 0.0f; // floating point division by zero should NOT produce undefined value
			1.0  % 0.0 ; // floating point remainder for zero should NOT produce undefined value
			85i64 % 0i64; // i64 remainder take for zero
			i32( - (1i64 << 31u) ) / -1; // min_int / -1
			i32( - (1i64 << 31u) ) % -1; // min_int / -1 remainder
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.size() == 5u );

	U_TEST_ASSERT( build_result.errors[0u].code == CodeBuilderErrorCode::ConstantExpressionResultIsUndefined );
	U_TEST_ASSERT( build_result.errors[0u].file_pos.line == 4u );
	U_TEST_ASSERT( build_result.errors[1u].code == CodeBuilderErrorCode::ConstantExpressionResultIsUndefined );
	U_TEST_ASSERT( build_result.errors[1u].file_pos.line == 5u );
	U_TEST_ASSERT( build_result.errors[2u].code == CodeBuilderErrorCode::ConstantExpressionResultIsUndefined );
	U_TEST_ASSERT( build_result.errors[2u].file_pos.line == 8u );
	U_TEST_ASSERT( build_result.errors[3u].code == CodeBuilderErrorCode::ConstantExpressionResultIsUndefined );
	U_TEST_ASSERT( build_result.errors[3u].file_pos.line == 9u );
	U_TEST_ASSERT( build_result.errors[4u].code == CodeBuilderErrorCode::ConstantExpressionResultIsUndefined );
	U_TEST_ASSERT( build_result.errors[4u].file_pos.line ==10u );
}

U_TEST( StaticAssertExpressionMustHaveBoolTypeTest0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			static_assert( 42 ); // Int value
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::StaticAssertExpressionMustHaveBoolType );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST( StaticAssertExpressionMustHaveBoolTypeTest1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var f32 constexpr pi= 3.1415926535f;
			static_assert( pi ); // float variable
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::StaticAssertExpressionMustHaveBoolType );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( StaticAssertExpressionMustHaveBoolTypeTest2 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			static_assert( i64 ); // typename
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::StaticAssertExpressionMustHaveBoolType );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST( StaticAssertExpressionIsNotConstantTest0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var f32 mut pi= 3.14f;
			static_assert( pi == 3.14f ); // pi is not constant
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::StaticAssertExpressionIsNotConstant );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( StaticAssertExpressionIsNotConstantTest1 )
{
	static const char c_program_text[]=
	R"(
		fn Bar() : bool { return true; }
		fn Foo()
		{
			static_assert( Bar() ); // function call result is not constant
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::StaticAssertExpressionIsNotConstant );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( ExpectedReferenceValue_ForConstexpr_Test0 )
{
	// Try mutate constexpr value.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 constexpr x= 42;
			static_assert( x == 42 );
			++x;
			x+= 1;
			x= x + 1;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.size() >= 3u );

	U_TEST_ASSERT( build_result.errors[0u].code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( build_result.errors[0u].file_pos.line == 6u );
	U_TEST_ASSERT( build_result.errors[1u].code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( build_result.errors[1u].file_pos.line == 7u );
	U_TEST_ASSERT( build_result.errors[2u].code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( build_result.errors[2u].file_pos.line == 8u );
}

U_TEST( ExpectedReferenceValue_ForConstexpr_Test1 )
{
	// Try mutate constexpr array.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 3u ] constexpr x[ 4, 8, 15 ];
			static_assert( x[0u] == 4 && x[1u] == 8 && x[2u] == 15 );
			++x[0u];
			x[1u]+= 1;
			x[2u]= x[2u] + 1;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.size() >= 3u );

	U_TEST_ASSERT( build_result.errors[0u].code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( build_result.errors[0u].file_pos.line == 6u );
	U_TEST_ASSERT( build_result.errors[1u].code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( build_result.errors[1u].file_pos.line == 7u );
	U_TEST_ASSERT( build_result.errors[2u].code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( build_result.errors[2u].file_pos.line == 8u );
}

U_TEST( ArrayIndexOutOfBoundsTest0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 5 ] arr= zero_init;
			var [ i32, 0 ] zero_arr= zero_init;
			arr[5u]; // index= size
			arr[6u]; // index > size
			arr[8278282u]; // index >> size
			zero_arr[0u]; // indexation of zero-size array
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.size() >= 4u );

	U_TEST_ASSERT( build_result.errors[0u].code == CodeBuilderErrorCode::ArrayIndexOutOfBounds );
	U_TEST_ASSERT( build_result.errors[0u].file_pos.line == 6u );
	U_TEST_ASSERT( build_result.errors[1u].code == CodeBuilderErrorCode::ArrayIndexOutOfBounds );
	U_TEST_ASSERT( build_result.errors[1u].file_pos.line == 7u );
	U_TEST_ASSERT( build_result.errors[2u].code == CodeBuilderErrorCode::ArrayIndexOutOfBounds );
	U_TEST_ASSERT( build_result.errors[2u].file_pos.line == 8u );
	U_TEST_ASSERT( build_result.errors[3u].code == CodeBuilderErrorCode::ArrayIndexOutOfBounds );
	U_TEST_ASSERT( build_result.errors[3u].file_pos.line == 9u );
}

} // namespace U
