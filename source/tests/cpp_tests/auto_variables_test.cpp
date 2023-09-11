#include <cstdlib>

#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST(AutoVariableTest0)
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 2017 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(AutoVariableTest1)
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 1237 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(AutoVariableTest2)
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 1237 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(AutoVariableTest3)
{
	// Mutable reference to array.
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		var [ i32, 4 ] mut a[ 0, 0, 1237, 0 ];
		auto &mut x= a;
		x[0u]= x[2u] + 5;
		return x[0u];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 1242 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(AutoVariableTest4)
{
	// Immutalbe reference to struct.
	static const char c_program_text[]=
	R"(
	struct S{ f32 x; f32 y; }
	fn Foo() : f32
	{
		var S imut s{ .x(34.5f), .y= -34.0f };
		auto &imut x= s;
		return x.x + x.y;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 0.5f == result_value.FloatVal );
}

U_TEST(AutoVariableTest5)
{
	// Auto-variable for struct type.
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; }
		fn Foo() : i32
		{
			var S s0{ .x= 58471 };
			auto s1= s0;
			return s1.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(58471) == result_value.IntVal.getLimitedValue() );
}

U_TEST(AutoVariableTest6)
{
	// Auto-variable for struct type with reference inside.
	static const char c_program_text[]=
	R"(
		struct S{ i32& x; }
		fn Foo() : i32
		{
			var i32 x= 4136574;
			var S s0{ .x= x };
			auto s1= s0;
			return s1.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(4136574) == result_value.IntVal.getLimitedValue() );
}

} // namespace

} // namespace U
