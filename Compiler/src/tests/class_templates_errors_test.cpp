#include "tests.hpp"

namespace U
{

U_TEST( InvalidValueAsTemplateArgumentTest0 )
{
	static const char c_program_text[]=
	R"(
		fn Bar(){}
		template</ i32 x /> struct S</ x />{}
		fn Foo()
		{
			var S</ Bar /> s; // Overloaded functions set as template argument not allowed.
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InvalidValueAsTemplateArgument );
	//U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( InvalidTypeOfTemplateVariableArgumentTest0 )
{
	static const char c_program_text[]=
	R"(
		template</ i32 x /> struct S</ x />{}
		fn Foo()
		{
			var S</ 0.5f /> s; // float type template arguments forbidden.
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InvalidTypeOfTemplateVariableArgument );
	//U_TEST_ASSERT( error.file_pos.line == 6u );
}

// TODO - InvalidTypeOfTemplateVariableArgument for arrays, structs.

U_TEST( ClassPrepass_ErrorsTest0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		class CC</ T />
		{
			fn Foo()
			{
				var i32 x= 0.25f; // all components are not template dependnent - generate error.
			}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST( ClassPrepass_ErrorsTest1 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		class CC</ T />
		{
			fn Foo()
			{
				var [ i32, 42 ] s{}; // struct initializer for non-struct.
			}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::StructInitializerForNonStruct );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST( ClassPrepass_ErrorsTest2 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		class CC</ T />
		{
			fn Foo()
			{
				var i32 something_mutable= 34;
				auto constexpr x= something_mutable;
			}
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::VariableInitializerIsNotConstantExpression );
	U_TEST_ASSERT( error.file_pos.line == 8u );
}

} // namespace U
