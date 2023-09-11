#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST(CallingConventionDeclaration_Test0)
{
	static const char c_program_text[]=
	R"(
		fn Foo() { halt; }
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
		fn Foo() call_conv("C") { halt; }
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
		fn Foo() unsafe call_conv("fast") : u32 { halt; }
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
		fn Foo() unsafe call_conv("Ü") : u32 { halt; }
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
		fn Foo() unsafe call_conv("default") : u32 { halt; }
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
		type FastCallFn = fn(i32 x) call_conv("fast") : i32&;
	)";

	BuildProgram( c_program_text );
}

U_TEST(CallingConventionDeclaration_Test6)
{
	static const char c_program_text[]=
	R"(
		fn Foo() unsafe call_conv("system") : u32 { halt; }
	)";

	BuildProgram( c_program_text );
}

U_TEST(CallingConventionDeclaration_Test7)
{
	static const char c_program_text[]=
	R"(
		fn Foo() unsafe call_conv("cold") : u32 { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const function= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	U_TEST_ASSERT( function->getCallingConv() == llvm::CallingConv::Cold );
}

U_TEST(CallForFunctionWithCustomCallingConvention_Test0)
{
	static const char c_program_text[]=
	R"(
		fn Bar() call_conv("fast") : i32 { return 102934; }
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
		fn Bar(u32 x) call_conv("fast") : u32 { return x / 3u; }
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

U_TEST(CallForFunctionWithCustomCallingConvention_Test3)
{
	static const char c_program_text[]=
	R"(
		fn Bar(u32 x) call_conv("cold") : u32 { return x / 5u; }
		fn Foo() : u32
		{
			var (fn(u32 x) call_conv("cold") : u32) mut ptr= Bar;
			return ptr(123456u);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	const llvm::GenericValue result= engine->runFunction( function, {} );

	U_TEST_ASSERT( result.IntVal.getLimitedValue() == uint64_t(123456u / 5u) );
}

U_TEST(CallingConventionAliases_Test0)
{
	// Currently calling conventions "C", "default", "Ü" are aliases for "C" calling convention.
	static const char c_program_text[]=
	R"(
		fn Foo() call_conv("C");
		fn Bar() call_conv("default");
		fn Baz() call_conv("Ü");
		fn FooBar();

		type F= fn() call_conv("default");

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
			fn Foo() call_conv("default");
		}
		// Ok, define body for existing function using alias for calling convention.
		fn Abc::Foo() call_conv("C") {}
	)";

	BuildProgram( c_program_text );
}

U_TEST(CallingConventionMakesFunctionTypeDifferent_Test0)
{
	static const char c_program_text[]=
	R"(
		fn Foo() call_conv("fast");
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
		fn Foo() call_conv("fast");
		var (fn() call_conv("fast")) foo_ptr(Foo);
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
		fn Foo() call_conv("C");
		fn Foo() call_conv("fast"); // Overloading by calling convention is not possible.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT(
		HaveError( build_result.errors, CodeBuilderErrorCode::CouldNotOverloadFunction, 2 ) ||
		HaveError( build_result.errors, CodeBuilderErrorCode::CouldNotOverloadFunction, 3 ) );
}

U_TEST(CallingConventionMakesFunctionTypeDifferent_Test3)
{
	static const char c_program_text[]=
	R"(
		namespace Abc{  fn Foo() call_conv("fast");  }
		fn Abc::Foo() call_conv("C") {}  // Error, there is no function with such type inside "Abc".
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::FunctionDeclarationOutsideItsScope, 3 ) );
}

U_TEST(CallingConventionMakesFunctionTypeDifferent_Test4)
{
	static const char c_program_text[]=
	R"(
		namespace Abc{  fn Foo() call_conv("fast");  }
		fn Abc::Foo() {}  // Error, there is no function with such type inside "Abc". No calling convention specified means default calling convention.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::FunctionDeclarationOutsideItsScope, 3 ) );
}

