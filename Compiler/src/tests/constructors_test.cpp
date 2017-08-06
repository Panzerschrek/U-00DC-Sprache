#include "tests.hpp"

namespace U
{

U_TEST(ConstructorTest0)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor()
			( x(42) )
			{
			}
		}
		fn Foo() : i32
		{
			var S s();
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

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest1)
{
	// Constructor with nonzero arguments.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			i32 y;
			fn constructor( i32 a )
			( x(a), y(a * 5) )
			{
			}
		}
		fn Foo() : i32
		{
			var S s( 5741 );
			return s.x * s.y;
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
		static_cast<uint64_t>( 5741 * 5741 * 5 ) ==
		result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest2)
{
	// Constructors overloading.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			i32 y;
			fn constructor( i32 a )
			( x(a), y(0) )
			{}
			fn constructor( i32 a, i32 b )
			( x(a), y(b) )
			{}
		}
		fn Foo() : i32
		{
			var S a( 58 ), b( -184, 9854 );
			if( a.x == 58 & a.y == 0 & b.x == -184 & b.y == 9854 )
			{ return 1; }
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

	U_TEST_ASSERT( static_cast<uint64_t>( 1 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest3)
{
	// Initialize one field, using another field
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			i32 y;
			fn constructor()
			(
				x( 458 ),
				y( 0 - x )
			)
			{}
		}
		fn Foo() : i32
		{
			var S s();
			if( s.x == 458 & s.y == -458 )
			{ return 1; }
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

	U_TEST_ASSERT( static_cast<uint64_t>(1) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest4)
{
	// Implicit call of default constructor.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor()
			( x( 458 ) )
			{}
		}
		fn Foo() : i32
		{
			var S s;
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

	U_TEST_ASSERT( static_cast<uint64_t>(458) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest5)
{
	// Implicit call of default constructor for array of structures.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor()
			( x( 458 ) )
			{}
		}
		fn Foo() : i32
		{
			var [ S, 3 ] s;
			return s[2u].x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(458) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest6)
{
	// Implicit call of field default constructor in other constructor.
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			fn constructor()
			( x(-1) )
			{}
		}
		struct B
		{
			A x;
			i32 y;
			fn constructor()
			( y(1) ) // constructor for "x" called implicitly.
			{}
		}
		fn Foo() : i32
		{
			var B b;
			if( b.x.x == -1 & b.y == 1 )
			{ return 1; }
			return -1;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(1) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest7)
{
	// Implicit call of field default constructor in other constructor.
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			fn constructor()
			( x(-1) )
			{}
		}
		struct B
		{
			[ A, 3 ] x;
			i32 y;
			fn constructor()
			( y(1) ) // constructor for "x" called implicitly.
			{}
		}
		fn Foo() : i32
		{
			var B b;
			if( b.x[1u].x == -1 & b.y == 1 )
			{ return 1; }
			return -1;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(1) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest8)
{
	// Using default-initialized filed for other filed initialization.
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			fn constructor()
			( x(-1) )
			{}
		}
		struct B
		{
			A x;
			i32 y;
			fn constructor()
			( y(x.x) )
			{}
		}
		fn Foo() : i32
		{
			var B b;
			if( b.x.x == -1 & b.y == -1 )
			{ return 1; }
			return -1;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(1) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest9)
{
	// Call method of default-initialized filed for other filed initialization.
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			fn constructor()
			( x(-1) )
			{}
			fn GetDoubleX( imut this ) : i32 { return x * 2; }
		}
		struct B
		{
			A x;
			i32 y;
			fn constructor()
			( y( x.GetDoubleX() ) )
			{}
		}
		fn Foo() : i32
		{
			var B b;
			if( b.x.x == -1 & b.y == -2 )
			{ return 1; }
			return -1;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ), true );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(1) == result_value.IntVal.getLimitedValue() );
}

} // namespace U
