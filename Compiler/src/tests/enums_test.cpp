#include "tests.hpp"

namespace U
{

U_TEST( EnumsDeclarationTest )
{
	static const char c_program_text[]=
	R"(
		enum GlobalEnum
		{
			A, B, C,
		}
		enum EmptyEnum {}

		class C
		{
			enum InClassEnum
			{
				FFF, RRR, TT,
				Lol
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( EnumVariableDeclarationTest )
{
	static const char c_program_text[]=
	R"(
		enum E { A, B, C }
		fn Foo()
		{
			var E e= E::B;
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace U
