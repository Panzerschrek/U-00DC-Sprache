#include "cpp_tests.hpp"

namespace U
{

namespace
{

enum class CallResult
{
	LifetimeStart,
	LifetimeEnd,
	CaptureValue,
};

struct LifetimeCallResult
{
	uint64_t address;
	CallResult call_result;
	std::vector<uint8_t> captured_data;
};

std::vector<LifetimeCallResult> g_lifetimes_call_sequence;
ExecutionEngine* g_current_execution_engine= nullptr;

llvm::GenericValue LifetimeStartCalled( llvm::FunctionType* , const llvm::ArrayRef<llvm::GenericValue> args )
{
	LifetimeCallResult res{};
	res.address= args[0].IntVal.getLimitedValue();
	res.call_result= CallResult::LifetimeStart;
	g_lifetimes_call_sequence.push_back(res);

	return llvm::GenericValue();
}

llvm::GenericValue LifetimeEndCalled( llvm::FunctionType* , const llvm::ArrayRef<llvm::GenericValue> args )
{
	LifetimeCallResult res{};
	res.address= args[0].IntVal.getLimitedValue();
	res.call_result= CallResult::LifetimeEnd;
	g_lifetimes_call_sequence.push_back(res);

	return llvm::GenericValue();
}

llvm::GenericValue ValueCaputeCalled( llvm::FunctionType* , const llvm::ArrayRef<llvm::GenericValue> args )
{
	const uint64_t data_address= args[0].IntVal.getLimitedValue();
	const size_t data_size= size_t(args[1].IntVal.getLimitedValue());

	LifetimeCallResult res{};
	res.address= data_address;
	res.call_result= CallResult::CaptureValue;
	res.captured_data.resize( data_size );
	g_current_execution_engine->ReadExecutinEngineData( res.captured_data.data(), data_address, data_size );

	g_lifetimes_call_sequence.push_back(res);
	return llvm::GenericValue();
}

void LifetimesTestPrepare(const EnginePtr& engine)
{
	// HACK! Use global variables in order to avoid passing arguments trough execution engine.
	g_current_execution_engine= engine.get();
	g_lifetimes_call_sequence.clear();

	engine->RegisterCustomFunction( "__U_debug_lifetime_start", LifetimeStartCalled );
	engine->RegisterCustomFunction( "__U_debug_lifetime_end", LifetimeEndCalled );
	engine->RegisterCustomFunction( "CaptureValue", ValueCaputeCalled );
}

U_TEST( StackVariableLifetime_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn nomangle CaptureValue(i32& data, u64 size);
		fn Foo()
		{
			var i32 x= 42;
			CaptureValue(x, 4u64);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 3 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[2].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeEnd );
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::CaptureValue );

	const int32_t expected_x= 42;
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[1].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( StackVariableLifetime_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn nomangle CaptureValue(f64& data, u64 size);
		fn Foo()
		{
			var f64 x= 42.0;
			CaptureValue(x, 8u64);
			{
				var f64 y= 34.5;
				CaptureValue(y, 8u64);
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 6 );

	// x
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[5].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart );
	U_TEST_ASSERT( g_lifetimes_call_sequence[5].call_result == CallResult::LifetimeEnd );

	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::CaptureValue );
	const double expected_x= 42.0f;
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[1].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );

	// y
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[4].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeStart );
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].call_result == CallResult::LifetimeEnd );

	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::CaptureValue );
	const double expected_y= 34.5;
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].captured_data.size() == sizeof(expected_y) && std::memcmp(g_lifetimes_call_sequence[3].captured_data.data(), &expected_y, sizeof(expected_y)) == 0 );
}

U_TEST( StackVariableLifetime_Test2 )
{
	static const char c_program_text[]=
	R"(
		fn nomangle CaptureValue(u16& data, u64 size);
		fn Foo()
		{
			{
				var u16 x= 42u16;
				CaptureValue(x, 2u64);
			}
			{
				var u16 y= 34u16;
				CaptureValue(y, 2u64);
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 6 );

	// x
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[2].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeEnd );

	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::CaptureValue );
	const uint16_t expected_x= 42;
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[1].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );

	// y
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].address == g_lifetimes_call_sequence[5].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::LifetimeStart );
	U_TEST_ASSERT( g_lifetimes_call_sequence[5].call_result == CallResult::LifetimeEnd );

	U_TEST_ASSERT( g_lifetimes_call_sequence[4].call_result == CallResult::CaptureValue );
	const uint16_t expected_y= 34;
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].captured_data.size() == sizeof(expected_y) && std::memcmp(g_lifetimes_call_sequence[4].captured_data.data(), &expected_y, sizeof(expected_y)) == 0 );
}

