#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( ReferncesTagsTest_BaseReferencesDefinition0 )
{
	static const char c_program_text[]=
	R"(
		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		fn Foo( i32 & x, i32 & y ) : i32 & imut @(return_references)
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
		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		fn Foo( i32 & x, i32 & y ) : i32 & imut @(return_references)
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
	U_TEST_ASSERT( error.src_loc.GetLine() == 12u );
}

U_TEST( ReferncesTagsTest_TryReturnUnallowedReference0 )
{
	static const char c_program_text[]=
	R"(
		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		fn Foo( i32 & x, i32 & y ) : i32 &imut @(return_references)
		{
			return y; // returning of "y" does not allowed.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReturningUnallowedReference );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( ReferncesTagsTest_TryReturnUnallowedReference1 )
{
	static const char c_program_text[]=
	R"(
		var [ [ char8, 2 ], 1 ] return_references[ "1_" ];
		fn Foo( i32 & x, i32 & y ) : i32 &imut @(return_references)
		{
			return x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReturningUnallowedReference );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( ReferncesTagsTest_TryReturnUnallowedReference2 )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32& x; }
		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		// No reference notation is specified here for inner return references.
		fn Foo( S& s ) : S & @(return_references)
		{
			// Returning "0a" inside "s" - it's not allowed.
			return s;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReturningUnallowedReference, 8u ) );
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
		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		fn PositiveVarOrZero( i32 & imut x ) : i32 &imut @(return_references)
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
		fn Foo( i32 & x, i32 & y, i32 & z ) : i32 & // Return value is untagged => can return reference to any argument
		{
			if( x >= y && x >= z ) { return x; }
			if( y >= x && y >= z ) { return y; }
			return z;
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ThisTag )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
			fn GetXRef( mut this, i32 &mut other ) : i32 &mut @(return_references)
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
