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

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

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

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( 2017.52 == result_value.DoubleVal );
}

static void ExpressionInitializerTest2()
{
	// Expression initializer for references
	static const char c_program_text[]=
	R"(
	fn Foo() : f64
	{
		let : f64 x = 2017.52;
		let : f64 &x_ref= x;
		return x_ref;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

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

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

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

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( 2017.52 == result_value.DoubleVal );
}

static void ConstructorInitializerForReferencesTest0()
{
	// Constructor initializer for floats
	static const char c_program_text[]=
	R"(
	fn Foo() : f64
	{
			let : f64 x = 2017.52;
			let : f64 &x_ref(x);
			return x_ref;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( 2017.52 == result_value.DoubleVal );
}

static void ArrayInitializerForFundamentalTypesTest0()
{
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		let : [ i32, 3u32 ] x[ 42, 34, 785 ];
		return x[0u32] * x[1u32] - x[2u32];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( static_cast<uint64_t>( 42 * 34 - 785 ) == result_value.IntVal.getLimitedValue() );
}

static void ArrayInitializerForFundamentalTypesTest1()
{
	// Array initializer with comma at end must works correctly
	static const char c_program_text[]=
	R"(
	fn Foo() : f32
	{
		let : [ f32, 3u32 ] x[ 42.5f32, 34.0f32, 785.7f32, ];
		return x[0u32] * x[1u32] - x[2u32];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( 42.5f * 34.0f - 785.7f == result_value.FloatVal );
}

static void TwodimensionalArrayInitializerTest0()
{
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		let : [ [ i32, 2u32 ], 3u32 ] mat
			[
				[ 175, -8 * 5, ],
				[ 95684, 48 ],
				[ -14, 2 + 2 * 2 ],
			];
		return
			mat[0u32][0u32] + mat[0u32][1u32] +
			mat[1u32][0u32] + mat[1u32][1u32] +
			mat[2u32][0u32] * mat[2u32][1u32];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT(
		static_cast<uint64_t>( (175) + (-8 * 5) + (95684) + (48) + (-14) * (2 + 2 * 2 ) ) ==
		result_value.IntVal.getLimitedValue() );
}

void RunInitializersTest()
{
	ExpressionInitializerTest0();
	ExpressionInitializerTest1();
	ExpressionInitializerTest2();
	ConstructorInitializerForFundamentalTypesTest0();
	ConstructorInitializerForFundamentalTypesTest1();
	ConstructorInitializerForReferencesTest0();
	ArrayInitializerForFundamentalTypesTest0();
	ArrayInitializerForFundamentalTypesTest1();
	TwodimensionalArrayInitializerTest0();
}

} // namespace U
