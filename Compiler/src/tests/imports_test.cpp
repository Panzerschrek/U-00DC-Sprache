#include "tests.hpp"

namespace U
{

U_TEST( ImportsTest0 )
{
	// Simple imports.
	static const char c_program_text[]=
	R"(
		import "a"
		import "b"
		import "std.ü"
		import "../fwd.ü"
		import "tests/this.ü"

		fn Foo() : i32
		{
			return 0;
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace U
