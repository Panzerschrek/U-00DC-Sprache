#include <cstdlib>
#include <iostream>

#include "tests.hpp"

namespace U
{

U_TEST(InvalidTypeForAutoVariableTest0)
{
	// Assign functions set.
	// SPRACHE_TODO - mayebe allow functions references?
	static const char c_program_text[]=
	R"(
		fn Bar(){}
		fn Foo()
		{
			auto x= Bar;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InvalidTypeForAutoVariable );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(InvalidTypeForAutoVariableTest1)
{
	// Assign function.
	static const char c_program_text[]=
	R"(
		class C{}
		fn Foo()
		{
			auto x= C;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InvalidTypeForAutoVariable );
	U_TEST_ASSERT( error.file_pos.line == 5u );
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

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.file_pos.line == 5u );
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

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.file_pos.line == 5u );
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

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.file_pos.line == 4u );
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

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.file_pos.line == 4u );
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

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

} // namespace U
