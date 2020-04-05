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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ConstructorOrDestructorOutsideClass );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST(ConstructorOutsideClassTest1)
{
	// Constructor prototype.
	static const char c_program_text[]=
	R"(
		fn constructor();
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ConstructorOrDestructorOutsideClass );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST(ConstructorMustReturnVoidTest0)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn constructor() : i32{}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ConstructorAndDestructorMustReturnVoid );
	U_TEST_ASSERT( error.file_pos.line == 4u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InitializationListInNonconstructor );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FieldIsNotInitializedYet );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST(FieldIsNotInitializedYetTest2)
{
	// Accessing base class field before "base" initialization.
	static const char c_program_text[]=
	R"(
		class A polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
		}
		class B : A
		{
			i32 y;
			fn constructor() ( y= x, base(42) ) {}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::FieldIsNotInitializedYet, 10u ) );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotSelectOverloadedFunction );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ThisUnavailable );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST( ExpectedInitializer_InConstructors_Test0 )
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST( ExpectedInitializer_InConstructors_Test1 )
{
	// Expected initializer for reference type
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 & x;
			i32 y;
			fn constructor()
			( y(0) ) // x left uninitialized
			{}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	U_TEST_ASSERT( error.file_pos.line == 11u );
}

U_TEST( DefaultConstructorNotFoundTest1 )
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			fn constructor() ( x= 2017 ) {}
		}
		struct B
		{
			A a;
			fn constructor( i32 x )
			{
				a.x= x;
			}
		}
		fn Foo()
		{
			var B b; // Default consructor for class "B" not generated, because this class have explicit noncopy constructor.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	U_TEST_ASSERT( error.file_pos.line == 17u );
}

U_TEST( DefaultConstructorNotFoundTest2 )
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			[ [ [ i32, 2 ], 3 ], 5 ] arr_3d;
		}
		fn Foo()
		{
			var A a; // Default consructor for class "B" not generated, because it contains non-default-constructible members.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	U_TEST_ASSERT( error.file_pos.line == 8u );
}

U_TEST( InitializerForBaseClassField_Test0 )
{
	static const char c_program_text[]=
	R"(
		class A polymorph
		{
			i32 a;
			fn constructor() ( a= 0 ) {}
		}
		class B : A
		{
			fn constructor() ( a= 42 ) {}   // Try initialize field of direct base
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InitializerForBaseClassField );
	U_TEST_ASSERT( error.file_pos.line == 9u );
}

U_TEST( InitializerForBaseClassField_Test1 )
{
	static const char c_program_text[]=
	R"(
		class A polymorph
		{
			i32 a;
			fn constructor() ( a= 0 ) {}
		}
		class B : A{}
		class C : B
		{
			fn constructor() ( a= 42 ) {}   // Try initialize field of non-direct base base
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InitializerForBaseClassField );
	U_TEST_ASSERT( error.file_pos.line == 10u );
}

} // namespace U
