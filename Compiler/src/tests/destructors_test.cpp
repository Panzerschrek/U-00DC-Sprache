#include <llvm/Support/DynamicLibrary.h>

#include "tests.hpp"

namespace U
{

static std::vector<int> g_destructors_call_sequence;

static bool g_destructors_handler_registered= false;
static llvm::GenericValue DestructorCalled(
	llvm::FunctionType*,
	llvm::ArrayRef<llvm::GenericValue> args )
{
	g_destructors_call_sequence.push_back( static_cast<int>(args[0].IntVal.getLimitedValue()) );
	return llvm::GenericValue();
}

static void DestructorTestPrepare()
{
	g_destructors_call_sequence.clear();

	if(g_destructors_handler_registered)
		return;

	// "lle_X_" - common prefix for all external functions, called from LLVM Interpreter
	llvm::sys::DynamicLibrary::AddSymbol( "lle_X__Z16DestructorCalledi", reinterpret_cast<void*>( &DestructorCalled ) );
	g_destructors_handler_registered= true;
}

U_TEST(DestructorsTest0)
{
	DestructorTestPrepare();

	static const char c_program_text[]=
	R"(
		// TODO - call destructors.
		fn DestructorCalled(i32 x);
		fn Foo() : i32
		{
			DestructorCalled(42);
			return 0;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );
}

} // namespace U
