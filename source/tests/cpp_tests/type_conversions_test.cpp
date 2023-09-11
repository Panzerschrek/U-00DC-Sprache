#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( TypeConversionTest0 )
{
	// i8 to i16, sign extension
	static const char c_program_text[]=
	R"(
		fn Foo( i8 x ) : i16
		{
			return i16(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooa" );
	U_TEST_ASSERT( function != nullptr );

	for( int i= std::numeric_limits<int8_t>::min(); i <= std::numeric_limits<int8_t>::max(); i++ )
	{
		llvm::GenericValue arg; arg.IntVal= llvm::APInt( 8, uint64_t(i) );
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( i == static_cast<int16_t>(result_value.IntVal.getLimitedValue()) );
	}
}

U_TEST( TypeConversionTest1 )
{
	// i8 to i32, sign extension
	static const char c_program_text[]=
	R"(
		fn Foo( i8 x ) : i32
		{
			return i32(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooa" );
	U_TEST_ASSERT( function != nullptr );

	for( int i= std::numeric_limits<int8_t>::min(); i <= std::numeric_limits<int8_t>::max(); i++ )
	{
		llvm::GenericValue arg; arg.IntVal= llvm::APInt( 8, uint64_t(i) );
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( i == int(result_value.IntVal.getLimitedValue()) );
	}
}

U_TEST( TypeConversionTest2 )
{
	// u8 to u32, zero extension
	static const char c_program_text[]=
	R"(
		fn Foo( u8 x ) : u32
		{
			return u32(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooh" );
	U_TEST_ASSERT( function != nullptr );

	for( unsigned int i= std::numeric_limits<uint8_t>::min(); i <= std::numeric_limits<uint8_t>::max(); i++ )
	{
		llvm::GenericValue arg; arg.IntVal= llvm::APInt( 8, i );
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( i == result_value.IntVal.getLimitedValue() );
	}
}

U_TEST( TypeConversionTest3 )
{
	// i8 to u32, zero extension
	static const char c_program_text[]=
	R"(
		fn Foo( i8 x ) : u32
		{
			return u32(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooa" );
	U_TEST_ASSERT( function != nullptr );

	for( int i= std::numeric_limits<int8_t>::min(); i <= std::numeric_limits<int8_t>::max(); i++ )
	{
		llvm::GenericValue arg; arg.IntVal= llvm::APInt( 8, uint64_t(i) );
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( static_cast<uint8_t>(i) == static_cast<uint32_t>( result_value.IntVal.getLimitedValue()) );
	}
}

U_TEST( TypeConversionTest4 )
{
	// u32 to u8, truncation
	static const char c_program_text[]=
	R"(
		fn Foo( u32 x ) : u8
		{
			return u8(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooj" );
	U_TEST_ASSERT( function != nullptr );

	static uint32_t c_values[]=
	{
		0u, 55u, 254u, 255u,
		256u, 256u * 2u, 256u * 3u, 256u * 10u, 256u * 17u + 5u,
		65536u, 65536u * 2u, 65536u * 5u, 65536u * 85u + 11u, 65536u + 44u, 65536u * 10u,
		std::numeric_limits<uint32_t>::max()
	};
	for( const uint32_t value : c_values )
	{
		llvm::GenericValue arg; arg.IntVal= llvm::APInt( 32, value );
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( static_cast<uint8_t>(value & 255u) == static_cast<uint8_t>(result_value.IntVal.getLimitedValue()) );
	}
}

U_TEST( TypeConversionTest5 )
{
	// i32 to i8, truncation
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x ) : i8
		{
			return i8(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	static int32_t c_values[]=
	{
		0,
		std::numeric_limits<int32_t>::min(),
		std::numeric_limits<int32_t>::max(),
		-1, -2, -56, -254, -255, -256, -257,
		+1, +2, +56, +254, +255, +256, +257,
		+65536, +65536 - 1, +65536 + 1, +65536 + 2, +65536 + 3, +65536 * 17, +65536 * 854, +65536 * 12584, +65536 * 12584 + 584,
		-65536, -65536 + 1, -65536 - 1, -65536 - 2, -65536 - 3, -65536 * 17, -65536 * 854, -65536 * 12584, -65536 * 12584 - 584,
	};
	for( const int32_t value : c_values )
	{
		llvm::GenericValue arg; arg.IntVal= llvm::APInt( 32, uint64_t(value) );
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( static_cast<int8_t>(value & 255) == static_cast<int8_t>(result_value.IntVal.getLimitedValue()) );
	}
}

U_TEST( TypeConversionTest6 )
{
	// i32 to u8, truncation
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x ) : u8
		{
			return u8(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	static int32_t c_values[]=
	{
		0,
		std::numeric_limits<int32_t>::min(),
		std::numeric_limits<int32_t>::max(),
		-1, -2, -56, -254, -255, -256, -257,
		+1, +2, +56, +254, +255, +256, +257,
		+65536, +65536 - 1, +65536 + 1, +65536 + 2, +65536 + 3, +65536 * 17, +65536 * 854, +65536 * 12584, +65536 * 12584 + 584,
		-65536, -65536 + 1, -65536 - 1, -65536 - 2, -65536 - 3, -65536 * 17, -65536 * 854, -65536 * 12584, -65536 * 12584 - 584,
	};
	for( const int32_t value : c_values )
	{
		llvm::GenericValue arg; arg.IntVal= llvm::APInt( 32, uint64_t(value) );
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( static_cast<uint8_t>(value & 255) == static_cast<uint8_t>(result_value.IntVal.getLimitedValue()) );
	}
}

U_TEST( TypeConversionTest7 )
{
	// u32 to i8, truncation
	static const char c_program_text[]=
	R"(
		fn Foo( u32 x ) : i8
		{
			return i8(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooj" );
	U_TEST_ASSERT( function != nullptr );

	static uint32_t c_values[]=
	{
		0u, 55u, 254u, 255u,
		256u, 256u * 2u, 256u * 3u, 256u * 10u, 256u * 17u + 5u,
		65536u, 65536u * 2u, 65536u * 5u, 65536u * 85u + 11u, 65536u + 44u, 65536u * 10u,
		std::numeric_limits<uint32_t>::max()
	};
	for( const uint32_t value : c_values )
	{
		llvm::GenericValue arg; arg.IntVal= llvm::APInt( 32, value );
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( static_cast<int8_t>(value & 255u) == static_cast<int8_t>(result_value.IntVal.getLimitedValue()) );
	}
}

U_TEST( TypeConversionTest8 )
{
	// f32 to f64 extension
	static const char c_program_text[]=
	R"(
		fn Foo( f32 x ) : f64
		{
			return f64(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foof" );
	U_TEST_ASSERT( function != nullptr );

	static float c_values[]=
	{
		+0.0f, -0.0f, +1.0f, -1.0f,
		std::numeric_limits<float>::quiet_NaN(),
		std::numeric_limits<float>::min(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::denorm_min(),
		std::numeric_limits<float>::max(), +std::numeric_limits<float>::infinity(), +std::numeric_limits<float>::denorm_min(),
		+0.1f, +0.2f, +0.3f, +0.4f, +0.5f, +0.6f, +0.7f, +0.8f, +0.9f, 1.05784648e15f,
		-0.1f, -0.2f, -0.3f, -0.4f, -0.5f, -0.6f, -0.7f, -0.8f, -0.9f, 1.05784648e15f,
		+3.1415926535f, -3.1415926535f,
	};
	for( const float value : c_values )
	{
		llvm::GenericValue arg; arg.FloatVal= value;
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		const double value_casted= static_cast<double>(value);
		U_TEST_ASSERT( std::memcmp( &value_casted, &result_value.DoubleVal, sizeof(double) ) == 0 );
	}
}

U_TEST( TypeConversionTest9 )
{
	// f64 to f32 truncation
	static const char c_program_text[]=
	R"(
		fn Foo( f64 x ) : f32
		{
			return f32(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Food" );
	U_TEST_ASSERT( function != nullptr );

	static double c_values[]=
	{
		+0.0, -0.0, +1.0, -1.0,
		std::numeric_limits<double>::quiet_NaN(),
		std::numeric_limits<double>::min(), -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::denorm_min(),
		std::numeric_limits<double>::max(), +std::numeric_limits<double>::infinity(), +std::numeric_limits<double>::denorm_min(),
		+0.1, +0.2, +0.3, +0.4, +0.5, +0.6, +0.7, +0.8, +0.9, 1.05784648e15,
		-0.1, -0.2, -0.3, -0.4, -0.5, -0.6, -0.7, -0.8, -0.9, 1.05784648e15,
		+1e256, -1e256,
		+3.1415926535, -3.1415926535,
	};
	for( const double value : c_values )
	{
		llvm::GenericValue arg; arg.DoubleVal= value;
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		const float value_casted= static_cast<float>(value);
		U_TEST_ASSERT( std::memcmp( &value_casted, &result_value.FloatVal, sizeof(float) ) == 0 );
	}
}

U_TEST( TypeConversionTest10 )
{
	// i32 to float
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x ) : f32
		{
			return f32(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	static int32_t c_values[]=
	{
		std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(),
		0, 1, 10, -5, -84, 65536, 256854741, -258256847, 8574125, 105239845, -85457868, 51398457, -8388608,
	};
	for( const int32_t value : c_values )
	{
		llvm::GenericValue arg; arg.IntVal= llvm::APInt( 32, uint64_t(value) );
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		const float value_casted= static_cast<float>(value);
		U_TEST_ASSERT( std::memcmp( &value_casted, &result_value.FloatVal, sizeof(float) ) == 0 );
	}
}

U_TEST( TypeConversionTest11 )
{
	// u32 to float
	static const char c_program_text[]=
	R"(
		fn Foo( u32 x ) : f32
		{
			return f32(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooj" );
	U_TEST_ASSERT( function != nullptr );

	static uint32_t c_values[]=
	{
		std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max(),
		0u, 1u, 10u, 5u, 84u, 65536u, 256854741u, 258256847u, 8574125u, 105239845u, 85457868u, 51398457u, 8388608u,
	};
	for( const uint32_t value : c_values )
	{
		llvm::GenericValue arg; arg.IntVal= llvm::APInt( 32, value );
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		const float value_casted= static_cast<float>(value);
		U_TEST_ASSERT( std::memcmp( &value_casted, &result_value.FloatVal, sizeof(float) ) == 0 );
	}
}

U_TEST( TypeConversionTest12 )
{
	// float to i32
	static const char c_program_text[]=
	R"(
		fn Foo( f32 x ) : i32
		{
			return i32(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foof" );
	U_TEST_ASSERT( function != nullptr );

	static float c_values[]=
	{
		float(std::numeric_limits<int32_t>::min()),
		float(std::numeric_limits<int32_t>::max()),
		-0.0f, +0.0f, +1.0f, -1.0f,
		+0.1f, +0.2f, +0.4f, +0.5f, +0.8f, +0.99f, +1.1f, +1.5f, +1.9f, +2.0f, +2.1f,
		-0.1f, -0.2f, -0.4f, -0.5f, -0.8f, -0.99f, -1.1f, -1.5f, -1.9f, -2.0f, -2.1f,
		528.0f, 847.3f, -854745.1f, 1694463188.1f, /*1.0e24f,*//*TODO - enable test for large number*/ 1.0e-24f
	};
	for( const float value : c_values )
	{
		llvm::GenericValue arg; arg.FloatVal= value;
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( static_cast<int32_t>(value) == static_cast<int32_t>(result_value.IntVal.getLimitedValue()) );
	}
}

U_TEST( TypeConversionTest13 )
{
	// float to u32
	static const char c_program_text[]=
	R"(
		fn Foo( f32 x ) : u32
		{
			return u32(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foof" );
	U_TEST_ASSERT( function != nullptr );

	static float c_values[]=
	{
		float(std::numeric_limits<uint32_t>::min()),
		float(std::numeric_limits<uint32_t>::max()),
		-0.0f, +0.0f, +1.0f,
		+0.1f, +0.2f, +0.4f, +0.5f, +0.8f, +0.99f, +1.1f, +1.5f, +1.9f, +2.0f, +2.1f,
		528.0f, 847.3f, 854745.1f, 1694463188.1f, /*1.0e24f,*//*TODO - enable test for large number*/ 1.0e-24f
	};
	for( const float value : c_values )
	{
		llvm::GenericValue arg; arg.FloatVal= value;
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( static_cast<uint32_t>(value) == static_cast<uint32_t>(result_value.IntVal.getLimitedValue()) );
	}
}

U_TEST( TypeConversionTest14 )
{
	// i32 to u32
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x ) : u32
		{
			return u32(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooi" );
	U_TEST_ASSERT( function != nullptr );

	static int32_t c_values[]=
	{
		std::numeric_limits<int32_t>::min(),
		std::numeric_limits<int32_t>::max(),
		0, -1, 1, 2, -2,
		+256, +256 - 1, +256 + 1, +65536 - 1, +65536, +65536 + 1, +461613186, +1841269571,
		-256, -256 + 1, -256 - 1, -65536 + 1, -65536, -65536 - 1, -461613186, -1841269571,
	};
	for( const int32_t value : c_values )
	{
		llvm::GenericValue arg; arg.IntVal= llvm::APInt( 32, uint64_t(value) );
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( static_cast<uint32_t>(value) == static_cast<uint32_t>(result_value.IntVal.getLimitedValue()) );
	}
}

U_TEST( TypeConversionTest15 )
{
	// u32 to i32
	static const char c_program_text[]=
	R"(
		fn Foo( u32 x ) : i32
		{
			return i32(x);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* function= engine->FindFunctionNamed( "_Z3Fooj" );
	U_TEST_ASSERT( function != nullptr );

	static uint32_t c_values[]=
	{
		std::numeric_limits<uint32_t>::min(),
		std::numeric_limits<uint32_t>::max(),
		1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 5684u, 16416156u, 813851318u,
		(2u<<31u) + 584u, (2u<<31u) + 854684u, 2u << 30u, 2u << 31u, (2u<<31u) - 1u,
	};
	for( const uint32_t value : c_values )
	{
		llvm::GenericValue arg; arg.IntVal= llvm::APInt( 32, value );
		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( &arg, 1u ) );

		U_TEST_ASSERT( static_cast<int32_t>(value) == static_cast<int32_t>(result_value.IntVal.getLimitedValue()) );
	}
}

} // namespace

} // namespace U
