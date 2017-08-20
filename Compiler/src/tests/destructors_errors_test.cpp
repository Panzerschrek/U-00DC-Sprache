#include "tests.hpp"

namespace U
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

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );

	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionBodyDuplication );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( DestructorOutsideClassTest0 )
{
	static const char c_program_text[]=
	R"(
		fn destructor();
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );

	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ConstructorOrDestructorOutsideClass );
	U_TEST_ASSERT( error.file_pos.line == 2u );
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

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );

	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ConstructorAndDestructorMustReturnVoid );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST( ExplicitArgumentsInDestructorTest0 )
{
	static const char c_program_text[]=
	R"(
		class S
		{
			fn destructor( this ) {} // Explicit "this"
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );

	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExplicitArgumentsInDestructor );
	U_TEST_ASSERT( error.file_pos.line == 4u );
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

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );

	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExplicitArgumentsInDestructor );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

} // namespace U
