#include "cpp_tests.hpp"

namespace U
{

namespace
{

template<class T>
void FillLLVMVal( llvm::GenericValue& llvm_val, const T& val )
{
	static_assert( std::is_integral<T>::value, "expected integer" );
	llvm_val.IntVal= llvm::APInt( sizeof(T) * 8, uint64_t(val) );
}

template<>
void FillLLVMVal<float>( llvm::GenericValue& llvm_val, const float& val )
{
	llvm_val.FloatVal= val;
}

template<>
void FillLLVMVal<double>( llvm::GenericValue& llvm_val, const double& val )
{
	llvm_val.DoubleVal= val;
}

template<class T>
bool IsEqual( const llvm::GenericValue& l, const llvm::GenericValue& r )
{
	static_assert( std::is_integral<T>::value, "expected integer" );
	return l.IntVal.getLimitedValue() == r.IntVal.getLimitedValue();
}

template<>
bool IsEqual<float>( const llvm::GenericValue& l, const llvm::GenericValue& r )
{
	return l.FloatVal == r.FloatVal;
}

template<>
bool IsEqual<double>( const llvm::GenericValue& l, const llvm::GenericValue& r )
{
	return l.DoubleVal == r.DoubleVal;
}

template<class T, class Func>
static void DoTest(
	const char* const program_text,
	const Func& func,
	const std::vector< std::vector<T> >& args_set )
{
	const EnginePtr engine= CreateEngine( BuildProgram( program_text ) );

	std::string func_name= "_Z3Foo";
	if( args_set[0].empty() )
		func_name+= "v";
	else
	{
		for( size_t i= 0u; i < args_set[0].size(); i++ )
		{
			if( std::is_same<T, int>::value )
				func_name+= "i";
			if( std::is_same<T, unsigned int>::value )
				func_name+= "j";
			if( std::is_same<T, float>::value )
				func_name+= "f";
			if( std::is_same<T, double>::value )
				func_name+= "d";
		}
	}

	llvm::Function* function= engine->FindFunctionNamed( func_name );
	U_TEST_ASSERT( function != nullptr );

	for( const std::vector<T>& args : args_set )
	{
		std::vector<llvm::GenericValue> llvm_args;
		llvm_args.resize( args.size() );
		for( size_t i= 0u; i < args.size(); i++ )
			FillLLVMVal( llvm_args[i], args[i] );

		const llvm::GenericValue result_value=
			engine->runFunction(
				function,
				llvm_args );

		const T expected_func_result= func(args);
		llvm::GenericValue llvm_expected_func_result;
		FillLLVMVal( llvm_expected_func_result, expected_func_result );

		U_TEST_ASSERT( IsEqual<T>( result_value, llvm_expected_func_result ) );
	}
}

U_TEST( OperatorsPriorityTest0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 a, i32 b, i32 c ) : i32
		{
			return a * b + c;
		}
	)";

	const auto expected=
	[]( const std::vector<int>& args ) -> int
	{
		U_TEST_ASSERT( args.size() == 3u );
		return args[0] * args[1] + args[2];
	};

