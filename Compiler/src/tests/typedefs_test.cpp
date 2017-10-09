#include "tests.hpp"

namespace U
{

U_TEST( TypedefsTest0 )
{
	// Simple test.
	static const char c_program_text[]=
	R"(
		type size_type= u64; // global typedef
		struct C
		{
			type index_type= size_type; // typedef inside struct
		}
		namespace S
		{
			type C= ::C; // typedef inside namespace
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace U
