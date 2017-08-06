#include "tests.hpp"

namespace U
{

U_TEST(ConstructorTest0)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor( this )
			( x(42) )
			{
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ), true );
	// Must just be compiled.
}

} // namespace U
