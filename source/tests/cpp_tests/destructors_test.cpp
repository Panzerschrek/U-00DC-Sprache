#include "cpp_tests.hpp"

namespace U
{

namespace
{

std::vector<int> g_destructors_call_sequence;

llvm::GenericValue DestructorCalled(
	llvm::FunctionType*,
	llvm::ArrayRef<llvm::GenericValue> args )
{
	g_destructors_call_sequence.push_back( static_cast<int>(args[0].IntVal.getLimitedValue()) );
	return llvm::GenericValue();
}

void DestructorTestPrepare(const EnginePtr& engine)
{
	g_destructors_call_sequence.clear();

	engine->RegisterCustomFunction( "_Z16DestructorCalledi", DestructorCalled );
}

U_TEST(DestructorsTest0)
{
	// Must call destructor for variable.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor() ( x= 0 ) {}
			fn destructor()
			{
				x= 854;
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			var S s;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence.size() == 1u && g_destructors_call_sequence.front() == 854 );
}

U_TEST(DestructorsTest1)
{
	// Must call destructors in reverse order.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			var S s0(0), s1(1), s2(2);
			var S imut s3(3);
			{ var S s4(4); } // s4 must be destructed before s5
			var S s5(5);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 4, 5, 3, 2, 1, 0 } ) );
}

U_TEST(DestructorsTest2)
{
	// Destructors for arguments.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Bar( S arg0, S arg1 )
		{
			var S s_local(0);
		}
		fn Foo()
		{
			Bar( S(88), S(66) );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 0,  66, 88 } ) );
}

U_TEST(DestructorsTest3)
{
	// Must call destructors after return.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Bar( bool b )
		{
			var S s0(0);
			if( b )
			{
				var S s1(1);
				return;
			}
			var S s2(2);
		}
		fn Foo()
		{
			Bar(false);
			Bar(true);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 2, 0,    1, 0 } ) );
}

U_TEST(DestructorsTest4)
{
	// Must call destructors after return.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Bar( bool b )
		{
			var S s0(0);
			if( b )
			{
				var S s1(1);
				return;
			}
			else
			{
				return;
			}
			// Here must be no destructors code.
		}
		fn Foo()
		{
			Bar(false);
			Bar(true);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 0,    1, 0 } ) );
}

U_TEST(DestructorsTest5)
{
	// Must call destructors of arguments in each return.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Bar( S s )
		{
			if( s.x == 0 ) { return; }
			if( s.x == 1 ) {{{ return; }}}
			if( s.x == 2 ) { return; }
			{ var S new_local(555); }
		}
		fn Foo()
		{
			Bar(S(0));
			Bar(S(1));
			Bar(S(2));
			Bar(S(3));
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 0,  1,  2,  555, 3, } ) );
}

U_TEST(DestructorsTest6)
{
	// Must call destructors of loop local variables before "break".
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Foo()
		{
			var S live_longer_than_loop(-1);
			while(true)
			{
				var S s0(0);
				{
					var S s1(1), s2(2);
					var S s3(3);
					if( true )
					{
						var S s4(4); // must destroy s4, s3, s2, s1, s0
						break;
					}
				}
			}

			while(true)
			{
				var S s5(5);
				break;
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 4, 3, 2, 1, 0,  5,  -1 } ) );
}

U_TEST(DestructorsTest7)
{
	// Must call destructors of inner loop, but not call destructors of outer loop before "break".
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Foo()
		{
			var S s0(0);
			while(true)
			{
				var S s1(1), s2(2);
				var S s3(3);
				while(true)
				{
					var S s4(4);
					if( true )
					{
						var S s5(5), s6(6);
						break;
					}
				}
				var S s7(7);
				break;
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 6, 5, 4, 7, 3, 2, 1, 0 } ) );
}

U_TEST(DestructorsTest8)
{
	// Explicit destructor must contains implicit calls to destructors of members.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		class T
		{
			S s; i32 y;
			fn constructor( i32 x, i32 in_y ) ( s(x), y(in_y) ) {}
			fn destructor()
			{
				DestructorCalled(y);
			}
		}
		fn Foo()
		{
			var T t(111, 666);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 666, 111} ) );
}

U_TEST(DestructorsTest9)
{
	// Explicit destructor must contains implicit calls to destructors of members.
	// Members destructors must be called in all return ways.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		class T
		{
			S s; i32 y;
			fn constructor( i32 x, i32 in_y ) ( s(x), y(in_y) ) {}
			fn destructor()
			{
				if( ( y & 1 ) != 0 )
				{
					DestructorCalled(y);
					return;
				}
				else
				{
					DestructorCalled( -y );
				}
			}
		}
		fn Foo()
		{
			var T t0(111, 666);
			var T t1(500, 999);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 999, 500, -666, 111 } ) );
}

U_TEST(DestructorsTest10)
{
	// Must call generated destructor.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		class SWrapper
		{
			S s;
			fn constructor( i32 x ) ( s(x) ) {}
			// This class must have generated destructor, that calls destructor for members.
		}
		fn Foo()
		{
			var SWrapper s_wrapper(14789325);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 14789325 } ) );
}

U_TEST(DestructorsTest11)
{
	// Destructors for temporaries must be called.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		struct S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Bar( i32 x ) : S
		{
			return S(x);
		}
		fn Baz( S s ){}
		fn Fuz( S &imut s ){}
		fn Foo()
		{
			var i32 x= S(0).x + S(1).x; // Must destroy S(0) and S(1)
			var bool y= S(2).x == 0 || S(3).x == 0; // Must destroy both S(3) and S(2)
			var bool z= S(4).x == 0 && S(5).x == 0; // Must destroy only S(4)

			var [ i32, 2 ] mut arr= zero_init;
			// Type conversion to u32 must destroy temporary variable of class type.
			arr[ u32( S(6).x / 10 ) ]= S(7).x;  // Must destroy index and temporary in right part
			arr[ u32( S(8).x / 10 ) ]+= S(9).x;  // Must destroy index and temporary in right part

			auto i= Bar(10).x; // Must destroy returned from function result and move variable inside function.
			Baz( S(11) ); // Must destroy argument in function, and move temporary variable.
			Fuz( S(12) ); // Bind value to immutable reference parameter. Must destroy only temporary.

			var S nontemporary(13);
			Baz( nontemporary ); // Value copied to argument. Must call destructor for argument.

			S(14); // Simple expression - must destroy temporary.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction(
		function,
		llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 0, 1,   2, 3,   4,   7, 6,   9, 8,   10,  11,   12,   13,   14,   13 } ) );
}

