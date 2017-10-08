#include "tests.hpp"

namespace U
{

U_TEST( GlobalVariableMustBeConstexpr_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo() : i32 { return 0; }
		var i32 x= Foo();  // Initializer expression is not constant.
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::GlobalVariableMustBeConstexpr );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST( GlobalVariableMustBeConstexpr_Test1 )
{
	static const char c_program_text[]=
	R"(
		var f32 constexpr pi= 3.1415926535f;
		fn GetPi() : f32 &imut { return pi; }
		var f32 &imut pi_ref= GetPi();  // Initializer expression for global reference is not constant.
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::GlobalVariableMustBeConstexpr );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST( GlobalVariableMustBeConstexpr_Test2 )
{
	static const char c_program_text[]=
	R"(
		fn GetTwo() : u32 { return 2u; }
		auto imut x= 2u * GetTwo();  // Initializer expression is not constant.
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::GlobalVariableMustBeConstexpr );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

} // namespace U
