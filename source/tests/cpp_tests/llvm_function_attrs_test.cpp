#include "tests.hpp"

namespace U
{

U_TEST( LLVMFunctionAttrsTest_FundamentalTypeValueParamsAttrs )
{
	// No special attributes must be set for fundamental types value params.

	static const char c_program_text[]=
	R"(
		fn Foo( i32 x, f32 mut y, bool z ){}
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3Fooifb" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );

	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );

	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::NoAlias ) );
}

U_TEST( LLVMFunctionAttrsTest_FundamentalTypeImutReferenceParamsAttrs )
{
	// Immutable reference params should have only "nonnull" attr, but not "noalias".

	static const char c_program_text[]=
	R"(
		fn Foo( i32& x, f32& y, bool& z ){}
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3FooRKiRKfRKb" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::NoAlias ) );
}

U_TEST( LLVMFunctionAttrsTest_FundamentalTypeMutReferenceParamsAttrs )
{
	// Mutable reference params should have "nonnull" and "noalias" attrs.
	// Add "noalias" because it is forbidden by reference checking to create two mutable references to same data.

	static const char c_program_text[]=
	R"(
		fn Foo( i32 &mut x, f32 &mut y, bool &mut z ){}
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3FooRiRfRb" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::NoAlias ) );
}
} // namespace U