U_TEST( DestructorsTest12_ShouldCorrectlyReturnValueFromDestructibleStruct )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn destructor()
			{
				x= 0;
			}
		}

		fn Foo() : i32
		{
			var S s{ .x= 55841 };
			// Destructor set "x" to zero. We must read "x" before destructor call.
			return s.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 55841 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( DestructorsTest13_ShouldBeDesdtroyedAfterUsage0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor( i32 in_x )
			( x= in_x ) {}
			fn destructor()
			{
				x= 0;
			}
		}
		struct T{ i32 x; }

		fn Foo() : i32
		{
			var T t { .x= S(124586).x }; // Must destroy temporary inner variable after initialization via expression-initializer.
			return t.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>(124586) == result_value.IntVal.getLimitedValue() );
}

U_TEST( DestructorsTest14_ShouldBeDesdtroyedAfterUsage1 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor( i32 in_x )
			( x= in_x ) {}
			fn destructor()
			{
				x= 0;
			}
		}
		struct T{ i32 x; }

		fn Foo() : i32
		{
			var T t { .x( S(4536758).x ) }; // Must destroy temporary inner variable after initialization via constructor-initializer.
			return t.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>(4536758) == result_value.IntVal.getLimitedValue() );
}

U_TEST( DestructorsTest15_ShouldBeDesdtroyedAfterUsage2 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor( i32 in_x )
			( x= in_x ) {}
			fn destructor()
			{
				x= 0;
			}
		}

		fn Foo() : i32
		{
			var [ i32, 1 ] t[ S(985624).x ]; // Must destroy temporary inner variable after initialization of array member via expression-initializer.
			return t[0u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>(985624) == result_value.IntVal.getLimitedValue() );
}

U_TEST( DestructorsTest16_ShouldBeDesdtroyedAfterUsage3 )
{
	static const char c_program_text[]=
	R"(
		struct B
		{
			bool b;
			fn constructor() ( b= true ) {}
			fn destructor() { b= false; }
		}

		fn Foo() : bool
		{
			auto r= B().b && B().b; // Must destroy both temp variables after evaluation of && or ||.
			return r; // Must return true.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>(1) == result_value.IntVal.getLimitedValue() );
}

U_TEST( DestructorsTest17_ShouldBeDesdtroyedAfterUsage4 )
{
	static const char c_program_text[]=
	R"(
		struct B
		{
			bool b;
			fn constructor() ( b= true ) {}
			fn destructor() { b= false; }
		}

		fn Foo() : i32
		{
			if( B().b ) // Temporary variable must be destroyed after evaluation of condition.
			{ return 5245; } // Must return in this branch of 'if'.
			return 123475;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>(5245) == result_value.IntVal.getLimitedValue() );
}

U_TEST( DestructorsTest18_ShouldBeDesdtroyedAfterUsage5 )
{
	static const char c_program_text[]=
	R"(
		struct B
		{
			bool b;
			fn constructor() ( b= true ) {}
			fn destructor() { b= false; }
		}

		fn Foo() : i32
		{
			while( B().b ) // Temporary variable must be destroyed after evaluation of condition.
			{ return 7698577; } // Must return in 'while'.
			return 9641;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>(7698577) == result_value.IntVal.getLimitedValue() );
}

U_TEST(DestructorsTest19_DestuctorForInterface)
{
	// Must call destructor for of interface-parent.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class A interface
		{
			fn destructor()
			{
				DestructorCalled( 5558414 );
			}
		}
		class B : A{}
		fn Foo()
		{
			var B b;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence.size() == 1u && g_destructors_call_sequence.front() == 5558414 );
}

U_TEST(DestructorsTest20_EarlyDestructorCallUsingMoveOperator)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Foo()
		{
			var S mut s0( 99985 );
			move(s0); // Must call destructor here.
			var S s1( 8852 );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 99985, 8852 } ) );
}

U_TEST(DestructorsTest21_ChangeDestructionOrderUsingMoveOperator)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Foo()
		{
			var S mut s0( 111 );
			var S s1( 222 );
			{
				var S s2= move(s0);
				var S s3( 333 );
				// Must destroy 333, 111 here.
			}
			// Must destroy 222 here.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 333, 111, 222 } ) );
}

U_TEST(DestructorsTest22_DestructorForTemporaryConvertedReferenceArgument)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}

		fn Bar( S& s ){}
		fn Foo()
		{
			Bar( 66 );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 66 } ) );
}

U_TEST(DestructorsTest22_DestructorForTuples)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}

		fn Foo()
		{
			var tup[ i32, S ] t0[ 0, S( 52 ) ];
			var tup[ S, f32 ] t1[ S(21), 0.25f ];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 21, 52 } ) );
}

U_TEST(DestructorsTest23_DestructorForTuples)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}

		struct T
		{
			tup[ S, bool ] t[ ( 885 ), false ];
		}

		fn Foo()
		{
			var T t;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 885 } ) );
}

U_TEST(DestructorsTest24_DestructorForTuples)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}

		fn Bar( tup[ i32, S ] s ){}
		fn Foo()
		{
			var tup[ i32, S ] mut t0[ 0, ( 66 ) ];
			Bar( move(t0) ); // Destructor of moved to function argument tuplemust be called.
			var tup[ i32, S ] t1[ 0, (41) ];

		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 66, 41 } ) );
}

