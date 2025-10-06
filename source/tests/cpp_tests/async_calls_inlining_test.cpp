#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Verifier.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "cpp_tests.hpp"

namespace U
{

namespace
{

void EnsureModuleIsValid( const llvm::Module& module )
{
	std::string err_stream_str;
	llvm::raw_string_ostream err_stream( err_stream_str );
	if( llvm::verifyModule( module, &err_stream ) )
	{
		std::cerr << "Module verify error:\n" << err_stream.str() << std::endl;
		U_TEST_ASSERT(false);
	}
}

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

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4GetXv" ) == nullptr ); // Should inline it.

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

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4GetXv" ) == nullptr ); // Should inline it.

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

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z8DoubleItj" ) == nullptr ); // Should inline it.

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

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Divff" ) == nullptr ); // Should inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInlining_Test4)
{
	static const char c_program_text[]=
	R"(
		fn async DoubleIt( u32 x ) : u32
		{
			yield;
			return x * 2u;
		}
		// Should inline both calls.
		fn async QuadrupleIt( u32 x ) : u32
		{
			return DoubleIt( DoubleIt( x ).await ).await;
		}
		fn Foo()
		{
			auto mut f= QuadrupleIt( 7865u );
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 7865u * 4u );
					break;
				}
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z8DoubleItj" ) == nullptr ); // Should inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInlining_Test5)
{
	static const char c_program_text[]=
	R"(
		fn async Mul2( u32 x ) : u32
		{
			yield;
			return x * 2u;
		}
		fn async Div3( u32 x ) : u32
		{
			return x / 3u;
		}
		// Should inline both calls.
		fn async Bar( u32 x ) : u32
		{
			return Div3( Mul2( x ).await ).await;
		}
		fn Foo()
		{
			auto mut f= Bar( 5455u );
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 5455u * 2u / 3u );
					break;
				}
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4Mul2j" ) == nullptr ); // Should inline it.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4Div3j" ) == nullptr ); // Should inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInlining_Test6)
{
	static const char c_program_text[]=
	R"(
		fn async Mul2( u32 x ) : u32
		{
			yield;
			return x * 2u;
		}
		fn async Div3( u32 x ) : u32
		{
			return x / 3u;
		}
		// Should inline both calls.
		fn async Bar( u32 x ) : u32
		{
			return Mul2( x ).await + Div3( x ).await;
		}
		fn Foo()
		{
			auto mut f= Bar( 4433u );
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 4433u * 2u + 4433u / 3u );
					break;
				}
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4Mul2j" ) == nullptr ); // Should inline it.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4Div3j" ) == nullptr ); // Should inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInlining_Test7)
{
	static const char c_program_text[]=
	R"(
		fn async SquaresSum( u32 num ) : u32
		{
			var u32 mut res= 0u;
			for( auto mut i= 0u; i < num; ++i )
			{
				yield;
				res += i * i;
			}
			return res;
		}
		fn async Bar() : u32
		{
			return SquaresSum( 4u ).await;
		}
		fn Foo()
		{
			auto mut f= Bar();
			auto mut num_iterations= 0s;
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 0u * 0u + 1u * 1u + 2u * 2u + 3u * 3u );
					halt if( num_iterations != 4s );
					break;
				}
				++num_iterations;
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z10SquaresSumj" ) == nullptr ); // Should inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInlining_Test8)
{
	static const char c_program_text[]=
	R"(
		// Should use inlining order inverse to topological.
		fn async Fun0( u32 x ) : u32
		{
			yield;
			return x * 4u;
		}
		fn async Fun1( u32 x ) : u32
		{
			yield;
			return Fun0(x).await + 17u;
		}
		fn async Fun2( u32 x ) : u32
		{
			yield;
			return Fun1(x).await / 3u;
		}
		fn async Fun3( u32 x ) : u32
		{
			yield;
			return Fun2(x).await - 5u;
		}
		fn Foo()
		{
			auto mut f= Fun3( 625412u );
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != (((625412u * 4u) + 17u) / 3u) - 5u );
					break;
				}
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4Fun0j" ) == nullptr ); // Should inline it.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4Fun1j" ) == nullptr ); // Should inline it.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4Fun2j" ) == nullptr ); // Should inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInlining_Test9)
{
	static const char c_program_text[]=
	R"(
		// Should use inlining order inverse to topological.
		fn async Mul2( u32 x ) : u32
		{
			yield;
			return x * 2u;
		}
		fn async Mul6( u32 x ) : u32
		{
			yield;
			return Mul2(x).await * 3u;
		}
		fn async Mul10( u32 x ) : u32
		{
			yield;
			return Mul2(x).await * 5u;
		}
		fn async Mul16( u32 x ) : u32
		{
			return Mul6(x).await + Mul10(x).await;
		}
		fn Foo()
		{
			auto mut f= Mul16( 541u );
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 541u * 16u );
					break;
				}
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4Mul2j" ) == nullptr ); // Should inline it.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Mul10j" ) == nullptr ); // Should inline it.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4Mul6j" ) == nullptr ); // Should inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInlining_Test10)
{
	static const char c_program_text[]=
	R"(
		// Should not inline recursive function.
		fn async Factorial( u64 x ) : u64
		{
			if( x <= 1u64 )
			{
				return 1u64;
			}
			return x * Factorial( x - 1u64 ).await;
		}
		fn async FactorialWrapper( u64 x ) : u64
		{
			return Factorial(x).await;
		}
		fn Foo()
		{
			auto mut f= FactorialWrapper( 10u64 );
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 3628800u64 );
					break;
				}
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z9Factorialy" ) != nullptr ); // Should NOT inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInlining_Test11)
{
	static const char c_program_text[]=
	R"(
		// Should inline non-recursive function.
		fn async Get1() : u64
		{
			yield;
			return 1u64;
		}
		// Should not inline recursive function.
		fn async Factorial( u64 x ) : u64
		{
			if( x <= 1u64 )
			{
				return Get1().await;
			}
			return x * Factorial( x - 1u64 ).await;
		}
		fn async FactorialWrapper( u64 x ) : u64
		{
			return Factorial(x).await;
		}
		// Should inline "FactorialWrapper".
		fn async FactorialWrapper2( u64 x ) : u64
		{
			return FactorialWrapper(x).await;
		}
		// Should inline "FactorialWrapper2".
		fn async FactorialWrapper3( u64 x ) : u64
		{
			return FactorialWrapper2(x).await;
		}
		fn Foo()
		{
			auto mut f= FactorialWrapper3( 11u64 );
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 39916800u64 );
					break;
				}
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4Get1v" ) == nullptr ); // Should inline it.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z9Factorialy" ) != nullptr ); // Should NOT inline it.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z16FactorialWrappery" ) == nullptr ); // Should inline it.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z17FactorialWrapper2y" ) == nullptr ); // Should inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInlining_Test12)
{
	static const char c_program_text[]=
	R"(
		// Collatz conjecture.

		// Should inline non-recursive function.
		fn async Get1() : u32
		{
			yield;
			return 1u;
		}
		// At least some functions in the loop should not be inlined.
		fn async Div2( u32 x, u32 step ) : u32
		{
			return ProcessNextElement( x / 2u, step  ).await;
		}
		fn async Mul3Plus1( u32 x, u32 step ) : u32
		{
			return ProcessNextElement( x * 3u + 1u, step ).await;
		}
		fn async ProcessNextElement( u32 x, u32 step ) : u32
		{
			if( x == Get1().await )
			{
				return step;
			}

			if( (x & 1u) == 0u )
			{
				return Div2(x, step + 1u ).await;
			}
			else
			{
				return Mul3Plus1( x, step + 1u ).await;
			}
		}
		// This function sould be inlined because it itself isn't recursive.
		fn async GetNumSteps( u32 x ) : u32
		{
			return ProcessNextElement( x, 0u ).await;
		}
		fn async GetNumStepsWrapper( u32 x ) : u32
		{
			return GetNumSteps( x ).await;
		}
		fn Foo()
		{
			auto mut f= GetNumStepsWrapper( 12u );
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 9u );
					break;
				}
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4Get1v" ) == nullptr ); // Should inline it.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z11GetNumStepsj" ) == nullptr ); // Should inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInlining_Test13)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			i32 y;

			fn async SomeMethod( this ) : i32
			{
				return x * y;
			}
		}

		fn async Bar() : i32
		{
			// Should properly inline here a call to a method with "this" argument, where "this" fields are accessed inside.
			var S s{ .x= 2, .y= 3 };
			return s.SomeMethod().await;
		}

		fn Foo() : i32
		{
			if_coro_advance( r : Bar() )
			{
				return r;
			}
			halt;
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z8DoubleItj" ) == nullptr ); // Should inline it.

	const llvm::GenericValue val= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( val.IntVal.getLimitedValue() == 6 );
}

