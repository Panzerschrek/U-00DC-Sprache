#include <iostream>

#include "../assert.hpp"
#include "../code_builder_llvm.hpp"
#include "../lexical_analyzer.hpp"
#include "../syntax_analyzer.hpp"

#include "code_builder_errors_test.hpp"

namespace Interpreter
{

static CodeBuilderLLVM::BuildResult BuildProgram( const char* const text )
{
	const LexicalAnalysisResult lexical_analysis_result=
		LexicalAnalysis( ToProgramString( text ) );

	for( const std::string& lexical_error_message : lexical_analysis_result.error_messages )
		std::cout << lexical_error_message << "\n";
	U_ASSERT( lexical_analysis_result.error_messages.empty() );

	const SyntaxAnalysisResult syntax_analysis_result=
		SyntaxAnalysis( lexical_analysis_result.lexems );

	for( const SyntaxErrorMessage& syntax_error_message : syntax_analysis_result.error_messages )
		std::cout << syntax_error_message << "\n";
	U_ASSERT( syntax_analysis_result.error_messages.empty() );

	for( const std::string& syntax_error_message : syntax_analysis_result.error_messages )
		std::cout << syntax_error_message << "\n";
	U_ASSERT( syntax_analysis_result.error_messages.empty() );

	return
		CodeBuilderLLVM().BuildProgram( syntax_analysis_result.program_elements );
}

static void NameNotFoundTest0()
{
	// Unknown named oberand.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			return y;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_ASSERT( error.file_pos.line == 4u );
}

static void NameNotFoundTest1()
{
	// Unknown type name.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			let x : UnknownType;
			return 42;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_ASSERT( error.file_pos.line == 4u );
}

static void NameNotFoundTest2()
{
	// Unknown member name.
	static const char c_program_text[]=
	R"(
		class S{};
		fn Foo() : i32
		{
			let x : S;
			return x.unexistent_field;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_ASSERT( error.file_pos.line == 6u );
}

static void UsingKeywordAsName0()
{
	// Function name is keyword.
	static const char c_program_text[]=
	R"(
		fn let() : i32
		{
			return 0;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_ASSERT( error.file_pos.line == 2u );
}

static void UsingKeywordAsName1()
{
	// Arg name is keyword.
	static const char c_program_text[]=
	R"(
		fn Foo( continue : i32 ) : i32
		{
			return 0;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_ASSERT( error.file_pos.line == 2u );
}

static void UsingKeywordAsName2()
{
	// class name is keyword.
	static const char c_program_text[]=
	R"(
		class while{};
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_ASSERT( error.file_pos.line == 2u );
}

static void UsingKeywordAsName3()
{
	// Arg name is keyword.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			let void : i32;
			return 0;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_ASSERT( error.file_pos.line == 4u );
}

static void Redefinition0()
{
	// Variable redefinition in same scope.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			let x : i32;
			let x : i32;
			return 0;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_ASSERT( error.file_pos.line == 5u );
}

static void Redefinition1()
{
	// Variable redefinition in different scopes.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			let x : i32;
			{ let x : i32; }
			return 0;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );
	U_ASSERT( build_result.errors.empty() );
}

static void Redefinition2()
{
	// Class redefinition.
	static const char c_program_text[]=
	R"(
		class AA{};
		fn Foo() : i32
		{ return 0; }
		class AA{};
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_ASSERT( error.file_pos.line == 5u );
}

static void Redefinition3()
{
	// Function redefinition.
	// TODO - write new tests, when functions overloading will be implemented
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{ return 0; }

		fn Bar() : i32
		{ return 1; }

		fn Foo( x : f32 ) : i32
		{ return 42; }
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_ASSERT( error.file_pos.line == 8u );
}

void RunCodeBuilderErrorsTests()
{
	NameNotFoundTest0();
	NameNotFoundTest1();
	NameNotFoundTest2();
	UsingKeywordAsName0();
	UsingKeywordAsName1();
	UsingKeywordAsName2();
	UsingKeywordAsName3();
	Redefinition0();
	Redefinition1();
	Redefinition2();
	Redefinition3();
}

} // namespace Interpreter