U_TEST(DestructorsTest25_DestructorForTuples)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}

		fn Foo()
		{
			var tup[ i32, f32, bool ] t= zero_init;
			var i32 mut iterations(0);
			for( &e : t )
			{
				var S s0( iterations * 1000 + 93 );
				{
					var S s1( iterations * 1000 + 56 );
					++iterations;
					continue;
				}
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 56, 93, 1056, 1093, 2056, 2093} ) );
}

U_TEST(DestructorsTest26_DestructorForTuples)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}

		fn Foo()
		{
			var tup[ i32, f32, bool ] t= zero_init;
			var i32 mut iterations(0);
			for( &e : t )
			{
				var S s0( iterations * 100 + 7 );
				{
					var S s1( iterations * 100 + 2 );
					++iterations;
					break;
				}
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 2, 7 } ) );
}

U_TEST(DestructorsTest27_DestructorForTuples)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}

		fn Foo()
		{
			var tup[ i32, f32, bool, char8, i16, u8 ] t= zero_init;
			var i32 mut iterations(0);
			for( &e : t )
			{
				++iterations;
				var S s0( iterations * 100 + 12 );
				if( ( iterations & 1 ) == 0 )
				{
					var S s1( iterations * 1000 + 5 );
					continue;
				}
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 112,   2005, 212,   312,   4005, 412,   512,   6005, 612 } ) );
}

U_TEST(DestructorsTest28_DestructorForTuples)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn constructor( mut this, S &imut other )( x= other.x * 3 ) {}
			fn destructor() { DestructorCalled(x); }
		}

		fn Foo()
		{
			var tup[ S, S ] t[ (17), (29) ];
			auto mut iterations= 0u;
			for( e : t ) // Make copy of tuple element.
			{
			}
			DestructorCalled( 11111 );
			// Should destroy tuple here.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 17 * 3, 29 * 3, 11111, 17, 29 } ) );
}

U_TEST(DestructorsTest29_DestructorForTuples)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn constructor( mut this, S &imut other )( x= other.x * 3 ) {}
			fn destructor() { DestructorCalled(x); }
		}

		fn Foo()
		{
			var tup[ f32, S, i32 ] t[ 0.0f, S(34), 0 ];
			auto mut iterations= 0u;
			for( e : t ) // Make copy of tuple element.
			{
				if( iterations == 1u )
				{
					break; // Should call here destructor for copy of tuple element.
				}
				++iterations;
			}
			DestructorCalled( 55555 );
			// Should destroy tuple here.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 34 * 3, 55555, 34 } ) );
}

U_TEST(DestructorsTest30_MembersDestructorCallOrder)
{
	// Destructors of members should be called in fields order.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		struct T ordered
		{
			S a;
			S b;
			S c;
			S w;
			S z;
			S y;
			S x;
		}
		fn Foo()
		{
			var T t
			{
				.a(0),
				.b(1),
				.c(2),
				.w(3),
				.z(4),
				.y(5),
				.x(6),
			};
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {});

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 0, 1, 2, 3, 4, 5, 6} ) );
}

