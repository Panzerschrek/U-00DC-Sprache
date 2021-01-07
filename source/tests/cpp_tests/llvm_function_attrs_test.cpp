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

U_TEST( LLVMFunctionAttrsTest_StructTypeValueParamsAttrs )
{
	// Structs (and other composite types) passed by pointer.
	// This pointer is non-null and actual storage is unique for each arg, so "noalias" must present.
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; f32 y; }
		struct E{}
		fn Foo( S  mut s, E  mut e ){}
		fn Bar( S imut s, E imut e ){}
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foo1S1E" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( foo->getFunctionType()->getParamType(0)->isPointerTy() ); // Passed by pointer.

	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( foo->getFunctionType()->getParamType(1)->isPointerTy() ); // Passed by pointer.

	const llvm::Function* bar= module->getFunction( "_Z3Bar1S1E" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( bar->getFunctionType()->getParamType(0)->isPointerTy() ); // Passed by pointer.

	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( bar->getFunctionType()->getParamType(1)->isPointerTy() ); // Passed by pointer.
}

U_TEST( LLVMFunctionAttrsTest_StructTypeImutReferenceParamsAttrs )
{
	// Immutalbe reference params of struct type marked as "nonnull", but not as "noalias".
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; f32 y; }
		struct E{}
		fn Foo( S &imut s, E &imut e ){}
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3FooRK1SRK1E" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(0)->isPointerTy() );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(1)->isPointerTy() );
}

U_TEST( LLVMFunctionAttrsTest_StructTypeMutReferenceParamsAttrs )
{
	// Mutable reference params of struct type marked both as "nonnull"and "noalias".
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; f32 y; }
		struct E{}
		fn Foo( S &mut s, E &mut e ){}
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3FooR1SR1E" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(0)->isPointerTy() );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(1)->isPointerTy() );
}

U_TEST( LLVMFunctionAttrsTest_StructTypeReturnValueAttrs )
{
	// For functions, returning struct values, create hidden pointer param, where returned value placed.

	static const char c_program_text[]=
	R"(
		struct S{ i32 x; f32 y; }
		struct E{}
		fn Foo() : S { var S s= zero_init; return s; }
		fn Bar() : E { var E e= zero_init; return e; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::StructRet ) );
	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( foo->getFunctionType()->getNumParams() == 1 );
	U_TEST_ASSERT( foo->getFunctionType()->getParamType(0)->isPointerTy() );
	U_TEST_ASSERT( foo->getReturnType()->isVoidTy() );

	const llvm::Function* bar= module->getFunction( "_Z3Barv" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::StructRet ) );
	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( bar->getFunctionType()->getNumParams() == 1 );
	U_TEST_ASSERT( bar->getFunctionType()->getParamType(0)->isPointerTy() );
	U_TEST_ASSERT( bar->getReturnType()->isVoidTy() );
}

U_TEST( LLVMFunctionAttrsTest_StructTypeReturnReferenceAttrs )
{
	// For functions, returning references to struct, return just pointer.

	static const char c_program_text[]=
	R"(
		struct S{ i32 x; f32 y; }
		fn Foo() : S & mut { halt; }
		fn Bar() : S &imut { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->getFunctionType()->getNumParams() == 0 );
	U_TEST_ASSERT( foo->getReturnType()->isPointerTy() );

	const llvm::Function* bar= module->getFunction( "_Z3Barv" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->getFunctionType()->getNumParams() == 0 );
	U_TEST_ASSERT( bar->getReturnType()->isPointerTy() );
}

} // namespace U
