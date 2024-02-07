#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( FunctionDeclarationOutsideItsScopeTest0 )
{
	// Bar doesn`t exists.
	static const char c_program_text[]=
	R"(
		fn Bar::Foo();
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( FunctionDeclarationOutsideItsScopeTest1 )
{
	// Function with same name exists, nut new declaration have different type.
	static const char c_program_text[]=
	R"(
		namespace Bar
		{
			fn Foo( i32 x );
		}
		fn Bar::Foo( f32 x );
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionDeclarationOutsideItsScope );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( FunctionDeclarationOutsideItsScopeTest2 )
{
	// Namespace exists, but not contains name.
	static const char c_program_text[]=
	R"(
		namespace Bar{}
		fn Bar::Foo();
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( Redefinition_ForNamespaces_Test0 )
{
	// Namespace have same name, as differend thing in same scope.
	static const char c_program_text[]=
	R"(
		fn Bar(){}
		namespace Bar
		{}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( UsingKeywordAsName_ForNamespaces_Test0 )
{
	static const char c_program_text[]=
	R"(
		namespace struct {}
		namespace this {}
		namespace public {}
		namespace virtual {}
		namespace break {}
		namespace pure {}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= 6 );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::UsingKeywordAsName, 2 ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::UsingKeywordAsName, 3 ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::UsingKeywordAsName, 4 ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::UsingKeywordAsName, 5 ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::UsingKeywordAsName, 6 ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::UsingKeywordAsName, 7 ) );
}

} // namespace

} // namespace U
