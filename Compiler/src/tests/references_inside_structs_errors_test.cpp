#include "tests.hpp"

namespace U
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( error.file_pos.line == 12u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	U_TEST_ASSERT( error.file_pos.line == 9u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.file_pos.line == 6u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

} // namespace U
