#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( Embed_Test0 )
{
	static const char c_program_text_embed[]= "some text";

	static const char c_program_text_root[]=
	R"(
		fn Foo()
		{
			embed( "embed.txt" );
		}
	)";

	BuildMultisourceProgram(
		{
			{ "embed.txt", c_program_text_embed },
			{ "root", c_program_text_root }
		},
		"root" );
}

} // namespace

} // namespace U