	const std::vector< std::vector<int> > args
	{
		{ 4, 17, 54 },
		{ -85, 745, 324 },
		{ 7416468, -88, -88 },
		{ 0, 58, -41 },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 a, i32 b, i32 c ) : i32
		{
			return a + b * c;
		}
	)";

	const auto expected=
	[]( const std::vector<int>& args ) -> int
	{
		U_TEST_ASSERT( args.size() == 3u );
		return args[0] + args[1] * args[2];
	};

	const std::vector< std::vector<int> > args
	{
		{ 4, 17, 54 },
		{ -85, 745, 324 },
		{ 7416468, -88, -88 },
		{ 0, 58, -41 },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest2 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 a, i32 b, i32 c ) : i32
		{
			return a / b / c;
		}
	)";

	const auto expected=
	[]( const std::vector<int>& args ) -> int
	{
		U_TEST_ASSERT( args.size() == 3u );
		return args[0] / args[1] / args[2];
	};

	const std::vector< std::vector<int> > args
	{
		{ 1518548, 5, -8 },
		{ 58, 3, 2 },
		{ 85, 5, 3 },
		{ -85, 11, 1 },
		{ 0, 58, 865 },
		{ -897, 7, -3 },
		{ 875875, 17, 5 },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest3 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( f32 a, f32 b, f32 c, f32 d ) : f32
		{
			return a / b / c * d;
		}
	)";

	const auto expected=
	[]( const std::vector<float>& args ) -> float
	{
		U_TEST_ASSERT( args.size() == 4u );
		return args[0] / args[1] / args[2] * args[3];
	};

	const std::vector< std::vector<float> > args
	{
		{ 5.1f, -85.5f, 4684.4f, 14.0f },
		{ 475.165f, -85.5f, 944.4f, 14848.1f },
		{ 105.1f, -85.85f, 1684.4f, 1.0f },
		{ -5.58f, 185.41f, 74.1f, -14.0f },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest4 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 a, i32 b ) : i32
		{
			return a / b * b;
		}
	)";

	const auto expected=
	[]( const std::vector<int>& args ) -> int
	{
		U_TEST_ASSERT( args.size() == 2u );
		return args[0] / args[1] * args[1];
	};

	const std::vector< std::vector<int> > args
	{
		{ 1518548, 5 },
		{ -745, -17 },
		{ 78458, 24 },
		{ 1651818, 15 },
		{ 15151, 42 },
		{ 14, 1 },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest5 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 a, i32 b ) : i32
		{
			return a * b / b;
		}
	)";

	const auto expected=
	[]( const std::vector<int>& args ) -> int
	{
		U_TEST_ASSERT( args.size() == 2u );
		return args[0] * args[1] / args[1];
	};

	const std::vector< std::vector<int> > args
	{
		{ 1518548, 5 },
		{ -745, -17 },
		{ 78458, 24 },
		{ 1651818, 15 },
		{ 15151, 42 },
		{ 14, 1 },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest6 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 a, i32 b, i32 c ) : i32
		{
			if( a >= b + c ) { return 1; }
			return 0;
		}
	)";

	const auto expected=
	[]( const std::vector<int>& args ) -> int
	{
		U_TEST_ASSERT( args.size() == 3u );
		if( args[0] >= args[1] + args[2] ) return 1;
		return 0;
	};

	const std::vector< std::vector<int> > args
	{
		{ 5, 0, 1 },
		{ 854, -17, -19 },
		{ 0, 58, 48 },
		{ -17, 58, 45 },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest7 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 a, i32 b, i32 c ) : i32
		{
			if( 0 >= a + b + c ) { return 1; }
			return 0;
		}
	)";

	const auto expected=
	[]( const std::vector<int>& args ) -> int
	{
		U_TEST_ASSERT( args.size() == 3u );
		if( 0 >= args[0] + args[1] + args[2] ) return 1;
		return 0;
	};

	const std::vector< std::vector<int> > args
	{
		{ -5, 0, 1 },
		{ -854, 17, 19 },
		{ 0, 58, 48 },
		{ -17, 58, 45 },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest8 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( f64 a, f64 b, f64 c ) : f64
		{
			if( 0.0 >= a * b + c ) { return 1.0; }
			return 0.0;
		}
	)";

	const auto expected=
	[]( const std::vector<double>& args ) -> double
	{
		U_TEST_ASSERT( args.size() == 3u );
		if( 0.0 >= args[0] * args[1] + args[2] ) return 1.0;
		return 0.0;
	};

	const std::vector< std::vector<double> > args
	{
		{ 14.0, -1.0, 0.0 },
		{ 1.5, 27.0, -26984.0 },
		{ 0.0, 156189.0, -58.0 },
		{ -14.0, -.0, 0.0 },
		{ -1.5, -27.0, 26984.0 },
		{ 0.0, -156189.0, 58.0 },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest9 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( f32 a, f32 b, f32 c ) : f32
		{
			if( 0.0f >= a + b * c ) { return 1.0f; }
			return 0.0f;
		}
	)";

	const auto expected=
	[]( const std::vector<float>& args ) -> float
	{
		U_TEST_ASSERT( args.size() == 3u );
		if( 0.0f >= args[0] + args[1] * args[2] ) return 1.0f;
		return 0.0f;
	};

	const std::vector< std::vector<float> > args
	{
		{ -14.0f, 8455881351.0f, -1.0f },
		{ -1e24f, 0.5f, -1.0f },

		{ 14.0f, -8455881351.0f, 1.0f },
		{ 1e24f, -0.5f, 1.0f },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest10 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( f32 a, f32 b, f32 c ) : f32
		{
			if( a > 0.0f & a > b + c ) { return 1.0f; }
			return 0.0f;
		}
	)";

	const auto expected=
	[]( const std::vector<float>& args ) -> float
	{
		U_TEST_ASSERT( args.size() == 3u );
		if( ( args[0] > 0.0f ) & ( args[0] > args[1] + args[2] ) ) return 1.0f;
		return 0.0f;
	};

	const std::vector< std::vector<float> > args
	{
		{ 1.0f, 0.1f, -0.1f },
		{ 1.0f, 584.0f, 0.5f },
		{ 1.0f, 584.0f, -584.0f },
		{ -0.5f, 85648.0f, 416518.8f },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest11 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( f32 a, f32 b, f32 c ) : f32
		{
			if( a > 0.0f & a * b > c ) { return 1.0f; }
			return 0.0f;
		}
	)";

	const auto expected=
	[]( const std::vector<float>& args ) -> float
	{
		U_TEST_ASSERT( args.size() == 3u );
		if( ( args[0] > 0.0f ) & ( args[0] * args[1] > args[2] ) ) return 1.0f;
		return 0.0f;
	};

	const std::vector< std::vector<float> > args
	{
		{ 1.0f, 0.1f, -0.1f },
		{ 1.0f, 0.1f, 584.0f },
		{ 5.0f, 4.0f, 19.9f },
		{ 5.0f, 4.0f, 20.1f },
		{ -0.5f, 85648.0f, 416518.8f },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest12 )
{
	static const char c_program_text[]=
	R"(
		// Execute addition first, than compare.
		fn Foo( i32 a, i32 b, i32 c ) : i32
		{
			return a <=> b + c;
		}
	)";

	const auto expected=
	[]( const std::vector<int>& args ) -> int
	{
		U_TEST_ASSERT( args.size() == 3u );

		const int sum= args[1] + args[2];
		if( args[0] < sum ) return -1;
		if( args[0] > sum ) return +1;
		return 0;
	};

	const std::vector< std::vector<int> > args
	{
		{ 7, 33, 11 },
		{ 999, 2, 3 },
		{ 10, 7, 3 },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest13 )
{
	static const char c_program_text[]=
	R"(
		// Execute "<=>" before ">"
		fn Foo( i32 a, i32 b, i32 c, i32 d, i32 e ) : i32
		{
			if ( a <=>b > c ){ return d; }
			else { return e; }
		}
	)";

	const auto expected=
	[]( const std::vector<int>& args ) -> int
	{
		U_TEST_ASSERT( args.size() == 5u );

		int cmp= 0;
		if( args[0] < args[1] ) cmp= -1;
		if( args[0] > args[1] ) cmp= +1;

		return cmp > args[2] ? args[3] : args[4];
	};

	const std::vector< std::vector<int> > args
	{
		{ 5, 10,  0, 44, 77 },
		{ 5, 10, -1, 44, 77 },
		{ 5, 10, +1, 44, 77 },
		{ 5, 10, -2, 44, 77 },
		{ 5, 10, +2, 44, 77 },
		{ 10, 5,  0, 44, 77 },
		{ 10, 5, -1, 44, 77 },
		{ 10, 5, +1, 44, 77 },
		{ 10, 5, -2, 44, 77 },
		{ 10, 5, +2, 44, 77 },
		{ 7, 7,  0, 44, 77 },
		{ 7, 7, -1, 44, 77 },
		{ 7, 7, +1, 44, 77 },
		{ 7, 7, -2, 44, 77 },
		{ 7, 7, +2, 44, 77 },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest14 )
{
	static const char c_program_text[]=
	R"(
		// Execute "<=>" before "<"
		fn Foo( i32 a, i32 b, i32 c, i32 d, i32 e ) : i32
		{
			if ( c < a <=> b ){ return d; }
			else { return e; }
		}
	)";

	const auto expected=
	[]( const std::vector<int>& args ) -> int
	{
		U_TEST_ASSERT( args.size() == 5u );

		int cmp= 0;
		if( args[0] < args[1] ) cmp= -1;
		if( args[0] > args[1] ) cmp= +1;

		return args[2] < cmp ? args[3] : args[4];
	};

	const std::vector< std::vector<int> > args
	{
		{ 5, 10,  0, 44, 77 },
		{ 5, 10, -1, 44, 77 },
		{ 5, 10, +1, 44, 77 },
		{ 5, 10, -2, 44, 77 },
		{ 5, 10, +2, 44, 77 },
		{ 10, 5,  0, 44, 77 },
		{ 10, 5, -1, 44, 77 },
		{ 10, 5, +1, 44, 77 },
		{ 10, 5, -2, 44, 77 },
		{ 10, 5, +2, 44, 77 },
		{ 7, 7,  0, 44, 77 },
		{ 7, 7, -1, 44, 77 },
		{ 7, 7, +1, 44, 77 },
		{ 7, 7, -2, 44, 77 },
		{ 7, 7, +2, 44, 77 },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest15 )
{
	// Bitwise operators priority must be same, as in C++.

	static const char c_program_text[]= " fn Foo( i32 a, i32 b, i32 c, i32 d ) : i32 { return a & b ^ c | d; } ";

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
#endif

	const auto expected=
	[]( const std::vector<int>& args ) -> int
	{
		U_TEST_ASSERT( args.size() == 4u );

		return args[0] & args[1] ^ args[2] | args[3];
	};

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

	const std::vector< std::vector<int> > args
	{
		{ 00, 00, 00, 00 },
		{ 00, 00, 00, 11 },
		{ 00, 00, 11, 00 },
		{ 00, 00, 11, 11 },
		{ 00, 11, 00, 00 },
		{ 00, 11, 00, 11 },
		{ 00, 11, 11, 00 },
		{ 00, 11, 11, 11 },
		{ 11, 00, 00, 00 },
		{ 11, 00, 00, 11 },
		{ 11, 00, 11, 00 },
		{ 11, 00, 11, 11 },
		{ 11, 11, 00, 00 },
		{ 11, 11, 00, 11 },
		{ 11, 11, 11, 00 },
		{ 11, 11, 11, 11 },
	};

	DoTest( c_program_text, expected, args );
}

