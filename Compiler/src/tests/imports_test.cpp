#include "tests.hpp"

namespace U
{

U_TEST( ImportsTest0 )
{
	static const char c_program_text_a[]=
	R"(
		fn Bar() : i32
		{
			return  586;
		}
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"

		fn Foo() : i32
		{
			return Bar();
		}
	)";

	BuildMultisourceProgram(
		{
			{ "a"_SpC, c_program_text_a },
			{ "root"_SpC, c_program_text_root }
		},
		"root"_SpC );
}

} // namespace U
