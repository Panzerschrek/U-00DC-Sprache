#include <llvm/Support/DynamicLibrary.h>

#include "tests.hpp"

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
	void* address;
	CallResult call_result;
	std::vector<uint8_t> captured_data;
};

std::vector<LifetimeCallResult> g_lifetimes_call_sequence;

llvm::GenericValue LifetimeStartCalled( llvm::FunctionType* , const llvm::ArrayRef<llvm::GenericValue> args )
{
	LifetimeCallResult res{};
	res.address= args[0].PointerVal;
	res.call_result= CallResult::LifetimeStart;
	g_lifetimes_call_sequence.push_back(res);

	return llvm::GenericValue();
}

llvm::GenericValue LifetimeEndCalled( llvm::FunctionType* , const llvm::ArrayRef<llvm::GenericValue> args )
{
	LifetimeCallResult res{};
	res.address= args[0].PointerVal;
	res.call_result= CallResult::LifetimeEnd;
	g_lifetimes_call_sequence.push_back(res);

	return llvm::GenericValue();
}

llvm::GenericValue ValueCaputeCalled( llvm::FunctionType* , const llvm::ArrayRef<llvm::GenericValue> args )
{
	LifetimeCallResult res{};
	res.address= args[0].PointerVal;
	res.call_result= CallResult::CaptureValue;
	res.captured_data.insert( res.captured_data.end(), reinterpret_cast<uint8_t*>(res.address), reinterpret_cast<uint8_t*>(res.address) + args[1].IntVal.getLimitedValue() );
	g_lifetimes_call_sequence.push_back(res);

	return llvm::GenericValue();
}

void LifetimesTestPrepare()
{
	g_lifetimes_call_sequence.clear();

	// "lle_X_" - common prefix for all external functions, called from LLVM Interpreter
	llvm::sys::DynamicLibrary::AddSymbol( "lle_X___U_debug_lifetime_start", reinterpret_cast<void*>( &LifetimeStartCalled ) );
	llvm::sys::DynamicLibrary::AddSymbol( "lle_X___U_debug_lifetime_end", reinterpret_cast<void*>( &LifetimeEndCalled ) );
	llvm::sys::DynamicLibrary::AddSymbol( "lle_X_CaptureValue", reinterpret_cast<void*>( &ValueCaputeCalled ) );
}

} // namespace

U_TEST( StackVariableLifetime_Test0 )
{
	LifetimesTestPrepare();

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
	LifetimesTestPrepare();

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
	LifetimesTestPrepare();

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

} // namespace U