U_TEST( StackVariableLifetime_Test3 )
{
	static const char c_program_text[]=
	R"(
		fn nomangle CaptureValue([f32, 4]& data, u64 size);
		fn Foo()
		{
			var [f32, 4] arr[ 0.25f, 2.0f, 17.6f, -0.1f ];
			CaptureValue(arr, 16u64);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 3 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[2].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeEnd );
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::CaptureValue );

	const float expected_arr[4]= { 0.25f, 2.0f, 17.6f, -0.1f };
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].captured_data.size() == sizeof(expected_arr) && std::memcmp(g_lifetimes_call_sequence[1].captured_data.data(), &expected_arr, sizeof(expected_arr)) == 0 );
}

U_TEST( StackVariableLifetime_Test4 )
{
	static const char c_program_text[]=
	R"(
		fn nomangle CaptureValue(u64& data, u64 size);
		fn Foo()
		{
			auto x= 55u64;
			CaptureValue(x, 8u64);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 3 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[2].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeEnd );
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::CaptureValue );

	const uint64_t expected_x= 55;
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[1].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( StackVariableLifetime_Test5 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor()(x= 678){}
		}
		fn nomangle CaptureValue(S& data, u64 size);
		fn Foo()
		{
			auto x= S(); // Lifetime declaration should be done once because of auto variable move initialization.
			CaptureValue(x, 4u64);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 3 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[2].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeEnd );
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::CaptureValue );

	const int32_t expected_x= 678;
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[1].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( StackVariableLifetime_Test6 )
{
	static const char c_program_text[]=
	R"(
		fn nomangle CaptureValue(u64& data, u64 size);
		fn Foo()
		{
			with( mut x : 785u64 ) // Lifetime for 'with' operator variable.
			{
				CaptureValue(x, 8u64);
				x= 852u64;
				CaptureValue(x, 8u64);
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 4 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[2].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart );
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::LifetimeEnd );
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::CaptureValue );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::CaptureValue );

	uint64_t expected_x= 785;
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[1].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );

	expected_x= 852;
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[2].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( StackVariableLifetime_Test7 )
{
	static const char c_program_text[]=
	R"(
		fn nomangle CaptureValue(u32& data, u64 size);
		var u32 constexpr g_x= 102938u;
		fn Foo()
		{
			// No lifetimes are created for reference.
			var u32& x_ref= g_x;
			CaptureValue(x_ref, 4u64);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 1 );
	const uint32_t expected_x= 102938;
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[0].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( StackVariableLifetime_Test8 )
{
	static const char c_program_text[]=
	R"(
		fn nomangle CaptureValue(u32& data, u64 size);
		var u32 constexpr g_x= 951u;
		fn Foo()
		{
			// No lifetimes are created for auto-reference.
			auto& x_ref= g_x;
			CaptureValue(x_ref, 4u64);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 1 );
	const uint32_t expected_x= 951;
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[0].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( ArgVariableLifetime_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn nomangle CaptureValue(u8& data, u64 size);
		fn Bar(u8 x)
		{
			CaptureValue(x, 1u64);
		}
		fn Foo()
		{
			Bar(67u8);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 3 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[2].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeEnd );
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::CaptureValue );

	const uint8_t expected_x= 67;
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[1].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( ArgVariableLifetime_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn nomangle CaptureValue(char16& data, u64 size);
		fn Bar(char16& x)
		{
			// Lifetime instructions are not created for reference arg.
			CaptureValue(x, 2u64);
		}
		var char16 constexpr ccc(423);
		fn Foo()
		{
			Bar(ccc);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 1 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::CaptureValue );

	const char16_t expected_x= 423;
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[0].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( ArgVariableLifetime_Test2 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			i32 y= 0;
			fn constructor()(x= 99996){}
		}
		fn nomangle CaptureValue(S& data, u64 size);
		fn Bar(S s)
		{
			// Lifetime declaration is not created for value arguments of composite types, because such arguments are passed by-reference.
			CaptureValue(s, 4u64);
		}
		fn Foo()
		{
			Bar(S()); // Lifetime declarations are created here.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 3 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[2].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeEnd );
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::CaptureValue );

	const int32_t expected_x= 99996;
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[1].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( ArgVariableLifetime_Test3 )
{
	static const char c_program_text[]=
	R"(
		struct S // This type contains single scalar inside and passed in register.
		{
			i32 x;
			fn constructor()(x= 99596){}
		}
		fn nomangle CaptureValue(S& data, u64 size);
		fn Bar(S s)
		{
			// Create lifetime start.
			CaptureValue(s, 4u64);
			// Create lifetime end.
		}
		fn Foo()
		{
			Bar(S()); // Create lifetime start here, load value into register, perform the call and only than create lifetime end.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 5 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[4].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[3].address );

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart );
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::LifetimeStart );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::CaptureValue );
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::LifetimeEnd );
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].call_result == CallResult::LifetimeEnd );

	const int32_t expected_x= 99596;
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[2].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( ReturnValueLifetime_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn Bar() : i32
		{
			return 0; // No lifetimes created for return by-value.
		}
		fn Foo()
		{
			Bar();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 0 );
}

