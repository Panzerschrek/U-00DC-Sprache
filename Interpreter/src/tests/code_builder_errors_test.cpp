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

static void OperationNotSupportedForThisTypeTest0()
{
	// Binary operations errors.
	static const char c_program_text[]=
	R"(
		class S{};
		fn Bar(){}
		fn Foo()
		{
			let s : S;
			let arr : [ i32, 5 ];
			false + true; // No binary operators for booleans.
			1u8 - 4u8; // Operation not supported for small integers.
			arr * arr; // Operation not supported for arrays.
			Bar / Bar; // Operation not supported for functions.
			0.35f32 & 1488.42f32; // Bit operator for floats.
			false > true; // Comparision of bools.
			Bar == Bar; // Exact comparision of functions.
			arr <= arr; // Comparision of arrays.
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( build_result.errors.size() >= 8u );
	U_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[0].file_pos.line == 8u );
	U_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[1].file_pos.line == 9u );
	U_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[2].file_pos.line == 10u );
	U_ASSERT( build_result.errors[3].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[3].file_pos.line == 11u );
	U_ASSERT( build_result.errors[4].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[4].file_pos.line == 12u );
	U_ASSERT( build_result.errors[5].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[5].file_pos.line == 13u );
	U_ASSERT( build_result.errors[6].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[6].file_pos.line == 14u );
	U_ASSERT( build_result.errors[7].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[7].file_pos.line == 15u );
}

static void OperationNotSupportedForThisTypeTest1()
{
	// Indexation operators.
	static const char c_program_text[]=
	R"(
		class S{};
		fn Bar(){}
		fn Foo()
		{
			let var : f32;
			let s : S;
			var[ 42u32 ]; // Indexation of variable.
			Bar[ 0u32 ]; // Indexation of function.
			s[ 45u32 ]; // Indexation of class variable.
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( build_result.errors.size() >= 3u );
	U_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[0].file_pos.line == 8u );
	U_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[1].file_pos.line == 9u );
	U_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[2].file_pos.line == 10u );
}

static void OperationNotSupportedForThisTypeTest2()
{
	// Member access operators.
	static const char c_program_text[]=
	R"(
		fn Bar(){}
		fn Foo()
		{
			let var : f32;
			let s : [ u8, 16 ];
			var.m; // Member access of variable.
			Bar.member; // Member access of function.
			s.size; // Member access of array.
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( build_result.errors.size() >= 3u );
	U_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[0].file_pos.line == 7u );
	U_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[1].file_pos.line == 8u );
	U_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[2].file_pos.line == 9u );
}

static void OperationNotSupportedForThisTypeTest3()
{
	// Unary minus.
	static const char c_program_text[]=
	R"(
		class S{};
		fn Bar(){}
		fn Foo()
		{
			let s : S;
			let a : [ u8, 16 ];
			-s; // Unary minus for class variable.
			-Bar; // Unary minus for of function.
			-a; // Unary minus for array.
			-false; // Unary minus for bool
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( build_result.errors.size() >= 4u );
	U_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[0].file_pos.line == 8u );
	U_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[1].file_pos.line == 9u );
	U_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[2].file_pos.line == 10u );
	U_ASSERT( build_result.errors[3].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_ASSERT( build_result.errors[3].file_pos.line == 11u );
}

static void TypesMismatchTest0()
{
	// Expected 'bool' in 'if'.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			if( 42 )
			{
			}
			return;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_ASSERT( error.file_pos.line == 4u );
}

static void TypesMismatchTest1()
{
	// Expected 'bool' in 'while'.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			while( 0.25f32 )
			{
				break;
			}
			return;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_ASSERT( error.file_pos.line == 4u );
}

static void TypesMismatchTest2()
{
	// Unexpected type in assignment.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let x : i32;
			x= 3.1415926535f32;
			return;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_ASSERT( error.file_pos.line == 5u );
}

static void TypesMismatchTest3()
{
	// Unexpected type in return.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			return 0.25f32;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_ASSERT( error.file_pos.line == 4u );
}

static void TypesMismatchTest4()
{
	// Unexpected void in return.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			return;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_ASSERT( error.file_pos.line == 4u );
}

static void FunctionSignatureMismatchTest0()
{
	// Argument count mismatch.
	// TODO - support functions overloading.
	static const char c_program_text[]=
	R"(
		fn Bar( a : i32, b : bool ) : bool { return false; }
		fn Foo()
		{
			Bar( 1 );
			Bar( 1, false, 0.32f );
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( build_result.errors.size() >= 2u );

	U_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::FunctionSignatureMismatch );
	U_ASSERT( build_result.errors[0].file_pos.line == 5u );
	U_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::FunctionSignatureMismatch );
	U_ASSERT( build_result.errors[1].file_pos.line == 6u );
}

