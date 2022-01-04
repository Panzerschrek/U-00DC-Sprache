#include "tests.hpp"

namespace U
{

namespace
{

U_TEST( CopyAssignmentOperatorForStructsWithReferencesDeleted )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut r;
		}

		fn Foo()
		{
			var i32 mut x= 42, mut y= 34;
			var S mut s0{ .r= x };
			var S mut s1{ .r= y };
			s0= s1;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( error.src_loc.GetLine() == 12u );
}

U_TEST( StructsWithReferencesHaveNoGeneratedDefaultConstructor )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut r;
		}

		fn Foo()
		{
			var S s; // Needs connstructor or initializer.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	U_TEST_ASSERT( error.src_loc.GetLine() == 9u );
}

U_TEST( BindingConstReferenceToNonConstReference_InReferenceFieldInitialization_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut r; }
		fn Foo()
		{
			auto imut x= 42;
			var S s{ .r= x };
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( BindingConstReferenceToNonConstReference_InReferenceFieldInitialization_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut r;
			fn constructor( i32 &imut in_r )
			( r= in_r )
			{}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( ExpectedVariable_InStructReferenceInitialization )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut r;
			fn constructor( i32 &imut in_r )
			( r= i32 ) // type name instead of variable
			{}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 6u ) );
}

U_TEST( ExpectedReferenceValue_InStructReferenceInitialization )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut r;
			fn constructor( i32 &imut in_r )
			( r= 42 ) // got value instead reference
			{}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( AssignToImmutableReferenceInsideStruct_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &imut r;
			fn Assign( mut this, i32 x )
			{
				r= x; // Assign, using implicit this
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.src_loc.GetLine() == 7u );
}

U_TEST( AssignToImmutableReferenceInsideStruct_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &imut r; }
		fn Foo()
		{
			auto x= 0;
			var S s{ .r= x };
			s.r= 45; // Assign, using member access
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.src_loc.GetLine() == 7u );
}

} // namespace

} // namespace U