U_TEST(AsyncCallInlining_Test14)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			i32 y;

			fn async SomeMethod( this ) : i32
			{
				return x * y;
			}
		}

		fn async Bar( [ S, 1 ]& s_arr ) : i32
		{
			// Should properly inline here a call to a method with "this" argument, where "this" fields are accessed inside.
			return s_arr[0].SomeMethod().await;
		}

		fn Foo() : i32
		{
			var [ S, 1 ] s_arr[ { .x= 5, .y= 7 } ];
			if_coro_advance( r : Bar( s_arr ) )
			{
				return r;
			}
			halt;
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z8DoubleItj" ) == nullptr ); // Should inline it.

	const llvm::GenericValue val= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( val.IntVal.getLimitedValue() == 35 );
}

U_TEST(AsyncCallInliningFail_Test0)
{
	static const char c_program_text[]=
	R"(
		fn async GetX() : i32
		{
			return 6564;
		}
		// Should not inline - only inlining for await call is posible.
		fn async GetXWrapper() : i32
		{
			auto mut f= GetX();
			loop
			{
				if_coro_advance( x : f )
				{
					return x;
				}
				yield;
			}
		}
		fn Foo()
		{
			auto mut f= GetXWrapper();
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 6564 );
					break;
				}
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4GetXv" ) != nullptr ); // Should NOT inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInliningFail_Test1)
{
	static const char c_program_text[]=
	R"(
		fn async GetX() : i32
		{
			return 998;
		}
		// Should not inline - call to an async function via a pointer.
		fn async GetXWrapper() : i32
		{
			var ( fn() : (async : i32) ) ptr( GetX );
			return ptr().await;
		}
		fn Foo()
		{
			auto mut f= GetXWrapper();
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 998 );
					break;
				}
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4GetXv" ) != nullptr ); // Should NOT inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInliningFail_Test2)
{
	static const char c_program_text[]=
	R"(
		fn async GetX() : i32
		{
			return 12311;
		}
		fn GetXCoroutine() : (async : i32)
		{
			return GetX();
		}
		// Should not inline - call to an async function via an intermediate function.
		fn async GetXWrapper() : i32
		{
			return GetXCoroutine().await;
		}
		fn Foo()
		{
			auto mut f= GetXWrapper();
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 12311 );
					break;
				}
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4GetXv" ) != nullptr ); // Should NOT inline it.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z13GetXCoroutinev" ) != nullptr ); // Should NOT inline it too.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInliningFail_Test3)
{
	static const char c_program_text[]=
	R"(
		fn async GetX() : i32
		{
			return 998;
		}
		// Can't inline - coroutine object used for more than one "await" operator.
		fn async GetXWrapper() : i32
		{
			auto mut f= GetX();
			if( true )
			{
				return move(f).await;
			}
			else
			{
				return move(f).await;
			}
		}
		fn Foo()
		{
			auto mut f= GetXWrapper();
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 998 );
					break;
				}
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4GetXv" ) != nullptr ); // Should NOT inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInliningFail_Test4)
{
	static const char c_program_text[]=
	R"(
		fn async GetX() : i32
		{
			return 575;
		}
		// Can't inline - coroutine object used not only for "await" operator.
		fn async GetXWrapper() : i32
		{
			auto mut f= GetX();
			if( true )
			{
				return move(f).await;
			}
			else
			{
				return 1; // Call here destructor for "f" which prevents "await" inlining.
			}
		}
		fn Foo()
		{
			auto mut f= GetXWrapper();
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 575 );
					break;
				}
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4GetXv" ) != nullptr ); // Should NOT inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInliningFail_Test5)
{
	static const char c_program_text[]=
	R"(
		fn generator GetX() : i32
		{
			return 56443;
		}
		// Can't inline - generators instead of async functions.
		fn generator GetXWrapper() : i32
		{
			auto mut f= GetX();
			if_coro_advance( x : f )
			{
				return x;
			}
		}
		fn Foo()
		{
			auto mut f= GetXWrapper();
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 56443 );
					break;
				}
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4GetXv" ) != nullptr ); // Should NOT inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST(AsyncCallInliningFail_Test6)
{
	static const char c_program_text[]=
	R"(
		fn async GetX() : i32;
		// Can't inline - callable is external.
		fn async GetXWrapper() : i32
		{
			if( false )
			{
				return GetX().await;
			}
			return 898;
		}
		fn Foo()
		{
			auto mut f= GetXWrapper();
			loop
			{
				if_coro_advance( x : f )
				{
					halt if( x != 898 );
					break;
				}
			}
		}
	)";

	auto module= BuildProgramForAsyncFunctionsInliningTest( c_program_text );
	EnsureModuleIsValid( *module );
	const EnginePtr engine= CreateEngine( std::move(module) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4GetXv" ) != nullptr ); // Should NOT inline it.

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

} // namespace

} // namespace U
