#include "tests.hpp"

namespace U
{

U_TEST( NamespacesTest0 )
{
	// Should get functions from namespace.
	static const char c_program_text[]=
	R"(
		namespace SpaceA
		{
			namespace B
			{
				struct CFF
				{
					fn DoSomething(){}
				}
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z6SpaceA1B3CFF11DoSomething" );
	U_TEST_ASSERT( function != nullptr );
}

} // namespace U
