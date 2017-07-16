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

void RunAutoVariablesTest()
{
	AutoVariableTest0();
	AutoVariableTest1();
}

} // namespace U
