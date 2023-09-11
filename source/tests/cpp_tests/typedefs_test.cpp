#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( TypedefsTest0 )
{
	// Simple test.
	static const char c_program_text[]=
	R"(
		type my_size_type= u64; // global typedef
		struct C
		{
			type index_type= my_size_type; // typedef inside struct
		}
		namespace S
		{
			type C= ::C; // typedef inside namespace
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( TypedefsTest1_TypedefForFundamentalType )
{
	static const char c_program_text[]=
	R"(
		type i32_alias= i32;

		fn Foo() : i32
		{
			var i32_alias result= 55847;
			return result;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 55847 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( TypedefsTest2_TypedefForFundamentalTypeInsideClass )
{
	static const char c_program_text[]=
	R"(
		class S
		{
			type f64_alias= f64;
		}

		fn Foo() : f64
		{
			var S::f64_alias result= 3.1415;
			return result;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 3.1415 == result_value.DoubleVal );
}

U_TEST( TypedefsTest3_TypedefForStruct )
{
	static const char c_program_text[]=
	R"(
		struct Vec2_of_i32{ i32 x; i32 y; }

		type vec= Vec2_of_i32;

		fn Foo() : i32
		{
			var vec result{ .x= 558, .y= 9856474 };
			return result.y - result.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 9856474 - 558 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( TypedefsTest4_TypedefForGeneratedFromTemplateClass )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct Vec</ T />{ T x; T y; }

		type vecf= Vec</ f32 />;

		fn Foo() : f32
		{
			var vecf result{ .x= 541.2f, .y= 0.25f };
			return result.x * result.y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 541.2f * 0.25f == result_value.FloatVal );
}

U_TEST( TypedefsTest5_TypedefForArray )
{
	static const char c_program_text[]=
	R"(
		type vec3= [ f32, 3 ];

		fn Foo() : f32
		{
			var vec3 result[ 0.334f, 8457.1f, 8874.5f ];
			return result[0u] * result[1u] + result[2u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 0.334f * 8457.1f +  8874.5f == result_value.FloatVal );
}

U_TEST( TypedefsTest6_TypedefForTwodimensionalArray )
{
	static const char c_program_text[]=
	R"(
		type mat2x2= [ [ f32, 2 ], 2 ];

		fn Det( mat2x2 &imut m ) : f32
		{
			return m[0u][0u] * m[1u][1u] - m[0u][1u] * m[1u][0u];
		}

		fn Foo() : f32
		{
			var mat2x2 mat[ [ 5.0f, 45.0f ], [ 7.0f, 41.0f ] ];
			return Det(mat);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 5.0f * 41.0f - 45.0f * 7.0f == result_value.FloatVal );
}

U_TEST( TypedefsTest7_TypedefForTemplateparameter )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct Box</ T />
		{
			type Boxed= T;
			Boxed t;
		}

		fn Foo() : f32
		{
			var Box</ f64 /> box{ .t= 1.457 };
			return Box</ f32 />::Boxed( box.t );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 1.457f == result_value.FloatVal );
}

U_TEST( TypedefsTemplates_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T, u64 size /> struct ArrayWrapper</ T, size /> { [ T, size ] a; }

		template</ type T />
		type vec4</ T />= ArrayWrapper</ T, 4u64 />;

		struct SSS
		{
			template</ type T />
			type vec4_of_vec4</ T />= vec4</ vec4</ T /> />;
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( TypedefsTemplates_Test1 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Box</ T /> { T t; }   // base template

		template</ type T /> type Box_alias</ T /> = Box</ T />;   // alias for this template

		fn Foo() : i32
		{
			var Box_alias</ i32 /> box{ .t= 88884 };    // usage of alias
			return box.t;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 88884 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( TypedefsTemplates_Test2_TypedefTemplateForArray )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> type Pair</ T /> = [ T, 2 ];

		fn Foo() : f64
		{
			var Pair</ f64 /> p[ 4.5, 0.25 ];
			return p[0u] / p[1u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 4.5 / 0.25 == result_value.DoubleVal );
}

U_TEST( TypedefsTemplates_Test4_TypedefTemplateForArray )
{
	static const char c_program_text[]=
	R"(
		template</ type size_type, size_type size /> type Fvec</ size /> = [ f32, size ];

		fn Foo() : f32
		{
			var Fvec</ 3 /> v[ 4.5f, 0.25f, 54.1f ];
			return v[0u] + v[1u] + v[2u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 4.5f + 0.25f + 54.1f == result_value.FloatVal );
}

U_TEST( TypedefsTemplates_Test5_ComplexSignatureArgument )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Box</ T /> { T t; }

		template</ type T /> type Unbox</ Box</ T /> /> = T;

		fn Foo() : i32
		{
			var Unbox</ Box</ i32 /> /> unboxed= 5584;
			return unboxed;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 5584 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( TypedefsTemplates_Test6_TypedefTemplateForTypedefTemplate )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Box</ T /> { T t; }   // base template

		template</ type T /> type Box_alias</ T /> = Box</ T />;   // alias for this template
		template</ type T /> type Box_alias_alias</ T /> = Box_alias</ T />;   // alias for alias

		fn Foo() : i32
		{
			var Box_alias_alias</ i32 /> box{ .t= 88884 };    // usage of alias of alias
			return box.t;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 88884 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( TypedefsTemplates_Test7_ShortTypedefTemplateForm_Test0 )
{
	// Short typedef template form for array.
	static const char c_program_text[]=
	R"(
		template</ type T /> type Pair= [ T, 2 ];

		fn Foo() : f64
		{
			var Pair</ f64 /> p[ 4.55, 0.2 ];
			return p[0u] - p[1u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 4.55 - 0.2 == result_value.DoubleVal );
}

U_TEST( TypedefsTemplates_Test8_ShortTypedefTemplateForm_Test1 )
{
	// Short typedef template form for class template.
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Box</ T /> { T t; }   // base template

		template</ type T /> type Box_alias = Box</ T />;   // alias for this template

		fn Foo() : i32
		{
			var Box_alias</ i32 /> box{ .t= 23541 };    // usage of alias
			return box.t;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 23541 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( TypedefInsideFunction_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			type I= i32;
			var I i= 66;
			static_assert( i == 66 );
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( TypedefInsideFunction_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			type I= i32;
			{
				type I= u16; // Shadow type name from outer namespace
				var I i= 33u16;
				static_assert( i == 33u16 );
			}
			var I i= 77; // Ok, use previous value of type alias.
			static_assert( i == 77 );
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( TypedefInsideFunction_Test2 )
{
	static const char c_program_text[]=
	R"(
		type I= i32;
		fn Foo()
		{
			type I= f64; // Shadow global type alias.
			var I i= 99.5;
			static_assert( i == 99.5 );
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( TypedefInsideFunction_Test3 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			type I= char8;
			type J= I;
			var J i= "Q"c8;
			static_assert( i == "Q"c8 );
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( TypedefInsideFunction_Test4 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			auto s= 33s;
			type Arr= [ i32, s ]; // Use variable from function name space inside type name in type alias.
			static_assert( typeinfo</Arr/>.element_count == 33s );
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace

} // namespace U