U_TEST( ReturnValueLifetime_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			i32 y= 0;
			fn constructor()(x= 444555){}
		}
		fn nomangle CaptureValue(S& data, u64 size);
		fn Bar() : S
		{
			return S(); // Lifetime is created here for temp variable allocation, but after that temp variable allocation will be replaced with 's_ret'.
			// TODO - maybe remove "lifetime.start" for "s_ret" in such cases? Is it normal? Can such behaviour lead to unexpected bugs during LLVM optimization passes?
		}
		fn Foo()
		{
			auto s= Bar(); // Lifetime is created here for 's_ret'. Move initialization of auto variable is used here too.
			CaptureValue(s, 4u64);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 4 );

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart ); // s_ret
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::LifetimeStart ); // internal lifetime declaration for s_ret
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::LifetimeEnd );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[3].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::CaptureValue );

	const int32_t expected_x= 444555;
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[2].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( ReturnValueLifetime_Test2 )
{
	static const char c_program_text[]=
	R"(
		struct S // This struct contains single scalar inside and because of that is passed in register.
		{
			i32 x;
			fn constructor()(x= 6786){}
		}
		fn nomangle CaptureValue(S& data, u64 size);
		fn Bar() : S
		{
			var S mut s; // Create here lifetime for s
			return safe(s); // Create here lifetime for temporary copy, than load value into register and return ot.
		}
		fn Foo()
		{
			auto s= Bar(); // Lifetime is created here for result. Move initialization of auto variable is used here too.
			CaptureValue(s, 4u64);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 7 );

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart ); // start lifetime for call result "s".
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::LifetimeStart ); // start lifetime of variable "s".
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeStart ); // start temp return value of type "S".
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::LifetimeEnd ); // end temp return value of type "S".
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].call_result == CallResult::LifetimeEnd ); // end lifetime for variable "s".
	U_TEST_ASSERT( g_lifetimes_call_sequence[5].call_result == CallResult::CaptureValue ); // Capture "s".
	U_TEST_ASSERT( g_lifetimes_call_sequence[6].call_result == CallResult::LifetimeEnd ); // End lifetime for call result "s".

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[6].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].address == g_lifetimes_call_sequence[4].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[3].address );

	const int32_t expected_x= 6786;
	U_TEST_ASSERT( g_lifetimes_call_sequence[5].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[5].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( ReturnValueLifetime_Test3 )
{
	static const char c_program_text[]=
	R"(
		struct S // This struct contains single scalar inside and because of that is passed in register.
		{
			i32 x;
			fn constructor()(x= 6786){}
		}
		fn nomangle CaptureValue(S& data, u64 size);
		fn Bar() : S
		{
			var S s; // Create here lifetime for s
			return s; // Automatically move "s" here.
		}
		fn Foo()
		{
			auto s= Bar(); // Lifetime is created here for result. Move initialization of auto variable is used here too.
			CaptureValue(s, 4u64);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 5 );

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart ); // start lifetime for call result "s".
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::LifetimeStart ); // start lifetime of variable "s".
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeEnd ); // end lifetime for variable "s".
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::CaptureValue ); // Capture "s".
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].call_result == CallResult::LifetimeEnd ); // End lifetime for call result "s".

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[3].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[4].address );

	const int32_t expected_x= 6786;
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[3].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( LifetimeEndDuringInitialization_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn nomangle CaptureValue(i32& data, u64 size);
		fn Foo()
		{
			var i32 mut x= 678;
			CaptureValue(x, 4u64);
			var i32 x_moved= move(x); // Move-initialization via expression initializer.
			CaptureValue(x_moved, 4u64);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 6 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart ); // x
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::CaptureValue ); // x
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeStart ); // x_moved
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::LifetimeEnd ); // x
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].call_result == CallResult::CaptureValue ); // x_moved
	U_TEST_ASSERT( g_lifetimes_call_sequence[5].call_result == CallResult::LifetimeEnd ); // x_moved

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[1].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[3].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[4].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[5].address );

	const int32_t expected_x= 678;
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[1].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[4].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( LifetimeEndDuringInitialization_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn nomangle CaptureValue(i32& data, u64 size);
		fn Foo()
		{
			var i32 mut x= 9514789;
			CaptureValue(x, 4u64);
			var i32 x_moved(move(x)); // Move-initialization via constructor initializer.
			CaptureValue(x_moved, 4u64);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 6 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart ); // x
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::CaptureValue ); // x
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeStart ); // x_moved
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::LifetimeEnd ); // x
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].call_result == CallResult::CaptureValue ); // x_moved
	U_TEST_ASSERT( g_lifetimes_call_sequence[5].call_result == CallResult::LifetimeEnd ); // x_moved

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[1].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[3].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[4].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[5].address );

	const int32_t expected_x= 9514789;
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[1].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[4].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( LifetimeEndDuringInitialization_Test2 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor()(x= 66665){}
		}
		fn nomangle CaptureValue(S& data, u64 size);
		fn Foo()
		{
			var S mut s;
			CaptureValue(s, 4u64);
			var S s_moved(move(s)); // Move-initialization via constructor initializer.
			CaptureValue(s_moved, 4u64);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 6 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart ); // s
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::CaptureValue ); // s
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeStart ); // s_moved
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::LifetimeEnd ); // s
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].call_result == CallResult::CaptureValue ); // s_moved
	U_TEST_ASSERT( g_lifetimes_call_sequence[5].call_result == CallResult::LifetimeEnd ); // s_moved

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[1].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[3].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[4].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[5].address );

	const int32_t expected_x= 66665;
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[1].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[4].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( LifetimesForTakeOperator_Test )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor()(x= 66665){}
		}
		fn nomangle CaptureValue(S& data, u64 size);
		fn Foo()
		{
			var S mut s; // Create lifetime for 's'
			CaptureValue(s, 4u64);
			s.x= 111;
			auto s_moved= take(s); // Create temporary variable and lifetime for it.
			CaptureValue(s_moved, 4u64);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 6 );

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart ); // s
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::CaptureValue ); // s
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeStart ); // s_moved
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::CaptureValue ); // s_moved
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].call_result == CallResult::LifetimeEnd ); // s_moved
	U_TEST_ASSERT( g_lifetimes_call_sequence[5].call_result == CallResult::LifetimeEnd ); // s

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[1].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[5].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[3].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[4].address );

	int32_t expected_x= 66665;
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[1].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
	expected_x= 111;
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[3].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( LifetimesForRawPointers_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor()(x= 951){}
		}
		fn nomangle CaptureValue(S& data, u64 size);
		fn Foo()
		{
			var S mut s;
			CaptureValue(s, 4u64);
			auto ptr = $<(s);
			s.x= 159;
			CaptureValue(s, 4u64);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 6 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart ); // s
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::CaptureValue ); // s
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeStart ); // ptr
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::CaptureValue ); // s
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].call_result == CallResult::LifetimeEnd ); // ptr
	U_TEST_ASSERT( g_lifetimes_call_sequence[5].call_result == CallResult::LifetimeEnd ); // s

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[5].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[4].address );

	const int32_t expected_x0= 951;
	const int32_t expected_x1= 159;
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].captured_data.size() == sizeof(expected_x0) && std::memcmp(g_lifetimes_call_sequence[1].captured_data.data(), &expected_x0, sizeof(expected_x0)) == 0 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].captured_data.size() == sizeof(expected_x1) && std::memcmp(g_lifetimes_call_sequence[3].captured_data.data(), &expected_x1, sizeof(expected_x1)) == 0 );
}

