#include <cstdlib>
#include "tests.hpp"

namespace U
{

namespace
{

U_TEST( EmptyProgramTest0 )
{
	// Empty string is valid program.
	CreateEngine( BuildProgram( "" ) );
}

U_TEST( EmptyProgramTest1 )
{
	// String only with whitespaces is valid program.

	static const char c_program_text[]=
	u8R"(
	    	
	
	
               
	)";
	;

	BuildProgram( c_program_text );
}

U_TEST( UnicodeBOM_Test0 )
{
	static const char c_program_text[]= "\xEF\xBB\xBF fn Foo(){}";
	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	const auto function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, {} );
}

U_TEST( AdditionalSymbolsForIdentifiersTest0 )
{
	static const char c_program_text[]=
	u8R"(
		struct S{}
		fn Foo()
		{
			var S
				only_latin, русская_кириллица, бѣcъ, UPPERSCALE_КИРИЛЛИЦАЪ, ВсЯкАйА_CYRIllIC_Хрень_ҥ_Ѫ_Ѱ_ӂ_Ґ_ґ_Ғ_ғ,
				Ӽ, non_roman_latin_öžçšüä, Anschlußßß, Wörk, Børk, Æ_æ;
		}
	)";
	;

	BuildProgram( c_program_text );
}

U_TEST(SimpliestProgramTest0)
{
	static const char c_program_text[]=
	R"(
		fn Foo() : i32 { return 42; }
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, {} );

	U_TEST_ASSERT( result_value.IntVal.getLimitedValue() == uint64_t(42) );
}

U_TEST(SimpliestProgramTest1)
{
	static const char c_program_text[]=
	R"(
		fn Foo(i32 x ) : i32 { return x; }
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue arg;
	arg.IntVal= llvm::APInt( 32, uint64_t(852456) );
	const llvm::GenericValue result_value= engine->runFunction( function, { arg } );

	U_TEST_ASSERT( result_value.IntVal.getLimitedValue() == arg.IntVal );
}

