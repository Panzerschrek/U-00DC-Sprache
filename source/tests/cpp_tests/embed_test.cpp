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


} // namespace

} // namespace U