U_TEST( LifetimesForRawPointers_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor()(x= 32147){}
		}
		fn nomangle CaptureValue(S& data, u64 size);
		fn Bar( $(S) s_ptr ){ unsafe{  $>(s_ptr).x= 74123;  } }
		fn Foo()
		{
			var S mut s;
			CaptureValue(s, 4u64);
			Bar( $<(s) );
			CaptureValue(s, 4u64);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 6 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart ); // s
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::CaptureValue ); // s
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeStart ); // ptr
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::LifetimeEnd ); // ptr
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].call_result == CallResult::CaptureValue ); // s
	U_TEST_ASSERT( g_lifetimes_call_sequence[5].call_result == CallResult::LifetimeEnd ); // s

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[5].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[3].address );

	const int32_t expected_x0= 32147;
	const int32_t expected_x1= 74123;
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].captured_data.size() == sizeof(expected_x0) && std::memcmp(g_lifetimes_call_sequence[1].captured_data.data(), &expected_x0, sizeof(expected_x0)) == 0 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].captured_data.size() == sizeof(expected_x1) && std::memcmp(g_lifetimes_call_sequence[4].captured_data.data(), &expected_x1, sizeof(expected_x1)) == 0 );
}

U_TEST( LifetimesForAwaitOperator_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn nomangle CaptureValue(i32& data, u64 size);
		fn async SomeFunc() : i32
		{
			return 654;
		}
		fn async Bar()
		{
			auto x= SomeFunc().await;
			CaptureValue(x, 4u64);
		}
		fn Foo()
		{
			auto mut f= Bar();
			loop
			{
				if_coro_advance( x : f ) { break; }
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 7 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart ); // func
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::LifetimeStart ); // SomeFunc temp
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeStart ); // x
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::LifetimeEnd ); // SomeFunc temp
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].call_result == CallResult::CaptureValue ); // x
	U_TEST_ASSERT( g_lifetimes_call_sequence[5].call_result == CallResult::LifetimeEnd ); // x
	U_TEST_ASSERT( g_lifetimes_call_sequence[6].call_result == CallResult::LifetimeEnd ); // func

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[6].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].address == g_lifetimes_call_sequence[3].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[5].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].address == g_lifetimes_call_sequence[4].address );

	const int32_t expected_x= 654;
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].captured_data.size() == sizeof(expected_x) && std::memcmp(g_lifetimes_call_sequence[4].captured_data.data(), &expected_x, sizeof(expected_x)) == 0 );
}