U_TEST( OperatorsPriorityTest16 )
{
	// Bitwise operators priority must be same, as in C++.

	static const char c_program_text[]= " fn Foo( i32 a, i32 b, i32 c, i32 d ) : i32 { return a | b ^ c & d; } ";

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
#endif

	const auto expected=
	[]( const std::vector<int>& args ) -> int
	{
		U_TEST_ASSERT( args.size() == 4u );

		return args[0] | args[1] ^ args[2] & args[3];
	};

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

	const std::vector< std::vector<int> > args
	{
		{ 00, 00, 00, 00 },
		{ 00, 00, 00, 11 },
		{ 00, 00, 11, 00 },
		{ 00, 00, 11, 11 },
		{ 00, 11, 00, 00 },
		{ 00, 11, 00, 11 },
		{ 00, 11, 11, 00 },
		{ 00, 11, 11, 11 },
		{ 11, 00, 00, 00 },
		{ 11, 00, 00, 11 },
		{ 11, 00, 11, 00 },
		{ 11, 00, 11, 11 },
		{ 11, 11, 00, 00 },
		{ 11, 11, 00, 11 },
		{ 11, 11, 11, 00 },
		{ 11, 11, 11, 11 },
	};

	DoTest( c_program_text, expected, args );
}

} // namespace

} // namespace U