U_TEST(DestructorsTest31_DestructorNotCalledForReferenceField)
{
	// Destructors of members should be called in fields order.
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			fn destructor(){ DestructorCalled(5555); }
		}

		struct R
		{
			S& ref;
		}

		fn Foo()
		{
			var S s;
			var R r{ .ref= s };
		}
		)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {});

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 5555 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test0)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); x= 0; }
		}

		fn Extract( S& s ) : i32 { return s.x; }
		fn Pass( i32 x ) : i32
		{
			var S s( x * 11 );
			return x;
		}

		fn Foo()
		{
			auto x= Pass( Extract( S(854) ) );  // Temporary variable S mut be destroyed after call of "Extract", but before "Pass" execution
			//halt if( x != 854 );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 854, 854 * 11 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test1)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); x= 0; }
		}

		fn Bar( i32 x ) : i32
		{
			var S s( x * 2 );
			return x;
		}

		fn Foo()
		{
			auto x= Bar( S(44).x );  // Temporary variable S mut be destroyed, when Bar executed
			halt if( x != 44 );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 44, 88 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test2)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); x= 0; }
		}

		fn Bar( i32 x, i32 y ) : i32
		{
			var S s( x * y );
			return x / y;
		}

		fn Foo()
		{
			// Must construct S(95), take value, destroy it, then construct S(13), take value, destroy it.
			auto x= Bar( S(95).x, S(13).x );
			halt if( x != 95 / 13 );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 95, 13, 95 * 13 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test3)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); x= 0; }
		}

		fn Bar() : i32
		{
			var S s(33321);
			return s.x;
		}
		fn Foo()
		{
			auto x= S(21).x + Bar(); // temporary S(21) must be destroyed before "Bar" call
			halt if( x != 21 + 33321 );
			var S s(666);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 21, 33321, 666 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test4)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); x= 0; }
		}

		fn Bar( i32 x ) : i32
		{
			var S s(x);
			return s.x;
		}

		fn Foo()
		{
			auto x= ( Bar(955) * S(21).x ) / Bar(3); // temporary S(21) must be destroyed before Bar(3) call.
			halt if( x != 955 * 21 / 3 );
			var S s(666);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 955, 21, 3, 666 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test5)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) { DestructorCalled(-x); }
			fn destructor() { DestructorCalled(x); x= 0; }
			fn set_x( mut this, i32 in_x ){ x= in_x; }
		}

		fn Foo()
		{
			var [ S, 3 ] mut arr[ (100), (200), (300) ];
			arr[ size_type(S(1).x) ].set_x( S(124).x / S(5).x ); // Must destroy temporary S(1) before set_x argumnet calculation.

			halt if( arr[1u].x != 124 / 5 );
			var S s(666);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { -100, -200, -300,  -1, 1,  -124, 124,  -5, 5,  -666, 666,  100, 124 / 5, 300 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test6)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) { DestructorCalled(-x); }
			fn destructor() { DestructorCalled(x); x= 0; }
			fn set_x( mut this, i32 in_x ){ x= in_x; }
		}

		fn Foo()
		{
			var [ S, 3 ] mut arr[ (100), (200), (300) ];
			auto x= arr[ size_type(S(1).x) ].x + ( S(124).x / S(5).x ); // Must destroy temporary S(1) before op+ second argumnet calculation.

			halt if( x != 200 + 124 / 5 );
			var S s(666);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { -100, -200, -300,  -1, 1,  -124, 124,  -5, 5,  -666, 666,  100, 200, 300 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test7)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) { DestructorCalled(-x); }
			fn destructor() { DestructorCalled(x); x= 0; }
		}

		fn Foo()
		{
			var [ [ S, 2 ], 2 ] mut arr[ [ (900), (901) ], [ (910), (911) ] ];
			arr[ size_type(S(0).x) ][ size_type(S(1).x) ].x= 999; // Must destroy S(0) before second index calculation

			var S s(666);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { -900, -901, -910, -911,  -0, 0,  -1, 1,  -666, 666,  900, 999, 910, 911 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test8)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) { DestructorCalled(-x); }
			fn destructor() { DestructorCalled(x); x= 0; }
		}

		fn Foo()
		{
			var [ [ i32, 2 ], 3 ] mut arr= zero_init;
			arr[ size_type(S(2).x) ][ size_type(S(1).x) ]= 999; // Must destroy S(66) before second index calculation.

			halt if( arr[2u][1u] != 999 );
			var S s(666);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { -2, 2,  -1, 1,  -666, 666 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test9)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) { DestructorCalled(-x); }
			fn destructor() { DestructorCalled(x); x= 0; }
		}

		fn GetOne() : size_type
		{
			return size_type( S(1).x );
		}
		fn Foo()
		{
			var [ i32, 2 ] mut arr= zero_init;
			arr[ GetOne() ]= S(66).x; // Must destroy S(66) before evaluation of left part of assignment.

			halt if( arr[1u] != 66 );
			var S s(666);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { -66, 66,  -1, 1,  -666, 666 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test10)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) { DestructorCalled(-x); }
			fn destructor() { DestructorCalled(x); x= 0; }
		}

		fn GetOne() : size_type
		{
			return size_type( S(1).x );
		}
		fn Foo()
		{
			var [ i32, 2 ] mut arr[ 0, 33 ];
			arr[ GetOne() ]+= S(66).x; // Must destroy S(66) before evaluation of left part of additive assignment.

			halt if( arr[1u] != 66 + 33 );
			var S s(666);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { -66, 66,  -1, 1,  -666, 666 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test11)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) { DestructorCalled(-x); }
			fn destructor() { DestructorCalled(x); x= 0; }
		}

		fn GetTwo() : i32
		{
			return S(2).x;
		}
		fn Foo()
		{
			var [ i32, 2 ] arr[ S(1).x, GetTwo() ]; // Must destroy S(1) before GetTwo() call.
			var S s(666);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { -1, 1,  -2, 2,  -666, 666 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test12)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) { DestructorCalled(-x); }
			fn destructor() { DestructorCalled(x); x= 0; }
		}

		fn GetTwo() : i32
		{
			return S(2).x;
		}
		fn Foo()
		{
			var [ i32, 2 ] arr[ (S(1).x), GetTwo() ]; // Must destroy S(1) before GetTwo() call.
			var S s(666);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { -1, 1,  -2, 2,  -666, 666 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test13)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) { DestructorCalled(-x); }
			fn destructor() { DestructorCalled(x); x= 0; }
		}

		struct T
		{
			op[]( this, S& s ) : i32
			{
				return s.x;
			}
		}

		fn Foo()
		{
			var T t;
			auto x= t[ S(22) ] + S(66).x; // Must destroy S(22) before call to overloaded [] operator.
			halt if( x != 22 + 66 );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { -22, 22,  -66, 66 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test14)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) { DestructorCalled(-x); }
			fn destructor() { DestructorCalled(x); x= 0; }
		}

		struct T
		{
			i32 x;
			op>>=( mut this, i32 sh )
			{
				var S s(666);
				x >>= u32(sh);
			}
		}

		fn Foo()
		{
			var T mut t{ .x= 665214 };
			t >>= S(3).x; // Must destroy temporary S(3) before call to operator >>=.
			halt if( t.x != 665214 >> u32(3) );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { -3, 3,  -666, 666 } ) );
}

U_TEST(EralyTempVariablesDestruction_Test15)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) { DestructorCalled(-x); }
			fn destructor() { DestructorCalled(x); x= 0; }
		}

		fn Pass( S& s ) : S& { return s; }

		fn Bar( i32 x ) : S
		{
			DestructorCalled( x * 10 );
			return S(x * 3);
		}

		fn Foo()
		{
			// Should construct "S", call "Pass", read "S::x", destroy "S", call "Bar", constrcut "S" inside "Bar", return "S" and destroy it.
			Bar( Pass( S(54) ).x );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, {} );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { -54, 54,  54 * 10,  -54 * 3, 54 * 3 } ) );
}

U_TEST(DestructorTest_ForCStyleForOperator0)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			for( auto mut x= 0; x < 4; x+= S(x).x * 0 + 1 ){}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 0, 1, 2, 3 } ) );
}

U_TEST(DestructorTest_ForCStyleForOperator1)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			// Should execute only condition but not loop body and iteration
			for( auto mut x= 0; x == S(55).x; x+= S(33).x )
			{
				var S s(854);
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 55 } ) );
}

U_TEST(DestructorTest_ForCStyleForOperator2)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			// Should execute condition, than loop body, iteration, repeat this sequence and end with last condition
			for( auto mut x= 0; x != S(2).x; x+= S(2000 + x).x * 0 + 1 )
			{
				var S s(100 + x * 3);
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 2, 100, 2000,  2, 103, 2001,  2 } ) );
}

U_TEST(DestructorTest_ForCStyleForOperator3)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			// Should execute initialization one time, should execute iteration part in order
			for( auto mut x= S(45).x; x < 50; x+= S(100 + x).x * 0, x+= S(-x).x * 0, x+= 2 )
			{
				var S s(x * 7);
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { 45,  315, 145, -45,  329, 147, -47,  343, 149, -49  } ) );
}

