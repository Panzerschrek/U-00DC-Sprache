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

U_TEST( NamespacesTest1 )
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
					fn DoSomething() : i32 { return 8547; }
				}
			}
		}
		namespace CCCombo
		{
			fn Foo() : i32
			{
				return SpaceA::B::CFF::DoSomething();
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z7CCCombo3Foo" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 8547 ) == result_value.IntVal.getLimitedValue() );
}

} // namespace U
