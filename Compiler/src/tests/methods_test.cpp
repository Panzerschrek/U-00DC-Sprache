#include "tests.hpp"

namespace U
{

U_TEST(MethodTest0)
{
	// Simple declaration of static method.
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Get42() : i32 { return 42; }
		}
		fn Foo() : i32
		{
			var S s= zero_init;
			return s.Get42();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(MethodTest1)
{
	// Should call this method.
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Get42( this ) : i32 { return 42; }
		}
		fn Foo() : i32
		{
			var S s= zero_init;
			return s.Get42();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(MethodTest2)
{
	// Access to struct field via "this".
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn GetX( this ) : i32 { return this.x; }
		}
		fn Foo() : i32
		{
			var S s{ .x= 84167 };
			return s.GetX();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 84167 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(MethodTest3)
{
	// Access to struct field directly.
	static const char c_program_text[]=
	R"(
		struct S
		{
			[ i32, 4 ] something_unsignificant_;
			f64 x_;
			fn GetX( this ) : f64 { return x_; }
		}
		fn Foo() : f64
		{
			var S s{ .x_= 84167.1, .something_unsignificant_= zero_init };
			return s.GetX();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ), true );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 84167.1 == result_value.DoubleVal );
}

} // namespace U