U_TEST(DestructorTest_ForCStyleForOperator4)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			// Should destroy loop counter once.
			for( var S mut s(70); s.x < 80; s.x+= 2 )
			{
				var S inner_s(-s.x);
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { -70, -72, -74, -76, -78, 80 } ) );
}

U_TEST(DestructorTest_ForCStyleForOperator5)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			for( var S mut s(0); true; s.x+= 3 )
			{
				if( s.x >= 10 ){ break; } // Should destroy loop counter after "break".
				var S inner_s(-s.x);
			}
			var S end_s(666);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		g_destructors_call_sequence ==
		std::vector<int>( { -0, -3, -6, -9, 12, 666 } ) );
}

U_TEST(DestructorTest_For_WithOperator0)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			DestructorCalled( 1 );
			with( &s : S(99) ) // Bind temporary to reference - expand its lifetime.
			{
				DestructorCalled( 2 );
			} // temporary of type "S" should be destroyed here.
			DestructorCalled( 3 );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 1, 2, 99, 3 } ) );
}

U_TEST(DestructorTest_For_WithOperator1)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			DestructorCalled( 101 );
			with( &x : S(85).x ) // Bind temporary part to reference - expand its lifetime.
			{
				DestructorCalled( 102 );
			} // temporary of type "S" should be destroyed here.
			DestructorCalled( 103 );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 101, 102, 85, 103 } ) );
}

U_TEST(DestructorTest_For_WithOperator2)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			DestructorCalled( 11 );
			with( s : S(113) ) // Move temporary to variable.
			{
				DestructorCalled( 22 );
			} // 's' should be sestroyed here.
			DestructorCalled( 33 );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 11, 22, 113, 33 } ) );
}

U_TEST(DestructorTest_For_WithOperator3)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			DestructorCalled( 1000 );
			with( x : S(777).x ) // Copy part of temporary to variable. Should destroy temporary here.
			{
				DestructorCalled( 2000 );
			}
			DestructorCalled( 3000 );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 1000, 777, 2000, 3000 } ) );
}

U_TEST(DestructorTest_For_WithOperator4)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		struct S
		{
			i32 x;
			fn constructor( i32 in_x )( x= in_x ) {}
			fn destructor(){ DestructorCalled(x); }
		}
		struct R
		{
			S& s;
			fn constructor( this'x', S &'y in_s ) ' x <- y ' ( s= in_s ) {}
			fn destructor(){ DestructorCalled(5555); }
		}
		fn Foo()
		{
			var i32 mut res= 0;
			DestructorCalled( 901 );
			with( r : R(S(66123)) ) // Create temporary of type 'S', then create tomporary of type 'R', which has reference to 's', then temporary of type 'R' to 'r'.
			{
				DestructorCalled( 902 );
			} // 'r', than 's' should be destroyed here
			DestructorCalled( 903 );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 901, 902, 5555, 66123, 903 } ) );
}

U_TEST(DestructorTest_For_WithOperator5)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			DestructorCalled( 31 );
			with( x : S(777).x ) // Should destroy temporary of type 'S' here.
			{
				DestructorCalled( 32 );
			}
			DestructorCalled( 33 );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 31, 777, 32, 33 } ) );
}

U_TEST(DestructorTest_For_WithOperator6)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			var S s1(1);
			with( x : 42 )
			{
				var S s2(2);
				return; // Should destroy here 's2', than 's1'
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 2, 1 } ) );
}

U_TEST(DestructorTest_For_WithOperator7)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			var S s1(1);
			while(true)
			{
				var S s2(2);
				with( x : 42 )
				{
					var S s3(3);
					break; // should destroy 's3', 's2' here.
				}
			}
			DestructorCalled(555);
		} // should destroy 's1' here
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 3, 2, 555, 1 } ) );
}

U_TEST(DestructorTest_For_WithOperator8)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			with( s : S(55) )
			{
				DestructorCalled(1);
				return; // should destroy 's' here.
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 1, 55 } ) );
}

U_TEST(DestructorTest_For_WithOperator9)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}
		fn Foo()
		{
			with( &s : S(321) )
			{
				DestructorCalled(44);
				return; // should destroy temporary of type 'S' here.
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 44, 321 } ) );
}

U_TEST(DestructorTest_ForArrayValueArgument_Test0)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn constructor( mut this, S &other ) ( x= other.x + 100 ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}

		fn Bar( [ S, 2 ] a )
		{
		} // Should destroy array here.
		fn Foo()
		{
			var [ S, 2 ] a [ (4), (7) ];
			Bar( a );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 104, 107, 4, 7 } ) );
}

U_TEST(DestructorTest_ForArrayValueArgument_Test1)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);

		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn constructor( mut this, S &other ) ( x= other.x + 100 ) {}
			fn destructor()
			{
				DestructorCalled(x);
			}
		}

		fn Bar( [ S, 3 ] a )
		{
		} // Should destroy array here.
		fn Baz() : [ S, 3 ]
		{
			var [ S, 3 ] mut a[ (62), (11), (99) ];
			return move(a); // Should move 'a' here and not destroy it
		}
		fn Foo()
		{
			Bar(Baz()); // Should move value result to value argument.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 62, 11, 99 } ) );
}

U_TEST(CoroutineDestruction_Test0)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}

		fn generator SomeGen() : i32
		{
			var S s(789);
			yield 77;
		}
		fn Foo()
		{
			auto gen= SomeGen();
			var S s(14445);
			// Destroy here "s". internal "s" in "gen" is not destroyed, because it was not constructed.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 14445 } ) );
}

U_TEST(CoroutineDestruction_Test1)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}

		fn generator SomeGen() : i32
		{
			var S s(99957);
			yield 77;
		}
		fn Foo()
		{
			auto mut gen= SomeGen();
			var S s(5241);
			if_coro_advance( x : gen ) { halt if( x !=  77 ); }
			// Destroy here "s".
			// Destroy internal "s" inside "gen", because it was constructed after "advance".
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 5241, 99957 } ) );
}

