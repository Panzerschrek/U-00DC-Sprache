#include <iostream>

#include "../assert.hpp"
#include "../code_builder.hpp"
#include "../lexical_analyzer.hpp"
#include "../syntax_analyzer.hpp"

#include "code_builder_errors_test.hpp"

namespace U
{

static CodeBuilder::BuildResult BuildProgram( const char* const text )
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
		CodeBuilder().BuildProgram( syntax_analysis_result.program_elements );
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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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
			let : UnknownType x= 0;
			return 42;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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
			let : S x;
			return x.unexistent_field;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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
		fn Foo( i32 continue ) : i32
		{
			return 0;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_ASSERT( error.file_pos.line == 2u );
}

static void UsingKeywordAsName3()
{
	// Variable name is keyword.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			let : i32 void= 0;
			return 0;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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
			let : i32 x= 0;
			let : i32 x= 0;
			return 0;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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
			let : i32 x= 0;
			{ let : i32 x= 0; }
			return 0;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );
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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_ASSERT( error.file_pos.line == 5u );
}

static void Redefinition3()
{
	// Function redefinition.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{ return 0; }

		fn Bar() : i32
		{ return 1; }

		class Foo{};
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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
			let : S s;
			let : [ i32, 5 ] arr;
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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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
			let : f32 var= 0.0f32;
			let : S s;
			var[ 42u32 ]; // Indexation of variable.
			Bar[ 0u32 ]; // Indexation of function.
			s[ 45u32 ]; // Indexation of class variable.
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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
			let : f32 var= 0.0f32;
			let : [ u8, 16 ] s;
			var.m; // Member access of variable.
			Bar.member; // Member access of function.
			s.size; // Member access of array.
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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
			let : S s;
			let : [ u8, 16 ] a;
			-s; // Unary minus for class variable.
			-Bar; // Unary minus for of function.
			-a; // Unary minus for array.
			-false; // Unary minus for bool
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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
			let : i32 x= 0;
			x= 3.1415926535f32;
			return;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_ASSERT( error.file_pos.line == 4u );
}

static void TypesMismatchTest5()
{
	// Unexpected type in bindind to reference.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let : i32 x= 0;
			let : i8 &x_ref= x;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_ASSERT( error.file_pos.line == 5u );
}

static void FunctionSignatureMismatchTest0()
{
	// Argument count mismatch.
	// TODO - support functions overloading.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 a, bool b ) : bool { return false; }
		fn Foo()
		{
			Bar( 1 );
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	U_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::FunctionSignatureMismatch );
	U_ASSERT( build_result.errors[0].file_pos.line == 5u );;
}

static void FunctionSignatureMismatchTest1()
{
	// Argument count mismatch.
	// TODO - support functions overloading.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 a, bool b ) : bool { return false; }
		fn Foo()
		{
			Bar( 1, false, 0.2 );
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	U_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::FunctionSignatureMismatch );
	U_ASSERT( build_result.errors[0].file_pos.line == 5u );
}

static void FunctionSignatureMismatchTest2()
{
	// Argumenst type mismatch.
	// TODO - support functions overloading.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 a, bool b ) : bool { return false; }
		fn Foo()
		{
			Bar( 0.5f32, false );
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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
			let : [ i32, 5.0f32 ] x;
			return;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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
			let : Bar i;
			return;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );
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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );
	U_ASSERT( build_result.errors.empty() );
}

static void UnreachableCodeTest5()
{
	// Unreachable code, when break/continue.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			while( true )
			{
				break;
				42;
			}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_ASSERT( error.file_pos.line == 7u );
}

static void UnreachableCodeTest6()
{
	// Unreachable code, when break/continue.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			while( true )
			{
				{ continue; }
				42;
			}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_ASSERT( error.file_pos.line == 7u );
}

static void UnreachableCodeTest7()
{
	// Unreachable code, when break/continue.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			while( true )
			{
				if( true ) { continue; } else { break; }
				42;
			}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_ASSERT( error.file_pos.line == 7u );
}

static void UnreachableCodeTest8()
{
	// Should not generate unreachable code, when break or continue is not in all if-branches.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			while( true )
			{
				if( true ) { continue; } else { }
				42;
			}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );
	U_ASSERT( build_result.errors.empty() );
}

static void UnreachableCodeTest9()
{
	// Should not generate unreachable code, when "if" block does not contains unconditional "else".
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			while( true )
			{
				if( true ) { continue; } else if( false ) { break; }
				42;
			}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );
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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );
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

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );
	U_ASSERT( build_result.errors.empty() );
}

static void ExpectedInitializerTest0()
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let : i32 x;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	U_ASSERT( error.file_pos.line == 4u );
}

static void ExpectedReferenceValueTest0()
{
	// Assign to non-reference value.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			1 + 2 = 42;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_ASSERT( error.file_pos.line == 4u );
}

