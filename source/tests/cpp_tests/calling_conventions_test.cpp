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

	const llvm::Function* const func= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( func->getCallingConv() == llvm::CallingConv::C );
}

U_TEST(CallingConventionDeclaration_Test1)
{
	static const char c_program_text[]=
	R"(
		fn Foo() calling_conv("C");
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const func= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( func->getCallingConv() == llvm::CallingConv::C );
}

U_TEST(CallingConventionDeclaration_Test2)
{
	static const char c_program_text[]=
	R"(
		fn Foo() unsafe calling_conv("fast") : u32;
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const func= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( func->getCallingConv() == llvm::CallingConv::Fast );
}

U_TEST(CallingConventionDeclaration_Test3)
{
	static const char c_program_text[]=
	R"(
		fn Foo() unsafe calling_conv("Ü") : u32;
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const func= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( func->getCallingConv() == llvm::CallingConv::C );
}

U_TEST(CallingConventionDeclaration_Test4)
{
	static const char c_program_text[]=
	R"(
		fn Foo() unsafe calling_conv("default") : u32;
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const func= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( func->getCallingConv() == llvm::CallingConv::C );
}

U_TEST(CallingConventionDeclaration_Test5)
{
	static const char c_program_text[]=
	R"(
		type FastCallFn = fn(i32 x) calling_conv("fast") : i32&;
	)";

	BuildProgram( c_program_text );
}

} // namespace

} // namespace U