U_TEST( LifetimesForLambdas_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			// Call lifetime start for lambda, move it to auto variable, call lifetime end at lambda destruction.
			auto f= lambda(){};
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 2 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart ); // f
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::LifetimeEnd ); // f

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[1].address );
}

U_TEST( LifetimesForLambdas_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			// Call lifetime start for lambda, call it, call lifetime end at lambda destruction.
			var i32 x= lambda() : i32 { return 123; } ();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 4 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart ); // x
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::LifetimeStart ); // lambda
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeEnd ); // lambda
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::LifetimeEnd ); // x

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[3].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].address == g_lifetimes_call_sequence[2].address );
}

U_TEST( LifetimesForLambdas_Test2 )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; }
		fn Foo()
		{
			// Call lifetime start for "s".
			var S mut s{ .x= 123 };
			// Call lifetime start for lambda.
			// Call lifetime end for "s".
			auto f= lambda[ s= move(s) ] () : i32 { return s.x; };
			// Call lifetime start for "f_res".
			auto f_res= f();
			// Call lifetime end for "f_res".
			// Call lifetime end for lambda.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForLifetimesTest( c_program_text ) );
	LifetimesTestPrepare(engine);

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );

	U_TEST_ASSERT( g_lifetimes_call_sequence.size() == 6 );
	U_TEST_ASSERT( g_lifetimes_call_sequence[0].call_result == CallResult::LifetimeStart ); // s
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].call_result == CallResult::LifetimeStart ); // lambda
	U_TEST_ASSERT( g_lifetimes_call_sequence[2].call_result == CallResult::LifetimeEnd ); // s
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].call_result == CallResult::LifetimeStart ); // f_res
	U_TEST_ASSERT( g_lifetimes_call_sequence[4].call_result == CallResult::LifetimeEnd ); // f_res
	U_TEST_ASSERT( g_lifetimes_call_sequence[5].call_result == CallResult::LifetimeEnd ); // lambda

	U_TEST_ASSERT( g_lifetimes_call_sequence[0].address == g_lifetimes_call_sequence[2].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[1].address == g_lifetimes_call_sequence[5].address );
	U_TEST_ASSERT( g_lifetimes_call_sequence[3].address == g_lifetimes_call_sequence[4].address );
}

} // namespace

} // namespace U
