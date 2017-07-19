#include <cstdlib>
#include <iostream>

#include "tests.hpp"

#define ASSERT_NEAR( x, y, eps ) U_TEST_ASSERT( std::abs( (x) - (y) ) <= eps )

namespace U
{

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

	int arg0= 100500, arg1= 1488, arg2= 42;

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue args[3];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );
	args[2].IntVal= llvm::APInt( 32, arg2 );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 3 ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 + arg1 + arg2 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ArgumentsAssignmentTest)
{
	static const char c_program_text[]=
	"\
	fn Foo( i32 a ) : i32\
	{\
		a= a * a;\
		return a;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue arg;
	arg.IntVal= llvm::APInt( 32, 5648 );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( arg ) );

	U_TEST_ASSERT( static_cast<uint64_t>( 5648 * 5648 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(BasicBinaryOperationsTest)
{
	static const char c_program_text[]=
	"\
	fn Foo( i32 a, i32 b, i32 c ) : i32\
	{\
		return a * a + b / b - c;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 77, arg1= 1488, arg2= 42;

	llvm::GenericValue args[3];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );
	args[2].IntVal= llvm::APInt( 32, arg2 );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 3 ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 * arg0 + arg1 / arg1 - arg2 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(BasicBinaryOperationsFloatTest)
{
	static const char c_program_text[]=
	"\
	fn Foo( f32 a, f32 b, f32 c ) : f32\
	{\
		return a * a + b / b - c;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	float arg0= 77.0f, arg1= 1488.5f, arg2= -42.31415926535f;

	llvm::GenericValue args[3];
	args[0].FloatVal= arg0;
	args[1].FloatVal= arg1;
	args[2].FloatVal= arg2;

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 3 ) );

	ASSERT_NEAR( arg0 * arg0 + arg1 / arg1 - arg2, result_value.FloatVal, 0.1f );
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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 1488, arg1= 77;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );

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
		var i32 tmp= a - b, r= 1;\
		tmp= tmp * r;\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 1488, arg1= 77;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );

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

	llvm::Function* function= engine->FindFunctionNamed( "Foo32" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 1488, arg1= 77;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );

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

	llvm::Function* function= engine->FindFunctionNamed( "Foo64" );
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

	llvm::Function* function= engine->FindFunctionNamed( "Pi" );
	U_TEST_ASSERT( function != nullptr );


	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	ASSERT_NEAR( 3.1415926535f, result_value.FloatVal, 0.0001f );
}

U_TEST(UnaryMinusTest)
{
	static const char c_program_text[]=
	"\
	fn Foo( i32 x ) : i32\
	{\
		var i32 tmp= -x;\
		return -tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg_value= 54785;
	llvm::GenericValue arg;
	arg.IntVal= llvm::APInt( 32, arg_value );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( arg ) );

	U_TEST_ASSERT( static_cast<uint64_t>( - - arg_value ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(UnaryMinusFloatTest)
{
	static const char c_program_text[]=
	"\
	fn Foo( f64 x ) : f64\
	{\
		var f64 tmp= -x;\
		return -tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	double arg_value= 54785;
	llvm::GenericValue arg;
	arg.DoubleVal= arg_value;

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( arg ) );

	ASSERT_NEAR( -( - arg_value ), result_value.DoubleVal, 0.01f );
}

U_TEST(ArraysTest0)
{
	static const char c_program_text[]=
	"\
	fn Foo(i32 x ) : i32\
	{\
		var [ i32, 17 ] tmp= zero_init;\
		tmp[5u32]= x;\
		return tmp[5u] + 5;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg_value= 54785;
	llvm::GenericValue arg;
	arg.IntVal= llvm::APInt( 32, arg_value );

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
		var [ [ [ i32, 3 ], 5 ], 17 ] tmp= zero_init;\
		tmp[5u32][3u32][1u32]= x;\
		return tmp[5u][3u][1u] + 5;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg_value= 54785;
	llvm::GenericValue arg;
	arg.IntVal= llvm::APInt( 32, arg_value );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( arg ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg_value + 5 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(LogicalBinaryOperationsTest)
{
	static const char c_program_text[]=
	"\
	fn Foo( i32 a, i32 b, i32 c ) : i32\
	{\
		return ( (a & b) ^ c ) + ( a | b | c );\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 77, arg1= 1488, arg2= 42;

	llvm::GenericValue args[3];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );
	args[2].IntVal= llvm::APInt( 32, arg2 );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 3 ) );

	U_TEST_ASSERT(
		static_cast<uint64_t>( ( (arg0 & arg1) ^ arg2 ) + ( arg0 | arg1 | arg2 ) ) ==
		result_value.IntVal.getLimitedValue() );
}

