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

static void StructNamedInitializersTest0()
{
	static const char c_program_text[]=
	R"(
	class Point{ x : i32; y : i32; }
	fn Foo() : i32
	{
		let : Point point{ .x= 5877, .y(13) };
		return point.x / point.y;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( static_cast<uint64_t>( 5877 / 13 ) == result_value.IntVal.getLimitedValue() );
}

static void StructNamedInitializersTest1()
{
	// Members may be initialized in any order.
	static const char c_program_text[]=
	R"(
	class Point{ x : i32; y : i32; z : [ bool, 3 ]; }
	fn Foo() : i32
	{
		let : Point point{ .y(13), .z[ false, false, true ], .x= 5877 };
		if( point.z[0u32] == false & point.z[1u32] == false & point.z[2u32] == true )
		{
			return point.x / point.y;
		}
		return 0;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( static_cast<uint64_t>( 5877 / 13 ) == result_value.IntVal.getLimitedValue() );
}

static void StructNamedInitializersTest2()
{
	// Struct inside struct.
	static const char c_program_text[]=
	R"(
	class Double{ d : f64; }
	class Point2d{ x : Double; y : Double; }
	class Point3d{ xy : Point2d; z : Double; }
	class Point{ x : i32; y : i32; z : [ bool, 3 ]; }
	fn Foo() : f64
	{
		let : Point3d point
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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT( ( -5.4 + 548.7 / 14.2 ) == result_value.DoubleVal );
}

static void StructNamedInitializersTest3()
{
	// Array inside struct inside array inside struct.
	static const char c_program_text[]=
	R"(
	class A{ arr : [ i32, 2 ]; }
	class B{ a_arr : [ A, 3 ]; }
	fn Foo() : i32
	{
		let : B bb
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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_ASSERT(
		static_cast<uint64_t>( (5) * (-7) + (874) / 81 + 458 - 24 ) ==
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
	StructNamedInitializersTest0();
	StructNamedInitializersTest1();
	StructNamedInitializersTest2();
	StructNamedInitializersTest3();
}

} // namespace U