static void ExpectedReferenceValueTest1()
{
	// Assign to function. Functions is const-reference values.
	static const char c_program_text[]=
	R"(
		fn Bar(){}
		fn Baz(){}
		fn Foo()
		{
			Bar= Baz;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_ASSERT( error.file_pos.line == 6u );
}

static void ExpectedReferenceValueTest2()
{
	// Assign to value.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 a, i32 b )
		{
			a / b = b;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_ASSERT( error.file_pos.line == 4u );
}

static void ExpectedReferenceValueTest3()
{
	// Assign to immutable value.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let : f64 imut a= 3.1415926535f64;
			a = 0.0f64;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_ASSERT( error.file_pos.line == 5u );
}

static void ExpectedReferenceValueTest4()
{
	// Assign to immutable argument.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 imut a )
		{
			a = -45;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_ASSERT( error.file_pos.line == 4u );
}

static void ExpectedReferenceValueTest5()
{
	// Initialize reference using value-object.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let : i32 a= 42, b= 24;
			let : i32 &x= a - b;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_ASSERT( error.file_pos.line == 5u );
}

static void ExpectedReferenceValueTest6()
{
	// Using value in reference - function argument.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &x ) {}
		fn Foo()
		{
			Bar(42);
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_ASSERT( error.file_pos.line == 5u );
}

static void ExpectedReferenceValueTest7()
{
	// Using value in reference - function return value.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32 &
		{
			return 42;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_ASSERT( error.file_pos.line == 4u );
}

static void BindingConstReferenceToNonconstReferenceTest0()
{
	// Initialize reference using value-object.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let : i32 imut a= 42;
			let : i32 &a_ref= a;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_ASSERT( error.file_pos.line == 5u );
}

static void BindingConstReferenceToNonconstReferenceTest1()
{
	// Initialize reference using value-object.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x ){}
		fn Foo()
		{
			let : i32 imut x = 0;
			Bar( x );
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_ASSERT( error.file_pos.line == 6u );
}

static void BindingConstReferenceToNonconstReferenceTest2()
{
	// Return reference, when return value is const reference.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &imut x ) : i32 &mut
		{
			return x;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_ASSERT( error.file_pos.line == 4u );
}

static void CouldNotOverloadFunctionTest0()
{
	// No difference.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x, f64 &imut y ) {}
		fn Foo( i32 x, f64 &imut y ) {}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_ASSERT( error.file_pos.line == 3u );
}

static void CouldNotOverloadFunctionTest1()
{
	// Different are only mutability modifiers for value parameters.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 mut x ) {}
		fn Foo( i32 imut x ) {}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_ASSERT( error.file_pos.line == 3u );
}

static void CouldNotOverloadFunctionTest2()
{
	// One parameter is value, other is const-reference.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 mut x ) {}
		fn Foo( i32 &imut x ) {}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_ASSERT( error.file_pos.line == 3u );
}

static void CouldNotOverloadFunctionTest3()
{
	// Const and nonconst reference-parameters are different.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &mut x ) {}
		fn Foo( i32 &imut x ) {}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( build_result.errors.empty() );
}

static void CouldNotOverloadFunctionTest4()
{
	// Functions with zero args.
	static const char c_program_text[]=
	R"(
		fn Foo() {}
		fn Foo() {}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_ASSERT( error.file_pos.line == 3u );
}

static void CouldNotSelectOverloadedFunction0()
{
	// Different actual args and args from functions set.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x ) {}
		fn Foo( f32 x ) {}
		fn Bar()
		{
			Foo( false );
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::CouldNotSelectOverloadedFunction );
	U_ASSERT( error.file_pos.line == 6u );
}

static void CouldNotSelectOverloadedFunction1()
{
	// Different actual args count and args from functions set.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x ) {}
		fn Foo( f32 x ) {}
		fn Bar()
		{
			Foo( 1, 2, 3, 4 );
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgram( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::CouldNotSelectOverloadedFunction );
	U_ASSERT( error.file_pos.line == 6u );
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
	TypesMismatchTest5();
	FunctionSignatureMismatchTest0();
	FunctionSignatureMismatchTest1();
	FunctionSignatureMismatchTest2();
	ArraySizeIsNotInteger();
	BreakOutsideLoopTest();
	ContinueOutsideLoopTest();
	NameIsNotTypeNameTest();
	UnreachableCodeTest0();
	UnreachableCodeTest1();
	UnreachableCodeTest2();
	UnreachableCodeTest3();
	UnreachableCodeTest4();
	UnreachableCodeTest5();
	UnreachableCodeTest6();
	UnreachableCodeTest7();
	UnreachableCodeTest8();
	UnreachableCodeTest9();
	NoReturnInFunctionReturningNonVoidTest0();
	NoReturnInFunctionReturningNonVoidTest1();
	NoReturnInFunctionReturningNonVoidTest2();
	NoReturnInFunctionReturningNonVoidTest3();
	NoReturnInFunctionReturningNonVoidTest4();
	ExpectedInitializerTest0();
	ExpectedReferenceValueTest0();
	ExpectedReferenceValueTest1();
	ExpectedReferenceValueTest2();
	ExpectedReferenceValueTest3();
	ExpectedReferenceValueTest4();
	ExpectedReferenceValueTest5();
	ExpectedReferenceValueTest6();
	ExpectedReferenceValueTest7();
	BindingConstReferenceToNonconstReferenceTest0();
	BindingConstReferenceToNonconstReferenceTest1();
	BindingConstReferenceToNonconstReferenceTest2();
	CouldNotOverloadFunctionTest0();
	CouldNotOverloadFunctionTest1();
	CouldNotOverloadFunctionTest2();
	CouldNotOverloadFunctionTest3();
	CouldNotOverloadFunctionTest4();
	CouldNotSelectOverloadedFunction0();
	CouldNotSelectOverloadedFunction1();
}

} // namespace U
