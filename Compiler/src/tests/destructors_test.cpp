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

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 0, 66, 88 } ) );
}

U_TEST(DestructorsTest3)
{
	DestructorTestPrepare();

	// Must call destructors after return.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Bar( bool b )
		{
			var S s0(0);
			if( b )
			{
				var S s1(1);
				return;
			}
			var S s2(2);
		}
		fn Foo()
		{
			Bar(false);
			Bar(true);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 2, 0,    1, 0 } ) );
}

U_TEST(DestructorsTest4)
{
	DestructorTestPrepare();

	// Must call destructors after return.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Bar( bool b )
		{
			var S s0(0);
			if( b )
			{
				var S s1(1);
				return;
			}
			else
			{
				return;
			}
			// Here must be no destructors code.
		}
		fn Foo()
		{
			Bar(false);
			Bar(true);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 0,    1, 0 } ) );
}

U_TEST(DestructorsTest5)
{
	DestructorTestPrepare();

	// Must call destructors of arguments in each return.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Bar( S s )
		{
			if( s.x == 0 ) { return; }
			if( s.x == 1 ) {{{ return; }}}
			if( s.x == 2 ) { return; }
			{ var S new_local(555); }
		}
		fn Foo()
		{
			Bar(S(0));
			Bar(S(1));
			Bar(S(2));
			Bar(S(3));
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 0,    1,    2,    555, 3 } ) );
}

U_TEST(DestructorsTest6)
{
	DestructorTestPrepare();

	// Must call destructors of loop local variables before "break".
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Foo()
		{
			var S live_longer_than_loop(-1);
			while(true)
			{
				var S s0(0);
				{
					var S s1(1), s2(2);
					var S s3(3);
					if( true )
					{
						var S s4(4); // must destroy s4, s3, s2, s1, s0
						break;
					}
				}
			}

			while(true)
			{
				var S s5(5);
				break;
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 4, 3, 2, 1, 0,  5,  -1 } ) );
}

U_TEST(DestructorsTest7)
{
	DestructorTestPrepare();

	// Must call destructors of inner loop, but not call destructors of outer loop before "break".
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Foo()
		{
			var S s0(0);
			while(true)
			{
				var S s1(1), s2(2);
				var S s3(3);
				while(true)
				{
					var S s4(4);
					if( true )
					{
						var S s5(5), s6(6);
						break;
					}
				}
				var S s7(7);
				break;
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 6, 5, 4, 7, 3, 2, 1, 0 } ) );
}

U_TEST(DestructorsTest8)
{
	DestructorTestPrepare();

	// Explicit destructor must contains implicit calls to destructors of members.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		class T
		{
			S s; i32 y;
			fn constructor( i32 x, i32 in_y ) ( s(x), y(in_y) ) {}
			fn destructor()
			{
				DestructorCalled(y);
			}
		}
		fn Foo()
		{
			var T t(111, 666);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 666, 111} ) );
}

U_TEST(DestructorsTest9)
{
	DestructorTestPrepare();

	// Explicit destructor must contains implicit calls to destructors of members.
	// Members destructors must be called in all return ways.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		class T
		{
			S s; i32 y;
			fn constructor( i32 x, i32 in_y ) ( s(x), y(in_y) ) {}
			fn destructor()
			{
				if( ( y & 1 ) != 0 )
				{
					DestructorCalled(y);
					return;
				}
				else
				{
					DestructorCalled( -y );
				}
			}
		}
		fn Foo()
		{
			var T t0(111, 666);
			var T t1(500, 999);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 999, 500, -666, 111 } ) );
}

U_TEST(DestructorsTest10)
{
	DestructorTestPrepare();

	// Must call generated destructor.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		class SWrapper
		{
			S s;
			fn constructor( i32 x ) ( s(x) ) {}
			// This class must have generated destructor, that calls destructor for members.
		}
		fn Foo()
		{
			var SWrapper s_wrapper(14789325);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 14789325 } ) );
}

} // namespace U