U_TEST(SimpleProgramTest)
{
	static const char c_program_text[]=
	"\
	fn Foo( i32 a, i32 b, i32 c ) : i32\
	{\
		return a + b + c;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	int32_t arg0= 100500, arg1= 1488, arg2= 42;

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooiii" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue args[3];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );
	args[1].IntVal= llvm::APInt( 32, uint64_t(arg1) );
	args[2].IntVal= llvm::APInt( 32, uint64_t(arg2) );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 3 ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 + arg1 + arg2 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ArgumentsAssignmentTest)
{
	static const char c_program_text[]=
	R"(
	fn Foo( i32 mut a ) : i32
	{
		a= a * a;
		return a;
	})"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue arg;
	arg.IntVal= llvm::APInt( 32, 5648 );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( arg ) );

	U_TEST_ASSERT( static_cast<uint64_t>( 5648 * 5648 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(VariablesTest0)
{
	static const char c_program_text[]=
	"\
	fn Foo( i32 a, i32 b ) : i32\
	{\
		var i32 tmp= a - b;\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooii" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 1488, arg1= 77;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );
	args[1].IntVal= llvm::APInt( 32, uint64_t(arg1) );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 - arg1 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(VariablesTest1)
{
	// Multiple variables declaration.
	static const char c_program_text[]=
	"\
	fn Foo( i32 a, i32 b ) : i32\
	{\
		var i32 mut tmp= a - b, r= 1;\
		tmp= tmp * r;\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooii" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 1488, arg1= 77;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );
	args[1].IntVal= llvm::APInt( 32, uint64_t(arg1) );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 - arg1 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(NumericConstantsTest0)
{
	static const char c_program_text[]=
	"\
	fn Foo32( i32 a, i32 b ) : i32\
	{\
		return a * 7 +  b - 22 / 4 + 458;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z5Foo32ii" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 1488, arg1= 77;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );
	args[1].IntVal= llvm::APInt( 32, uint64_t(arg1) );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 * 7 + arg1 - 22 / 4 + 458 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(NumericConstantsTest1)
{
	static const char c_program_text[]=
	"\
	fn Foo64() : i64\
	{\
		return 45783984055402i64;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z5Foo64v" );
	U_TEST_ASSERT( function != nullptr );


	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 45783984055402ll ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(NumericConstantsTest2)
{
	static const char c_program_text[]=
	"\
	fn Pi() : f32\
	{\
		return 3.1415926535f;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z2Piv" );
	U_TEST_ASSERT( function != nullptr );


	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	ASSERT_NEAR( 3.1415926535f, result_value.FloatVal, 0.0001f );
}

U_TEST(ArraysTest0)
{
	static const char c_program_text[]=
	"\
	fn Foo( i32 x ) : i32\
	{\
		var [ i32, 17 ] mut tmp= zero_init;\
		tmp[5]= x;\
		return tmp[5] + 5;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg_value= 54785;
	llvm::GenericValue arg;
	arg.IntVal= llvm::APInt( 32, uint64_t(arg_value) );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( arg ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg_value + 5 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ArraysTest1)
{
	static const char c_program_text[]=
	"\
	fn Foo( i32 x ) : i32\
	{\
		var [ [ [ i32, 3 ], 5 ], 17 ] mut tmp= zero_init;\
		tmp[5][3][1]= x;\
		return tmp[5][3][1] + 5;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg_value= 54785;
	llvm::GenericValue arg;
	arg.IntVal= llvm::APInt( 32, uint64_t(arg_value) );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( arg ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg_value + 5 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(BooleanBasicTest)
{
	static const char c_program_text[]=
	"\
	fn Foo( bool a, bool b, bool c ) : bool\
	{\
		var bool unused= false;\
		var bool mut tmp= a & b;\
		tmp= tmp & ( true );\
		tmp= tmp | false;\
		return tmp ^ c ;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foobbb" );
	U_TEST_ASSERT( function != nullptr );

	bool arg0= true, arg1= false, arg2= true;

	llvm::GenericValue args[3];
	args[0].IntVal= llvm::APInt( 1, arg0 );
	args[1].IntVal= llvm::APInt( 1, arg1 );
	args[2].IntVal= llvm::APInt( 1, arg2 );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 3 ) );

	U_TEST_ASSERT(
		static_cast<uint64_t>(  ( arg0 & arg1 ) ^ arg2  ) ==
		result_value.IntVal.getLimitedValue() );
}

U_TEST(CallTest0)
{
	static const char c_program_text[]=
	"\
	fn Bar( i32 x ) : i32 \
	{\
		return x * x + 42;\
	}\
	fn Foo( i32 a, i32 b ) : i32\
	{\
		return Bar( a ) + Bar( b );\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooii" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 77, arg1= 1488;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );
	args[1].IntVal= llvm::APInt( 32, uint64_t(arg1) );

	const llvm::GenericValue result_value= engine->runFunction( function, args );

	U_TEST_ASSERT(
		static_cast<uint64_t>( arg0 * arg0 + 42 + arg1 * arg1 + 42 ) ==
		result_value.IntVal.getLimitedValue() );
}

U_TEST(CallTest1)
{
	static const char c_program_text[]=
	"\
	fn Bar( i32 x ) : void \
	{\
		x + x;\
		return;\
	}\
	fn FullyVoid() { return; }\
	fn Foo( i32 a, i32 b ) : i32\
	{\
		Bar( a );\
		FullyVoid();\
		return a / b;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooii" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 775678, arg1= 1488;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );
	args[1].IntVal= llvm::APInt( 32, uint64_t(arg1) );

	const llvm::GenericValue result_value= engine->runFunction( function, args );

	U_TEST_ASSERT(
		static_cast<uint64_t>( arg0 / arg1 ) ==
		result_value.IntVal.getLimitedValue() );
}

U_TEST(RecursiveCallTest)
{
	static const char c_program_text[]=
	R"(
		fn Factorial( u32 x ) : u32
		{
			if( x <= 1u32 ) { return 1u32; }
			else { return x * Factorial( x - 1u32 ); }
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z9Factorialj" );
	U_TEST_ASSERT( function != nullptr );

	uint32_t arg_val= 9u;

	const auto factorial=
	[]( const uint32_t x ) -> uint32_t
	{
		uint32_t result= 1u;
		for( uint32_t i= 2u; i <= x; i++ )
			result*= i;
		return result;
	};

	llvm::GenericValue arg;
	arg.IntVal= llvm::APInt( 32, arg_val );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( &arg, 1 ) );

	U_TEST_ASSERT(
		static_cast<uint64_t>(factorial(arg_val)) ==
		result_value.IntVal.getLimitedValue() );
}

U_TEST(CallTest3)
{
	// Value-parameter of class type.
	static const char c_program_text[]=
	R"(
		struct S { i32 y; f64 fff; i32 x; }
		fn Bar( S s ) : i32 { return s.x; }
		fn Foo() : i32
		{
			var S s{ .x= 5841254, .y= 0, .fff= 0.0 };
			return Bar(s);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(5841254) == result_value.IntVal.getLimitedValue() );
}

