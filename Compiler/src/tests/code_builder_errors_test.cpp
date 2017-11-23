#include <iostream>

#include "tests.hpp"

namespace U
{

U_TEST(NameNotFoundTest0)
{
	// Unknown named oberand.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			return y;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(NameNotFoundTest1)
{
	// Unknown type name.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var UnknownType x= 0;
			return 42;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(NameNotFoundTest2)
{
	// Unknown member name.
	static const char c_program_text[]=
	R"(
		struct S{}
		fn Foo() : i32
		{
			var S x{};
			return x.unexistent_field;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST(UsingKeywordAsName0)
{
	// Function name is keyword.
	static const char c_program_text[]=
	R"(
		fn var() : i32
		{
			return 0;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST(UsingKeywordAsName1)
{
	// Arg name is keyword.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 continue ) : i32
		{
			return 0;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST(UsingKeywordAsName2)
{
	// struct name is keyword.
	static const char c_program_text[]=
	R"(
		struct while{}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST(UsingKeywordAsName3)
{
	// Variable name is keyword.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var i32 void= 0;
			return 0;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(Redefinition0)
{
	// Variable redefinition in same scope.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var i32 x= 0;
			var i32 x= 0;
			return 0;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(Redefinition1)
{
	// Variable redefinition in different scopes.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var i32 x= 0;
			{ var i32 x= 0; }
			return 0;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.empty() );
}

U_TEST(Redefinition3)
{
	// Function redefinition.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{ return 0; }

		fn Bar() : i32
		{ return 1; }

		struct Foo{}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.file_pos.line == 8u );
}

U_TEST(UnknownNumericConstantTypeTest0)
{
	// unknown name
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			return 45fty584s;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnknownNumericConstantType );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(UnknownNumericConstantTypeTest1)
{
	// existent type name in upper case
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			return 45I32;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnknownNumericConstantType );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(OperationNotSupportedForThisTypeTest0)
{
	// Binary operations errors.
	static const char c_program_text[]=
	R"(
		struct S{}
		fn Foo()
		{
			var S s= zero_init;
			var [ i32, 5 ] arr= zero_init;
			false + true; // No binary operators for booleans.
			1u8 - 4u8; // Operation not supported for small integers.
			arr * arr; // Operation not supported for arrays.
			0.35f32 & 1488.42f32; // Bit operator for floats.
			false > true; // Comparision of bools.
			arr <= arr; // Comparision of arrays.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= 6u );
	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[0].file_pos.line == 7u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[1].file_pos.line == 8u );
	U_TEST_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[2].file_pos.line == 9u );
	U_TEST_ASSERT( build_result.errors[3].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[3].file_pos.line == 10u );
	U_TEST_ASSERT( build_result.errors[4].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[4].file_pos.line == 11u );
	U_TEST_ASSERT( build_result.errors[5].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[5].file_pos.line == 12u );
}

U_TEST(OperationNotSupportedForThisTypeTest1)
{
	// Indexation operators.
	static const char c_program_text[]=
	R"(
		struct S{}
		fn Bar(){}
		fn Foo()
		{
			var f32 variable= 0.0f32;
			var S s= zero_init;
			variable[ 42u32 ]; // Indexation of variable.
			Bar[ 0u32 ]; // Indexation of function.
			s[ 45u32 ]; // Indexation of struct variable.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= 3u );
	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[0].file_pos.line == 8u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[1].file_pos.line == 9u );
	U_TEST_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[2].file_pos.line == 10u );
}

U_TEST(OperationNotSupportedForThisTypeTest2)
{
	// Member access operators.
	static const char c_program_text[]=
	R"(
		fn Bar(){}
		fn Foo()
		{
			var f32 variable= 0.0f32;
			var [ u8, 16 ] s= zero_init;
			variable.m; // Member access of variable.
			Bar.member; // Member access of function.
			s.size; // Member access of array.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= 3u );
	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[0].file_pos.line == 7u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[1].file_pos.line == 8u );
	U_TEST_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[2].file_pos.line == 9u );
}

U_TEST(OperationNotSupportedForThisTypeTest3)
{
	// Unary minus.
	static const char c_program_text[]=
	R"(
		struct S{}
		fn Bar(){}
		fn Foo()
		{
			var S s= zero_init;
			var [ u8, 16 ] a= zero_init;
			-s; // Unary minus for struct variable.
			-Bar; // Unary minus for of function.
			-a; // Unary minus for array.
			-false; // Unary minus for bool
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= 4u );
	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[0].file_pos.line == 8u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[1].file_pos.line == 9u );
	U_TEST_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[2].file_pos.line == 10u );
	U_TEST_ASSERT( build_result.errors[3].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[3].file_pos.line == 11u );
}

U_TEST(TypesMismatchTest0)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(TypesMismatchTest1)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(TypesMismatchTest2)
{
	// Unexpected type in assignment.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			x= 3.1415926535f32;
			return;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(TypesMismatchTest3)
{
	// Unexpected type in return.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			return 0.25f32;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(TypesMismatchTest4)
{
	// Unexpected void in return.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			return;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(TypesMismatchTest5)
{
	// Unexpected type in bindind to reference.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			var i8 &x_ref= x;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(TypesMismatchTest6)
{
	// Unexpected type in construction - "bool" to integer.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 i(false);
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(TypesMismatchTest7)
{
	// Unexpected type in construction - "bool" to float.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var f32 f(true);
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(TypesMismatchTest8)
{
	// Unexpected type in construction - integer to "bool".
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var bool b(58i16);
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(TypesMismatchTest9)
{
	// Unexpected type in construction - float to "bool".
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var bool b( 0.5f64 );
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(TypesMismatchTest10)
{
	// Unexpected type in assignment - assign struct to integer.
	static const char c_program_text[]=
	R"(
		struct S{}
		fn Foo()
		{
			var bool b= S();
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(NoMatchBinaryOperatorForGivenTypesTest0)
{
	// Add for array and int.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			var [ i32, 4 ] y= zero_init;
			x + y;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NoMatchBinaryOperatorForGivenTypes );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST(NoMatchBinaryOperatorForGivenTypesTest1)
{
	// Add for arrays.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			var [ i32, 4 ] y= zero_init;
			x + y;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NoMatchBinaryOperatorForGivenTypes );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST(FunctionSignatureMismatchTest0)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::FunctionSignatureMismatch );
	U_TEST_ASSERT( build_result.errors[0].file_pos.line == 5u );;
}

U_TEST(FunctionSignatureMismatchTest1)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::FunctionSignatureMismatch );
	U_TEST_ASSERT( build_result.errors[0].file_pos.line == 5u );
}

U_TEST(FunctionSignatureMismatchTest2)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionSignatureMismatch );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(ArraySizeIsNotInteger)
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 5.0f32 ] x;
			return;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ArraySizeIsNotInteger );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(ArraySizeIsNegative)
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, -5 ] x;
			return;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ArraySizeIsNegative );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST( ExpectedVariableInArraySizeTest0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, f64 ] x; // Array size iz typename, but expected variable
			return;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedVariableInArraySize );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(BreakOutsideLoopTest)
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			break;
			return;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BreakOutsideLoop );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(ContinueOutsideLoopTest)
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			continue;
			return;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ContinueOutsideLoop );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(NameIsNotTypeNameTest)
{
	static const char c_program_text[]=
	R"(
		fn Bar(){}
		fn Foo()
		{
			var Bar i;
			return;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameIsNotTypeName );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(UnreachableCodeTest0)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(UnreachableCodeTest1)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(UnreachableCodeTest2)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST(UnreachableCodeTest3)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.empty() );
}

U_TEST(UnreachableCodeTest4)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.empty() );
}

