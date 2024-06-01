#include "cpp_tests.hpp"

namespace U
{

namespace
{

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
			{ "embed.bin", c_program_text_embed },
			{ "root", c_program_text_root }
		},
		"root" );
}

} // namespace

} // namespace U
