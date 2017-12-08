#include "tests.hpp"

namespace U
{

U_TEST( ReferncesTagsTest_BaseReferencesDefinition0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &'a x, i32 &'b y ) : i32 &'a imut
		{
			return x;
		}

		fn Bar()
		{
			var i32 mut a= 0, mut b= 0;
			auto & r= Foo( a, b ); // "r" refers only to "a", not "b".
			b= 42; // So, we can mutate "b".
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReferncesTagsTest_BaseReferencesDefinition1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &'a x, i32 &'b y ) : i32 &'a imut
		{
			return x;
		}

		fn Bar()
		{
			var i32 mut a= 0, mut b= 0;
			auto & r= Foo( a, b ); // "r" refers only to "a", not "b".
			a= 42; // So, we can not mutate "a".
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 11u );
}

} // namespace U
