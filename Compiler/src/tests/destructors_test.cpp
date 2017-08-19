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

	// Must call destructor for variable.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor() ( x= 0 ) {}
			fn destructor()
			{
				x= 854;
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			var S s;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence.size() == 1u && g_destructors_call_sequence.front() == 854 );
}

U_TEST(DestructorsTest1)
{
	DestructorTestPrepare();

	// Must call destructors in reverse order.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			var S s0(0), s1(1), s2(2);
			var S imut s3(3);
			{ var S s4(4); } // s4 must be destructed before s5
			var S s5(5);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 4, 5, 3, 2, 1, 0 } ) );
}

U_TEST(DestructorsTest2)
{
	DestructorTestPrepare();

	// Destructors for arguments.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Bar( S arg0, S arg1 )
		{
			var S s_local(0);
		}
		fn Foo()
		{
			Bar( S(88), S(66) );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ), true );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 0, 66, 88 } ) );
}

} // namespace U
