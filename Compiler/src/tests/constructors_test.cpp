#include "tests.hpp"

namespace U
{

U_TEST(ConstructorTest0)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor()
			( x(42) )
			{
			}
		}
		fn Foo() : i32
		{
			var S s();
			return s.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

} // namespace U
