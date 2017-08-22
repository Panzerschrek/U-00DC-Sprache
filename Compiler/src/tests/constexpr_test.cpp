#include "tests.hpp"

namespace U
{

U_TEST(ConstexprTest0)
{
	// Simple integer constant expression for array size.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var [ i32, 7 - 3 ] arr= zero_init;
			arr[3u]= 42;
			return arr[3u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstexprTest1)
{
	// Negation must produce constant value.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var [ i32, -(-2) ] arr= zero_init;
			arr[1u]= 42;
			return arr[1u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstexprTest2)
{
	// Shift operators must produce constant value.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var [ i32, ( ( 42 << 4u ) >> 6u8 ) >> 1u16 ] arr= zero_init; // Must be array of size "5"
			arr[4u]= 42;
			return arr[4u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstexprTest3)
{
	// Simple constexpr variable, "=" initializer.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var i32 constexpr x= 42;
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

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstexprTest4)
{
	// Simple constexpr variable, constructor initializer.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var i32 constexpr x(77758);
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

	U_TEST_ASSERT( static_cast<uint64_t>( 77758 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstexprTest5)
{
	// constexpr auto variable.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			auto constexpr x= 58457;
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

	U_TEST_ASSERT( static_cast<uint64_t>( 58457 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstexprTest6)
{
	// Constexpr varable used for array size.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var i32 constexpr s= 3 + 2;
			var [ i32, s ] arr= zero_init;
			arr[4u]= 85124;
			return arr[4u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 85124 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstexprTest7)
{
	// Fundamental to fundamental cast must produce constant values for constant arguments.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var i32 constexpr s= i32( 3.1f + 2.7f ); // float to int
			return s;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 5 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstexprTest8)
{
	// Fundamental to fundamental cast must produce constant values for constant arguments.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var i32 constexpr s= i32( 3u + 2u ); // int to int
			return s;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 5 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstexprTest9)
{
	// Fundamental to fundamental cast must produce constant values for constant arguments.
	static const char c_program_text[]=
	R"(
		fn Foo() : f32
		{
			var f32 constexpr s= f32( i32(444u) + 222 ); // int to int + int to float
			return s;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 666.0f == result_value.FloatVal );
}

} // namespace U
