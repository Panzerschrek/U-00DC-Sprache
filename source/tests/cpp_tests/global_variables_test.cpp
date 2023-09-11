#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( GlobalVariablesTest0 )
{
	// Simple global variable.
	static const char c_program_text[]=
	R"(
		var f64 constexpr pi= 3.1415926535;

		fn Foo() : f64
		{
			return pi;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 3.1415926535 == result_value.DoubleVal );
}

U_TEST( GlobalVariablesTest1 )
{
	// Simple global auto variable.
	static const char c_program_text[]=
	R"(
		auto constexpr g_current_year= 2017u;

		fn Foo() : u32
		{
			return g_current_year;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(2017u) == result_value.IntVal.getLimitedValue() );
}

U_TEST( GlobalVariablesTest2_ShouldTakeAddressOfGlobalVariable )
{
	static const char c_program_text[]=
	R"(
		fn Proxy( f64 &imut x ) : f64 { return x; }

		var f64 constexpr g_centimeters_in_inch= 2.54;

		fn Foo() : f64
		{
			return Proxy( g_centimeters_in_inch );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 2.54 == result_value.DoubleVal );
}

U_TEST( GlobalVariablesTest3_ShouldTakeAddressOfGlobalAutoVariable )
{
	static const char c_program_text[]=
	R"(
		fn Proxy( f32 &imut x ) : f32 { return x; }

		auto constexpr g_centimeters_in_inch= 2.54f;

		fn Foo() : f32
		{
			return Proxy( g_centimeters_in_inch );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 2.54f == result_value.FloatVal );
}

U_TEST( GlobalVariablesTest4_GlobalConstantArray )
{
	static const char c_program_text[]=
	R"(
		var [ u8, 6 ] constexpr arr[ 4u8, 8u8, 15u8, 16u8, 23u8, 42u8 ];

		fn Foo() : u32
		{
			static_assert( arr[0u] ==  4u8 );
			static_assert( arr[1u] ==  8u8 );
			static_assert( arr[2u] == 15u8 );
			static_assert( arr[3u] == 16u8 );
			static_assert( arr[4u] == 23u8 );
			static_assert( arr[5u] == 42u8 );
			var u32 mut i= 0u, mut s= 0u;
			while( i < 6u )
			{
				s+= u32(arr[i]);
				++i;
			}
			return s;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 4 + 8 + 15 + 16 + 23 + 42 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( GlobalVariablesTest5_GlobalReferences )
{
	static const char c_program_text[]=
	R"(
		var i16 constexpr g_x= i16(-5847), constexpr g_y(18547);
		var i16 &constexpr g_x_ref= g_x;
		auto &constexpr g_y_ref= g_y;

		fn Convert( i16 &imut x ) : i32 { return i32(x); }
		fn Foo() : i32
		{
			return Convert(g_x_ref) + i32(g_y_ref);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( -5847 + 18547 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( GlobalVariablesTest6_GlobalVariableUsedInOtherGlobalVariableInitializer )
{
	static const char c_program_text[]=
	R"(
		var [ i32, 2 ] constexpr g_a[ 42, 558 ];
		auto constexpr g_b= 55847;
		var f32 constexpr g_c(3.14);

		var i32 constexpr g_res= ( g_a[0u] * g_a[1u] + g_b ) / i32(g_c); // Init this variable using another variables.

		fn Proxy( i32 &imut x ) : i32 { return x; }

		fn Foo() : i32
		{
			return Proxy( g_res );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( ( 42 * 558 + 55847 ) / int(3.14) ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( GlobalVariablesTest7_ImutGlobalVariablesWithConstantInitializers )
{
	static const char c_program_text[]=
	R"(
		// "imut" here implicitly convertred into "constexpr", because initializers are constant.
		var i32 imut g_x= 55;
		auto imut g_y= 8845;

		fn Foo() : i32
		{
			return g_y - g_x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 8845 - 55 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( GlobalVariablesTest8_GlobalVariablesInsideNamespaces )
{
	static const char c_program_text[]=
	R"(
		namespace Math
		{
			var f32 imut pi= 3.1415926535f;
			auto constexpr e= 2.718281828f;
		}

		fn Foo() : f32
		{
			return Math::pi * Math::e;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 3.1415926535f * 2.718281828f == result_value.FloatVal );
}

U_TEST( GlobalVariablesTest9_GlobalVariablesInsideClasses )
{
	static const char c_program_text[]=
	R"(
		struct Math
		{
			var f32 imut pi= 3.1415926535f;
			auto constexpr e= 2.718281828f;
		}

		fn Foo() : f32
		{
			return Math::pi * Math::e;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 3.1415926535f * 2.718281828f == result_value.FloatVal );
}

U_TEST( GlobalVariablesTest10_GlobalVariablesInsideClassTemplates )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct MathConstants</ T />
		{
			var T imut pi= T(3.1415926535);
			auto constexpr e= T(2.718281828);
		}

		fn Foo() : f32
		{
			return MathConstants</f32/>::pi * MathConstants</f32/>::e;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 3.1415926535f * 2.718281828f == result_value.FloatVal );
}

U_TEST( GlobalStaticAssert_Test0 )
{
	static const char c_program_text[]=
	R"(
		auto constexpr pi= 3.1415;
		static_assert( pi > 3.0 && pi < 4.0 );
	)";

	BuildProgram( c_program_text );
}

U_TEST( GlobalStaticAssert_Test1 )
{
	static const char c_program_text[]=
	R"(
		auto constexpr pi= 3.1415;
		static_assert( pi < 0.0 );
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::StaticAssertionFailed );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( StaticAssertInsideClass_Test0 )
{
	static const char c_program_text[]=
	R"(
		auto constexpr pi= 3.1415;
		struct S
		{
			static_assert( pi > 3.0 && pi < 4.0 );
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( StaticAssertInsideClass_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			auto constexpr pi= 3.1415;
			static_assert( pi < 0.0 );
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::StaticAssertionFailed );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

} // namespace

} // namespace U