U_TEST(CallingConventionMakesFunctionTypeDifferent_Test5)
{
	static const char c_program_text[]=
	R"(
		template<//> struct S</ fn () call_conv("fast") />
		{
			auto x= 11;
		}

		template<//> struct S</ fn () call_conv("cold") />
		{
			auto x= 22;
		}

		template<//> struct S</ fn () call_conv("C") />
		{
			auto x= 33;
		}

		static_assert( S</ fn () call_conv("fast") />::x == 11 );
		static_assert( S</ fn () call_conv("cold") />::x == 22 );
		static_assert( S</ fn () call_conv("C") />::x == 33 );
	)";

	BuildProgram( c_program_text );
}

U_TEST(UnknownCallingConvention_Test0)
{
	static const char c_program_text[]=
	R"(
		fn Foo() call_conv("SomeCrazyCallingCov");
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::UnknownCallingConvention, 2 ) );
}

U_TEST(UnknownCallingConvention_Test1)
{
	static const char c_program_text[]=
	R"(
		fn Foo() call_conv("");
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::UnknownCallingConvention, 2 ) );
}


U_TEST(UnknownCallingConvention_Test2)
{
	static const char c_program_text[]=
	R"(
		fn Foo() call_conv("Rust");
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::UnknownCallingConvention, 2 ) );
}

U_TEST(UnknownCallingConvention_Test3)
{
	static const char c_program_text[]=
	R"(
		type F= fn() call_conv("6") : bool;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::UnknownCallingConvention, 2 ) );
}

U_TEST(NonDefaultCallingConventionForClassMethod_Test0)
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			fn Foo(this) call_conv("fast");
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::NonDefaultCallingConventionForClassMethod, 4 ) );
}

U_TEST(NonDefaultCallingConventionForClassMethod_Test1)
{
	static const char c_program_text[]=
	R"(
		class A
		{
			fn Foo(mut this, bool x) call_conv("cold") : i32; // Custom calling convention for this-call method.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::NonDefaultCallingConventionForClassMethod, 4 ) );
}

U_TEST(NonDefaultCallingConventionForClassMethod_Test2)
{
	static const char c_program_text[]=
	R"(
		class A
		{
			fn constructor() call_conv("fast"); // Custom calling convention for constructor.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::NonDefaultCallingConventionForClassMethod, 4 ) );
}

U_TEST(NonDefaultCallingConventionForClassMethod_Test3)
{
	static const char c_program_text[]=
	R"(
		class A
		{
			fn destructor() call_conv("cold"); // Custom calling convention for destructor.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::NonDefaultCallingConventionForClassMethod, 4 ) );
}

U_TEST(NonDefaultCallingConventionForClassMethod_Test4)
{
	static const char c_program_text[]=
	R"(
		class A
		{
			op()(this, i32 x) call_conv("fast"); // Custom calling convention for this-call operator.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::NonDefaultCallingConventionForClassMethod, 4 ) );
}

U_TEST(NonDefaultCallingConventionForClassMethod_Test5)
{
	static const char c_program_text[]=
	R"(
		class A
		{
			op<=>(A& l, A& r) call_conv("fast") : i32; // Custom calling convention for binary operator.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::NonDefaultCallingConventionForClassMethod, 4 ) );
}

U_TEST(NonDefaultCallingConventionForClassMethod_Test6)
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			op~(A a) call_conv("cold") : A; // Custom calling convention for unary operator.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::NonDefaultCallingConventionForClassMethod, 4 ) );
}

U_TEST(NonDefaultCallingConventionForClassMethod_Test7)
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			fn Foo(A& a) call_conv("fast"); // Ok - custom calling cobnvetion allowed for non-this call function.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST(NonDefaultCallingConventionForClassMethod_Test8)
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			fn Foo(this) call_conv("default"); // Ok - default calling connvention.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST(NonDefaultCallingConventionForClassMethod_Test9)
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			fn Foo(mut this, i32& x) call_conv("Ü"); // Ok - default calling connvention.
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace

} // namespace U
