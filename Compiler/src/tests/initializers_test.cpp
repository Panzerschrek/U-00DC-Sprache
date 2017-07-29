#include <cstdlib>
#include <iostream>

#include "tests.hpp"

namespace U
{

U_TEST(ExpressionInitializerTest0)
{
	// Expression initializer for integers
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		var i32 x= 2017;
		return x;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 2017 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ExpressionInitializerTest1)
{
	// Expression initializer for floats
	static const char c_program_text[]=
	R"(
	fn Foo() : f64
	{
		var f64 x = 2017.52;
		return x;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 2017.52 == result_value.DoubleVal );
}

U_TEST(ExpressionInitializerTest2)
{
	// Expression initializer for references
	static const char c_program_text[]=
	R"(
	fn Foo() : f64
	{
		var f64 x = 2017.52;
		var f64 &x_ref= x;
		return x_ref;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 2017.52 == result_value.DoubleVal );
}

U_TEST(ConstructorInitializerForFundamentalTypesTest0)
{
	// Constructor initializer for integers
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		var i32 x( 2017 );
		return x;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 2017 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorInitializerForFundamentalTypesTest1)
{
	// Constructor initializer for floats
	static const char c_program_text[]=
	R"(
	fn Foo() : f64
	{
		var f64 x( 2017.52 );
		return x;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 2017.52 == result_value.DoubleVal );
}

U_TEST(ConstructorInitializerForReferencesTest0)
{
	// Constructor initializer for floats
	static const char c_program_text[]=
	R"(
	fn Foo() : f64
	{
			var f64 x = 2017.52;
			var f64 &x_ref(x);
			return x_ref;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 2017.52 == result_value.DoubleVal );
}

U_TEST(ArrayInitializerForFundamentalTypesTest0)
{
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		var [ i32, 3u32 ] x[ 42, 34, 785 ];
		return x[0u32] * x[1u32] - x[2u32];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 42 * 34 - 785 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ArrayInitializerForFundamentalTypesTest1)
{
	// Array initializer with comma at end must works correctly
	static const char c_program_text[]=
	R"(
	fn Foo() : f32
	{
		var [ f32, 3u32 ] x[ 42.5f32, 34.0f32, 785.7f32, ];
		return x[0u32] * x[1u32] - x[2u32];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 42.5f * 34.0f - 785.7f == result_value.FloatVal );
}

U_TEST(TwodimensionalArrayInitializerTest0)
{
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		var [ [ i32, 2u32 ], 3u32 ] mat
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		static_cast<uint64_t>( (175) + (-8 * 5) + (95684) + (48) + (-14) * (2 + 2 * 2 ) ) ==
		result_value.IntVal.getLimitedValue() );
}