U_TEST(CallTest4)
{
	// Value-parameter of huge class type.
	static const char c_program_text[]=
	R"(
		struct S { [ i32, 512 ] arr; }
		fn Bar( S s ) : i32 { return s.arr[42u]; }
		fn Foo() : i32
		{
			var S mut s=zero_init;
			s.arr[42u]= 11145;
			return Bar(s);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(11145) == result_value.IntVal.getLimitedValue() );
}

U_TEST(CallTest5)
{
	// return structure.
	static const char c_program_text[]=
	R"(
		struct S { [ f32, 17 ] arr; i32 x; }
		fn Bar() : S
		{
			var S mut result= zero_init;
			result.x= 888;
			return result;
		}
		fn Foo() : i32
		{
			return Bar().x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(888) == result_value.IntVal.getLimitedValue() );
}

U_TEST(CallTest6)
{
	// return structure and copy it.
	static const char c_program_text[]=
	R"(
		struct S { [ f32, 17 ] arr; i32 x; }
		fn Bar() : S
		{
			var S mut result= zero_init;
			result.x= 888;
			return result;
		}
		fn Foo() : i32
		{
			var S s( Bar() );
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

	U_TEST_ASSERT( static_cast<uint64_t>(888) == result_value.IntVal.getLimitedValue() );
}

U_TEST(CallTest7)
{
	// Return value of struct type in thiscall method.
	static const char c_program_text[]=
	R"(
		struct S { [ f32, 17 ] arr; i32 x; }
		struct A
		{
			i32 x;
			fn Bar( this ) : S
			{
				var S mut result= zero_init;
				result.x= 888 * x;
				return result;
			}
		}
		fn Foo() : i32
		{
			var A a{ .x= 21 };
			return a.Bar().x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(21 * 888) == result_value.IntVal.getLimitedValue() );
}

U_TEST(CallTest8)
{
	// Value-parameter of struct type in thiscall method.
	static const char c_program_text[]=
	R"(
		struct S { [ f32, 17 ] arr; i32 x; }
		struct A
		{
			i32 x;
			fn Bar( this, S s ) : i32
			{
				return s.x * x;
			}
		}
		fn Foo() : i32
		{
			var A a{ .x= 17 };
			var S mut s= zero_init;
			s.x= 555874;
			return a.Bar( s );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(17 * 555874) == result_value.IntVal.getLimitedValue() );
}

U_TEST(CallTest9)
{
	// Value-parameter of struct type and return value of struct type in thiscall method.
	static const char c_program_text[]=
	R"(
		struct S { [ f32, 17 ] arr; i32 x; }
		struct A
		{
			i32 x;
			fn Bar( this, S mut s ) : S
			{
				s.x= s.x * x; // Function changes value of parameter
				return s;
			}
		}
		fn Foo() : i32
		{
			var A a{ .x= 746984 };
			var S mut s= zero_init;
			s.x= 25;
			return a.Bar( s ).x + s.x; // s.x must left unchanged after call.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(25 * 746984 + 25) == result_value.IntVal.getLimitedValue() );
}

U_TEST(TempVariableConstructionTest0)
{
	// Construction of temp variable of int type.
	static const char c_program_text[]=
	R"(
		fn Sum( i32 a, i32 b ) : i32 { return a + b; }
		fn Foo() : i32
		{
			return Sum( i32(455), i32(-35) );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(455 - 35) == result_value.IntVal.getLimitedValue() );
}

U_TEST(TempVariableConstructionTest1)
{
	// Construction of temp variable of struct type, using constructor.
	static const char c_program_text[]=
	R"(
		struct fixed16
		{
			i32 x;
			fn constructor( i32 i )
			( x= i * 65536 )
			{}
		}
		fn Fixed16ToInt( fixed16 f ) : i32
		{
			return f.x / 65536;
		}
		fn Fixed16ToIntRef( fixed16 &imut f ) : i32
		{
			return f.x / 65536;
		}
		fn Foo() : i32
		{
			return Fixed16ToInt( fixed16( 6211 ) ) + Fixed16ToIntRef( fixed16( 412 ) );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(6211 + 412) == result_value.IntVal.getLimitedValue() );
}

U_TEST(WhileOperatorTest)
{
	static const char c_program_text[]=
	"\
	fn Foo( i32 a, i32 b ) : i32\
	{\
		var i32 mut x= a;\
		while( x > 0 )\
		{\
			x= x - 1i32;\
		}\
		x = x + 34i32;\
		while( x != x ) {}\
		return x + b;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooii" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 77, arg1= 1488;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );
	args[1].IntVal= llvm::APInt( 32, uint64_t(arg1) );
	const llvm::GenericValue result_value= engine->runFunction( function, args );

	U_TEST_ASSERT(
		static_cast<uint64_t>( 34 + arg1 ) ==
		result_value.IntVal.getLimitedValue() );
}

U_TEST(IfOperatorTest0)
{
	// Simple if without else/elseif.
	static const char c_program_text[]=
	"\
	fn Foo( i32 x ) : i32\
	{\
		var i32 mut tmp= x;\
		if( x < 0 ) { tmp= -x; }\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue arg;

	{
		arg.IntVal= llvm::APInt( 32, 654 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 654 ) == result_value.IntVal.getLimitedValue() );
	}
	{
		arg.IntVal= llvm::APInt( 32, uint64_t(-2564) );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 2564 ) == result_value.IntVal.getLimitedValue() );
	}
}

