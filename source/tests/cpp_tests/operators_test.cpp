#include <cmath>

#include "tests.hpp"

namespace U
{

namespace
{

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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooiii" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 77, arg1= 1488, arg2= 42;

	llvm::GenericValue args[3];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );
	args[1].IntVal= llvm::APInt( 32, uint64_t(arg1) );
	args[2].IntVal= llvm::APInt( 32, uint64_t(arg2) );

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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foofff" );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooiii" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 77, arg1= 1488, arg2= 42;

	llvm::GenericValue args[3];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );
	args[1].IntVal= llvm::APInt( 32, uint64_t(arg1) );
	args[2].IntVal= llvm::APInt( 32, uint64_t(arg2) );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 3 ) );

	U_TEST_ASSERT(
		static_cast<uint64_t>( ( (arg0 & arg1) ^ arg2 ) + ( arg0 | arg1 | arg2 ) ) ==
		result_value.IntVal.getLimitedValue() );
}

U_TEST(AdditiveOperationsTest0)
{
	// += test
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x, i32 y ) : i32
		{
			auto mut result= x;
			result+= y;
			return result;
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

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 + arg1 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(AdditiveOperationsTest1)
{
	// -= test
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x, i32 y ) : i32
		{
			auto mut result= x;
			result-= y;
			return result;
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

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 - arg1 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(AdditiveOperationsTest2)
{
	// /= for array member
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x, i32 y ) : i32
		{
			var [ i32, 8 ] mut result= zero_init;
			result[5u]= x;
			result[5u]/= y;
			return result[5u];
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

	U_TEST_ASSERT( static_cast<uint64_t>( arg0 / arg1 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(LeftShiftTest0)
{
	// Shift signed value.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x, u32 y ) : i32
		{
			return x << y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Fooij" );
	U_TEST_ASSERT( function != nullptr );

	static const int32_t values[]=
	{
		std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), 0,
		1, -1, 2, -2, 3, 12, 17, 65536, 54284964, -2000000000, 2000000000, -58, 84, 1831516318,
	};
	for( const int32_t value : values )
	{
		llvm::GenericValue args[2];
		args[0].IntVal= llvm::APInt( 32, uint64_t(value) );
		for( uint32_t shift= 0u; shift < 32u; shift++ )
		{
			args[1].IntVal= llvm::APInt( 32, shift );

			llvm::GenericValue result_value=
				engine->runFunction(
					function,
					llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

			U_TEST_ASSERT( ( uint32_t(value) << shift ) == static_cast<uint32_t>(result_value.IntVal.getLimitedValue()) );
		}
	}
}

U_TEST(LeftShiftTest1)
{
	// Shift unsigned value.
	static const char c_program_text[]=
	R"(
		fn Foo( u32 x, u8 y ) : u32
		{
			return x << y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foojh" );
	U_TEST_ASSERT( function != nullptr );

	static const uint32_t values[]=
	{
		std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max(),
		1, 2, 34, 5, 10, 12, 16, 256, 8461631, 161681818, 4000000000, 65536, 256, 854716, 41894198,
	};
	for( const uint32_t value : values )
	{
		llvm::GenericValue args[2];
		args[0].IntVal= llvm::APInt( 32, value );
		for( unsigned int shift= 0u; shift < 32u; shift++ )
		{
			args[1].IntVal= llvm::APInt( 8, shift );

			llvm::GenericValue result_value=
				engine->runFunction(
					function,
					llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

			U_TEST_ASSERT( ( value << shift ) == static_cast<uint32_t>(result_value.IntVal.getLimitedValue()) );
		}
	}
}

U_TEST(LeftShiftTest2)
{
	// Shift value is greater, than type bit width. Should properly handle such case.
	static const char c_program_text[]=
	R"(
		fn Foo( u32 x, u8 y ) : u32
		{
			return x << y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foojh" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, 357 );
	args[1].IntVal= llvm::APInt( 8, 35 );

	const llvm::GenericValue result_value= engine->runFunction( function, args );

	U_TEST_ASSERT( uint32_t(result_value.IntVal.getLimitedValue()) == (357 << (35&31)) );
}

U_TEST(LeftShiftTest3)
{
	// Constexpr shift value is greater, than type bit width. Should properly handle such case.
	static const char c_program_text[]=
	R"(
		static_assert( (357u << 35u) == (357u << (35u&31u)) );
	)";

	BuildProgram( c_program_text );
}

U_TEST(RightShiftTest0)
{
	// Shift signed value.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x, u32 y ) : i32
		{
			return x >>  y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Fooij" );
	U_TEST_ASSERT( function != nullptr );

	static const int32_t values[]=
	{
		std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), 0,
		1, -1, 2, -2, 3, 12, 17, 65536, 54284964, -2000000000, 2000000000, -58, 84, 1831516318,
	};
	for( const int32_t value : values )
	{
		llvm::GenericValue args[2];
		args[0].IntVal= llvm::APInt( 32, uint64_t(value) );
		for( unsigned int shift= 0u; shift < 32u; shift++ )
		{
			args[1].IntVal= llvm::APInt( 32, shift );

			llvm::GenericValue result_value=
				engine->runFunction(
					function,
					llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

			U_TEST_ASSERT( ( value >> shift ) == static_cast<int32_t>(result_value.IntVal.getLimitedValue()) );
		}
	}
}

U_TEST(RightShiftTest1)
{
	// Shift unsigned value.
	static const char c_program_text[]=
	R"(
		fn Foo( u32 x, u64 y ) : u32
		{
			return x >> y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foojy" );
	U_TEST_ASSERT( function != nullptr );

	static const uint32_t values[]=
	{
		std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max(),
		1, 2, 34, 5, 10, 12, 16, 256, 8461631, 161681818, 4000000000, 65536, 256, 854716, 41894198,
	};
	for( const uint32_t value : values )
	{
		llvm::GenericValue args[2];
		args[0].IntVal= llvm::APInt( 32, value );
		for( unsigned int shift= 0u; shift < 32u; shift++ )
		{
			args[1].IntVal= llvm::APInt( 64, shift );

			llvm::GenericValue result_value=
				engine->runFunction(
					function,
					llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

			U_TEST_ASSERT( ( value >> shift ) == static_cast<uint32_t>(result_value.IntVal.getLimitedValue()) );
		}
	}
}

U_TEST(RightShiftTest2)
{
	// Shift value is greater, than type bit width. Should properly handle such case.
	static const char c_program_text[]=
	R"(
		fn Foo( u32 x, u8 y ) : u32
		{
			return x >> y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foojh" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, 357 );
	args[1].IntVal= llvm::APInt( 8, 35 );

	const llvm::GenericValue result_value= engine->runFunction( function, args );

	U_TEST_ASSERT( uint32_t(result_value.IntVal.getLimitedValue()) == 357 >> (35&31) );
}

U_TEST(RightShiftTest3)
{
	// Constexpr shift value is greater, than type bit width. Should properly handle such case.
	static const char c_program_text[]=
	R"(
		static_assert( (357u >> 35u) == (357u >> (35u&31u)) );
	)";

	BuildProgram( c_program_text );
}


U_TEST(RightShiftAndAssignTest0)
{
	static const char c_program_text[]=
	R"(
		fn Foo( u32 mut x, u16 y ) : u32
		{
			x>>= y;
			return x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foojt" );
	U_TEST_ASSERT( function != nullptr );

	static const uint32_t values[]=
	{
		std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max(),
		1, 2, 34, 5, 10, 12, 16, 256, 8461631, 161681818, 4000000000, 65536, 256, 854716, 41894198,
	};
	for( const uint32_t value : values )
	{
		llvm::GenericValue args[2];
		args[0].IntVal= llvm::APInt( 32, value );
		for( unsigned int shift= 0u; shift < 32u; shift++ )
		{
			args[1].IntVal= llvm::APInt( 16, shift );

			llvm::GenericValue result_value=
				engine->runFunction(
					function,
					llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

			U_TEST_ASSERT( ( value >> shift ) == static_cast<uint32_t>(result_value.IntVal.getLimitedValue()) );
		}
	}
}

U_TEST(LeftShiftAndAssignTest0)
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 mut x, u32 y ) : i32
		{
			x<<= y;
			return x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Fooij" );
	U_TEST_ASSERT( function != nullptr );

	static const int32_t values[]=
	{
		std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), 0,
		1, -1, 2, -2, 3, 12, 17, 65536, 54284964, -2000000000, 2000000000, -58, 84, 1831516318,
	};
	for( const int32_t value : values )
	{
		llvm::GenericValue args[2];
		args[0].IntVal= llvm::APInt( 32, uint64_t(value) );
		for( uint32_t shift= 0u; shift < 32u; shift++ )
		{
			args[1].IntVal= llvm::APInt( 32, shift );

			llvm::GenericValue result_value=
				engine->runFunction(
					function,
					llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

			U_TEST_ASSERT( ( uint32_t(value) << shift ) == static_cast<uint32_t>(result_value.IntVal.getLimitedValue()) );
		}
	}
}

U_TEST(IncrementTest0)
{
	// Increment for signed value.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 mut x ) : i32
		{
			++x;
			return x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	static const int32_t values[]=
	{
		std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), 0,
		1, -1, 2, -2, 3, 12, 17, 65536, 54284964, -2000000000, 2000000000, -58, 84, 1831516318,
	};
	for( const int32_t value : values )
	{
		llvm::GenericValue arg;
		arg.IntVal= llvm::APInt( 32, uint64_t(value) );

		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( ( value + 1 ) == static_cast<int32_t>(result_value.IntVal.getLimitedValue()) );
	}
}

U_TEST(IncrementTest1)
{
	// Increment for unsigned value.
	static const char c_program_text[]=
	R"(
		fn Foo( u64 mut x ) : u64
		{
			++x;
			return x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Fooy" );
	U_TEST_ASSERT( function != nullptr );

	static const uint64_t values[]=
	{
		std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max(), 0,
		1u, 2u, 3u, 12u, 17u, 65536u, 54284964u, 2000000000, 58u, 84u, 1831516318u,
	};
	for( const uint64_t value : values )
	{
		llvm::GenericValue arg;
		arg.IntVal= llvm::APInt( 64, value );

		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( ( value + 1u ) == static_cast<uint64_t>(result_value.IntVal.getLimitedValue()) );
	}
}

U_TEST(DecrementTest0)
{
	// Decrement for signed value.
	static const char c_program_text[]=
	R"(
		fn Foo( i8 mut x ) : i8
		{
			--x;
			return x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Fooa" );
	U_TEST_ASSERT( function != nullptr );

	static const int8_t values[]=
	{
		std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), 0,
		1, -1, 2, -2, 3, 12, 17, -84, 84, -126, -127, 126, 127,
	};
	for( const int8_t value : values )
	{
		llvm::GenericValue arg;
		arg.IntVal= llvm::APInt( 8, uint64_t(value) );

		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( static_cast<int8_t>( value - 1 ) == static_cast<int8_t>(result_value.IntVal.getLimitedValue()) );
	}
}

U_TEST(DecrementTest1)
{
	// Decrement for unsigned value.
	static const char c_program_text[]=
	R"(
		fn Foo( u16 mut x ) : u16
		{
			--x;
			return x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foot" );
	U_TEST_ASSERT( function != nullptr );

	static const uint16_t values[]=
	{
		std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max(), 0u,
		1u, 2u, 3u, 12u, 17u, 65534u, 58u, 84u,
	};
	for( const uint16_t value : values )
	{
		llvm::GenericValue arg;
		arg.IntVal= llvm::APInt( 16, value );

		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( static_cast<uint16_t>( value - 1u ) == static_cast<uint16_t>(result_value.IntVal.getLimitedValue()) );
	}
}

U_TEST(UnaryMinusTest0)
{
	static const char c_program_text[]=
	"\
	fn Foo( i32 x ) : i32\
	{\
		return -x;\
	}"
	;

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	const int32_t arg_value= -874;
	llvm::GenericValue arg;
	arg.IntVal= llvm::APInt( 32, uint64_t(arg_value) );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>( arg ) );
	U_TEST_ASSERT( uint64_t( - arg_value ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(UnaryMinusTest1)
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	const int32_t arg_value= 54785;
	llvm::GenericValue arg;
	arg.IntVal= llvm::APInt( 32, uint64_t(arg_value) );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>( arg ) );
	U_TEST_ASSERT( uint64_t( - - arg_value ) == result_value.IntVal.getLimitedValue() );
}


U_TEST(LogicalNotTest)
{
	static const char c_program_text[]=
	R"(
		fn Foo( bool b ) : bool
		{
			return !b;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foob" );
	U_TEST_ASSERT( function != nullptr );

	for( unsigned int i= 0u; i < 2u; i++ )
	{
		llvm::GenericValue arg;
		arg.IntVal= llvm::APInt( 1, i );

		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );

		U_TEST_ASSERT( static_cast<uint64_t>(i^1u) == result_value.IntVal.getLimitedValue() );
	}
}

U_TEST(BitwiseNotTest)
{
	static const char c_program_text[]=
	R"(
		fn Foo( u32 x ) : u32
		{
			return ~x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooj" );
	U_TEST_ASSERT( function != nullptr );

	static const uint32_t values[]=
	{
		0u, std::numeric_limits<uint32_t>::max(),
		1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u,
		1024u, 55564u, 123u, 98567u, 46489641u, 3999999998u, 255u
	};
	for( const uint32_t value : values )
	{
		llvm::GenericValue arg;
		arg.IntVal= llvm::APInt( 32u, value );

		llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( arg ) );

		U_TEST_ASSERT( (~value) == static_cast<uint32_t>(result_value.IntVal.getLimitedValue()) );
	}
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Food" );
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

U_TEST(UnaryOperatorsOrderTest)
{
	static const char c_program_text[]=
	R"(
		fn NegNot( i32 x ) : i32
		{
			return -~x;
		}
		fn NotNeg( i32 x ) : i32
		{
			return ~-x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const neg_not= engine->FindFunctionNamed( "_Z6NegNoti" );
	llvm::Function* const not_neg= engine->FindFunctionNamed( "_Z6NotNegi" );
	U_TEST_ASSERT( neg_not != nullptr );
	U_TEST_ASSERT( not_neg != nullptr );

	static const int32_t values[]=
	{
		0, -1, 1, 5, -7, 65434, -5436463, 66535, 65536, -65535, -65536,
		std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(),
	};
	for( const int32_t value : values )
	{
		llvm::GenericValue arg;
		arg.IntVal= llvm::APInt( 32u, uint64_t(value) );

		const llvm::GenericValue result_value= engine->runFunction( neg_not, llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( int32_t(result_value.IntVal.getLimitedValue()) == (-~value) );
	}
	for( const int32_t value : values )
	{
		llvm::GenericValue arg;
		arg.IntVal= llvm::APInt( 32u, uint64_t(value) );

		const llvm::GenericValue result_value= engine->runFunction( not_neg, llvm::ArrayRef<llvm::GenericValue>( arg ) );
		U_TEST_ASSERT( int32_t(result_value.IntVal.getLimitedValue()) == (~-value) );
	}
}

U_TEST(LazyLogicalAndTest0)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn GetAndMutate( mut this, bool v, i32 i ) : bool
			{
				x= i;
				return v;
			}
		}
		fn Foo() : i32
		{
			var S mut s{ .x= 896 };
			if( true && s.GetAndMutate( true, 42 ) ) // Must call function and mutate
			{ return s.x; }
			return 0;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( ) );

	U_TEST_ASSERT( static_cast<uint64_t>(42) == result_value.IntVal.getLimitedValue() );
}

U_TEST(LazyLogicalAndTest1)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn GetAndMutate( mut this, bool v, i32 i ) : bool
			{
				x= i;
				return v;
			}
		}
		fn Foo() : i32
		{
			var S mut s{ .x= 896 };
			if( false && s.GetAndMutate( false, 42 ) ) // Must not call function
			{ return 0; }
			return s.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( ) );

	U_TEST_ASSERT( static_cast<uint64_t>(896) == result_value.IntVal.getLimitedValue() );
}

U_TEST(LazyLogicalOrTest0)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn GetAndMutate( mut this, bool v, i32 i ) : bool
			{
				x= i;
				return v;
			}
		}
		fn Foo() : i32
		{
			var S mut s{ .x= 896 };
			if( true || s.GetAndMutate( true, 42 ) ) // Must not call function
			{ return s.x; }
			return 0;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( ) );

	U_TEST_ASSERT( static_cast<uint64_t>(896) == result_value.IntVal.getLimitedValue() );
}

U_TEST(LazyLogicalOrTest1)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn GetAndMutate( mut this, bool v, i32 i ) : bool
			{
				x= i;
				return v;
			}
		}
		fn Foo() : i32
		{
			var S mut s{ .x= 896 };
			if( false || s.GetAndMutate( true, 42 ) ) // Must call function and mutate
			{ return s.x; }
			return 0;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( ) );

	U_TEST_ASSERT( static_cast<uint64_t>(42) == result_value.IntVal.getLimitedValue() );
}

U_TEST(LazyLogicalAndTest2)
{
	static const char c_program_text[]=
	R"(
		fn Foo( bool a, bool b, bool c ) : bool
		{
			return a && b && c;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foobbb" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue args[3];
	for( unsigned int i= 0u; i < 8u; i++ )
	{
		const bool a= ( i & 1u ) != 0u;
		const bool b= ( i & 2u ) != 0u;
		const bool c= ( i & 4u ) != 0u;
		args[0].IntVal= llvm::APInt( 1, a );
		args[1].IntVal= llvm::APInt( 1, b );
		args[2].IntVal= llvm::APInt( 1, c );

		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( args, 3u ) );

		U_TEST_ASSERT( ( a && b && c ) == (result_value.IntVal.getLimitedValue() != 0) );
	}
}

U_TEST(LazyLogicalOrTest2)
{
	static const char c_program_text[]=
	R"(
		fn Foo( bool a, bool b, bool c ) : bool
		{
			return a || b || c;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foobbb" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue args[3];
	for( unsigned int i= 0u; i < 8u; i++ )
	{
		const bool a= ( i & 1u ) != 0u;
		const bool b= ( i & 2u ) != 0u;
		const bool c= ( i & 4u ) != 0u;
		args[0].IntVal= llvm::APInt( 1, a );
		args[1].IntVal= llvm::APInt( 1, b );
		args[2].IntVal= llvm::APInt( 1, c );

		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( args, 3u ) );

		U_TEST_ASSERT( ( a || b || c ) == (result_value.IntVal.getLimitedValue() != 0) );
	}
}

U_TEST(lazyLogicalCombinedTest)
{
	static const char c_program_text[]=
	R"(
		fn Foo( u32 c ) : bool
		{
			return c <= 16u || ( c >= 20u && c <= 30u );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooj" );
	U_TEST_ASSERT( function != nullptr );

	unsigned int test_data[][2]
	{
			{  0, 1 },
			{ 16, 1 },
			{ 17, 0 },
			{ 20, 1 },
			{ 25, 1 },
			{ 30, 1 },
			{ 31, 0 },
			{ 40, 0 },
	};

	for( const auto& test_case : test_data )
	{
		llvm::GenericValue arg;
		llvm::GenericValue ret;
		arg.IntVal= llvm::APInt( 32, test_case[0] );
		ret.IntVal= llvm::APInt(  1, test_case[1] );

		const llvm::GenericValue result_value= engine->runFunction( function, {arg} );

		U_TEST_ASSERT( result_value.IntVal.getLimitedValue() == ret.IntVal.getLimitedValue() );
	}
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
		"_Z4Lessii", "_Z11LessOrEqualii", "_Z7Greaterii", "_Z14GreaterOrEqualii",
	};
	for( unsigned int func_n= 0u; func_n < 4u; func_n++ )
	{
		llvm::Function* function= engine->FindFunctionNamed( c_func_names[ func_n ] );
		U_TEST_ASSERT( function != nullptr );

		llvm::GenericValue args[2];
		llvm::GenericValue result_value;

		// TODO - add more test-cases.

		// Less
		args[0].IntVal= llvm::APInt( 32, uint64_t(-1488) );
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
		"_Z4Lessjj", "_Z11LessOrEqualjj", "_Z7Greaterjj", "_Z14GreaterOrEqualjj",
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
		"_Z4Lessff", "_Z11LessOrEqualff", "_Z7Greaterff", "_Z14GreaterOrEqualff",
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

U_TEST(OrderCompare_Test0)
{
	static const char c_program_text[]=
	R"(
		// Result of "<=>" is always "i32".
		// Use functions to avoid "constexpr" folding.
		fn CompareOrder(i32 a, i32 b) : i32
		{
			return a <=> b;
		}
		fn CompareOrder(u64 a, u64 b) : i32
		{
			return a <=> b;
		}
		fn CompareOrder(f32 a, f32 b) : i32
		{
			return a <=> b;
		}
		fn CompareOrder(char16 a, char16 b) : i32
		{
			return a <=> b;
		}
		fn CompareOrder($(i32) a, $(i32) b) : i32
		{
			return a <=> b;
		}

		fn Foo()
		{
			halt if( CompareOrder( 34, 764 ) != -1 );
			halt if( CompareOrder( 764, 34 ) != +1 );
			halt if( CompareOrder( -644, -2 ) != -1 );
			halt if( CompareOrder( -2, -644 ) != +1 );
			halt if( CompareOrder( -77, -77 ) != 0 );
			halt if( CompareOrder( 123, 123 ) != 0 );

			halt if( CompareOrder( 34u64, 764u64 ) != -1 );
			halt if( CompareOrder( 764u64, 34u64 ) != +1 );
			halt if( CompareOrder( 1u64, ~0u64 ) != -1 );
			halt if( CompareOrder( ~0u64, 1u64 ) != +1 );
			halt if( CompareOrder( ~0u64, ~0u64 ) != 0 );
			halt if( CompareOrder( 0u64, 0u64 ) != 0 );

			halt if( CompareOrder( 64.43f, 785.1f ) != -1 );
			halt if( CompareOrder( 785.1f, 64.43f ) != +1 );
			halt if( CompareOrder( -0.1f, +0.1f ) != -1 );
			halt if( CompareOrder( +0.1f, -0.1f ) != +1 );
			halt if( CompareOrder( 1.718281828f, 1.718281828f ) != 0 );
			halt if( CompareOrder( +0.0f, -0.0f ) != 0 );
			auto nan= 0.0f / 0.0f;
			halt if( CompareOrder( nan, +100.0f ) != 0 );
			halt if( CompareOrder( +100.0f, nan ) != 0 );
			halt if( CompareOrder( nan, -100.0f ) != 0 );
			halt if( CompareOrder( -100.0f, nan ) != 0 );
			halt if( CompareOrder( nan, 0.0f ) != 0 );
			halt if( CompareOrder( 0.0f, nan ) != 0 );
			halt if( CompareOrder( nan, nan ) != 0 );
			var f32 plus_inf = (+1.0f) / 0.0f, minus_inf= (-1.0f) / 0.0f;
			halt if( CompareOrder( plus_inf, 0.0f ) != +1 );
			halt if( CompareOrder( 0.0f, plus_inf ) != -1 );
			halt if( CompareOrder( minus_inf, 0.0f ) != -1 );
			halt if( CompareOrder( 0.0f, minus_inf ) != +1 );
			halt if( CompareOrder( plus_inf, minus_inf ) != +1 );
			halt if( CompareOrder( minus_inf, plus_inf ) != -1 );
			halt if( CompareOrder( plus_inf, nan ) != 0 );
			halt if( CompareOrder( nan, plus_inf ) != 0 );
			halt if( CompareOrder( minus_inf, nan ) != 0 );
			halt if( CompareOrder( nan, minus_inf ) != 0 );

			halt if( CompareOrder( "g"c16, "x"c16 ) != -1 );
			halt if( CompareOrder( "x"c16, "g"c16 ) != +1 );
			halt if( CompareOrder( "Я"c16, "Я"c16 ) != 0 );

			var [ i32, 3 ] mut arr= zero_init;
			halt if( CompareOrder( $<(arr[0]), $<(arr[2]) ) != -1 );
			halt if( CompareOrder( $<(arr[2]), $<(arr[0]) ) != +1 );
			halt if( CompareOrder( $<(arr[1]), $<(arr[1]) ) != 0 );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	const auto function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

U_TEST(OrderCompare_Test1)
{
	static const char c_program_text[]=
	R"(
		// Result of "<=>" is always "i32".
		template</type T/>
		fn CompareOrder(T a, T b) : i32
		{
			return a <=> b;
		}

		static_assert( CompareOrder( 34, 764 ) == -1 );
		static_assert( CompareOrder( 764, 34 ) == +1 );
		static_assert( CompareOrder( -644, -2 ) == -1 );
		static_assert( CompareOrder( -2, -644 ) == +1 );
		static_assert( CompareOrder( -77, -77 ) == 0 );
		static_assert( CompareOrder( 123, 123 ) == 0 );

		static_assert( CompareOrder( 34u64, 764u64 ) == -1 );
		static_assert( CompareOrder( 764u64, 34u64 ) == +1 );
		static_assert( CompareOrder( 1u64, ~0u64 ) == -1 );
		static_assert( CompareOrder( ~0u64, 1u64 ) == +1 );
		static_assert( CompareOrder( ~0u64, ~0u64 ) == 0 );
		static_assert( CompareOrder( 0u64, 0u64 ) == 0 );

		static_assert( CompareOrder( 64.43f, 785.1f ) == -1 );
		static_assert( CompareOrder( 785.1f, 64.43f ) == +1 );
		static_assert( CompareOrder( -0.1f, +0.1f ) == -1 );
		static_assert( CompareOrder( +0.1f, -0.1f ) == +1 );
		static_assert( CompareOrder( 1.718281828f, 1.718281828f ) == 0 );
		static_assert( CompareOrder( +0.0f, -0.0f ) == 0 );
		auto nan= 0.0f / 0.0f;
		static_assert( CompareOrder( nan, +100.0f ) == 0 );
		static_assert( CompareOrder( +100.0f, nan ) == 0 );
		static_assert( CompareOrder( nan, -100.0f ) == 0 );
		static_assert( CompareOrder( -100.0f, nan ) == 0 );
		static_assert( CompareOrder( nan, 0.0f ) == 0 );
		static_assert( CompareOrder( 0.0f, nan ) == 0 );
		static_assert( CompareOrder( nan, nan ) == 0 );
		var f32 plus_inf = (+1.0f) / 0.0f, minus_inf= (-1.0f) / 0.0f;
		static_assert( CompareOrder( plus_inf, 0.0f ) == +1 );
		static_assert( CompareOrder( 0.0f, plus_inf ) == -1 );
		static_assert( CompareOrder( minus_inf, 0.0f ) == -1 );
		static_assert( CompareOrder( 0.0f, minus_inf ) == +1 );
		static_assert( CompareOrder( plus_inf, minus_inf ) == +1 );
		static_assert( CompareOrder( minus_inf, plus_inf ) == -1 );
		static_assert( CompareOrder( plus_inf, nan ) == 0 );
		static_assert( CompareOrder( nan, plus_inf ) == 0 );
		static_assert( CompareOrder( minus_inf, nan ) == 0 );
		static_assert( CompareOrder( nan, minus_inf ) == 0 );

		static_assert( CompareOrder( "g"c16, "x"c16 ) == -1 );
		static_assert( CompareOrder( "x"c16, "g"c16 ) == +1 );
		static_assert( CompareOrder( "Я"c16, "Я"c16 ) == 0 );
	)";

	BuildProgram( c_program_text );
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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooiii" );
	U_TEST_ASSERT( function != nullptr );

	int32_t arg0= 77, arg1= 1488, arg2= 42;

	llvm::GenericValue args[3];
	args[0].IntVal= llvm::APInt( 32, uint64_t(arg0) );
	args[1].IntVal= llvm::APInt( 32, uint64_t(arg1) );
	args[2].IntVal= llvm::APInt( 32, uint64_t(arg2) );

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

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foofff" );
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

U_TEST( RemOperatorTest0 )
{
	// % for unsigned integers
	static const char c_program_text[]=
	R"(
		fn Foo( u32 x, u32 y ) : u32
		{
			return x % y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foojj" );
	U_TEST_ASSERT( function != nullptr );

	static const uint32_t values[]=
	{
		std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max(),
		1, 2, 34, 5, 10, 12, 16, 256, 8461631, 161681818, 4000000000, 65536, 256, 854716, 41894198,
	};

	for( const uint32_t x : values )
	for( const uint32_t y : values )
	{
		if( y == 0u )
			continue;
		llvm::GenericValue args[2];
		args[0].IntVal= llvm::APInt( 32, x );
		args[1].IntVal= llvm::APInt( 32, y );
		const llvm::GenericValue result=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( args, 2u ) );
		U_TEST_ASSERT( (x % y) == static_cast<uint32_t>(result.IntVal.getLimitedValue()) );
	}
}

U_TEST( RemOperatorTest1 )
{
	// %= for signed integers
	static const char c_program_text[]=
	R"(
		fn Foo( i32 mut x, i32 y ) : i32
		{
			x%= y;
			return x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooii" );
	U_TEST_ASSERT( function != nullptr );

	static const int32_t values[]=
	{
		std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), 0,
		1, -1, 2, -2, 3, 12, 17, 65536, 54284964, -2000000000, 2000000000, -58, 84, 1831516318,
	};

	for( const int32_t x : values )
	for( const int32_t y : values )
	{
		if( y == 0 )
			continue;
		if( x ==  std::numeric_limits<int32_t>::min() && y == -1 ) // Case of overflow is undefined behaviour
			continue;

		llvm::GenericValue args[2];
		args[0].IntVal= llvm::APInt( 32, uint64_t(x) );
		args[1].IntVal= llvm::APInt( 32, uint64_t(y) );
		const llvm::GenericValue result=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( args, 2u ) );
		U_TEST_ASSERT( (x % y) == static_cast<int32_t>(result.IntVal.getLimitedValue()) );
	}
}

U_TEST( RemOperatorTest2 )
{
	// % for floats
	static const char c_program_text[]=
	R"(
		fn Foo( f32 x, f32 y ) : f32
		{
			return x % y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooff" );
	U_TEST_ASSERT( function != nullptr );

	static const float values[]=
	{
		+std::numeric_limits<float>::min(), +std::numeric_limits<float>::max(),
		-std::numeric_limits<float>::min(), -std::numeric_limits<float>::max(),
		0.0f, +1.0f, -1.0f,
		+0.1f, +0.2f, +0.4f, +0.5f, +0.7f, +0.99f, +584.4f, +8464984.0f, +1e24f, +3.1415926535f,
		-0.1f, -0.2f, -0.4f, -0.5f, -0.7f, -0.99f, -584.4f, -8464984.0f, -1e24f, -3.1415926535f,
	};

	for( const float x : values )
	for( const float y : values )
	{
		llvm::GenericValue args[2];
		args[0].FloatVal= x;
		args[1].FloatVal= y;
		const llvm::GenericValue result=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( args, 2u ) );
		const float cpp_rem= std::fmod(x, y);
		if( cpp_rem != cpp_rem  /* cpp_rem == NaN */ )
		{
			U_TEST_ASSERT( result.FloatVal != result.FloatVal );
		}
		else
		{
			const float eps= std::abs( cpp_rem ) / ( 1024.0f * 1024.0f );
			ASSERT_NEAR( cpp_rem, result.FloatVal, eps );
		}
	}
}

} // namespace

} // namespace U
