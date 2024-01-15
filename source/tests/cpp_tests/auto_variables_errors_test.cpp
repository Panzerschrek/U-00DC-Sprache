#include <cstdlib>
#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST(InvalidTypeForAutoVariableTest0)
{
	// Assign functions set.
	// SPRACHE_TODO - mayebe allow functions references?
	static const char c_program_text[]=
	R"(
		fn Bar();
		fn Bar( i32 x );
		fn Foo()
		{
			auto x= Bar;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedVariable );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST(InvalidTypeForAutoVariableTest1)
{
	// Assign struct.
	static const char c_program_text[]=
	R"(
		struct C{}
		fn Foo()
		{
			auto x= C;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedVariable );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(RedefinitionTest0)
{
	// Redefinition of other auto-variable.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			auto x= 42;
			auto x= 34;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(RedefinitionTest1)
{
	// Redefinition of nonauto-variable.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 42;
			auto x= 34;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(ExpectedReferenceValueTest0)
{
	// Bind value to mutable auto-reference.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			auto &mut x= 34;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(ExpectedReferenceValueTest1)
{
	// Bind value to immutable auto-reference.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			auto &imut x= 34;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(BindingConstReferenceToNonconstReferenceTest0)
{
	// Bind immutable reference to mutable auto-reference.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i16 imut x= 6428i16;
			auto &mut x_ref= x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

} // namespace

} // namespace U
