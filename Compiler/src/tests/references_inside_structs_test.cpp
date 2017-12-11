#include "tests.hpp"

namespace U
{

U_TEST( ReferenceClassFiledDeclaration )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 value_field;
			i32 mut mut_value_field;
			i32 imut imut_value_field;

			f32 & ref_field;
			f32 &mut mut_ref_field;
			f32 &imut imut_ref_field;
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace U
