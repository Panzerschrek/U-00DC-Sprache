#include "cpp_tests.hpp"

namespace U
{

namespace
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

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 11u );
}

U_TEST( ReferncesTagsTest_TryReturnUnallowedReference0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &'a x, i32 &'b y ) : i32 &'a imut
		{
			return y; // returning of "y" does not allowed.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReturningUnallowedReference );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST( ReferncesTagsTest_TryReturnUnallowedReference1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 & x, i32 &'b y ) : i32 &'b imut    // "x" untagged and can not be returned, because return value tagged
		{
			return x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReturningUnallowedReference );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST( ReferncesTagsTest_ReturnReferenceToGlobalConstant0 )
{
	static const char c_program_text[]=
	R"(
		auto constexpr ccc= 5654;
		fn Foo() : i32 &imut
		{
			return ccc; // Ok, return reference to global constant
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReferncesTagsTest_ReturnReferenceToGlobalConstant1 )
{
	static const char c_program_text[]=
	R"(
		auto constexpr ccc= 5654;
		fn PositiveVarOrZero( i32 &'a imut x ) : i32 &'a imut
		{
			if( x > 0 ) { return x; } // Ok, return global constant
			return ccc; // Ok, return allowed reference.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReferncesTagsTest_UntaggedReturValueMustBeOk )
{
	static const char c_program_text[]=
	R"(
		auto constexpr ccc= 5654;
		fn Foo( i32 &'a x, i32 &'b y, i32 & z ) : i32 & // Return value is untagged => can return reference to any argument
		{
			if( x >= y && x >= z ) { return x; }
			if( y >= x && y >= z ) { return y; }
			return z;
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( NameNotFound_ForReturnReferenceTag_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &'F x ) : i32 &'unexistent_tag
		{
			return x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( NameNotFound_ForReturnReferenceTag_Test1 )
{
	static const char c_program_text[]=
	R"(
		auto constexpr XXX= 457;
		fn Foo() : i32 &'unexistent_tag
		{
			return XXX;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( ImplicitThisTag )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn GetXRef( mut this, i32 &mut other ) : i32 &'this mut // use implicitly defined for "this" argument "this" tag.
			{
				x= other;
				return x;
			}
		}
		fn Foo()
		{
			var S mut s= zero_init;
			auto mut y= 0;
			auto &mut r= s.GetXRef(y); // "r" referes to "s" only.
			y= 2; // ok, can do this.
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace

} // namespace U
