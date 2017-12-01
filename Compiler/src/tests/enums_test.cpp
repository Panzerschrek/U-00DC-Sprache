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

U_TEST( EnumsAssignmentAndReturnTest )
{
	static const char c_program_text[]=
	R"(
		enum ColorComponent{ r, g, b }
		fn Foo() : ColorComponent
		{
			var ColorComponent cc= zero_init;
			cc= ColorComponent::b;
			return cc;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 2 ) == result_value.IntVal.getLimitedValue() );
}

} // namespace U
