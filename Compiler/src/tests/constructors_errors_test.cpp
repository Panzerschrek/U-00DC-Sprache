#include "tests.hpp"

namespace U
{

U_TEST(ConstructorOutsideClassTest0)
{
	// Constructor body.
	static const char c_program_text[]=
	R"(
		fn constructor(){}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ConstructorOutsideClass );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST(ConstructorOutsideClassTest1)
{
	// Constructor prototype.
	static const char c_program_text[]=
	R"(
		fn constructor();
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ConstructorOutsideClass );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST(InitializationListInNonconstructorTest0)
{
	// Initialization list in nonclass function.
	static const char c_program_text[]=
	R"(
		fn Bar() : i32
		()
		{ return 0;}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InitializationListInNonconstructor );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST(InitializationListInNonconstructorTest1)
{
	// Initialization list in method.
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Bar() : i32
			()
			{ return 0;}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InitializationListInNonconstructor );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(ClassHaveNoConstructorsTest0)
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; }
		fn Foo()
		{
			var S s();
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ClassHaveNoConstructors );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(ExplicitThisInConstructorParamtersTest0)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor( this )
			( x(0) )
			{}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExplicitThisInConstructorParamters );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(FieldIsNotInitializedYetTest0)
{
	// Initialize field, using unitialized field value.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x; i32 y;
			fn constructor()
			(
				x(y),
				y(0) )
			{}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FieldIsNotInitializedYet );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST(FieldIsNotInitializedYetTest1)
{
	// Field self-initialization.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor()
			( x(x) )
			{}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FieldIsNotInitializedYet );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST(MethodsCallInConstructorInitializerListIsForbiddenTest0)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn Bar( this ) : i32{ return 0; }
			fn constructor()
			( x(Bar()) )
			{}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::MethodsCallInConstructorInitializerListIsForbidden );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}


U_TEST( ThisUnavailable_InConstructors_Test0 )
{
	// Field self-initialization.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor()
			( x(this.x) )
			{}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ThisUnavailable );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( ThisUnavailable_InConstructors_Test1 )
{
	// Access to method in constructor, using "this".
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn Bar( this ) : i32 { return 0; }
			fn constructor()
			( x(this.Bar()) )
			{}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ThisUnavailable );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST( MissingStructMemberInitializer_InConstructors_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			i32 y;
			fn constructor()
			( y(0) ) // x left uninitialized
			{}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::MissingStructMemberInitializer );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST( InitializerForNonfieldStructMember_InConstructors_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Bar(){}
			fn constructor()
			( Bar(0) )
			{}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InitializerForNonfieldStructMember );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( DuplicatedStructMemberInitializer_InConstructors_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x; i32 y;
			fn constructor()
			( y(0), x(-1), y(5) )
			{}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DuplicatedStructMemberInitializer );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( DefaultConstructorNotFoundTest0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor( i32 a )
			( x(a) )
			{}
		}
		fn Foo()
		{
			var S s; // Default constructor is missing - needs explicit initialization.
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionSignatureMismatch );
	//U_TEST_ASSERT( error.file_pos.line == 11u );
}

} // namespace U
