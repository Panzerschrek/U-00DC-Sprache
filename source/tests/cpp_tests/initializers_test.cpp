#include <cstdlib>
#include "cpp_tests.hpp"

namespace U
{

namespace
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 2017.52 == result_value.DoubleVal );
}

U_TEST(ExpressionInitializerTest3)
{
	// Expression initializer for structs.
	static const char c_program_text[]=
	R"(
		struct Vec{ i32 x; i32 y; }
		fn Foo() : i32
		{
			var Vec v0{ .x=584, .y=-98 };
			var Vec v1= v0;
			return v1.x - v1.y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 584 - (-98) ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ExpressionInitializerTest4)
{
	// Expression initializer for tuples.
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; i32 y; }
		fn Foo()
		{
			var tup[ i32, f32, S, [ bool, 2 ] ] mut t= zero_init;
			t[0u]= 43;
			t[1u]= 0.25f;
			t[2u].x= 11;
			t[2u].y= 336;
			t[3u][0u]= true;
			t[3u][1u]= false;

			// Here tuple must be recursively copied. For structs copy constructor must be called.
			var tup[ i32, f32, S, [ bool, 2 ] ] t_copy= t;

			halt if( t_copy[0u] != 43 );
			halt if( t_copy[1u] != 0.25f );
			halt if( t_copy[2u].x != 11 );
			halt if( t_copy[2u].y != 336 );
			halt if( t_copy[3u][0u] != true );
			halt if( t_copy[3u][1u] != false );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, {} );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 2017.52 == result_value.DoubleVal );
}


U_TEST(ConstructorInitializer_ForTuples_Test0)
{
	// Constructor initializer for tuples - make copy of tuple.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var tup[ i32, f32 ] t[ 562, 3.0f + 2.0f ];
			var tup[ i32, f32 ] t_copy(t);
			return i32(t_copy[0u]) - i32(t_copy[1u]);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, {} );
	U_TEST_ASSERT( result_value.IntVal.getLimitedValue() == 562u - 5u );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		static_cast<uint64_t>( (175) + (-8 * 5) + (95684) + (48) + (-14) * (2 + 2 * 2 ) ) ==
		result_value.IntVal.getLimitedValue() );
}

U_TEST(ArrayInitializer_ForTuples_Test0)
{
	// Array initializer for tuples
	static const char c_program_text[]=
	R"(
		fn Foo() : f64
		{
			var tup[ i32, f32 ] t[ 668, 2.0f + 2.0f ];
			return f64(t[0u]) - f64(t[1u]);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, {} );
	U_TEST_ASSERT( result_value.DoubleVal == 664.0 );
}

U_TEST(ArrayInitializer_ForTuples_Test1)
{
	// Array initializer for tuples with 1 element.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var tup[ i32 ] t[ 666 ];
			var tup[ i32 ] t_copy(t);
			return t_copy[0u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, {} );
	U_TEST_ASSERT( result_value.IntVal.getLimitedValue() == 666u );
}

U_TEST(ArrayInitializer_ForTuples_Test2)
{
	// Array initializer for empty tuple.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var tup[] t[];
		}
	)";

	BuildProgram( c_program_text );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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
		var u32 mut i= 0u;
		var i32 mut result= 0;
		while( i < 4096u )
		{
			result= result | arr[i];
			i= i + 1u;
		}
		return result;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 0 == result_value.IntVal.getLimitedValue() );
}

U_TEST(EmptyInitializerTest0)
{
	// Initialize array of empty structs with default constructor.
	static const char c_program_text[]=
	R"(
	struct S{}
	fn Foo()
	{
		var [ S, 64 ] s;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
}

U_TEST(EmptyInitializerTest1)
{
	// Initialize struct with default contructor.
	static const char c_program_text[]=
	R"(
	struct A
	{
		[ i32, 24 ] a;
		fn constructor()
		( a=zero_init )
		{
			a[17u]= 586;
		}
	}
	struct S // this struct must have generated default constructor, because it have all fields default-constructible
	{
		[ A, 11 ] as;
	}
	fn Foo() : i32
	{
		var S s;
		return s.as[5u].a[17u];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 586 == result_value.IntVal.getLimitedValue() );
}

U_TEST(DefaultInitializationForStructMembersTest0)
{
	static const char c_program_text[]=
	R"(
	struct vec
	{
		[ i32, 3 ] xyz;
		fn constructor() ( xyz[ 0, zero_init, 0 ] ) {}
	}
	struct S
	{
		vec min; vec max;
	}
	fn Foo() : i32
	{
		var S s{}; // S members default-initialized.
		return s.min.xyz[0u] + s.min.xyz[1u] + s.min.xyz[2u] + s.max.xyz[0u] + s.max.xyz[1u] + s.max.xyz[2u];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 0 == result_value.IntVal.getLimitedValue() );
}

U_TEST(DefaultInitializationForStructMembersTest1)
{
	static const char c_program_text[]=
	R"(
	struct vec
	{
		[ i32, 3 ] xyz;
		fn constructor() ( xyz[ 0, zero_init, 0 ] ) {}
	}
	struct S
	{
		vec min; i32 c;
	}
	fn Foo() : i32
	{
		var S s{ .c= 5 }; // "min" member default-initialized.
		return s.min.xyz[0u] + s.min.xyz[1u] + s.min.xyz[2u] + s.c;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 5 == result_value.IntVal.getLimitedValue() );
}

U_TEST(InitializerForZeroSizedArray_Test0)
{
	static const char c_program_text[]=
	R"(
		struct S{ i32& x; } // Struct with reference, without any initializer.
		struct A{ [ S, 0 ] s; } // Have generated default initilizer, because all fields have default initializers.
		fn Foo()
		{
			var A a; // Ok, default initializer used.
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace

} // namespace U
