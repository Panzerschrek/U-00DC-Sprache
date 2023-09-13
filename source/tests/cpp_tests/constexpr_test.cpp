#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST(ConstexprTest0)
{
	// Simple integer constant expression for array size.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var [ i32, 7 - 3 ] mut arr= zero_init;
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
			var [ i32, -(-2) ] mut arr= zero_init;
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
			var [ i32, ( ( 42 << 4u ) >> 6u8 ) >> 1u16 ] mut arr= zero_init; // Must be array of size "5"
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
			var [ i32, s ] mut arr= zero_init;
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

U_TEST( StaticAssertTest0 )
{
	// Simple static assert with "true" expression.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			static_assert( true );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
}

U_TEST( StaticAssertTest1 )
{
	// Simple static assert with "false" expression - must produce error.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			static_assert( false );
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );

	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::StaticAssertionFailed );
	U_TEST_ASSERT( build_result.errors[0].src_loc.GetLine() == 4u );
}

U_TEST( StaticAssertTest2 )
{
	// All expressions must be constant and true.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			static_assert( 0.0f / 0.0f != 0.0f / 0.0f ); // NaN is not NaN
			static_assert( true );
			static_assert( !false );
			static_assert( !!true );
			static_assert( true && !false && true == true && false == false && false != true && true != false );
			static_assert( 0 == 0 );
			static_assert( 0u == 0u );
			static_assert( 0.0f == 0.0f );
			static_assert( 1 != 2 );
			static_assert( 1 > 0 );
			static_assert( -1 <= 2 );
			static_assert( 5869 <= 5869 );
			static_assert( -857 > -11545 );
			static_assert( 1 + 1 > 1 );
			static_assert( ~0u == 4294967295u );
			static_assert( i32( 3.1415926535f ) == 3 );
			static_assert( f32( 1993u ) == 1993.0f );
			static_assert( 2.0 * 2.0 == 4.0 );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
}

U_TEST(ConstexprTest10)
{
	// constexpr array.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var[ f32, 22 ] constexpr zero_arr= zero_init;
			static_assert( zero_arr[7] == 0.0f );

			var [ i32, 3 ] constexpr arr[ 2, 5, -8 ];
			static_assert( arr[0u] == 2 );
			static_assert( arr[1u] == 5 );
			static_assert( arr[2u] == -8 );
			var u32 i2= 2u;
			return arr[0u] + arr[1u] * arr[i2]; // Also, accessing constexpr value using non-constexpr index.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		( 2 + 5 * (-8) ) ==
		static_cast<int32_t>(result_value.IntVal.getLimitedValue()) );
}

U_TEST(ConstexprTest11)
{
	// constexpr twodimensional array.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var [ [ i32, 2 ], 3 ] constexpr arr[ [2, 4 - 5], [5, -0 ], [-8, 887] ];
			static_assert( arr[0u][0u] == 2 );
			static_assert( arr[0u][1u] == 4 - 5 );
			static_assert( arr[1u][0u] == 5 );
			static_assert( arr[1u][1u] == -0 );
			static_assert( arr[2u][0u] == -8 );
			static_assert( arr[2u][1u] == 887 );
			return arr[0u][1u] - arr[2u][0u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		( ( 4 - 5 ) - (-8) ) ==
		static_cast<int32_t>(result_value.IntVal.getLimitedValue()) );
}

U_TEST(ConstexprTest12)
{
	// Taking immutable reference to constant array must work.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &imut x ) : i32 { return x; }
		fn Foo() : i32
		{
			var [ i32, 2 ] constexpr arr[ 158546, 9856 ];
			return Bar( arr[1u] );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 9856 == static_cast<int32_t>(result_value.IntVal.getLimitedValue()) );
}

U_TEST( ImplicitConstexprTest0 )
{
	// Immutable variable without "constexpr", but actually constant must be "constexpr".
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 imut x= 42;
			static_assert( x == x );
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ImplicitConstexprTest1 )
{
	// Immutable array without "constexpr", but actually constant must be "constexpr".
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ f32, 3 ] imut x[ 587.2f, -895.75f, 22254.0f ];
			static_assert( x[0u] == 587.2f && x[1u] == -895.75f && x[2u] == 22254.0f );
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ImplicitConstexprTest2 )
{
	// Immutable auto-variable without "constexpr", but actually constant must be "constexpr".
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			auto imut x= 32769u16;
			static_assert( x == 32769u16 );
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ConstexprReferenceTest0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var i32 constexpr x= 89005;
			var i32 &constexpr x_ref= x; // x_ref must be also "constexpr"
			static_assert( x_ref == x );
			return x_ref;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 89005 == static_cast<int32_t>(result_value.IntVal.getLimitedValue()) );
}

U_TEST( ConstexprReferenceTest1 )
{
	// Auto constexpr reference to array.
	static const char c_program_text[]=
	R"(
		fn Foo() : f64
		{
			var [ f64, 4 ] constexpr arr[ 5.1, -89.11, 3.1415926535, 8451161000.0 ];
			auto &constexpr arr_ref= arr;
			static_assert( arr_ref[0u] == arr[0u] );
			static_assert( arr_ref[1u] == arr[1u] );
			static_assert( arr_ref[2u] == arr[2u] );
			static_assert( arr_ref[3u] == arr[3u] );
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

	U_TEST_ASSERT( 8451161000.0 == result_value.DoubleVal );
}

U_TEST( ConstexprReferenceTest2 )
{
	// Auto constexpr reference to auto constexpr variable.
	static const char c_program_text[]=
	R"(
		fn Foo() : u32
		{
			auto constexpr x= 99999u;
			auto &constexpr x_ref= x;
			return x_ref;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 99999u == static_cast<uint32_t>(result_value.IntVal.getLimitedValue()) );
}

U_TEST( ZeroInitConstexprTest0 )
{
	// Zero initializer must produce constant values.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var i32 constexpr x= zero_init;
			static_assert( x == 0 );
			return 0;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 0 == static_cast<int32_t>(result_value.IntVal.getLimitedValue()) );
}

U_TEST( ZeroInitConstexprTest1 )
{
	// Zero initializer for array must produce constant values.
	static const char c_program_text[]=
	R"(
		fn Foo() : f32
		{
			var [ f32, 16 ] constexpr x= zero_init;
			static_assert( x[9u] == 0.0f );
			return x[9u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 0.0f == result_value.FloatVal );
}

U_TEST( ConstexprVariableInTemplateClass_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ u32 x />
		struct Square
		{
			auto constexpr r= x * x;
		}

		var u32 constexpr square_2017= Square</ 2017u />::r;
		static_assert( square_2017 == 4068289u );
	)";

	BuildProgram( c_program_text );
}

U_TEST( EnumsAreConstexpr )
{
	static const char c_program_text[]=
	R"(
		enum Colors{ Red, Green, Blue, Black, White, Orange, Magenta }
		auto constexpr default_bg_color= Colors::White;
		static_assert( default_bg_color != Colors::Black );
	)";

	BuildProgram( c_program_text );
}

} // namespace

} // namespace U
