#include <cstdlib>

#include "tests.hpp"

namespace U
{

namespace
{

U_TEST(CallingConventionDeclaration_Test0)
{
	static const char c_program_text[]=
	R"(
		fn Foo();
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const function= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	U_TEST_ASSERT( function->getCallingConv() == llvm::CallingConv::C );
}

U_TEST(CallingConventionDeclaration_Test1)
{
	static const char c_program_text[]=
	R"(
		fn Foo() calling_conv("C");
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const function= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	U_TEST_ASSERT( function->getCallingConv() == llvm::CallingConv::C );
}

U_TEST(CallingConventionDeclaration_Test2)
{
	static const char c_program_text[]=
	R"(
		fn Foo() unsafe calling_conv("fast") : u32;
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const function= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	U_TEST_ASSERT( function->getCallingConv() == llvm::CallingConv::Fast );
}

U_TEST(CallingConventionDeclaration_Test3)
{
	static const char c_program_text[]=
	R"(
		fn Foo() unsafe calling_conv("Ü") : u32;
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const function= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	U_TEST_ASSERT( function->getCallingConv() == llvm::CallingConv::C );
}

U_TEST(CallingConventionDeclaration_Test4)
{
	static const char c_program_text[]=
	R"(
		fn Foo() unsafe calling_conv("default") : u32;
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const function= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	U_TEST_ASSERT( function->getCallingConv() == llvm::CallingConv::C );
}

U_TEST(CallingConventionDeclaration_Test5)
{
	static const char c_program_text[]=
	R"(
		type FastCallFn = fn(i32 x) calling_conv("fast") : i32&;
	)";

	BuildProgram( c_program_text );
}

U_TEST(CallForFunctionWithCustomCallingConvention_Test0)
{
	static const char c_program_text[]=
	R"(
		fn Bar() calling_conv("fast") : i32 { return 102934; }
		fn Foo() : i32
		{
			return Bar();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	const llvm::GenericValue result= engine->runFunction( function, {} );

	U_TEST_ASSERT( result.IntVal.getLimitedValue() == uint64_t(102934) );
}

U_TEST(CallForFunctionWithCustomCallingConvention_Test1)
{
	static const char c_program_text[]=
	R"(
		fn Bar(u32 x) calling_conv("fast") : u32 { return x / 3u; }
		fn Foo() : u32
		{
			return Bar(951u);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	const llvm::GenericValue result= engine->runFunction( function, {} );

	U_TEST_ASSERT( result.IntVal.getLimitedValue() == uint64_t(951u / 3u) );
}

} // namespace

} // namespace U