U_TEST(IfOperatorTest1)
{
	// If-else.
	static const char c_program_text[]=
	"\
	fn Foo( i32 x ) : i32\
	{\
		var i32 mut tmp= 0;\
		var i32 bits= x & 1;\
		if( bits == 0 ) { tmp= x; }\
		else { tmp= x * 2; }\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue arg;

	{
		arg.IntVal= llvm::APInt( 32, 654 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 654 ) == result_value.IntVal.getLimitedValue() );
	}
	{
		arg.IntVal= llvm::APInt( 32, 655 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 655 * 2 ) == result_value.IntVal.getLimitedValue() );
	}
}

U_TEST(IfOperatorTest2)
{
	// If-else-if.
	static const char c_program_text[]=
	"\
	fn Foo( i32 x ) : i32\
	{\
		var i32 mut tmp= 0;\
		var i32 bits= x & 3;\
		tmp= 1488;\
		if( bits == 0 ) { tmp= x; }\
		else if( bits == 1 ) { tmp= x * 2; }\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue arg;

	{
		arg.IntVal= llvm::APInt( 32, 16 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 16 ) == result_value.IntVal.getLimitedValue() );
	}
	{
		arg.IntVal= llvm::APInt( 32, 17 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 17 * 2 ) == result_value.IntVal.getLimitedValue() );
	}
	{
		arg.IntVal= llvm::APInt( 32, 18 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 1488 ) == result_value.IntVal.getLimitedValue() );
	}
}

U_TEST(IfOperatorTest3)
{
	// If-else-if-else.
	static const char c_program_text[]=
	"\
	fn Foo( i32 x ) : i32\
	{\
		var i32 mut tmp= 0;\
		var i32 mut bits= 0;\
		bits= x & 3;\
		if( bits == 0 ) { tmp= x; }\
		else if( bits == 1 ) { tmp= x * 2; }\
		else { tmp= 1488; }\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue arg;

	{
		arg.IntVal= llvm::APInt( 32, 16 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 16 ) == result_value.IntVal.getLimitedValue() );
	}
	{
		arg.IntVal= llvm::APInt( 32, 17 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 17 * 2 ) == result_value.IntVal.getLimitedValue() );
	}
	{
		arg.IntVal= llvm::APInt( 32, 18 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 1488 ) == result_value.IntVal.getLimitedValue() );
	}
}

U_TEST(IfOperatorTest4)
{
	// If else-if else-if.
	static const char c_program_text[]=
	"\
	fn Foo( i32 x ) : i32\
	{\
		var i32 mut tmp= 0;\
		var i32 mut bits= 0;\
		bits= x & 3;\
		tmp= 1488;\
		if( bits == 0 ) { tmp= x; }\
		else if( bits == 1 ) { tmp= x * 2; }\
		else if( bits == 2 ) { tmp= x * 3; }\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue arg;

	{
		arg.IntVal= llvm::APInt( 32, 16 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 16 ) == result_value.IntVal.getLimitedValue() );
	}
	{
		arg.IntVal= llvm::APInt( 32, 17 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 17 * 2 ) == result_value.IntVal.getLimitedValue() );
	}
	{
		arg.IntVal= llvm::APInt( 32, 18 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 18 * 3 ) == result_value.IntVal.getLimitedValue() );
	}
	{
		arg.IntVal= llvm::APInt( 32, 19 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 1488 ) == result_value.IntVal.getLimitedValue() );
	}
}

