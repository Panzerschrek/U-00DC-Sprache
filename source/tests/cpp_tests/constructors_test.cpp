#include "cpp_tests.hpp"

namespace U
{

namespace
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
			var S imut s();
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
			var S imut s( 5741 );
			return s.x * s.y;
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
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
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
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(1) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest8)
{
	// Using default-initialized field for other field initialization.
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
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(1) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest9)
{
	// Call method of default-initialized field for other field initialization.
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

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(1) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest10)
{
	// Zero initializers, assignemnt initializers in initializers list.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			i32 y;
			i32 z;
			fn constructor()
			( x= zero_init, y=x + 5, z(x * y + 42) )
			{
			}
		}
		fn Foo() : i32
		{
			var S s;
			if( s.x == 0 & s.y == 5 & s.z == 42 )
			{ return 1; }
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

	U_TEST_ASSERT( static_cast<uint64_t>(1) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest11)
{
	// Struct named initializer enabled, because struct have no explicit noncopy constructors.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor( S &imut other )
			( x( -other.x ) )
			{}
		}
		fn Foo() : i32
		{
			var S s{ .x= 5 };
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

	U_TEST_ASSERT( static_cast<uint64_t>(5) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest12)
{
	// Zero-initializer enabled, because struct have no explicit noncopy constructors.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor( S &imut other )
			( x= -other.x )
			{}
		}
		fn Foo() : i32
		{
			var S s= zero_init;
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

	U_TEST_ASSERT( static_cast<uint64_t>(0) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest13)
{
	// Generate default-constructor for struct with all fields default-constructible.
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			fn constructor() ( x= 2017 ) {}
		}
		struct B
		{
			A a;
		}
		fn Foo() : i32
		{
			var B b; // should find and call generated default constructor
			return b.a.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(2017) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest14)
{
	// Generate default-constructor for struct with all fields default-constructible and explicit copy-constructor.
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			fn constructor() ( x= 5555578 ) {}
		}
		struct B
		{
			A a;
			fn constructor( B &imut other )
			{
				a.x= other.a.x;
			}
		}
		fn Foo() : i32
		{
			var B b; // should find and call generated default constructor
			return b.a.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(5555578) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest15)
{
	// Generate default-constructor for struct with all fields default-constructible.
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			fn constructor() ( x= 2017 ) {}
		}
		struct B
		{
			[ A, 4 ] a; // Should generate default-constructor, because array of default-constructivle classes is default constructible.
			[ [ [ A, 5 ], 2 ], 3 ] a_3d; // And for multidimensional arrays too.
		}
		fn Foo() : i32
		{
			var B b; // should find and call generated default constructor
			return b.a[2u].x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(2017) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest16)
{
	// Empty struct is default constructible.
	static const char c_program_text[]=
	R"(
		struct A {}
		fn Foo()
		{
			var A a;
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST(ConstructorTest17)
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			f64 y;
		}
		fn Foo() : i32
		{
			var A a{ .x= 58, .y= 11.0 };
			var A a_copy( a ); // "A" class must have auto-generated copy constructor.
			return a_copy.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(58) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest18)
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			[ [ i32, 2 ], 5 ] x;
		}
		fn Foo() : i32
		{
			var A mut a= zero_init;
			a.x[2u][1u]= 54741;
			var A a_copy( a ); // Array fields must be copied.
			return a_copy.x[2u][1u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(54741) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest19)
{
	static const char c_program_text[]=
	R"(
		struct AA{ i32 x; }
		struct A
		{
			AA aa;
		}
		fn Foo() : i32
		{
			var A a{ .aa{ .x= 111112 } };
			var A a_copy( a ); // Copy constructor must copy class field.
			return a_copy.aa.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(111112) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest20)
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			fn constructor() (x= 5566) {}
		}
		struct B
		{
			A a;
			fn constructor(){} // Must call here constructor for 'a' even if there is no initializer list.
		}
		fn Foo() : i32
		{
			var B b;
			return b.a.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(5566) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest21)
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			fn constructor( i32 in_x ) (x= in_x) {}
		}
		struct B
		{
			A imut a;
			fn constructor()
				( a( 4765 ) ) // Call constructor for "imut" value field
			{}
		}
		fn Foo() : i32
		{
			var B b;
			return b.a.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, {} );

	U_TEST_ASSERT( static_cast<uint64_t>(4765) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest22)
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			fn constructor( i32 in_x ) (x= in_x) {}
		}
		struct B
		{
			A imut a;
		}
		fn Foo() : i32
		{
			var B b{ .a( 7886 ) }; // Call constructor of "imut" field
			return b.a.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, {} );

	U_TEST_ASSERT( static_cast<uint64_t>(7886) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest23)
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			fn constructor( i32 in_x ) (x= in_x) {}
		}
		struct B
		{
			A imut a(5554);
			fn constructor() {} // Call constructor of "imut" field

		}
		fn Foo() : i32
		{
			var B b;
			return b.a.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, {} );

	U_TEST_ASSERT( static_cast<uint64_t>(5554) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ConstructorTest24)
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			fn constructor() (x= 987654) {}
		}
		struct B
		{
			A imut a;
			fn constructor() {} // Call constructor of "imut" field

		}
		fn Foo() : i32
		{
			var B b;
			return b.a.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, {} );

	U_TEST_ASSERT( static_cast<uint64_t>(987654) == result_value.IntVal.getLimitedValue() );
}

} // namespace

} // namespace U
