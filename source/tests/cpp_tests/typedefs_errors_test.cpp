#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( UsingKeywordAsName_ForTypedef_Test0 )
{
	static const char c_program_text[]=
	R"(
		type virtual= f64;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( UsingKeywordAsName_ForTypedef_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			type default= bool;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST( NameNotFound_ForTypedef_Test0 )
{
	static const char c_program_text[]=
	R"(
		type T= wtf;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( NameNotFound_ForTypedef_Test1 )
{
	static const char c_program_text[]=
	R"(
		type T= [ wtf, 4 ];
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( NameNotFound_ForTypedef_Test2 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			type T= lol;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST( NameNotFound_ForTypedef_Test3 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			type Kek= Wat; // "Wat" is not yet visible here.
			type Wat= i32;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST( NameIsNotTypeName_ForTypedef_Test0 )
{
	static const char c_program_text[]=
	R"(
		namespace SSS{}
		type T= SSS;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameIsNotTypeName );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( NameIsNotTypeName_ForTypedef_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo();
		type T= Foo;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameIsNotTypeName );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( Redefinition_ForTypedef_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct SSS{}
		type SSS= i32;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( Redefinition_ForTypedef_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo(){}
		type Foo= bool;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( Redefinition_ForTypedef_Test2 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			type x= f64;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( NameNotFound_ForTypedefTemplate_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		type Box</ T />= Can</ T />;    // "Can" - unknown

		type BoxI= Box</i32/>;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );

	bool found= false;
	for( const CodeBuilderError& error : build_result.errors )
	{
		if( error.template_context == nullptr )
			continue;
		found= true;

		U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TemplateContext );
		U_TEST_ASSERT( error.template_context != nullptr );
		U_TEST_ASSERT( !error.template_context->errors.empty() );
		U_TEST_ASSERT( error.template_context->errors.front().code == CodeBuilderErrorCode::NameNotFound );
		U_TEST_ASSERT( error.template_context->errors.front().src_loc.GetLine() == 3u );
	}
	U_TEST_ASSERT(found);
}

U_TEST( NameNotFound_ForTypedefTemplate_Test1 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		type Unbox</ Box</ T /> />= T;    // "Box" - unknown
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::NameNotFound, 3u ) );
}

U_TEST( TemplateArgumentNotUsedInSignature_ForTypedefTemplate_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		type Box</ />= i32;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TemplateArgumentNotUsedInSignature );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

} // namespace

} // namespace U
