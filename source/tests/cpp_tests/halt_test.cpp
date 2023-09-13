#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( HaltTest0 )
{
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

U_TEST( HaltTest2_ShouldHaltWithDeepCallStack )
{
	static const char c_program_text[]=
	R"(
		fn Bar()
		{
			halt;
		}

		fn Baz( i32 level )
		{
			if( level > 0 )
			{ Baz( level - 1 ); }
			else
			{ Bar(); }
		}

		fn Foo()
		{
			Baz( 25 );
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

U_TEST( HaltTest3_CodeAfterHaltMustBeUnreachable )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			halt;
			1 + 2;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( HaltTest4_HaltIsLikeReturn )
{
	static const char c_program_text[]=
	R"(
		fn SometimesReturns( bool cond ) : i32
		{
			if( cond )
			{ return 0; }
			else{ halt; }
			// Here "return" doesn`t needs, because this point is unreachable.
		}

		fn NeverReturns() : f32
		{
			halt; // ok, doesn`t return, "halt" terminates block.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( HaltIfTest0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( bool cond ) : i32
		{
			halt if(cond);
			return 0;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foob" );
	U_TEST_ASSERT( function != nullptr );

	// Should not halt if condition is false.
	try
	{
		llvm::GenericValue val[1];
		val[0].IntVal= llvm::APInt( 1, 0 );
		engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>( val, 1 ) );
		U_TEST_ASSERT(true);
	}
	catch( const HaltException& )
	{
		U_TEST_ASSERT(false);
	}

	// Should halt if condition is true.
	try
	{
		llvm::GenericValue val[1];
		val[0].IntVal= llvm::APInt( 1, 1 );
		engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>( val, 1 ) );
		U_TEST_ASSERT(false);
	}
	catch( const HaltException& )
	{
		U_TEST_ASSERT(true);
	}
}

U_TEST( ArrayOutOfBoundsShouldHalt0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 45 ] arr= zero_init;
			var u32 mut index= 58u; // index is greater, than array size
			auto x= arr[ index ];
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

U_TEST( ArrayOutOfBoundsShouldHalt1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 24 ] arr= zero_init;
			var u32 mut index= 24u; // index is equal to array size
			auto x= arr[ index ];
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

U_TEST( ArrayOutOfBoundsShouldHalt2 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 13 ] arr= zero_init;
			var u32 mut index= 10u; // index is less than array size
			auto x= arr[ index ];
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
		U_TEST_ASSERT(false);
		return;
	}
	U_TEST_ASSERT(true);
}

} // namespace

} // namespace U