U_TEST(IfOperatorTest5)
{
	// If else-if else-if else.
	static const char c_program_text[]=
	"\
	fn Foo( i32 x ) : i32\
	{\
		var i32 mut tmp= 0;\
		var i32 mut bits= 0;\
		bits= x & 3;\
		if( bits == 0 ) { tmp= x; }\
		else if( bits == 1 ) { tmp= x * 2; }\
		else if( bits == 2 ) { tmp= x * 3; }\
		else { tmp= 1488; }\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue arg;

	{
		arg.IntVal= llvm::APInt( 32, 16 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 16 ) == result_value.IntVal.getLimitedValue() );
	}
	{
		arg.IntVal= llvm::APInt( 32, 17 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 17 * 2 ) == result_value.IntVal.getLimitedValue() );
	}
	{
		arg.IntVal= llvm::APInt( 32, 18 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 18 * 3 ) == result_value.IntVal.getLimitedValue() );
	}
	{
		arg.IntVal= llvm::APInt( 32, 19 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 1488 ) == result_value.IntVal.getLimitedValue() );
	}
}

U_TEST(BreakOperatorTest0)
{
	static const char c_program_text[]=
	"fn Foo( i32 x ) : i32\
	{\
		var i32 mut tmp= x;\
		while( x < 0 ) { tmp= -x; if( true ) { break; } else {} tmp= 0; }\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue arg;

	{
		arg.IntVal= llvm::APInt( 32, 654 );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 654 ) == result_value.IntVal.getLimitedValue() );
	}
	{
		arg.IntVal= llvm::APInt( 32, uint64_t(-2564) );
		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( static_cast<uint64_t>( 2564 ) == result_value.IntVal.getLimitedValue() );
	}
}

