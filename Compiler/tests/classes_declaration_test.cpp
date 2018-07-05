#include "tests.hpp"

namespace U
{

U_TEST( ClassesDeclarationTest0 )
{
	// Declare class, then, declare it body in same namespace.
	static const char c_program_text[]=
	R"(
		struct A;
		fn GetAX( A &imut a ) : i32;
		struct A { i32 x; }
		fn GetAX( A &imut a ) : i32
		{
			return a.x;
		}
		fn Foo() : i32
		{
			var A a{ .x= 586 };
			return GetAX(a);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 586 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassesDeclarationTest1 )
{
	// Declare class, then, declare it body in different namespace.
	static const char c_program_text[]=
	R"(
		namespace AA{ struct A; }
		fn GetAX( AA::A &imut a ) : i32;
		struct AA::A { i32 x; }
		fn GetAX( AA::A &imut a ) : i32
		{
			return a.x;
		}
		fn Foo() : i32
		{
			var AA::A a{ .x= 586 };
			return GetAX(a);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 586 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassesDeclarationTest2 )
{
	// Classes, declared inside namespace is differnet from classes from outer space.
	static const char c_program_text[]=
	R"(
		struct A{}
		struct B;
		struct C{}
		struct D;
		namespace NN
		{
			struct A{}
			struct B;
			struct C;
			struct D{}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ClassesDeclarationTest3 )
{

	static const char c_program_text[]=
	R"(
		namespace Private
		{
			struct A;
		}
		struct Private::A // full class body declared in other namespace
		{ i32 x; }
		fn Foo() : i32
		{
			var Private::A a{ .x= 586 };
			return a.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 586 ) == result_value.IntVal.getLimitedValue() );
}

} // namespace U