U_TEST(CoroutineDestruction_Test2)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}

		fn generator SomeGen() : i32
		{
			yield S(1114).x; // temp of type "S" is destroyed in evaluation of "yield" operator.
		}
		fn Foo()
		{
			auto mut gen= SomeGen();
			var S s(4441);
			if_coro_advance( x : gen ) { halt if( x !=  1114 ); }
			// Destroy "s" here.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 1114, 4441 } ) );
}

U_TEST(CoroutineDestruction_Test3)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}

		fn generator SomeGen(S s) : i32
		{
			yield 123;
		}
		fn Foo()
		{
			var S s0(999);
			auto mut gen= SomeGen(S(887));
			var S s1(778);
			// Do not advance.
			// Destroy "s0" here.
			// Destroy generator argument of type "S" here. Even if coroutine was not advanced, arguments still should be destroyed.
			// Destroy "s1" here.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 778, 887, 999 } ) );
}

U_TEST(CoroutineDestruction_Test4)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}

		fn generator SomeGen() : i32
		{
			var S s(77744);
			yield 432;
		}
		fn Foo()
		{
			var S s0(852);
			auto mut gen= SomeGen();
			var S s1(258);
			if_coro_advance( x : gen ) { halt if( x !=432 ); }
			var S s2(678);
			if_coro_advance( x : gen ) { halt; } // Reach here final suspention point. All destructors shold be called here.
			var S s3(907);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 77744, 907, 678, 258, 852 } ) );
}

U_TEST(CoroutineDestruction_Test5)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		template</type T/> struct Box{ T t; }
		template</type T/> fn MakeBox(T mut t) : Box</T/> { var Box</T/> mut box{ .t= move(t) }; return move(box); }
		fn generator SomeGen() : i32
		{
			var S s(954);
			yield 77;
		}
		fn Foo()
		{
			auto mut gen= SomeGen();
			var S s(774411);
			if_coro_advance( x : gen ) { halt if( x != 77 ); }
			auto box= MakeBox( move(gen) );
			// Destroy "gen" inside "box" here, including internal variable "s".
			// Destroy local "s".
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 954, 774411 } ) );
}

U_TEST(IfCoroAdvance_Destruction_Test0)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn generator SomeGen() : S
		{
			yield S(555);
		}
		fn Foo()
		{
			auto mut gen= SomeGen();
			var S s(11);
			if_coro_advance( gen_s : gen )
			{
				halt if( gen_s.x != 555 );
				var S inner_s(887766);
				// Destroy "inner_s" hee.
				// Destroy "gen_s" here.
			}
			// Destroy "s" here.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 887766, 555, 11 } ) );
}

U_TEST(IfCoroAdvance_Destruction_Test1)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( mut this, S& other )= default;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn generator SomeGen(S& s) : S&
		{
			yield s;
		}
		fn Foo()
		{
			var S s(776677);
			auto mut gen= SomeGen(s);
			if_coro_advance( & gen_s : gen ) // Take just reference here.
			{
				halt if( gen_s.x != 776677 );
				var S inner_s(990099);
				// Destroy "inner_s" hee.
			}
			// Destroy "s" here.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 990099, 776677 } ) );
}

U_TEST(IfCoroAdvance_Destruction_Test2)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( mut this, S& other )= default;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn generator SomeGen(S& s) : S&
		{
			yield s;
		}
		fn Foo()
		{
			var S s(65456);
			auto mut gen= SomeGen(s);
			if_coro_advance( gen_s : gen ) // Create copy of refernce-result of generator here.
			{
				halt if( gen_s.x != 65456 );
				var S inner_s(888);
				// Destroy "inner_s" hee.
				// Destroy "gen_s" here.
			}
			// Destroy "s" here.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 888, 65456, 65456 } ) );
}

U_TEST(IfCoroAdvance_Destruction_Test3)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( mut this, S& other )= default;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn generator SomeGen(S s) : i32
		{
			yield 343;
			yield 555;
		}
		fn Foo()
		{
			var S s(222343);
			var S other_s(787);
			if_coro_advance( x : SomeGen(s) ) // Create temporary value of generator here.
			{
				halt if( x != 343 );
				var S inner_s(998);
				// Destroy "inner_s" here.
			}
			// Destroy "s" copy as "SomeGen" argument here.
			// Destroy "other_s" here.
			// Destroy "s" here.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 998, 222343, 787, 222343 } ) );
}

U_TEST(BreakContinueToOuterLoop_Destructor_Test0)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Foo()
		{
			for( auto mut i= 0; i < 3; ++i ) label outer
			{
				var S s_outer( 3 + i * 100 );
				for( auto mut j= 0; j < 100; ++j )
				{
					var S s_inner( 7 + j * 13 );
					if( j >= 1 )
					{
						continue label outer; // Destroy here both s_inner and s_outer
					}
				}
				var S s_unreachable( 666 );
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 7, 20, 3,  7, 20, 103,  7, 20, 203, } ) );
}

U_TEST(BreakContinueToOuterLoop_Destructor_Test1)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Foo()
		{
			for( auto mut i= 0; i < 3; ++i ) label outer
			{
				var S s_outer( 5 + i * 1000 );
				for( auto mut j= 0; j < 100; ++j )
				{
					var S s_inner( 8 + j * 13 );
					if( j >= 1 )
					{
						break label outer; // Destroy here both s_inner and s_outer
					}
				}
				var S s_unreachable( 666 );
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 8, 21, 5, } ) );
}

U_TEST(BreakContinueToOuterLoop_Destructor_Test2)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Foo()
		{
			var tup[f32, i32, u32] t[ 0.0f, 1, 2u ];
			for( el : t ) label outer
			{
				var S outer_s( 75 + i32(el) );
				for( auto mut i= 0; i < 100; ++i )
				{
					var S inner_s( i + 100 * i32(el) );
					if( i == 2 )
					{
						continue label outer;  // Destroy here both "inner_s" and "outer_s"
					}
				}
				var S s_unreachable( 666 );
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 0, 1, 2, 75,  100, 101, 102, 76,  200, 201, 202, 77 } ) );
}

