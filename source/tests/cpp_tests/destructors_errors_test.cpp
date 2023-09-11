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

} // namespace

} // namespace U
