#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( ClassBodyDuplicationTest0 )
{
	static const char c_program_text[]=
	R"(
		struct Bar{}
		struct Bar{}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( UsingIncompleteTypeTest10 )
{
	// Value arg of parent class type must be ok.
	static const char c_program_text[]=
	R"(
		struct X
		{
			fn Foo( X x ){}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( UsingIncompleteTypeTest13 )
{
	// Returning value of parent class type must be ok.
	static const char c_program_text[]=
	R"(
		struct X
		{
			fn Make() : X { return X(); }
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace

} // namespace U