U_TEST(BooleanBasicTest)
{
	static const char c_program_text[]=
	"\
	fn Foo( bool a, bool b, bool c ) : bool\
	{\
		var bool unused= false;\
		var bool tmp= a & b;\
		tmp= tmp & ( true );\
		tmp= tmp | false;\
		return tmp ^ c ;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 77, arg1= 1488;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 3 ) );

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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 775678, arg1= 1488;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 3 ) );

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

	llvm::Function* function= engine->FindFunctionNamed( "Factorial" );
	U_TEST_ASSERT( function != nullptr );

	unsigned int arg_val= 9u;

	const auto factorial=
	[]( const unsigned int x ) -> unsigned int
	{
		unsigned int result= 1u;
		for( unsigned int i= 2u; i <= x; i++ )
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

U_TEST(EqualityOperatorsTest)
{
	static const char c_program_text[]=
	"\
	fn Foo( i32 a, i32 b, i32 c ) : bool\
	{\
		return ( a == b ) | ( a != c ) ;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 77, arg1= 1488, arg2= 42;

	llvm::GenericValue args[3];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );
	args[2].IntVal= llvm::APInt( 32, arg2 );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 3 ) );

	U_TEST_ASSERT(
		static_cast<uint64_t>( ( arg0 == arg1 ) | ( arg0 != arg2 ) ) ==
		result_value.IntVal.getLimitedValue() );
}

U_TEST(EqualityFloatOperatorsTest)
{
	static const char c_program_text[]=
	"\
	fn Foo( f32 a, f32 b, f32 c ) : bool\
	{\
		return ( a == b ) | ( a != c );\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	float arg0= 77, arg1= 1488, arg2= 42;

	llvm::GenericValue args[3];
	args[0].FloatVal= arg0;
	args[1].FloatVal= arg1;
	args[2].FloatVal= arg2;

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 3 ) );

	U_TEST_ASSERT(
		static_cast<uint64_t>( ( arg0 == arg1 ) | ( arg0 != arg2 ) ) ==
		result_value.IntVal.getLimitedValue() );
}

