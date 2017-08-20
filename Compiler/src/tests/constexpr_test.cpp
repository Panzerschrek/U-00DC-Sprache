#include "tests.hpp"

namespace U
{

U_TEST(ConstexprTest0)
{
	// Simple integer constant expression for array size.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var [ i32, 7 - 3 ] arr= zero_init;
			arr[3u]= 42;
			return arr[3u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstexprTest1)
{
	// Negation must produce constant value.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var [ i32, -(-2) ] arr= zero_init;
			arr[1u]= 42;
			return arr[1u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

} // namespace U
