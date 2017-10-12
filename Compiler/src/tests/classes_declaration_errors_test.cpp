#include "tests.hpp"

namespace U
{

U_TEST( ClassDeclarationOutsideItsScopeTest0 )
{
	static const char c_program_text[]=
	R"(
		struct Foo::Bar;
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ClassDeclarationOutsideItsScope );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST( ClassDeclarationOutsideItsScopeTest1 )
{
	static const char c_program_text[]=
	R"(
		struct Foo::Bar{}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ClassDeclarationOutsideItsScope );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST( ClassBodyDuplicationTest0 )
{
	static const char c_program_text[]=
	R"(
		struct Bar{}
		struct Bar{}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ClassBodyDuplication );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST( ClassBodyDuplicationTest1 )
{
	static const char c_program_text[]=
	R"(
		namespace Foo{ struct Bar{} }
		struct Foo::Bar{}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ClassBodyDuplication );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST( Redefinition_ForClassDeclarations_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct Foo;
		struct Foo;
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST( UsingIncompleteTypeTest0 )
{
	// Incomplete struct in variable declaration.
	static const char c_program_text[]=
	R"(
		struct S;
		fn Foo()
		{
			var S s= zero_init;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( UsingIncompleteTypeTest1 )
{
	// Array of incomplete structs in variable declaration.
	static const char c_program_text[]=
	R"(
		struct S;
		fn Foo()
		{
			var [ S, 4 ] s= zero_init;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( UsingIncompleteTypeTest2 )
{
	// Access to incomplete type.
	static const char c_program_text[]=
	R"(
		struct S;
		fn Foo( S& s )
		{
			s.x;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( UsingIncompleteTypeTest3 )
{
	// Field of incomplete type.
	static const char c_program_text[]=
	R"(
		struct S;
		struct Bar{ S s; }
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST( UsingIncompleteTypeTest4 )
{
	// Field of incomplete "this" type.
	static const char c_program_text[]=
	R"(
		struct Bar{ Bar bar; }
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST( UsingIncompleteTypeTest5 )
{
	// Arg of inline method of incomplete type.
	static const char c_program_text[]=
	R"(
		struct X;
		struct Y
		{
			fn Foo( X x ) {}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

} // namespace U
