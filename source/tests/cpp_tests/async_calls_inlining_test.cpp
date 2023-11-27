#include "cpp_tests.hpp"


namespace U
{

namespace
{

U_TEST(AsyncCallInlining_Test0)
{
	static const char c_program_text[]=
	R"(
		// Simplest async function.
		fn async GetX() : i32
		{
			return 76767;
		}
		fn async GetXWrapper() : i32
		{
			return GetX().await;
		}
		fn Foo()
		{
			auto mut f= GetXWrapper();
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 76767 );
					break;
				}
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForAsyncFunctionsInliningTest( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInlining_Test1)
{
	static const char c_program_text[]=
	R"(
		// Async function with "yield" inside.
		fn async GetX() : u32
		{
			yield;
			return 3433u;
		}
		fn async GetXWrapper() : u32
		{
			return GetX().await;
		}
		fn Foo()
		{
			auto mut f= GetXWrapper();
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 3433u );
					break;
				}
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForAsyncFunctionsInliningTest( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInlining_Test2)
{
	static const char c_program_text[]=
	R"(
		// Async function with an argument.
		fn async DoubleIt( u32 x ) : u32
		{
			yield;
			return x * 2u;
		}
		fn async Bar() : u32
		{
			return DoubleIt( 76u ).await;
		}
		fn Foo()
		{
			auto mut f= Bar();
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 76u * 2u );
					break;
				}
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForAsyncFunctionsInliningTest( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInlining_Test3)
{
	static const char c_program_text[]=
	R"(
		// Async function with multiple arguments.
		fn async Div( f32 x, f32 y ) : f32
		{
			yield;
			return x / y;
		}
		fn async Div5( f32 x ) : f32
		{
			return Div( x, 5.0f ).await;
		}
		fn Foo()
		{
			auto mut f= Div5( 273.5f );
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 273.5f / 5.0f );
					break;
				}
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForAsyncFunctionsInliningTest( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

} // namespace

} // namespace U
