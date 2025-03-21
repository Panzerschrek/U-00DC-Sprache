#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( NamespacesTest0 )
{
	// Should get functions from namespace.
	static const char c_program_text[]=
	R"(
		namespace SpaceA
		{
			namespace B
			{
				struct CFF
				{
					fn DoSomething(){}
				}
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_ZN6SpaceA1B3CFF11DoSomethingEv" );
	U_TEST_ASSERT( function != nullptr );
}

U_TEST( NamespacesTest1 )
{
	// Should get functions from namespace.
	static const char c_program_text[]=
	R"(
		namespace SpaceA
		{
			namespace B
			{
				struct CFF
				{
					fn DoSomething() : i32 { return 8547; }
				}
			}
		}
		namespace CCCombo
		{
			fn Foo() : i32
			{
				return SpaceA::B::CFF::DoSomething();
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_ZN7CCCombo3FooEv" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 8547 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( NamespacesTest2 )
{
	// Prototype inside namespace and body outside namespace.
	static const char c_program_text[]=
	R"(
		namespace SpaceA
		{
			fn DoSomething() : i32;
		}
		fn SpaceA::DoSomething() : i32
		{
			return 5555555;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_ZN6SpaceA11DoSomethingEv" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 5555555 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( NamespacesTest3 )
{
	// declaration of namespace extends previous namespace with same name.
	static const char c_program_text[]=
	R"(
		namespace SpaceA
		{
			fn DoSomething() : i32 { return 666*666; }
		}
		namespace SpaceA
		{
			fn Foo() : i32 { return DoSomething(); }
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_ZN6SpaceA3FooEv" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 666*666 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( NamespacesTest5 )
{
	// Function, defined outside parent scope, can access to it.
	static const char c_program_text[]=
	R"(
		namespace A
		{
			fn Foo();
			struct Internal
			{
				i32 x;
			}
		}
		fn A::Foo()
		{
			var Internal i= zero_init;
			var ::A::Internal ii= zero_init;
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace

} // namespace U
