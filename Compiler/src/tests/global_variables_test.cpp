#include "tests.hpp"

namespace U
{

U_TEST( GlobalVariablesTest0 )
{
	// Simple global variable
	static const char c_program_text[]=
	R"(
		var f64 constexpr pi= 3.1415926535;

		fn Foo() : f64
		{
			return pi;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 3.1415926535 == result_value.DoubleVal );
}

} // namespace U
