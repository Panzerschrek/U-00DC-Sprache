#include <cstdlib>
#include <iostream>

#include "../assert.hpp"
#include "tests.hpp"

#include "initializers_test.hpp"

namespace U
{

static void ExpressionInitializerTest0()
{
	// Expression initializer for integers
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		let : i32 x= 2017;
		return x;
	}
	)";

	llvm::ExecutionEngine* const engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( static_cast<uint64_t>( 2017 ) == result_value.IntVal.getLimitedValue() );
}

static void ExpressionInitializerTest1()
{
	// Expression initializer for floats
	static const char c_program_text[]=
	R"(
	fn Foo() : f64
	{
		let : f64 x = 2017.52;
		return x;
	}
	)";

	llvm::ExecutionEngine* const engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( 2017.52 == result_value.DoubleVal );
}


static void ConstructorInitializerForFundamentalTypesTest0()
{
	// Constructor initializer for integers
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		let : i32 x( 2017 );
		return x;
	}
	)";

	llvm::ExecutionEngine* const engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( static_cast<uint64_t>( 2017 ) == result_value.IntVal.getLimitedValue() );
}

static void ConstructorInitializerForFundamentalTypesTest1()
{
	// Constructor initializer for floats
	static const char c_program_text[]=
	R"(
	fn Foo() : f64
	{
		let : f64 x( 2017.52 );
		return x;
	}
	)";

	llvm::ExecutionEngine* const engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( 2017.52 == result_value.DoubleVal );
}

void RunInitializersTest()
{
	ExpressionInitializerTest0();
	ExpressionInitializerTest1();
	ConstructorInitializerForFundamentalTypesTest0();
	ConstructorInitializerForFundamentalTypesTest1();
}

} // namespace U
