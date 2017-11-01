#include <llvm/Support/DynamicLibrary.h>

#include "tests.hpp"

namespace U
{

class HaltException final : public std::exception
{
public:
	virtual const char* what() const noexcept override
	{
		return "Halt exception";
	}
};

static llvm::GenericValue HaltCalled( llvm::FunctionType*, llvm::ArrayRef<llvm::GenericValue> )
{
	// Return from interpreter, using native exception.
	throw HaltException();
}

static bool g_halt_handler_registered= false;

static void HaltTestPrepare()
{
	if(g_halt_handler_registered)
		return;

	// "lle_X_" - common prefix for all external functions, called from LLVM Interpreter
	llvm::sys::DynamicLibrary::AddSymbol( "lle_X___U_halt", reinterpret_cast<void*>( &HaltCalled ) );
	g_halt_handler_registered= true;
}

U_TEST( HaltTest0 )
{
	HaltTestPrepare();

	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			halt;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	try
	{
		engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	}
	catch( const HaltException& )
	{
		U_TEST_ASSERT(true);
		return;
	}
	U_TEST_ASSERT( false );
}

U_TEST( HaltTest1_ShouldHaltInsteadOfReturn )
{
	HaltTestPrepare();

	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			if( true )
			{
				halt;
			}
			return 55;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	try
	{
		engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	}
	catch( const HaltException& )
	{
		U_TEST_ASSERT(true);
		return;
	}
	U_TEST_ASSERT( false );
}

} // namespace U
