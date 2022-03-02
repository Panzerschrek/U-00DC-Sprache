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

U_TEST(CallingConventionAliases_Test0)
{
	// Currently calling conventions "C", "default", "Ü" are aliases for "C" calling convention.
	static const char c_program_text[]=
	R"(
		fn Foo() calling_conv("C");
		fn Bar() calling_conv("default");
		fn Baz() calling_conv("Ü");
		fn FooBar();

		type F= fn() calling_conv("default");

		var F f0(Foo), f1(Bar), f2(Baz), f3(FooBar);
	)";

	BuildProgram( c_program_text );
}

U_TEST(CallingConventionAliases_Test1)
{
	static const char c_program_text[]=
	R"(
		namespace Abc
		{
			fn Foo() calling_conv("default");
		}
		// Ok, define body for existing function using alias for calling convention.
		fn Abc::Foo() calling_conv("C") {}
	)";

	BuildProgram( c_program_text );
}

U_TEST(CallingConventionMakesFunctionTypeDifferent_Test0)
{
	static const char c_program_text[]=
	R"(
		fn Foo() calling_conv("fast");
		var (fn()) ptr(Foo);
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::CouldNotSelectOverloadedFunction, 3 ) );
}

U_TEST(CallingConventionMakesFunctionTypeDifferent_Test1)
{
	static const char c_program_text[]=
	R"(
		fn Foo() calling_conv("fast");
		var (fn() calling_conv("fast")) foo_ptr(Foo);
		var (fn()) foo_ptr_copy= foo_ptr;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::TypesMismatch, 4 ) );
}

U_TEST(CallingConventionMakesFunctionTypeDifferent_Test2)
{
	static const char c_program_text[]=
	R"(
		fn Foo() calling_conv("C");
		fn Foo() calling_conv("fast"); // Overloading by calling convention is not possible.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::CouldNotOverloadFunction, 3 ) );
}

U_TEST(CallingConventionMakesFunctionTypeDifferent_Test3)
{
	static const char c_program_text[]=
	R"(
		namespace Abc{  fn Foo() calling_conv("fast");  }
		fn Abc::Foo() calling_conv("C") {}  // Error, there is no function with such type inside "Abc".
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::FunctionDeclarationOutsideItsScope, 3 ) );
}

U_TEST(CallingConventionMakesFunctionTypeDifferent_Test4)
{
	static const char c_program_text[]=
	R"(
		namespace Abc{  fn Foo() calling_conv("fast");  }
		fn Abc::Foo() {}  // Error, there is no function with such type inside "Abc". No calling convention specified means default calling convention.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::FunctionDeclarationOutsideItsScope, 3 ) );
}

U_TEST(UnknownCallingConvention_Test0)
{
	static const char c_program_text[]=
	R"(
		fn Foo() calling_conv("SomeCrazyCallingCov");
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::UnknownCallingConvention, 2 ) );
}

U_TEST(UnknownCallingConvention_Test1)
{
	static const char c_program_text[]=
	R"(
		fn Foo() calling_conv("");
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::UnknownCallingConvention, 2 ) );
}


U_TEST(UnknownCallingConvention_Test2)
{
	static const char c_program_text[]=
	R"(
		fn Foo() calling_conv("Rust");
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::UnknownCallingConvention, 2 ) );
}

U_TEST(UnknownCallingConvention_Test3)
{
	static const char c_program_text[]=
	R"(
		type F= fn() calling_conv("6") : bool;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::UnknownCallingConvention, 2 ) );
}

} // namespace

} // namespace U
