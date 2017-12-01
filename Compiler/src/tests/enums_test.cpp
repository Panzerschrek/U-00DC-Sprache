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

U_TEST( InitializationOfEnumVariables )
{
	static const char c_program_text[]=
	R"(
		enum Letters { A, B, C, D, E, F }
		fn Foo()
		{
			var Letters b= Letters::B; // Expression initializer
			var Letters e( Letters::E ); // Constructor initializer
			var Letters c= zero_init; // Zero initializer
			auto d= Letters::D; // auto variable of enum type
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace U
