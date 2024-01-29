#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( FunctionBodyDuplication_ForDestructors_Test0 )
{
	static const char c_program_text[]=
	R"(
		class S
		{
			fn destructor(){}
			fn destructor(){}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );

	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionBodyDuplication );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( DestructorOutsideClassTest0 )
{
	static const char c_program_text[]=
	R"(
		fn destructor();
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );

	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ConstructorOrDestructorOutsideClass );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( DestructorMustReturnVoidTest0 )
{
	static const char c_program_text[]=
	R"(
		class S
		{
			fn destructor() : i32 { return 0; }
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );

	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ConstructorAndDestructorMustReturnVoid );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST( ExplicitArgumentsInDestructorTest1 )
{
	static const char c_program_text[]=
	R"(
		class S
		{
			fn destructor( i32 a ) {}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );

	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExplicitArgumentsInDestructor );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST( MutableReferenceFieldAccessInDestructor_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut x;
			fn destructor()
			{
				auto &mut x_ref= x; // Access mutable reference field here.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::MutableReferenceFieldAccessInDestructor, 7u ) );
}

U_TEST( MutableReferenceFieldAccessInDestructor_Test1 )
{
	static const char c_program_text[]=
	R"(
		class S
		{
			u64 &mut x;
			fn destructor()
			{
				auto x_val= x; // Even reading of mutable reference is not allowed.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::MutableReferenceFieldAccessInDestructor, 7u ) );
}

U_TEST( MutableReferenceFieldAccessInDestructor_Test2 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			f32 &mut x;
			fn destructor()
			{
				Bar(x); // Access mutable reference field here by passing it into a function.
			}
		}
		fn Bar( f32 &mut x );
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::MutableReferenceFieldAccessInDestructor, 7u ) );
}

U_TEST( AccessingFieldWithMutableReferencesInsideInDestructor_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut x;
		}
		struct T
		{
			S s;
			fn destructor()
			{
				auto &s_ref= s; // Even taking immutable reference to a field with mutable references inside isn't allowed.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::AccessingFieldWithMutableReferencesInsideInDestructor, 11u ) );
}

U_TEST( AccessingFieldWithMutableReferencesInsideInDestructor_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut x;
		}
		struct T
		{
			tup[ bool, S, i32 ] s;
			fn destructor()
			{
				auto i= s[2]; // Even access to tuple member of field with mutable references inside isn't allowed.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::AccessingFieldWithMutableReferencesInsideInDestructor, 11u ) );
}

U_TEST( AccessingFieldWithMutableReferencesInsideInDestructor_Test2 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut x;
		}
		struct T
		{
			S s;
			fn destructor()
			{
				Foo(s); // Passing a reference to a field with mutable references inside into a function isn't allowed.
			}
		}
		fn Foo(S& s);
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::AccessingFieldWithMutableReferencesInsideInDestructor, 11u ) );
}

U_TEST( ThisUnavailable_InDestructorOfStructWithReferencesInside_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut x;
			fn destructor()
			{
				auto& self= this; // "this" is unavailable, because this struct contains mutable references inside.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ThisUnavailable, 7u ) );
}

U_TEST( ThisUnavailable_InDestructorOfStructWithReferencesInside_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut x;
			fn destructor()
			{
				Foo(); // "this" is unavailable, because this struct contains mutable references inside. So, this-call method isn't accessible too.
			}
			fn Foo(this);
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::CouldNotSelectOverloadedFunction, 7u ) );
}


} // namespace

} // namespace U
