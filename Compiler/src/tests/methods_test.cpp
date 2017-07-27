#include "tests.hpp"

namespace U
{

U_TEST(MethodTest0)
{
	// Simple declaration of static method.
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Get42() : i32 { return 42; }
		}
		fn Foo() : i32
		{
			var S s= zero_init;
			return s.Get42();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

} // namespace U
