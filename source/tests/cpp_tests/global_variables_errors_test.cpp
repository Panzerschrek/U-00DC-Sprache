#include "tests.hpp"

namespace U
{

U_TEST( GlobalVariableMustBeConstexpr_Test0 )
{
	static const char c_program_text[]=
	R"(
		auto mut x= 42;
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::GlobalVariableMustBeConstexpr );
	U_TEST_ASSERT( error.file_pos.GetLine() == 2u );
}

} // namespace U
