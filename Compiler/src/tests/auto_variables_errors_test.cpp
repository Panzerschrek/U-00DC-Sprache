#include <cstdlib>
#include <iostream>

#include "../assert.hpp"
#include "tests.hpp"

#include "auto_variables_errors_test.hpp"

namespace U
{

static void InvalidTypeForAutoVariableTest0()
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

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::InvalidTypeForAutoVariable );
	U_ASSERT( error.file_pos.line == 5u );
}

static void InvalidTypeForAutoVariableTest1()
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

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::InvalidTypeForAutoVariable );
	U_ASSERT( error.file_pos.line == 5u );
}

static void RedefinitionTest0()
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

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_ASSERT( error.file_pos.line == 5u );
}

static void RedefinitionTest1()
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

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_ASSERT( error.file_pos.line == 5u );
}

static void ExpectedReferenceValueTest0()
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

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_ASSERT( error.file_pos.line == 4u );
}

static void ExpectedReferenceValueTest1()
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

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_ASSERT( error.file_pos.line == 4u );
}

static void BindingConstReferenceToNonconstReferenceTest0()
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

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_ASSERT( error.file_pos.line == 5u );
}

void RunAutoVariablesErrorsTest()
{
	InvalidTypeForAutoVariableTest0();
	InvalidTypeForAutoVariableTest1();
	RedefinitionTest0();
	RedefinitionTest1();
	ExpectedReferenceValueTest0();
	ExpectedReferenceValueTest1();
	BindingConstReferenceToNonconstReferenceTest0();
}

} // namespace U