U_TEST(BreakOperatorTest1)
{
	// Should break from inner loop.
	static const char c_program_text[]=
	"fn Foo( i32 x ) : i32\
	{\
		var i32 mut tmp= 0;\
		var i32 mut counter= 1;\
		while( counter > 0 )\
		{\
			while( counter > 0 )\
			{\
				if( true ) { break; } else {}\
				tmp= 0;\
			}\
			tmp= x;\
			counter = counter - 1;\
		}\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue arg;
	arg.IntVal= llvm::APInt( 32, 654 );
	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( arg ) );
	U_TEST_ASSERT( static_cast<uint64_t>( 654 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(BreakOperatorTest2)
{
	// Should break from current loop with previous inner loop.
	static const char c_program_text[]=
	"fn Foo( i32 x ) : i32\
	{\
		var i32 mut tmp= 0;\
		while( true )\
		{\
			while( false ){}\
			tmp= x;\
			if( true ) { break; } else {}\
			tmp= 0;\
		}\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue arg;
	arg.IntVal= llvm::APInt( 32, 654 );
	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( arg ) );
	U_TEST_ASSERT( static_cast<uint64_t>( 654 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ContinueOperatorTest0)
{
	static const char c_program_text[]=
	"fn Foo( i32 x ) : i32\
	{\
		var i32 mut tmp= 0;\
		var i32 mut counter= 5;\
		while( counter > 0 )\
		{\
			tmp= x;\
			counter = counter - 1;\
			if( true ) { continue; } else {}\
			tmp= 0;\
		}\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue arg;
	arg.IntVal= llvm::APInt( 32, 654 );
	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( arg ) );
	U_TEST_ASSERT( static_cast<uint64_t>( 654 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ContinueOperatorTest1)
{
	// Should continue from inner loop.
	static const char c_program_text[]=
	"fn Foo( i32 x ) : i32\
	{\
		var i32 mut tmp= 0;\
		var i32 mut counter= 1;\
		while( counter > 0 )\
		{\
			while( tmp == 0 )\
			{\
				tmp= 5;\
				if( true ) { continue; } else {}\
				tmp= 0;\
			}\
			tmp= x;\
			counter = counter - 1;\
		}\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue arg;
	arg.IntVal= llvm::APInt( 32, 654 );
	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( arg ) );
	U_TEST_ASSERT( static_cast<uint64_t>( 654 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(StructTest0)
{
	static const char c_program_text[]=
	R"(
	struct Point
	{
		i32 x;
		[ i32, 4 ] zzz;
		i32 y;
	}
	fn Foo( i32 a, i32 b, i32 c ) : i32
	{
		var Point mut p= zero_init;
		var u32 mut index= 0u32;
		p.x= a;
		p.y = b;
		p.zzz[0u]= c;
		p.zzz[1u]= p.x * p.y;
		index= 2u;
		p.zzz[index]= p.zzz[1u] + c;
		return p.zzz[index];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooiii" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 1488, arg1= 77, arg2= 546;

	llvm::GenericValue args[3];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );
	args[1].IntVal= llvm::APInt( 32, uint64_t(arg1) );
	args[2].IntVal= llvm::APInt( 32, uint64_t(arg2) );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 3 ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 * arg1 + arg2 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(StructTest1)
{
	// Struct with struct inside.
	static const char c_program_text[]=
	R"(
	struct Dummy
	{
		f32 x;
		f64 y;
		[ f64, 2 ] z;
	}
	struct Point
	{
		i32 x;
		Dummy dummy;
		i32 y;
	}
	fn Foo( f64 a, f64 b ) : f64
	{
		var Point mut p= zero_init;
		p.dummy.y= a;
		p.dummy.z[1u]= b;
		return p.dummy.y - p.dummy.z[1u];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foodd" );
	U_TEST_ASSERT( function != nullptr );

	double arg0= 1488, arg1= 77;

	llvm::GenericValue args[2];
	args[0].DoubleVal= arg0;
	args[1].DoubleVal= arg1;

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

	ASSERT_NEAR( arg0 - arg1, result_value.DoubleVal, 0.001 );
}

U_TEST(BlocksTest0)
{
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			{
				return 333;
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, {} );

	U_TEST_ASSERT( result_value.IntVal.getLimitedValue() == uint64_t( 333 ) );
}

U_TEST(BlocksTest1)
{
	// Variable in inner block must shadow variable from outer block with same name.
	static const char c_program_text[]=
	R"(
	fn Foo( i32 a, i32 b ) : i32
	{
		var i32 x= a;
		{
			var i32 x= b;
			return x;
		}
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooii" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 1488, arg1= 77;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );
	args[1].IntVal= llvm::APInt( 32, uint64_t(arg1) );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg1 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ReferencesTest0)
{
	// Assignment to reference must affect referenced variable.
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		var i32 mut x= 0;
		{
			var i32 &mut x_ref= x;
			x_ref= 42;
		}
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

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ReferencesTest1)
{
	// Must read variable value, using reference.
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		var i32 x= 56845;
		var i32 &x_ref= x;
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

	U_TEST_ASSERT( static_cast<uint64_t>( 56845 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ReferencesTest2)
{
	// References must correctly work with arrays.
	static const char c_program_text[]=
	R"(
	fn Foo( i32 a, i32 b ) : i32
	{
		var [ i32, 4 ] mut arr= zero_init;
		{
			var [ i32, 4 ] &mut arr_ref= arr;
			arr_ref[0u]= a;
			arr_ref[1u]= b;
		}
		{
			var [ i32, 4 ] &mut arr_ref= arr;
			arr_ref[2u]= arr_ref[0u] * arr_ref[1u];
		}
		return arr[2u];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooii" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 1488, arg1= 77;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );
	args[1].IntVal= llvm::APInt( 32, uint64_t(arg1) );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 * arg1 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ReferencesTest3)
{
	// Reference to reference.
	static const char c_program_text[]=
	R"(
	fn Foo() : i32
	{
		var i32 imut x= 666;
		var i32 &imut x_ref= x;
		var i32 &imut x_ref_ref= x_ref;
		return x_ref_ref;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 666 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ReferencesTest4)
{
	// Reference to argument.
	static const char c_program_text[]=
	R"(
	fn Foo( i32 a ) : i32
	{
		var i32 &imut a_ref= a;
		return a_ref * 564;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 148;

	llvm::GenericValue args[1];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 1 ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 * 564 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ReferencesTest5)
{
	// Reference arguments.
	static const char c_program_text[]=
	R"(
	fn DoubleIt( i32 &imut x ) : i32
	{ return x * 2; }
	fn Foo( i32 a ) : i32
	{
		var i32 triple_a= a * 3;
		return DoubleIt( triple_a );
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 148;

	llvm::GenericValue args[1];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 1 ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 * 3 * 2 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ReferencesTest6)
{
	// Reference nonconst arguments.
	static const char c_program_text[]=
	R"(
	fn DoubleIt( i32 &mut x )
	{ x = x * 2; }
	fn Foo( i32 a ) : i32
	{
		var i32 mut triple_a= a * 3;
		DoubleIt( triple_a );
		return triple_a;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 148;

	llvm::GenericValue args[1];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 1 ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 * 3 * 2 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ReferencesTest7)
{
	// Reference nonconst argument of struct type.
	static const char c_program_text[]=
	R"(
	struct C
	{
		i32 x;
		[ i32, 4 ] zzz;
		i32 y;
	}
	fn Bar( C &mut c )
	{ c.zzz[2u] = 99985; }
	fn Foo() : i32
	{
		var C mut c= zero_init;
		Bar( c );
		return c.zzz[2u];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 99985 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ReferencesTest8)
{
	// Reference nonconst argument of array type.
	static const char c_program_text[]=
	R"(
	fn Bar( [ i32, 5 ] &mut arr )
	{ arr[3u] = 99985; }
	fn Foo() : i32
	{
		var [ i32, 5 ] mut arr= zero_init;
		Bar( arr );
		return arr[3u];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 99985 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ReferencesTest9)
{
	// Return reference.
	static const char c_program_text[]=
	R"(
	fn Max( i32 &a, i32 &b ) : i32&
	{
		if( a > b ) { return a; }
		else { return b; }
	}
	fn Foo( i32 x, i32 y ) : i32
	{
		return Max( x, y );
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooii" );
	U_TEST_ASSERT( function != nullptr );

	static const int32_t cases_args[3][2]= { { 7, 567 }, { 48454, 758 }, { 4468, 4468 } };
	for( uint32_t i= 0u; i < 3u; i++ )
	{
		llvm::GenericValue args[2];
		args[0].IntVal= llvm::APInt( 32, uint64_t(cases_args[i][0]) );
		args[1].IntVal= llvm::APInt( 32, uint64_t(cases_args[i][1]) );

		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

		U_TEST_ASSERT(
			static_cast<uint64_t>( std::max( cases_args[i][0], cases_args[i][1] ) ) ==
			result_value.IntVal.getLimitedValue() );
	}
}

U_TEST(ReferencesTest10)
{
	// Return reference of class type.
	static const char c_program_text[]=
	R"(
	struct S{ i32 x; }
	fn Max( S &a, S &b ) : S&
	{
		if( a.x > b.x ) { return a; }
		else { return b; }
	}
	fn Foo( i32 x, i32 y ) : i32
	{
		var S a{ .x= x }, b{ .x= y };
		return Max( a, b ).x;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooii" );
	U_TEST_ASSERT( function != nullptr );

	static const int32_t cases_args[3][2]= { { 7, 567 }, { 48454, 758 }, { 4468, 4468 } };
	for( uint32_t i= 0u; i < 3u; i++ )
	{
		llvm::GenericValue args[2];
		args[0].IntVal= llvm::APInt( 32, uint64_t(cases_args[i][0]) );
		args[1].IntVal= llvm::APInt( 32, uint64_t(cases_args[i][1]) );

		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

		U_TEST_ASSERT(
			static_cast<uint64_t>( std::max( cases_args[i][0], cases_args[i][1] ) ) ==
			result_value.IntVal.getLimitedValue() );
	}
}

U_TEST(ReferencesTest11)
{
	// Return reference of array type.
	static const char c_program_text[]=
	R"(
	fn Max( [i32, 3] &a, [i32, 3] &b ) : [i32, 3] &
	{
		if( a[1u] > b[1u] ) { return a; }
		else { return b; }
	}
	fn Foo( i32 x, i32 y ) : i32
	{
		var [i32, 3] a[0,x,0], b[0,y,0];
		return Max( a, b )[1u];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooii" );
	U_TEST_ASSERT( function != nullptr );

	static const int32_t cases_args[3][2]= { { 7, 567 }, { 48454, 758 }, { 4468, 4468 } };
	for( uint32_t i= 0u; i < 3u; i++ )
	{
		llvm::GenericValue args[2];
		args[0].IntVal= llvm::APInt( 32, uint64_t(cases_args[i][0]) );
		args[1].IntVal= llvm::APInt( 32, uint64_t(cases_args[i][1]) );

		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

		U_TEST_ASSERT(
			static_cast<uint64_t>( std::max( cases_args[i][0], cases_args[i][1] ) ) ==
			result_value.IntVal.getLimitedValue() );
	}
}

U_TEST(BindValueToConstReferenceTest0)
{
	// Bind value-result to const reference parameter.
	static const char c_program_text[]=
	R"(
	fn DoubleIt( i32 &imut x ) : i32
	{ return x * 2; }
	fn Foo( i32 a ) : i32
	{
		return DoubleIt( a * 3 );
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 666;

	llvm::GenericValue args[1];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 1 ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 * 3 * 2 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(FunctionsOverloadingTest0)
{
	// Different parameters count.
	static const char c_program_text[]=
	R"(
	fn Bar() : i32
	{
		return 42;
	}
	fn Bar( bool neg ) : i32
	{
		if( neg ) { return -1; }
		return 1;
	}
	fn Foo() : i32
	{
		return Bar() * Bar(false);
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );


	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>());

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(FunctionsOverloadingTest1)
{
	// Different parameters type.
	static const char c_program_text[]=
	R"(
	fn Bar( f64 val ) : i32
	{
		return 42;
	}
	fn Bar( i32 val ) : i32
	{
		return 24;
	}
	fn Foo() : i32
	{
		return Bar( 1.0 ) - Bar( 1 );
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>());

	U_TEST_ASSERT( static_cast<uint64_t>( 42 - 24 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(FunctionsOverloadingTest2)
{
	// Different parameters type and const-reference.
	static const char c_program_text[]=
	R"(
	fn Bar( f64 &imut val ) : i32
	{
		return 42;
	}
	fn Bar( i32 &imut val ) : i32
	{
		return 24;
	}
	fn Foo() : i32
	{
		return Bar( 1.0 ) - Bar( 1 );
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>());

	U_TEST_ASSERT( static_cast<uint64_t>( 42 - 24 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(FunctionsOverloadingTest3)
{
	// Different parameters type and const-reference for one of parameters.
	static const char c_program_text[]=
	R"(
	fn Bar( f64 &imut val ) : i32
	{
		return 42;
	}
	fn Bar( i32 imut val ) : i32
	{
		return 24;
	}
	fn Foo() : i32
	{
		return Bar( 1.0 ) - Bar( 1 );
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>());

	U_TEST_ASSERT( static_cast<uint64_t>( 42 - 24 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(FunctionPrototypeTest0)
{
	static const char c_program_text[]=
	R"(
		fn Minus2( i32 x ) : i32;
		fn Minus3( i32 x ) : i32;

		fn Foo() : i32
		{
			return Minus2( 79 );
		}

		fn Minus2( i32 x ) : i32
		{
			if( x < 2 ){ return 666; }
			return Minus3( x - 2 );
		}
		fn Minus3( i32 x ) : i32
		{
			if( x < 3 ){ return 666; }
			return Minus2( x - 3 );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>());

	U_TEST_ASSERT( static_cast<uint64_t>( 666 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(FunctionPrototypeTest1)
{
	// Different parameter name.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x ) : i32;
		fn Foo() : i32
		{
			return Bar( 79 );
		}
		fn Bar( i32 xxx ) : i32 { return xxx * 2; }
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>());

	U_TEST_ASSERT( static_cast<uint64_t>( 79 * 2 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(FunctionPrototypeTest3)
{
	// Prototypes must correctly work with overloading.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x ) : i32;
		fn Bar( f32 x ) : i32;
		fn Foo() : i32
		{
			return Bar(0) * Bar(0.0f);
		}
		fn Bar( i32 x ) : i32
		{
			return 666;
		}
		fn Bar( f32 x ) : i32
		{
			return 1937;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>());

	U_TEST_ASSERT( static_cast<uint64_t>( 666 * 1937 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(HeapUsage_Test0)
{
	// Test basic usage of malloc/free. Interpreter should process such functions specially.

	static const char c_program_text[]=
	R"(
		fn nomangle ust_memory_allocate_impl( size_type size ) unsafe : $(byte8);
		fn nomangle ust_memory_free_impl( $(byte8) ptr ) unsafe;

		fn Write( $(byte8) addr )
		{
			unsafe
			{
				$>(addr + 0s)= byte8("Q"c8);
				$>(addr + 1s)= byte8("y"c8);
				$>(addr + 2s)= byte8("!"c8);
			}
		}
		fn Read( $(byte8) addr )
		{
			unsafe
			{
				halt if( $>(addr + 0s) != byte8("Q"c8) );
				halt if( $>(addr + 1s) != byte8("y"c8) );
				halt if( $>(addr + 2s) != byte8("!"c8) );
			}
		}
		fn Free( $(byte8) addr )
		{
			unsafe( ust_memory_free_impl(addr) );
		}
		fn Foo()
		{
			auto addr= unsafe( ust_memory_allocate_impl(3s) );
			Write(addr);
			Read(addr);
			Free(addr);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>());
}

} // namespace

} // namespace U
