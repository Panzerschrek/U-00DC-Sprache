#include <cstdlib>
#include <iostream>

#include "../assert.hpp"
#include "tests.hpp"

#include "auto_variables_test.hpp"

namespace U
{

static void AutoVariableTest0()
{
	// Value-variable with value-expression assignment.
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		auto x= 2017;
		return x;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( static_cast<uint64_t>( 2017 ) == result_value.IntVal.getLimitedValue() );
}

static void AutoVariableTest1()
{
	// Value-variable with reference-expression assignment.
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		var i32 a= 1237;
		auto x= a;
		return x;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( static_cast<uint64_t>( 1237 ) == result_value.IntVal.getLimitedValue() );
}

static void AutoVariableTest2()
{
	// Immutable value-variable with reference-expression assignment.
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		var i32 a= 1237;
		auto imut x= a;
		return x;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( static_cast<uint64_t>( 1237 ) == result_value.IntVal.getLimitedValue() );
}

static void AutoVariableTest3()
{
	// Mutable reference to array.
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		var [ i32, 4 ] a[ 0, 0, 1237, 0 ];
		auto &x= a;
		x[0u]= x[2u] + 5;
		return x[0u];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( static_cast<uint64_t>( 1242 ) == result_value.IntVal.getLimitedValue() );
}

static void AutoVariableTest4()
{
	// Immutalbe reference to struct.
	static const char c_program_text[]=
	R"(
	class S{ x : f32; y : f32; }
	fn Foo() : f32
	{
		var S imut s{ .x(34.5f), .y= -34.0f };
		auto &imut x= s;
		return x.x + x.y;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( 0.5f == result_value.FloatVal );
}

void RunAutoVariablesTest()
{
	AutoVariableTest0();
	AutoVariableTest1();
	AutoVariableTest2();
	AutoVariableTest3();
	AutoVariableTest4();
}

} // namespace U
