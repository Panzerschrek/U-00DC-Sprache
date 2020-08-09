#include "tests.hpp"

namespace U
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

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ClassBodyDuplication );
	U_TEST_ASSERT( error.file_pos.GetLine() == 3u );
}

U_TEST( Redefinition_ForClassDeclarations_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct Foo;
		struct Foo;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.file_pos.GetLine() == 3u );
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

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( error.file_pos.GetLine() == 5u );
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

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( error.file_pos.GetLine() == 5u );
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

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT(
		HaveError( build_result.errors, CodeBuilderErrorCode::UsingIncompleteType, 3u ) ||
		HaveError( build_result.errors, CodeBuilderErrorCode::UsingIncompleteType, 5u ) );
}

U_TEST( UsingIncompleteTypeTest3 )
{
	// Field of incomplete type.
	static const char c_program_text[]=
	R"(
		struct S;
		struct Bar{ S s; }
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( error.file_pos.GetLine() == 3u );
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

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( error.file_pos.GetLine() == 5u );
}

U_TEST( UsingIncompleteTypeTest6 )
{
	// Look inside incomplete class.
	static const char c_program_text[]=
	R"(
		struct X;
		type b= X::diff;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT(
		HaveError( build_result.errors, CodeBuilderErrorCode::UsingIncompleteType, 3u ) ||
		HaveError( build_result.errors, CodeBuilderErrorCode::NameNotFound, 3u ) );
}

U_TEST( UsingIncompleteTypeTest7 )
{
	// Value arg for function with body is incomplete.
	static const char c_program_text[]=
	R"(
		struct X;
		fn Foo( X x ) {}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( error.file_pos.GetLine() == 3u );
}

U_TEST( UsingIncompleteTypeTest8 )
{
	// Value arg for function without body is incomplete. Must be ok.
	static const char c_program_text[]=
	R"(
		struct X;
		fn Foo( X x );
	)";

	BuildProgram( c_program_text );
}

U_TEST( UsingIncompleteTypeTest9 )
{
	// Reference tag competeness required for reference arguments in functions body.
	static const char c_program_text[]=
	R"(
		struct X;
		fn Foo( X& x ){}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( error.file_pos.GetLine() == 3u );
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

U_TEST( UsingIncompleteTypeTest11 )
{
	// Returning value of function with body have incomplete type.
	static const char c_program_text[]=
	R"(
		struct X;
		fn Foo() : X {}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( error.file_pos.GetLine() == 3u );
}

U_TEST( UsingIncompleteTypeTest12 )
{
	// Returning value of function with body must be ok.
	static const char c_program_text[]=
	R"(
		struct X;
		fn Foo() : X;
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

U_TEST( UsingIncompleteTypeTest14 )
{
	// Returning reference to incomplete type must be ok for functions with and without body.
	static const char c_program_text[]=
	R"(
		struct X;
		fn Foo() : X&;
		fn Bar() : X& { return Foo(); }
	)";

	BuildProgram( c_program_text );
}

U_TEST( UsingIncompleteTypeTest15 )
{
	// Calling function, returning value of incomplete type.
	static const char c_program_text[]=
	R"(
		struct X;
		fn Foo() : X;

		fn Bar()
		{
			Foo();
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( error.file_pos.GetLine() == 7u );
}

} // namespace U
