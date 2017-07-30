#include "tests.hpp"

namespace U
{

U_TEST( FunctionDeclarationOutsideItsScopeTest0 )
{
	static const char c_program_text[]=
	R"(
		fn Bar::Foo();
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionDeclarationOutsideItsScope );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST( FunctionDeclarationOutsideItsScopeTest1 )
{
	static const char c_program_text[]=
	R"(
		namespace Bar
		{
			fn Foo( i32 x );
		}
		fn Bar::Foo( f32 x );
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionDeclarationOutsideItsScope );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( Redefenition_ForNamespaces_Test0 )
{
	// Namespace have same name, as differend thing in same scope.
	static const char c_program_text[]=
	R"(
		fn Bar(){}
		namespace Bar
		{}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

} // namespace U