U_TEST(ComparisonSignedOperatorsTest)
{
	static const char c_program_text[]=
	"\
	fn Less( i32 a, i32 b ) : bool\
	{\
		return a < b;\
	}\
	fn LessOrEqual( i32 a, i32 b ) : bool\
	{\
		return a <= b;\
	}\
	fn Greater( i32 a, i32 b ) : bool\
	{\
		return a > b;\
	}\
	fn GreaterOrEqual( i32 a, i32 b ) : bool\
	{\
		return a >= b;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	// 0 means a < b, 1 means a == b, 2 means a > b
	static const bool c_true_matrix[4][3]=
	{
		{ true , false, false }, // less
		{ true , true , false }, // less    or equal
		{ false, false, true  }, // greater
		{ false, true , true  }, // greater or equal
	};
	static const char* const c_func_names[4]=
	{
		"Less", "LessOrEqual", "Greater", "GreaterOrEqual",
	};
	for( unsigned int func_n= 0u; func_n < 4u; func_n++ )
	{
		llvm::Function* function= engine->FindFunctionNamed( c_func_names[ func_n ] );
		U_TEST_ASSERT( function != nullptr );

		llvm::GenericValue args[2];
		llvm::GenericValue result_value;

		// TODO - add more test-cases.

		// Less
		args[0].IntVal= llvm::APInt( 32, -1488 );
		args[1].IntVal= llvm::APInt( 32, 51478 );
		result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );
		U_TEST_ASSERT( static_cast<uint64_t>( c_true_matrix[ func_n ][0] ) == result_value.IntVal.getLimitedValue() );

		// Equal
		args[0].IntVal= llvm::APInt( 32, 8596 );
		args[1].IntVal= llvm::APInt( 32, 8596 );
		result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );
		U_TEST_ASSERT( static_cast<uint64_t>( c_true_matrix[ func_n ][1] ) == result_value.IntVal.getLimitedValue() );

		// Greater
		args[0].IntVal= llvm::APInt( 32, 6545284 );
		args[1].IntVal= llvm::APInt( 32, 6544 );
		result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );
		U_TEST_ASSERT( static_cast<uint64_t>( c_true_matrix[ func_n ][2] ) == result_value.IntVal.getLimitedValue() );
	}
}

U_TEST(ComparisonUnsignedOperatorsTest)
{
	static const char c_program_text[]=
	"\
	fn Less( u32 a, u32 b ) : bool\
	{\
		return a < b;\
	}\
	fn LessOrEqual( u32 a, u32 b ) : bool\
	{\
		return a <= b;\
	}\
	fn Greater(  u32 a, u32 b ) : bool\
	{\
		return a > b;\
	}\
	fn GreaterOrEqual( u32 a, u32 b ) : bool\
	{\
		return a >= b;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	// 0 means a < b, 1 means a == b, 2 means a > b
	static const bool c_true_matrix[4][3]=
	{
		{ true , false, false }, // less
		{ true , true , false }, // less    or equal
		{ false, false, true  }, // greater
		{ false, true , true  }, // greater or equal
	};
	static const char* const c_func_names[4]=
	{
		"Less", "LessOrEqual", "Greater", "GreaterOrEqual",
	};
	for( unsigned int func_n= 0u; func_n < 4u; func_n++ )
	{
		llvm::Function* function= engine->FindFunctionNamed( c_func_names[ func_n ] );
		U_TEST_ASSERT( function != nullptr );

		llvm::GenericValue args[2];
		llvm::GenericValue result_value;

		// TODO - add more test-cases.

		// Less
		args[0].IntVal= llvm::APInt( 32,  1488 );
		args[1].IntVal= llvm::APInt( 32, 51478 );
		result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );
		U_TEST_ASSERT( static_cast<uint64_t>( c_true_matrix[ func_n ][0] ) == result_value.IntVal.getLimitedValue() );

		// Equal
		args[0].IntVal= llvm::APInt( 32, 8596 );
		args[1].IntVal= llvm::APInt( 32, 8596 );
		result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );
		U_TEST_ASSERT( static_cast<uint64_t>( c_true_matrix[ func_n ][1] ) == result_value.IntVal.getLimitedValue() );

		// Greater
		args[0].IntVal= llvm::APInt( 32, 6545284 );
		args[1].IntVal= llvm::APInt( 32, 6544 );
		result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );
		U_TEST_ASSERT( static_cast<uint64_t>( c_true_matrix[ func_n ][2] ) == result_value.IntVal.getLimitedValue() );
	}
}

