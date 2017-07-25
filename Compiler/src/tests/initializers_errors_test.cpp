#include <cstdlib>
#include <iostream>

#include "tests.hpp"

namespace U
{

U_TEST(ExpectedInitializerTest0)
{
	// Expected initializer for fundamental variable.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	//U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(ExpectedInitializerTest1)
{
	// Expected initializer for array.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 1024 ] x;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	//U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(ExpectedInitializerTest2)
{
	// Expected initializer for struct.
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; }
		fn Foo()
		{
			var S s;
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	//U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(ArrayInitializerForNonArrayTest0)
{
	// Array initializer for fundamental type.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x[ 5, 6, 7 ];
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ArrayInitializerForNonArray );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(ArrayInitializerForNonArrayTest1)
{
	// Array initializer for structes.
	static const char c_program_text[]=
	R"(
		struct C{}
		fn Foo()
		{
			var C x[ 5, 6, 7 ];
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ArrayInitializerForNonArray );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(ArrayInitializersCountMismatchTest0)
{
	// Not enough initializers.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 3u32 ] x[ 1 ];
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ArrayInitializersCountMismatch );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(ArrayInitializersCountMismatchTest1)
{
	// Too much initializers.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 3u32 ] x[ 1, 2, 3, 4, 5 ];
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ArrayInitializersCountMismatch );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}


U_TEST(FundamentalTypesHaveConstructorsWithExactlyOneParameterTest0)
{
	// Not enough parameters in constructor.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x();
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FundamentalTypesHaveConstructorsWithExactlyOneParameter );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(FundamentalTypesHaveConstructorsWithExactlyOneParameterTest1)
{
	// Too much parameters in constructor.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x( 0, 1, 2 );
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FundamentalTypesHaveConstructorsWithExactlyOneParameter );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(ReferencesHaveConstructorsWithExactlyOneParameterTest0)
{
	// Not enough parameters in constructor.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 & x();
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferencesHaveConstructorsWithExactlyOneParameter );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(ReferencesHaveConstructorsWithExactlyOneParameterTest1)
{
	// Too much parameters in constructor.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 z= 0;
			var i32 & x( z, z );
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferencesHaveConstructorsWithExactlyOneParameter );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(UnsupportedInitializerForReferenceTest0)
{
	// Array initializer for reference.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 z= 0;
			var i32 & x[ z ];
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnsupportedInitializerForReference );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(ConstructorInitializerForUnsupportedTypeTest0)
{
	// Constructor initializer for array.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 2u32 ] x( 0, 1, 2 );
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ConstructorInitializerForUnsupportedType );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(StructInitializerForNonStructTest0)
{
	// Struct initializer for array.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 2u32 ] x{};
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::StructInitializerForNonStruct );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(InitializerForNonfieldStructMemberTest0)
{
	// Struct initializer for array.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn Foo(){}
		}
		fn Foo()
		{
			var S s{ .x= 0, .Foo= 0 };
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InitializerForNonfieldStructMember );
	U_TEST_ASSERT( error.file_pos.line == 9u );
}

U_TEST(DuplicatedStructMemberInitializerTest0)
{
	static const char c_program_text[]=
	R"(
		struct Point{ i32 x; i32 y; }
		fn Foo()
		{
			var Point point{ .x= 42, .y= 34, .x= 0 };
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DuplicatedStructMemberInitializer );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(MissingStructMemberInitializerTest0)
{
	static const char c_program_text[]=
	R"(
		struct Point{ [ i32, 2 ] xy; i32 z; }
		fn Foo()
		{
			var Point point{ .xy[ 54, -785 ], };
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::MissingStructMemberInitializer );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}


} // namespace U
