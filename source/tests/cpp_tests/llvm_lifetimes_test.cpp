#include <llvm/Support/DynamicLibrary.h>

#include "tests.hpp"

namespace U
{

namespace
{

struct LifetimeCallResult
{
	void* address;
	bool is_start;
};

std::vector<LifetimeCallResult> g_lifetimes_call_sequence;

llvm::GenericValue LifetimeStartCalled( llvm::FunctionType* , const llvm::ArrayRef<llvm::GenericValue> args )
{
	LifetimeCallResult res{};
	res.address= args[0].PointerVal;
	res.is_start= true;
	g_lifetimes_call_sequence.push_back(res);

	return llvm::GenericValue();
}

llvm::GenericValue LifetimeEndCalled( llvm::FunctionType* , const llvm::ArrayRef<llvm::GenericValue> args )
{
	LifetimeCallResult res{};
	res.address= args[0].PointerVal;
	res.is_start= false;
	g_lifetimes_call_sequence.push_back(res);

	return llvm::GenericValue();
}

void LifetimesTestPrepare()
{
	g_lifetimes_call_sequence.clear();

	// "lle_X_" - common prefix for all external functions, called from LLVM Interpreter
	llvm::sys::DynamicLibrary::AddSymbol( "lle_X___U_debug_lifetime_start", reinterpret_cast<void*>( &LifetimeStartCalled ) );
	llvm::sys::DynamicLibrary::AddSymbol( "lle_X___U_debug_lifetime_end", reinterpret_cast<void*>( &LifetimeEndCalled ) );
}

} // namespace

U_TEST( StackVariableLifetime_Test0 )
{
	LifetimesTestPrepare();

	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 42;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 2 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[1].address );
	U_TEST_ASSERT(  g_lifetimes_call_sequence[0].is_start );
	U_TEST_ASSERT( !g_lifetimes_call_sequence[1].is_start );
}

U_TEST( StackVariableLifetime_Test1 )
{
	LifetimesTestPrepare();

	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 42;
			{
				var f32 y= 34.0f;
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 4 );

	// x
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[3].address );
	U_TEST_ASSERT(  g_lifetimes_call_sequence[0].is_start );
	U_TEST_ASSERT( !g_lifetimes_call_sequence[3].is_start );

	// y
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].address == g_lifetimes_call_sequence[2].address );
	U_TEST_ASSERT(  g_lifetimes_call_sequence[1].is_start );
	U_TEST_ASSERT( !g_lifetimes_call_sequence[2].is_start );
}

U_TEST( StackVariableLifetime_Test2 )
{
	LifetimesTestPrepare();

	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			{
				var i32 x= 42;
			}
			{
				var f32 y= 34.0f;
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 4 );

	// x
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[1].address );
	U_TEST_ASSERT(  g_lifetimes_call_sequence[0].is_start );
	U_TEST_ASSERT( !g_lifetimes_call_sequence[1].is_start );

	// y
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[3].address );
	U_TEST_ASSERT(  g_lifetimes_call_sequence[2].is_start );
	U_TEST_ASSERT( !g_lifetimes_call_sequence[3].is_start );
}

} // namespace U
