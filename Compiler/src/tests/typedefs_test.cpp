#include "tests.hpp"

namespace U
{

U_TEST( TypedefsTest0 )
{
	// Simple test.
	static const char c_program_text[]=
	R"(
		type size_type= u64; // global typedef
		struct C
		{
			type index_type= size_type; // typedef inside struct
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

} // namespace U