U_TEST(ComparisonFloatOperatorsTest)
{
	static const char c_program_text[]=
	"\
	fn Less( f32 a, f32 b ) : bool\
	{\
		return a < b;\
	}\
	fn LessOrEqual( f32 a, f32 b ) : bool\
	{\
		return a <= b;\
	}\
	fn Greater( f32 a, f32 b ) : bool\
	{\
		return a > b;\
	}\
	fn GreaterOrEqual( f32 a, f32 b ) : bool\
	{\
		return a >= b;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	// 0 means a < b, 1 means a == b, 2 means a > b
	static const bool c_true_matrix[4][3]=
	{
		{ true , false, false }, // less
		{ true , true , false }, // less    or equal
		{ false, false, true  }, // greater
		{ false, true , true  }, // greater or equal
	};
	static const char* const c_func_names[4]=
	{
		"Less", "LessOrEqual", "Greater", "GreaterOrEqual",
	};
	for( unsigned int func_n= 0u; func_n < 4u; func_n++ )
	{
		llvm::Function* function= engine->FindFunctionNamed( c_func_names[ func_n ] );
		U_TEST_ASSERT( function != nullptr );

		llvm::GenericValue args[2];
		llvm::GenericValue result_value;

		// TODO - add more test-cases.

		// Less
		args[0].FloatVal= -1488.0f;
		args[1].FloatVal=  51255.0f;
		result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );
		U_TEST_ASSERT( static_cast<uint64_t>( c_true_matrix[ func_n ][0] ) == result_value.IntVal.getLimitedValue() );

		// Equal
		args[0].FloatVal= -0.0f;
		args[1].FloatVal= +0.0f;
		result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );
		U_TEST_ASSERT( static_cast<uint64_t>( c_true_matrix[ func_n ][1] ) == result_value.IntVal.getLimitedValue() );

		// Greater
		args[0].FloatVal= 5482.3f;
		args[1].FloatVal= 785.2f;
		result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );
		U_TEST_ASSERT( static_cast<uint64_t>( c_true_matrix[ func_n ][2] ) == result_value.IntVal.getLimitedValue() );
	}
}

U_TEST(WhileOperatorTest)
{
	static const char c_program_text[]=
	"\
	fn Foo( i32 a, i32 b ) : i32\
	{\
		var i32 x= a;\
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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 77, arg1= 1488;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );
	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 3 ) );

	U_TEST_ASSERT(
		static_cast<uint64_t>( 34 + arg1 ) ==
		result_value.IntVal.getLimitedValue() );
}