U_TEST(StructNamedInitializersTest0)
{
	static const char c_program_text[]=
	R"(
	struct Point{ i32 x; i32 y; }
	fn Foo() : i32
	{
		var Point point{ .x= 5877, .y(13) };
		return point.x / point.y;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 5877 / 13 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(StructNamedInitializersTest1)
{
	// Members may be initialized in any order.
	static const char c_program_text[]=
	R"(
	struct Point{ i32 x; i32 y; [ bool, 3 ] z; }
	fn Foo() : i32
	{
		var Point point{ .y(13), .z[ false, false, true ], .x= 5877 };
		if( point.z[0u32] == false & point.z[1u32] == false & point.z[2u32] == true )
		{
			return point.x / point.y;
		}
		return 0;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 5877 / 13 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(StructNamedInitializersTest2)
{
	// Struct inside struct.
	static const char c_program_text[]=
	R"(
	struct Double{ f64 d; }
	struct Point2d{ Double x; Double y; }
	struct Point3d{ Point2d xy; Double z; }
	struct Point{ i32 x; i32 y; [ bool, 3 ] z; }
	fn Foo() : f64
	{
		var Point3d point
		{
			.xy
			{
				.y{ .d(548.7) },
				.x{ .d= -5.4 }
			},
			.z { .d= 14.2, }
		};
		return point.xy.x.d + point.xy.y.d / point.z.d;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( ( -5.4 + 548.7 / 14.2 ) == result_value.DoubleVal );
}

U_TEST(StructNamedInitializersTest3)
{
	// Array inside struct inside array inside struct.
	static const char c_program_text[]=
	R"(
	struct A{ [ i32, 2 ] arr; }
	struct B{ [ A, 3 ] a_arr; }
	fn Foo() : i32
	{
		var B bb
		{
			.a_arr
			[
				{ .arr[ 5, -7 ] },
				{ .arr[ 874, 81 ] },
				{ .arr[ 458, 24 ] },
			]
		};

		return
			bb.a_arr[0u32].arr[0u32] * bb.a_arr[0u32].arr[1u32] +
			bb.a_arr[1u32].arr[0u32] / bb.a_arr[1u32].arr[1u32] +
			bb.a_arr[2u32].arr[0u32] - bb.a_arr[2u32].arr[1u32];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		static_cast<uint64_t>( (5) * (-7) + (874) / 81 + 458 - 24 ) ==
		result_value.IntVal.getLimitedValue() );
}

U_TEST(ZeroInitilaizerTest0)
{
	// Zero-initialzier for int.
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		var i32 x= zero_init;
		return x;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 0 == result_value.IntVal.getLimitedValue() );
}

U_TEST(ZeroInitilaizerTest1)
{
	// Zero-initialzier for float.
	static const char c_program_text[]=
	R"(
	fn Foo() : f32
	{
		var f32 x= zero_init;
		return x;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 0.0f == result_value.FloatVal );
}

U_TEST(ZeroInitilaizerTest2)
{
	// Zero-initialzier for double.
	static const char c_program_text[]=
	R"(
	fn Foo() : f64
	{
		var f64 x= zero_init;
		return x;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 0.0 == result_value.DoubleVal );
}

U_TEST(ZeroInitilaizerTest3)
{
	// Zero-initialzier for bool.
	static const char c_program_text[]=
	R"(
	fn Foo() : bool
	{
		var bool x= zero_init;
		return x;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 0 == result_value.IntVal.getLimitedValue() );
}

U_TEST(ZeroInitilaizerTest4)
{
	// Zero-initialzier for int array.
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		var [ i32, 4 ] x= zero_init;
		return x[0u32] + x[1u32] + x[2u32] + x[3u32];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 0 == result_value.IntVal.getLimitedValue() );
}

U_TEST(ZeroInitilaizerTest5)
{
	// Zero-initialzier for struct.
	static const char c_program_text[]=
	R"(
	struct S{ f32 x; f32 y; }
	fn Foo() : f32
	{
		var S s= zero_init;
		return s.x + s.y;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 0.0f == result_value.FloatVal );
}

U_TEST(ZeroInitilaizerTest6)
{
	// Zero-initialzier for struct member.
	static const char c_program_text[]=
	R"(
	struct S{ f32 x; f32 y; }
	fn Foo() : f32
	{
		var S s{ .y= 42.0f32, .x= zero_init };
		return s.x;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 0.0f == result_value.FloatVal );
}

U_TEST(ZeroInitilaizerTest7)
{
	// Zero-initialzier for array member.
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		var [ i32, 2 ] x[ 42, zero_init, ];
		return x[1u32];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 0 == result_value.IntVal.getLimitedValue() );
}


U_TEST(ZeroInitilaizerTest8)
{
	// Zero-initialzier for very-large array.
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		var [ i32, 4096 ] imut arr= zero_init;
		var u32 i= 0u;
		var i32 result= 0;
		while( i < 4096u )
		{
			result= result | arr[i];
			i= i + 1u;
		}
		return result;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 0 == result_value.IntVal.getLimitedValue() );
}

} // namespace U