U_TEST(UnreachableCodeTest5)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST(UnreachableCodeTest6)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST(UnreachableCodeTest7)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST(UnreachableCodeTest8)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.empty() );
}

U_TEST(UnreachableCodeTest9)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.empty() );
}

U_TEST(NoReturnInFunctionReturningNonVoidTest0)
{
	// No return in non-void function;
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NoReturnInFunctionReturningNonVoid );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST(NoReturnInFunctionReturningNonVoidTest1)
{
	// Return not in all branches.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			if( true ) { return 0; }
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NoReturnInFunctionReturningNonVoid );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST(NoReturnInFunctionReturningNonVoidTest2)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NoReturnInFunctionReturningNonVoid );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST(NoReturnInFunctionReturningNonVoidTest3)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.empty() );
}

U_TEST(NoReturnInFunctionReturningNonVoidTest4)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.empty() );
}

U_TEST(ExpectedReferenceValueTest0)
{
	// Assign to non-reference value.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			1 + 2 = 42;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(ExpectedReferenceValueTest2)
{
	// Assign to value.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 a, i32 b )
		{
			42 = b;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(ExpectedReferenceValueTest3)
{
	// Assign to immutable value.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var f64 imut a= 3.1415926535f64;
			a = 0.0f64;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(ExpectedReferenceValueTest4)
{
	// Assign to immutable argument.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 imut a )
		{
			a = -45;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(ExpectedReferenceValueTest5)
{
	// Initialize reference using value-object.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 a= 42, b= 24;
			var i32 &x= a - b;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(ExpectedReferenceValueTest6)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(ExpectedReferenceValueTest7)
{
	// Using value in reference - function return value.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32 &
		{
			return 42;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(ExpectedReferenceValueTest8)
{
	// Non "const reference" value used in additive assignment.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 imut x= 2;
			x/= 5;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(ExpectedReferenceValueTest9)
{
	// Non "const reference" value used in additive assignment.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			5/= 5;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(ExpectedVariableInAssignmentTest0)
{
	static const char c_program_text[]=
	R"(
		struct C{}
		fn Bar(){}
		fn Foo()
		{
			var i32 x= 0;
			Bar= x; // assign variable to function
			x= Bar; // assign function to variable
			C= x;   // assign variable to struct
			x= C;   // assign struct to variable
			C= Bar; // assign function to struct
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= 6u );

	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::ExpectedVariableInAssignment );
	U_TEST_ASSERT( build_result.errors[0].file_pos.line == 7u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::ExpectedVariableInAssignment );
	U_TEST_ASSERT( build_result.errors[1].file_pos.line == 8u );
	U_TEST_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::ExpectedVariableInAssignment );
	U_TEST_ASSERT( build_result.errors[2].file_pos.line == 9u );
	U_TEST_ASSERT( build_result.errors[3].code == CodeBuilderErrorCode::ExpectedVariableInAssignment );
	U_TEST_ASSERT( build_result.errors[3].file_pos.line == 10u );
	U_TEST_ASSERT( build_result.errors[4].code == CodeBuilderErrorCode::ExpectedVariableInAssignment );
	U_TEST_ASSERT( build_result.errors[4].file_pos.line == 11u );
	U_TEST_ASSERT( build_result.errors[5].code == CodeBuilderErrorCode::ExpectedVariableInAssignment );
	U_TEST_ASSERT( build_result.errors[5].file_pos.line == 11u );
}

U_TEST(ExpectedVariableInBinaryOperatorTest0)
{
	static const char c_program_text[]=
	R"(
		struct C{}
		fn Bar(){}
		fn Foo()
		{
			var i32 x= 0;
			Bar + x;  // variable and function
			x / Bar;  // function and variable
			C * x;    // variable and struct
			x - C;    // struct and variable
			C == Bar; // function and struct
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= 6u );

	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::ExpectedVariableInBinaryOperator );
	U_TEST_ASSERT( build_result.errors[0].file_pos.line == 7u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::ExpectedVariableInBinaryOperator );
	U_TEST_ASSERT( build_result.errors[1].file_pos.line == 8u );
	U_TEST_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::ExpectedVariableInBinaryOperator );
	U_TEST_ASSERT( build_result.errors[2].file_pos.line == 9u );
	U_TEST_ASSERT( build_result.errors[3].code == CodeBuilderErrorCode::ExpectedVariableInBinaryOperator );
	U_TEST_ASSERT( build_result.errors[3].file_pos.line == 10u );
	U_TEST_ASSERT( build_result.errors[4].code == CodeBuilderErrorCode::ExpectedVariableInBinaryOperator );
	U_TEST_ASSERT( build_result.errors[4].file_pos.line == 11u );
	U_TEST_ASSERT( build_result.errors[5].code == CodeBuilderErrorCode::ExpectedVariableInBinaryOperator );
	U_TEST_ASSERT( build_result.errors[5].file_pos.line == 11u );
}

U_TEST(ExpectedVariableAsArgumentTest0)
{
	static const char c_program_text[]=
	R"(
		struct C{}
		fn Bar( i32 x ){}
		fn Foo()
		{
			var i32 x= 0;
			Bar( Bar ); // Function as argument
			Bar( C ); // struct as argument
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= 2u );

	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::ExpectedVariableAsArgument );
	U_TEST_ASSERT( build_result.errors[0].file_pos.line == 7u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::ExpectedVariableAsArgument );
	U_TEST_ASSERT( build_result.errors[1].file_pos.line == 8u );
}

U_TEST(ExpectedVariableInAdditiveAssignmentTest0)
{
	static const char c_program_text[]=
	R"(
		struct C{}
		fn Bar(){}
		fn Foo()
		{
			var i32 x= 0;
			Bar+= x;  // variable and function
			x/= Bar;  // function and variable
			C*= x;    // variable and struct
			x-= C;    // struct and variable
			C|= Bar; // function and struct
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= 6u );

	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::ExpectedVariableInAdditiveAssignment );
	U_TEST_ASSERT( build_result.errors[0].file_pos.line == 7u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::ExpectedVariableInAdditiveAssignment );
	U_TEST_ASSERT( build_result.errors[1].file_pos.line == 8u );
	U_TEST_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::ExpectedVariableInAdditiveAssignment );
	U_TEST_ASSERT( build_result.errors[2].file_pos.line == 9u );
	U_TEST_ASSERT( build_result.errors[3].code == CodeBuilderErrorCode::ExpectedVariableInAdditiveAssignment );
	U_TEST_ASSERT( build_result.errors[3].file_pos.line == 10u );
	U_TEST_ASSERT( build_result.errors[4].code == CodeBuilderErrorCode::ExpectedVariableInAdditiveAssignment );
	U_TEST_ASSERT( build_result.errors[4].file_pos.line == 11u );
	U_TEST_ASSERT( build_result.errors[5].code == CodeBuilderErrorCode::ExpectedVariableInAdditiveAssignment );
	U_TEST_ASSERT( build_result.errors[5].file_pos.line == 11u );
}

U_TEST(ExpectedVariableInIncrementOrDecrementTest0)
{
	static const char c_program_text[]=
	R"(
		struct C{}
		fn Bar(){}
		namespace NS{}
		fn Foo()
		{
			++Bar; // function
			--C;   // class name
			++u32; // type name
			--NS;  // namespace
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= 4u );

	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::ExpectedVariableInIncrementOrDecrement );
	U_TEST_ASSERT( build_result.errors[0].file_pos.line == 7u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::ExpectedVariableInIncrementOrDecrement );
	U_TEST_ASSERT( build_result.errors[1].file_pos.line == 8u );
	U_TEST_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::ExpectedVariableInIncrementOrDecrement );
	U_TEST_ASSERT( build_result.errors[2].file_pos.line == 9u );
	U_TEST_ASSERT( build_result.errors[3].code == CodeBuilderErrorCode::ExpectedVariableInIncrementOrDecrement );
	U_TEST_ASSERT( build_result.errors[3].file_pos.line == 10u );
}

U_TEST(CouldNotOverloadFunctionTest1)
{
	// Different are only mutability modifiers for value parameters.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 mut x ) {}
		fn Foo( i32 imut x ) {}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST(CouldNotOverloadFunctionTest2)
{
	// One parameter is value, other is const-reference.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 mut x ) {}
		fn Foo( i32 &imut x ) {}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST(CouldNotOverloadFunctionTest3)
{
	// Const and nonconst reference-parameters are different.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &mut x ) {}
		fn Foo( i32 &imut x ) {}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.empty() );
}

U_TEST(CouldNotSelectOverloadedFunction0)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotSelectOverloadedFunction );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST(CouldNotSelectOverloadedFunction1)
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotSelectOverloadedFunction );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST(FunctionPrototypeDuplicationTest0)
{
	// Simple prototype duplication.
	static const char c_program_text[]=
	R"(
		fn Bar();
		fn Bar();
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionPrototypeDuplication );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST(FunctionPrototypeDuplicationTest1)
{
	// Functions with args of same type but different name is same.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x, f64 y );
		fn Bar( i32 xx, f64 yy );
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionPrototypeDuplication );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST(FunctionPrototypeDuplicationTest2)
{
	// Prototype after body
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x ){}
		fn Bar( i32 x );
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionPrototypeDuplication );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST(FunctionBodyDuplicationTest0)
{
	// Simple body duplication.
	static const char c_program_text[]=
	R"(
		fn Bar(){}
		fn Bar(){}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionBodyDuplication );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST(FunctionBodyDuplicationTest1)
{
	// Functions with args of same type but different name is same.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x, f64 y ){}
		fn Bar( i32 xx, f64 yy ){}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionBodyDuplication );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST(ReturnValueDiffersFromPrototypeTest0)
{
	// Different return value type.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x, f64 y ) : i32;
		fn Bar( i32 x, f64 y ) : bool
		{
			return false;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReturnValueDiffersFromPrototype );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST(ReturnValueDiffersFromPrototypeTest1)
{
	// Different return value mutability.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x, f64 y ) : i32 imut;
		fn Bar( i32 x, f64 y ) : i32 mut
		{
			return 0;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReturnValueDiffersFromPrototype );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST(ReturnValueDiffersFromPrototypeTest2)
{
	// Different reference modificator.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x, f64 y ) : i32&;
		fn Bar( i32 x, f64 y ) : i32
		{
			return 0;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReturnValueDiffersFromPrototype );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST(ReturnValueDiffersFromPrototypeTest3)
{
	// Different mutability of returned reference-value.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x, f64 y ) : i32 &imut;
		fn Bar( i32 x, f64 y ) : i32 &mut
		{
			return 0;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReturnValueDiffersFromPrototype );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

} // namespace U