U_TEST(IfOperatorTest0)
{
	// Simple if without else/elseif.
	static const char c_program_text[]=
	"\
	fn SimpleIf( i32 x ) : i32\
	{\
		var i32 tmp= x;\
		if( x < 0 ) { tmp= -x; }\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "SimpleIf" );
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
		arg.IntVal= llvm::APInt( 32, -2564 );
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
	fn IfElse( i32 x ) : i32\
	{\
		var i32 tmp= 0;\
		var i32 bits= x & 1;\
		if( bits == 0 ) { tmp= x; }\
		else { tmp= x * 2; }\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "IfElse" );
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
	fn IfElseIf( i32 x ) : i32\
	{\
		var i32 tmp= 0;\
		var i32 bits= x & 3;\
		tmp= 1488;\
		if( bits == 0 ) { tmp= x; }\
		else if( bits == 1 ) { tmp= x * 2; }\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "IfElseIf" );
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
	fn IfElseIfElse( i32 x ) : i32\
	{\
		var i32 tmp= 0;\
		var i32 bits= 0;\
		bits= x & 3;\
		if( bits == 0 ) { tmp= x; }\
		else if( bits == 1 ) { tmp= x * 2; }\
		else { tmp= 1488; }\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "IfElseIfElse" );
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
	fn IfElseIfElseIf( i32 x ) : i32\
	{\
		var i32 tmp= 0;\
		var i32 bits= 0;\
		bits= x & 3;\
		tmp= 1488;\
		if( bits == 0 ) { tmp= x; }\
		else if( bits == 1 ) { tmp= x * 2; }\
		else if( bits == 2 ) { tmp= x * 3; }\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "IfElseIfElseIf" );
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
	fn IfElseIfElseIf( i32 x ) : i32\
	{\
		var i32 tmp= 0;\
		var i32 bits= 0;\
		bits= x & 3;\
		if( bits == 0 ) { tmp= x; }\
		else if( bits == 1 ) { tmp= x * 2; }\
		else if( bits == 2 ) { tmp= x * 3; }\
		else { tmp= 1488; }\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "IfElseIfElseIf" );
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
		var i32 tmp= x;\
		while( x < 0 ) { tmp= -x; if( true ) { break; } else {} tmp= 0; }\
		return tmp;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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
		arg.IntVal= llvm::APInt( 32, -2564 );
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
		var i32 tmp= 0;\
		var i32 counter= 1;\
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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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
		var i32 tmp= 0;\
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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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
		var i32 tmp= 0;\
		var i32 counter= 5;\
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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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
		var i32 tmp= 0;\
		var i32 counter= 1;\
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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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
	class Point
	{
		x : i32;
		zzz : [ i32, 4 ];
		y : i32;
	}
	fn Foo( i32 a, i32 b, i32 c ) : i32
	{
		var Point p= zero_init;
		var u32 index= 0u32;
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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 1488, arg1= 77, arg2= 546;

	llvm::GenericValue args[3];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );
	args[2].IntVal= llvm::APInt( 32, arg2 );

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
	class Dummy
	{
		x : f32;
		y : f64;
		z : [ f64, 2 ];
	}
	class Point
	{
		x : i32;
		dummy : Dummy;
		y : i32;
	}
	fn Foo( f64 a, f64 b ) : f64
	{
		var Point p= zero_init;
		p.dummy.y= a;
		p.dummy.z[1u]= b;
		return p.dummy.y - p.dummy.z[1u];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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

U_TEST(BlocksTest)
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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 1488, arg1= 77;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );

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
		var i32 x= 0;
		var i32 &x_ref= x;
		x_ref= 42;
		return x;
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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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
		var [ i32, 4 ] arr= zero_init;
		var [ i32, 4 ] &arr_ref= arr;
		arr_ref[0u]= a;
		arr_ref[1u]= b;
		arr_ref[2u]= arr_ref[0u] * arr_ref[1u];
		return arr[2u];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 1488, arg1= 77;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );

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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 148;

	llvm::GenericValue args[1];
	args[0].IntVal= llvm::APInt( 32, arg0 );

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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 148;

	llvm::GenericValue args[1];
	args[0].IntVal= llvm::APInt( 32, arg0 );

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
		var i32 triple_a= a * 3;
		DoubleIt( triple_a );
		return triple_a;
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 148;

	llvm::GenericValue args[1];
	args[0].IntVal= llvm::APInt( 32, arg0 );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 1 ) );

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 * 3 * 2 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(ReferencesTest7)
{
	// Reference nonconst argument of class type.
	static const char c_program_text[]=
	R"(
	class C
	{
		x : i32;
		zzz : [ i32, 4 ];
		y : i32;
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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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
		var [ i32, 5 ] arr= zero_init;
		Bar( arr );
		return arr[3u];
	}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	static const int cases_args[3][2]= { { 7, 567 }, { 48454, 758 }, { 4468, 4468 } };
	for( unsigned int i= 0u; i < 3u; i++ )
	{
		llvm::GenericValue args[2];
		args[0].IntVal= llvm::APInt( 32, cases_args[i][0] );
		args[1].IntVal= llvm::APInt( 32, cases_args[i][1] );

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

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	int arg0= 666;

	llvm::GenericValue args[1];
	args[0].IntVal= llvm::APInt( 32, arg0 );

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
	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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
	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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
	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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
	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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
	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
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
	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>());

	U_TEST_ASSERT( static_cast<uint64_t>( 79 * 2 ) == result_value.IntVal.getLimitedValue() );
}

} // namespace U