U_TEST(BreakContinueToOuterLoop_Destructor_Test3)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Foo()
		{
			var tup[f32, i32, u32] t[ 16.0f, 1, 2u ];
			for( el : t ) label outer
			{
				var S outer_s( 13 + i32(el) );
				for( auto mut i= 0; i < 100; ++i )
				{
					var S inner_s( i + 100 * i32(el) );
					if( i == 2 )
					{
						break label outer; // Destroy here both "inner_s" and "outer_s"
					}
				}
				halt;
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 1600, 1601, 1602, 29 } ) );
}

U_TEST(BreakFromBlock_Destructor_Test0)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Foo()
		{
			var S outer_s( 778 );
			{
				var S inner_s( 890 );
				if( true )
				{
					break label some; // Destroy here "inner_s".
				}
				var S unreachable_s( 789 );
			} label some
			var S another_outer_s( 812 );
			// Destroy here "another_outer_s", than "outer_s"
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 890, 812, 778 } ) );
}

U_TEST(BreakFromBlock_Destructor_Test1)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Foo()
		{
			var S outer_s( 5564 );
			{
				var S inner_s( 87 );
				{
					var S more_inner_s( 134 );
					if( true )
					{
						break label some; // Destroy here "more_inner_s", than "inner_s".
					}
					var S more_unreachable_s( 11 );
				} label another_label
				var S unreachable_s( 5453 );
			} label some
			var S another_outer_s( 7234 );
			// Destroy here "another_outer_s", than "outer_s"
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 134, 87, 7234, 5564 } ) );
}

U_TEST(SwitchOperatorDestructors_Test0)
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { DestructorCalled(x); }
		}
		fn Foo( i32 x )
		{
			var S s_init(123);
			switch(x)
			{
				1 ->
				{
					var S s1(456);
					// Destroy here "s1"
				},
				2 ->
				{
					var S s2(789);
					return; // Destroy here "s2", "s_init".
				},
				default -> {},
			}
			var S s_final(101112);
			// Destroy here "s_final",  "s_init".
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue arg;

	arg.IntVal= llvm::APInt( 32, 1 );
	engine->runFunction( function, { arg } );
	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 456, 101112, 123 } ) );
	g_destructors_call_sequence.clear();

	arg.IntVal= llvm::APInt( 32, 2 );
	engine->runFunction( function, { arg } );
	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 789, 123 } ) );
	g_destructors_call_sequence.clear();

	arg.IntVal= llvm::APInt( 32, 3 );
	engine->runFunction( function, { arg } );
	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 101112, 123 } ) );
	g_destructors_call_sequence.clear();
}

U_TEST( DerivedToBaseConversion_Destructors_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class Base polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {  DestructorCalled( -x ); }
			fn constructor( mut this, Base& other ) ( x= other.x ) { DestructorCalled( -x * 3 ); }
			fn destructor() { DestructorCalled(x); }
		}
		class Derived : Base
		{
			fn constructor( i32 in_x ) ( base( in_x ) ) { DestructorCalled( -x * 2 ); }
			fn constructor( mut this, Derived& other ) ( base( cast_ref</Base/>(other) ) ) { DestructorCalled( -x * 6 ); }
			fn destructor()
			{
				DestructorCalled( x * 2 );
			}
		}
		fn Bar( Base b ){ } // Destroy here base.
		fn Foo()
		{
			// Construct derived (and base inside), copy-construct base, destroy derived, destroy base function argument.
			Bar( Derived( 33 ) );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { -33, -66,  -99,  66, 33,  33 } ) );
}

U_TEST( DerivedToBaseConversion_Destructors_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class Base polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {  DestructorCalled( -x ); }
			fn constructor( mut this, Base& other ) ( x= other.x ) { DestructorCalled( -x * 3 ); }
			fn destructor() { DestructorCalled(x); }
		}
		class Derived : Base
		{
			fn constructor( i32 in_x ) ( base( in_x ) ) { DestructorCalled( -x * 2 ); }
			fn constructor( mut this, Derived& other ) ( base( cast_ref</Base/>(other) ) ) { DestructorCalled( -x * 6 ); }
			fn destructor()
			{
				DestructorCalled( x * 2 );
			}
		}
		fn Bar( Base b ){ } // Destroy here base.
		fn Foo()
		{
			var Derived derived( 33 ); // Construct derived (and base inside).
			Bar( derived ); // Copy-construct base, destroy base function argument.
			// Destroy derived.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { -33, -66,  -99,  33, 66,  33 } ) );
}

U_TEST( DerivedToBaseConversion_Destructors_Test2 )
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class Base polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {  DestructorCalled( -x ); }
			fn constructor( mut this, Base& other ) ( x= other.x ) { DestructorCalled( -x * 3 ); }
			fn destructor() { DestructorCalled(x); }
		}
		class Derived : Base
		{
			fn constructor( i32 in_x ) ( base( in_x ) ) { DestructorCalled( -x * 2 ); }
			fn constructor( mut this, Derived& other ) ( base( cast_ref</Base/>(other) ) ) { DestructorCalled( -x * 6 ); }
			fn destructor()
			{
				DestructorCalled( x * 2 );
			}
		}
		fn Bar() : Base
		{
			// Construct derived (and base inside), copy-construct base, destroy derived.
			return Derived( 33 );
		}
		fn Foo()
		{
			Bar();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { -33, -66,  -99,  66, 33,  33 } ) );
}

U_TEST( DerivedToBaseConversion_Destructors_Test3 )
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class Base polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {  DestructorCalled( -x ); }
			fn constructor( mut this, Base& other ) ( x= other.x ) { DestructorCalled( -x * 3 ); }
			fn destructor() { DestructorCalled(x); }
		}
		class Derived : Base
		{
			fn constructor( i32 in_x ) ( base( in_x ) ) { DestructorCalled( -x * 2 ); }
			fn constructor( mut this, Derived& other ) ( base( cast_ref</Base/>(other) ) ) { DestructorCalled( -x * 6 ); }
			fn destructor()
			{
				DestructorCalled( x * 2 );
			}
		}
		fn Bar() : Base
		{
			var Derived derived( 33 ); // Construct derived (and base inside)
			return derived; // copy-construct base, destroy derived.
		}
		fn Foo()
		{
			Bar();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { -33, -66,  -99,  66, 33,  33 } ) );
}

