#include <cstdlib>
#include <iostream>

#include "../assert.hpp"
#include "tests.hpp"

#include "initializers_errors_test.hpp"

namespace U
{

static void ArrayInitializerForNonArrayTest0()
{
	// Arrai initializer for fundamental type.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let : i32 x[ 5, 6, 7 ];
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ArrayInitializerForNonArray );
	U_ASSERT( error.file_pos.line == 4u );
}

static void ArrayInitializerForNonArrayTest1()
{
	// Array initializer for classes.
	static const char c_program_text[]=
	R"(
		class C{}
		fn Foo()
		{
			let : C x[ 5, 6, 7 ];
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ArrayInitializerForNonArray );
	U_ASSERT( error.file_pos.line == 5u );
}

static void ArrayInitializersCountMismatchTest0()
{
	// Not enough initializers.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let : [ i32, 3u32 ] x[ 1 ];
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ArrayInitializersCountMismatch );
	U_ASSERT( error.file_pos.line == 4u );
}

static void ArrayInitializersCountMismatchTest1()
{
	// Too much initializers.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let : [ i32, 3u32 ] x[ 1, 2, 3, 4, 5 ];
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ArrayInitializersCountMismatch );
	U_ASSERT( error.file_pos.line == 4u );
}


static void FundamentalTypesHaveConstructorsWithExactlyOneParameterTest0()
{
	// Not enough parameters in constructor.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let : i32 x();
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::FundamentalTypesHaveConstructorsWithExactlyOneParameter );
	U_ASSERT( error.file_pos.line == 4u );
}

static void FundamentalTypesHaveConstructorsWithExactlyOneParameterTest1()
{
	// Too much parameters in constructor.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let : i32 x( 0, 1, 2 );
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::FundamentalTypesHaveConstructorsWithExactlyOneParameter );
	U_ASSERT( error.file_pos.line == 4u );
}

static void ReferencesHaveConstructorsWithExactlyOneParameterTest0()
{
	// Not enough parameters in constructor.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let : i32 & x();
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ReferencesHaveConstructorsWithExactlyOneParameter );
	U_ASSERT( error.file_pos.line == 4u );
}

static void ReferencesHaveConstructorsWithExactlyOneParameterTest1()
{
	// Too much parameters in constructor.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let : i32 z= 0;
			let : i32 & x( z, z );
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ReferencesHaveConstructorsWithExactlyOneParameter );
	U_ASSERT( error.file_pos.line == 5u );
}

static void UnsupportedInitializerForReferenceTest0()
{
	// Array initializer for reference.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let : i32 z= 0;
			let : i32 & x[ z ];
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::UnsupportedInitializerForReference );
	U_ASSERT( error.file_pos.line == 5u );
}

static void ConstructorInitializerForUnsupportedTypeTest0()
{
	// Constructor initializer for array.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let : [ i32, 2u32 ] x( 0, 1, 2 );
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::ConstructorInitializerForUnsupportedType );
	U_ASSERT( error.file_pos.line == 4u );
}

static void StructInitializerForNonStructTest0()
{
	// Struct initializer for array.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			let : [ i32, 2u32 ] x{};
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::StructInitializerForNonStruct );
	U_ASSERT( error.file_pos.line == 4u );
}

static void DuplicatedStructMemberInitializerTest0()
{
	static const char c_program_text[]=
	R"(
		class Point{ x : i32; y : i32; }
		fn Foo()
		{
			let : Point point{ .x= 42, .y= 34, .x= 0 };
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::DuplicatedStructMemberInitializer );
	U_ASSERT( error.file_pos.line == 5u );
}

static void MissingStructMemberInitializerTest0()
{
	static const char c_program_text[]=
	R"(
		class Point{ xy : [ i32, 2 ]; z : i32; }
		fn Foo()
		{
			let : Point point{ .xy[ 54, -785 ], };
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_ASSERT( error.code == CodeBuilderErrorCode::MissingStructMemberInitializer );
	U_ASSERT( error.file_pos.line == 5u );
}

void RunInitializersErrorsTest()
{
	ArrayInitializerForNonArrayTest0();
	ArrayInitializerForNonArrayTest1();
	ArrayInitializersCountMismatchTest0();
	ArrayInitializersCountMismatchTest1();
	FundamentalTypesHaveConstructorsWithExactlyOneParameterTest0();
	FundamentalTypesHaveConstructorsWithExactlyOneParameterTest1();
	ReferencesHaveConstructorsWithExactlyOneParameterTest0();
	ReferencesHaveConstructorsWithExactlyOneParameterTest1();
	UnsupportedInitializerForReferenceTest0();
	ConstructorInitializerForUnsupportedTypeTest0();
	StructInitializerForNonStructTest0();
	DuplicatedStructMemberInitializerTest0();
	MissingStructMemberInitializerTest0();
}

} // namespace U