static void FunctionSignatureMismatchTest1()
{
	// Argumenst type mismatch.
	// TODO - support functions overloading.
	static const char c_program_text[]=
	R"(
		fn Bar( a : i32, b : bool ) : bool { return false; }
		fn Foo()
		{
			Bar( 0.5f32, false );
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::FunctionSignatureMismatch );
	U_ASSERT( error.file_pos.line == 5u );
}

static void ArraySizeIsNotInteger()
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let x : [ i32, 5.0f32 ];
			return;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ArraySizeIsNotInteger );
	U_ASSERT( error.file_pos.line == 4u );
}

static void BreakOutsideLoopTest()
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			break;
			return;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::BreakOutsideLoop );
	U_ASSERT( error.file_pos.line == 4u );
}

static void ContinueOutsideLoopTest()
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			continue;
			return;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ContinueOutsideLoop );
	U_ASSERT( error.file_pos.line == 4u );
}

static void NameIsNotTypeNameTest()
{
	static const char c_program_text[]=
	R"(
		fn Bar(){}
		fn Foo()
		{
			let i : Bar;
			return;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::NameIsNotTypeName );
	U_ASSERT( error.file_pos.line == 5u );
}

static void UnreachableCodeTest0()
{
	// Simple unreachable code.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			return;
			1 + 2;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_ASSERT( error.file_pos.line == 5u );
}

static void UnreachableCodeTest1()
{
	// Unreachable code, when return is in inner block.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			{ return; }
			1 + 2;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_ASSERT( error.file_pos.line == 5u );
}

static void UnreachableCodeTest2()
{
	// Unreachable code, when return is in if-else block.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			if( false ) { return; }
			else { return; }
			1 + 2;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_ASSERT( error.file_pos.line == 6u );
}

static void UnreachableCodeTest3()
{
	// Should not generate unreachable code, when if-else block returns not in all cases.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			if( false ) { }
			else { return; }
			1 + 2;
			return;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );
	U_ASSERT( build_result.errors.empty() );
}

static void UnreachableCodeTest4()
{
	// Should not generate unreachable code, when "if" block does not contains unconditional "else".
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			if( true ) { return; }
			else if( false ) { return; }
			1 + 2;
			return;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );
	U_ASSERT( build_result.errors.empty() );
}

static void NoReturnInFunctionReturningNonVoidTest0()
{
	// No return in non-void function;
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::NoReturnInFunctionReturningNonVoid );
	U_ASSERT( error.file_pos.line == 3u );
}

static void NoReturnInFunctionReturningNonVoidTest1()
{
	// Return not in all branches.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			if( true ) { return 0; }
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::NoReturnInFunctionReturningNonVoid );
	U_ASSERT( error.file_pos.line == 3u );
}

static void NoReturnInFunctionReturningNonVoidTest2()
{
	// Return not in all branches.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			while( true ) { return 0; }
			if( false ) {} else { return 1; }
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::NoReturnInFunctionReturningNonVoid );
	U_ASSERT( error.file_pos.line == 3u );
}

static void NoReturnInFunctionReturningNonVoidTest3()
{
	// Return exists in all branches.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			{
				if( true ) { return 42; }
			}
			2 + 2;
			return -1;
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );
	U_ASSERT( build_result.errors.empty() );
}

static void NoReturnInFunctionReturningNonVoidTest4()
{
	// Return exists in all branches.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			{
				if( true ) { return 42; }
				else { { return 666; } }
			}
		}
	)";

	const CodeBuilderLLVM::BuildResult build_result= BuildProgram( c_program_text );
	U_ASSERT( build_result.errors.empty() );
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
	OperationNotSupportedForThisTypeTest0();
	OperationNotSupportedForThisTypeTest1();
	OperationNotSupportedForThisTypeTest2();
	OperationNotSupportedForThisTypeTest3();
	TypesMismatchTest0();
	TypesMismatchTest1();
	TypesMismatchTest2();
	TypesMismatchTest3();
	TypesMismatchTest4();
	FunctionSignatureMismatchTest0();
	FunctionSignatureMismatchTest1();
	ArraySizeIsNotInteger();
	BreakOutsideLoopTest();
	ContinueOutsideLoopTest();
	NameIsNotTypeNameTest();
	UnreachableCodeTest0();
	UnreachableCodeTest1();
	UnreachableCodeTest2();
	UnreachableCodeTest3();
	UnreachableCodeTest4();
	NoReturnInFunctionReturningNonVoidTest0();
	NoReturnInFunctionReturningNonVoidTest1();
	NoReturnInFunctionReturningNonVoidTest2();
	NoReturnInFunctionReturningNonVoidTest3();
	NoReturnInFunctionReturningNonVoidTest4();
}

} // namespace Interpreter
