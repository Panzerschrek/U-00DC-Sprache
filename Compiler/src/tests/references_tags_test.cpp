#include "tests.hpp"

namespace U
{

U_TEST( ReferncesTagsTest_BaseReferencesDefinition )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &'a x, i32 &'b y ) : i32 &'a imut
		{
			return x;
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace U
