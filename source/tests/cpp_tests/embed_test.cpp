#include "cpp_tests.hpp"

namespace U
{

namespace
{

// Use this helper function, because C++ string_view can't be constructed properly from char array.
// Constructor from "const char*" is used instead, which causes "strlen" call, which produces wrong result for char arrays with zeros inside.
template<size_t S>
std::string_view MakeStringView( const char (&arr)[S] )
{
	return std::string_view( arr, S );
}

U_TEST( Embed_Test0 )
{
	static const char c_program_text_embed[]={ 0x37, 0x01, char(0xA6), char(0x8E) };

	static const char c_program_text_root[]=
	R"(
		fn Foo()
		{
			auto& embed_result= embed( "embed.bin" );
			var [ byte8, 4 ] expected_result[ (u8(0x37)), (u8(0x01)), (u8(0xA6)), (u8(0x8E)) ];
			static_assert( embed_result == expected_result );
		}
	)";

	BuildMultisourceProgram(
		{
			{ "embed.bin", MakeStringView(c_program_text_embed) },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( Embed_Test1 )
{
	static const char c_program_text_embed[]={ 0x37, 0x00, char(0xA6), char(0x8E), 0x00 };

	static const char c_program_text_root[]=
	R"(
		// Should handle zeros properly.
		auto& embed_result= embed( "embed.bin" );
		var [ byte8, 5 ] expected_result[ (u8(0x37)), (u8(0x00)), (u8(0xA6)), (u8(0x8E)), (u8(0x00)) ];
		static_assert( embed_result == expected_result );
	)";

	BuildMultisourceProgram(
		{
			{ "embed.bin", MakeStringView(c_program_text_embed) },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( TypesMismatch_ForEmbed_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			embed( 42 ); // Expected char8 array, got integer
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch, 4u ) );
}

U_TEST( TypesMismatch_ForEmbed_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( tup[ char8, char8 ]& t)
		{
			embed( t ); // Expected char8 array, got tuple
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch, 4u ) );
}

U_TEST( TypesMismatch_ForEmbed_Test2 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			embed( "file.txt"u16 ); // Expected char8 array, got char16 array
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch, 4u ) );
}

U_TEST( ExpectedConstantExpression_ForEmbed_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			auto mut f= "file.txt";
			embed( f ); // Given value isn't constant.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedConstantExpression, 5u ) );
}

U_TEST( EmbedFileNotFound_Test0 )
{
	static const char c_program_text_root[]=
	R"(
		auto& f= embed( "cot" ); // can't find this file.
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "cat", "contents" },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( HasError( result.errors, CodeBuilderErrorCode::EmbedFileNotFound, 2u ) );
}

U_TEST( EmbedFileNotFound_Test1 )
{
	static const char c_program_text_root[]=
	R"(
		auto& file_name= "qwwrty";
		auto& f= embed( file_name ); // can't find this file.
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "qwerty", "contents" },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( HasError( result.errors, CodeBuilderErrorCode::EmbedFileNotFound, 3u ) );
}

} // namespace

} // namespace U