U_TEST( DerivedToBaseConversion_Destructors_Test4 )
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class Base polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {  DestructorCalled( -x ); }
			fn constructor( mut this, Base& other ) ( x= other.x ) { DestructorCalled( -x * 3 ); }
			fn destructor() { DestructorCalled(x); }
		}
		class Derived : Base
		{
			fn constructor( i32 in_x ) ( base( in_x ) ) { DestructorCalled( -x * 2 ); }
			fn constructor( mut this, Derived& other ) ( base( cast_ref</Base/>(other) ) ) { DestructorCalled( -x * 6 ); }
			fn destructor()
			{
				DestructorCalled( x * 2 );
			}
		}
		fn Bar() : Derived
		{
			// Construct derived (and base inside).
			return Derived( 33 );
		}
		fn Foo()
		{
			var Base b= Bar(); // Call copy constructor for base, destroy derived result.
			// Destroy base bariable.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { -33, -66,  -99,  66, 33,  33 } ) );
}

U_TEST( DerivedToBaseConversion_Destructors_Test5 )
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class Base polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {  DestructorCalled( -x ); }
			fn constructor( mut this, Base& other ) ( x= other.x ) { DestructorCalled( -x * 3 ); }
			fn destructor() { DestructorCalled(x); }
		}
		class Derived : Base
		{
			fn constructor( i32 in_x ) ( base( in_x ) ) { DestructorCalled( -x * 2 ); }
			fn constructor( mut this, Derived& other ) ( base( cast_ref</Base/>(other) ) ) { DestructorCalled( -x * 6 ); }
			fn destructor()
			{
				DestructorCalled( x * 2 );
			}
		}
		fn Bar() : Derived
		{
			// Construct derived (and base inside).
			return Derived( 33 );
		}
		fn Foo()
		{
			var Base b( Bar() ); // Call copy constructor for base, destroy derived result.
			// Destroy base bariable.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { -33, -66,  -99,  66, 33,  33 } ) );
}

U_TEST( DerivedToBaseConversion_Destructors_Test6 )
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class Base polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {  DestructorCalled( -x ); }
			fn constructor( mut this, Base& other ) ( x= other.x ) { DestructorCalled( -x * 3 ); }
			fn destructor() { DestructorCalled(x); }
		}
		class Derived : Base
		{
			fn constructor( i32 in_x ) ( base( in_x ) ) { DestructorCalled( -x * 2 ); }
			fn constructor( mut this, Derived& other ) ( base( cast_ref</Base/>(other) ) ) { DestructorCalled( -x * 6 ); }
			fn destructor()
			{
				DestructorCalled( x * 2 );
			}
		}
		fn generator Bar() : Base
		{
			// Construct derived (and base inside), copy-construct base, destroy derived.
			yield Derived( 33 );
		}
		fn Foo()
		{
			auto mut gen= Bar();
			if_coro_advance( res : gen ){}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { -33, -66,  -99,  66, 33,  33 } ) );
}

U_TEST( DerivedToBaseConversion_Destructors_Test7 )
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		class Base polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {  DestructorCalled( -x ); }
			fn constructor( mut this, Base& other ) ( x= other.x ) { DestructorCalled( -x * 3 ); }
			fn destructor() { DestructorCalled(x); }
			op=( mut this, Base& other ) { x= other.x; DestructorCalled( -x * 5 ); }
		}
		class Derived : Base
		{
			fn constructor( i32 in_x ) ( base( in_x ) ) { DestructorCalled( -x * 2 ); }
			fn constructor( mut this, Derived& other ) ( base( cast_ref</Base/>(other) ) ) { DestructorCalled( -x * 6 ); }
			op=( mut this, Derived& other ) { cast_ref</Base/>(this)= cast_ref</Base/>(other); DestructorCalled( -x * 10 ); }
			fn destructor()
			{
				DestructorCalled( x * 2 );
			}
		}
		fn Foo()
		{
			var Base mut b( 7 ); // Construct base.
			b= Derived(33); // Construct derived (and base inside), copy-assign "b", destroy derived (and base inside).
			// Destroy "b".
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { -7,  -33, -66,  -165,  66, 33,  33 } ) );
}

U_TEST( ByValThisDestruction_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		struct S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x(in_x) ) { DestructorCalled(x); }
			fn constructor( mut this, S& other ) ( x(other.x) ) { DestructorCalled( x * 2 ); }
			fn destructor() { DestructorCalled( -x ); }
			fn Bar( byval this ) { DestructorCalled( x * 5 ); }
		}
		fn Foo()
		{
			var S s(11); // Construct 's'.
			S(7).Bar(); // Construct temp value, move it into byval function arg, destroy it inside 'Bar' call.
			// Destroy 's'.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ), true );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 11,  7, 35, -7,  -11 } ) );
}

U_TEST( ByValThisDestruction_Test2 )
{
	static const char c_program_text[]=
	R"(
		fn DestructorCalled(i32 x);
		struct S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x(in_x) ) { DestructorCalled(x); }
			fn constructor( mut this, S& other ) ( x(other.x) ) { DestructorCalled( x * 2 ); }
			fn destructor() { DestructorCalled( -x ); }
			fn Bar( byval this ) { DestructorCalled( x * 5 ); }
		}
		fn Foo()
		{
			var S s(11); // Construct 's'.
			s.Bar(); // Copy-construct 's' into byval function arg, destroy it inside 'Bar' call.
			// Destroy 's'.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ), true );
	DestructorTestPrepare(engine);

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_destructors_call_sequence == std::vector<int>( { 11,  22, 55, -11,  -11 } ) );
}

} // namespace

} // namespace U
