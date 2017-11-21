#include "tests.hpp"


namespace U
{

U_TEST( ReferenceCheckTest_MultipleMutableReferencesOnStack )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			auto &mut r0= x;
			auto &mut r1= x;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( ReferenceCheckTest_MutableReferenceAfterImmutableReferenceOnStack )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			auto &imut r0= x;
			auto & mut r1= x;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( ReferenceCheckTest_ImmutableReferenceAfterMutableReferenceOnStack )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			auto & mut r0= x;
			auto &imut r1= x;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( ReferenceCheckTest_MultipleImmutableReferencesShouldBeOk )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			auto &imut r0= x;
			auto &imut r1= x;
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReferenceCheckTest_MultipleMutableReferencesPassedToFunction )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x, i32 &mut y ) {}
		fn Foo()
		{
			var i32 x= 0;
			Bar( x, x );
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( ReferenceCheckTest_MutableAndImmutableReferencesPassedToFunction )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &imut x, i32 &mut y ) {}
		fn Foo()
		{
			var i32 x= 0;
			Bar( x, x );
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( ReferenceCheckTest_MultipleImmutableReferencesPassedToFunctionShouldBeOk )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &imut x, i32 &imut y ) {}
		fn Foo()
		{
			var i32 x= 0;
			Bar( x, x );
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace U
