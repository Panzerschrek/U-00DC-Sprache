#include "tests.hpp"

namespace U
{

U_TEST( Redefinition_ForEnums )
{
	static const char c_program_text[]=
	R"(
		struct S{}
		enum S{}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST( Redefinition_ForEnumMembers )
{
	static const char c_program_text[]=
	R"(
		enum E
		{
			A, B, C,
			B,
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( EnumsRestrictionsTest )
{
	static const char c_program_text[]=
	R"(
		enum E{ a, b }
		fn Foo()
		{
			E::a + E::b; // basic arithmetic operations for enums forbidden
			-E::a; // unary minus for enums forbidden
			var i32 x= E::a; // Implicit conversion to int forbidden
			var E y= E::b;
			y+= E::a; // additive assignment operations forbidden
			E::a | E::b; // Logical operations forbidden
			E::a && E::b; // Lazy logical operations forbidden
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.size() >= 6u );

	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[0].file_pos.line == 5u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[1].file_pos.line == 6u );
	U_TEST_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( build_result.errors[2].file_pos.line == 7u );
	U_TEST_ASSERT( build_result.errors[3].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[3].file_pos.line == 9u );
	U_TEST_ASSERT( build_result.errors[4].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[4].file_pos.line == 10u );
	U_TEST_ASSERT( build_result.errors[5].code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( build_result.errors[5].file_pos.line == 11u );
}

} // namespace U
