#include <llvm/Support/DynamicLibrary.h>
#include "tests.hpp"

namespace U
{

static std::vector<int> g_constructirs_call_sequence;
static std::vector<int>  g_destructors_call_sequence;

static llvm::GenericValue ConstructorCalled(
	llvm::FunctionType*,
	llvm::ArrayRef<llvm::GenericValue> args )
{
	g_constructirs_call_sequence.push_back( static_cast<int>(args[0].IntVal.getLimitedValue()) );
	return llvm::GenericValue();
}

static llvm::GenericValue DestructorCalled(
	llvm::FunctionType*,
	llvm::ArrayRef<llvm::GenericValue> args )
{
	g_destructors_call_sequence.push_back( static_cast<int>(args[0].IntVal.getLimitedValue()) );
	return llvm::GenericValue();
}

static void TestPrepare()
{
	g_constructirs_call_sequence.clear();
	 g_destructors_call_sequence.clear();

	// "lle_X_" - common prefix for all external functions, called from LLVM Interpreter
	llvm::sys::DynamicLibrary::AddSymbol( "lle_X__Z17ConstructorCalledi", reinterpret_cast<void*>( &ConstructorCalled ) );
	llvm::sys::DynamicLibrary::AddSymbol(  "lle_X__Z16DestructorCalledi", reinterpret_cast<void*>( & DestructorCalled ) );
}

U_TEST(TempVariablesMovingTest0_MoveTempVariableToArgument)
{
	TestPrepare();

	static const char c_program_text[]=
	R"(
		fn ConstructorCalled(i32 x);
		fn  DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor()
				( x= 0 )
			{
				ConstructorCalled(x);
			}
			fn constructor( i32 in_x )
				( x= in_x )
			{
				ConstructorCalled(x);
			}
			fn constructor( S& imut other )
				( x= other.x )
			{
				ConstructorCalled(x);
			}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}

		fn Bar( S s ){}

		fn Foo()
		{
			Bar( S(45) ); // Must move temporary variable to argument.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_constructirs_call_sequence == std::vector<int>( { 45 } ) &&
		 g_destructors_call_sequence == std::vector<int>( { 45 } ) );
}

U_TEST(TempVariablesMovingTest1_MoveTempVariableToReturnValue)
{
	TestPrepare();

	static const char c_program_text[]=
	R"(
		fn ConstructorCalled(i32 x);
		fn  DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor()
				( x= 0 )
			{
				ConstructorCalled(x);
			}
			fn constructor( i32 in_x )
				( x= in_x )
			{
				ConstructorCalled(x);
			}
			fn constructor( S& imut other )
				( x= other.x )
			{
				ConstructorCalled(x);
			}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}

		fn Bar() : S
		{
			return S(58424); // Must move temporary variable to result, not copy it.
		}

		fn Foo()
		{
			Bar();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_constructirs_call_sequence == std::vector<int>( { 58424 } ) &&
		 g_destructors_call_sequence == std::vector<int>( { 58424 } ) );
}

U_TEST(TempVariablesMovingTest2_MoveTempVariableInExpressionInitialization)
{
	TestPrepare();

	static const char c_program_text[]=
	R"(
		fn ConstructorCalled(i32 x);
		fn  DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor()
				( x= 0 )
			{
				ConstructorCalled(x);
			}
			fn constructor( i32 in_x )
				( x= in_x )
			{
				ConstructorCalled(x);
			}
			fn constructor( S& imut other )
				( x= other.x )
			{
				ConstructorCalled(x);
			}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}

		fn Foo()
		{
			var S s= S(66635); // Must move to initialized variable temp varaible.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_constructirs_call_sequence == std::vector<int>( { 66635 } ) &&
		 g_destructors_call_sequence == std::vector<int>( { 66635 } ) );
}

U_TEST(TempVariablesMovingTest3_MoveTempVariableInAutoVariableInitialization)
{
	TestPrepare();

	static const char c_program_text[]=
	R"(
		fn ConstructorCalled(i32 x);
		fn  DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor()
				( x= 0 )
			{
				ConstructorCalled(x);
			}
			fn constructor( i32 in_x )
				( x= in_x )
			{
				ConstructorCalled(x);
			}
			fn constructor( S& imut other )
				( x= other.x )
			{
				ConstructorCalled(x);
			}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}

		fn Foo()
		{
			auto s= S(11245678); // Must move to initialized variable temp varaible.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_constructirs_call_sequence == std::vector<int>( { 11245678 } ) &&
		 g_destructors_call_sequence == std::vector<int>( { 11245678 } ) );
}

U_TEST(TempVariablesMovingTest4_MoveFunctionResult)
{
	TestPrepare();

	static const char c_program_text[]=
	R"(
		fn ConstructorCalled(i32 x);
		fn  DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor()
				( x= 0 )
			{
				ConstructorCalled(x);
			}
			fn constructor( i32 in_x )
				( x= in_x )
			{
				ConstructorCalled(x);
			}
			fn constructor( S& imut other )
				( x= other.x )
			{
				ConstructorCalled(x);
			}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}

		fn Bar( i32 x ) : S
		{
			return S(x);
		}

		fn Foo()
		{
			auto s= Bar(88542); // Must move to initialized variable function result.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_constructirs_call_sequence == std::vector<int>( { 88542 } ) &&
		 g_destructors_call_sequence == std::vector<int>( { 88542 } ) );
}

} // namespace U
