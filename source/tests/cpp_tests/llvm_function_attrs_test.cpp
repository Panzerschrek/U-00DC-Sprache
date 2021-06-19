#include "tests.hpp"

namespace U
{

U_TEST( LLVMFunctionAttrsTest_FundamentalTypeValueParamsAttrs )
{
	// No special attributes must be set for fundamental types value params.

	static const char c_program_text[]=
	R"(
		fn Foo( i32 x, f32 mut y, bool z );
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3Fooifb" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );

	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::ReadOnly ) );

	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::ReadOnly ) );
}

U_TEST( LLVMFunctionAttrsTest_FundamentalTypeReturnValueAttrs )
{
	static const char c_program_text[]=
	R"(
		fn Foo() : i32;
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->getFunctionType()->getReturnType()->isIntegerTy() );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::ReturnIndex, llvm::Attribute::NonNull ) );
}

U_TEST( LLVMFunctionAttrsTest_FundamentalTypeReturnReferenceAttrs )
{
	// Return reference has nonnull attribute.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32 & mut;
		fn Bar() : i32 &imut;
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->getFunctionType()->getReturnType()->isPointerTy() );
	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::ReturnIndex, llvm::Attribute::NonNull ) );

	const llvm::Function* bar= module->getFunction( "_Z3Barv" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->getFunctionType()->getReturnType()->isPointerTy() );
	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::ReturnIndex, llvm::Attribute::NonNull ) );
}

U_TEST( LLVMFunctionAttrsTest_FundamentalTypeImutReferenceParamsAttrs )
{
	// Immutable reference params should have only "nonnull" and "readonly" attrs, but not "noalias".

	static const char c_program_text[]=
	R"(
		fn Foo( i32& x, f32& y, bool& z );
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3FooRKiRKfRKb" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::ReadOnly ) );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::ReadOnly ) );
}

U_TEST( LLVMFunctionAttrsTest_FundamentalTypeMutReferenceParamsAttrs )
{
	// Mutable reference params should have "nonnull" and "noalias" attrs, but non "readonly".
	// Add "noalias" because it is forbidden by reference checking to create two mutable references to same data.

	static const char c_program_text[]=
	R"(
		fn Foo( i32 &mut x, f32 &mut y, bool &mut z );
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3FooRiRfRb" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::ReadOnly ) );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::ReadOnly ) );
}

U_TEST( LLVMFunctionAttrsTest_StructTypeValueParamsAttrs )
{
	// Structs (and other composite types) passed by pointer.
	// This pointer is non-null and actual storage is unique for each arg, so "noalias" must present.
	// Also "readonly" must be set for immutable value args.
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; f32 y; }
		struct E{}
		fn Foo( S  mut s, E  mut e );
		fn Bar( S imut s, E imut e );
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foo1S1E" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( foo->getFunctionType()->getParamType(0)->isPointerTy() ); // Passed by pointer.

	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( foo->getFunctionType()->getParamType(1)->isPointerTy() ); // Passed by pointer.

	const llvm::Function* bar= module->getFunction( "_Z3Bar1S1E" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( bar->getFunctionType()->getParamType(0)->isPointerTy() ); // Passed by pointer.

	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( bar->getFunctionType()->getParamType(1)->isPointerTy() ); // Passed by pointer.
}

U_TEST( LLVMFunctionAttrsTest_StructTypeImutReferenceParamsAttrs )
{
	// Immutalbe reference params of struct type marked as "nonnull" and "readonly", but not as "noalias".
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; f32 y; }
		struct E{}
		fn Foo( S &imut s, E &imut e );
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3FooRK1SRK1E" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(0)->isPointerTy() );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(1)->isPointerTy() );
}

U_TEST( LLVMFunctionAttrsTest_StructTypeMutReferenceParamsAttrs )
{
	// Mutable reference params of struct type marked both as "nonnull"and "noalias", but not as "readonly".
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; f32 y; }
		struct E{}
		fn Foo( S &mut s, E &mut e );
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3FooR1SR1E" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(0)->isPointerTy() );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(1)->isPointerTy() );
}

U_TEST( LLVMFunctionAttrsTest_StructTypeReturnValueAttrs )
{
	// For functions, returning struct values, create hidden pointer param, where returned value placed.

	static const char c_program_text[]=
	R"(
		struct S{ i32 x; f32 y; }
		struct E{}
		fn Foo() : S;
		fn Bar() : E;
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::StructRet ) );
	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( foo->getFunctionType()->getNumParams() == 1 );
	U_TEST_ASSERT( foo->getFunctionType()->getParamType(0)->isPointerTy() );
	U_TEST_ASSERT( foo->getReturnType()->isVoidTy() );
	U_TEST_ASSERT( !foo->hasAttribute( llvm::AttributeList::ReturnIndex, llvm::Attribute::NonNull ) );

	const llvm::Function* bar= module->getFunction( "_Z3Barv" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::StructRet ) );
	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( bar->getFunctionType()->getNumParams() == 1 );
	U_TEST_ASSERT( bar->getFunctionType()->getParamType(0)->isPointerTy() );
	U_TEST_ASSERT( bar->getReturnType()->isVoidTy() );
	U_TEST_ASSERT( !bar->hasAttribute( llvm::AttributeList::ReturnIndex, llvm::Attribute::NonNull ) );
}

U_TEST( LLVMFunctionAttrsTest_StructTypeReturnReferenceAttrs )
{
	// For functions, returning references to struct, return just pointer.

	static const char c_program_text[]=
	R"(
		struct S{ i32 x; f32 y; }
		fn Foo() : S & mut;
		fn Bar() : S &imut;
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->getFunctionType()->getNumParams() == 0 );
	U_TEST_ASSERT( foo->getReturnType()->isPointerTy() );
	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::ReturnIndex, llvm::Attribute::NonNull ) );

	const llvm::Function* bar= module->getFunction( "_Z3Barv" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->getFunctionType()->getNumParams() == 0 );
	U_TEST_ASSERT( bar->getReturnType()->isPointerTy() );
	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::ReturnIndex, llvm::Attribute::NonNull ) );
}

U_TEST( LLVMFunctionAttrsTest_CompositeTypeValueParamsAttrs )
{
	// Composite type value-arguments passed by pointer.
	// This pointer is non-null and actual storage is unique for each arg, so "noalias" must present.
	// Also "readonly" must be set for mutable value-params.
	static const char c_program_text[]=
	R"(
		fn Foo( [ i32, 2 ] mut a, tup[ bool, f64 ] imut b );
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const function= module->getFunction( "_Z3FooA2_i3tupIbdE" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(0)->isPointerTy() ); // Passed by pointer.

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(1)->isPointerTy() ); // Passed by pointer.
}

U_TEST( LLVMFunctionAttrsTest_CompositeTypeImutReferenceParamsAttrs )
{
	// Immutalbe reference params of composite types marked as "nonnull" and "readonly", but not as "noalias".
	static const char c_program_text[]=
	R"(
		fn Foo( [ i32, 2 ] &imut a, tup[ bool, f64 ] &imut b );
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const function= module->getFunction( "_Z3FooRKA2_iRK3tupIbdE" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(0)->isPointerTy() ); // Passed by pointer.

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(1)->isPointerTy() ); // Passed by pointer.
}

U_TEST( LLVMFunctionAttrsTest_CompositeTypeMutReferenceParamsAttrs )
{
	// Mutable reference params of composite types marked both as "nonnull"and "noalias", but not as "readonly".
	static const char c_program_text[]=
	R"(
		fn Foo( [ i32, 2 ] &mut a, tup[ bool, f64 ] &mut b );
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const function= module->getFunction( "_Z3FooRA2_iR3tupIbdE" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(0)->isPointerTy() ); // Passed by pointer.

	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(1)->isPointerTy() ); // Passed by pointer.
}

U_TEST( LLVMFunctionAttrsTest_CompositeTypeReturnValueAttrs )
{
	// For functions, returning composite type values, create hidden pointer param, where returned value placed.
	static const char c_program_text[]=
	R"(
		fn Foo() : [ f32, 16 ];
		fn Bar() : tup[ bool, [ i32, 2 ], f32 ];
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::StructRet ) );
	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !foo->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( foo->getFunctionType()->getNumParams() == 1 );
	U_TEST_ASSERT( foo->getFunctionType()->getParamType(0)->isPointerTy() );
	U_TEST_ASSERT( foo->getReturnType()->isVoidTy() );
	U_TEST_ASSERT( !foo->hasAttribute( llvm::AttributeList::ReturnIndex, llvm::Attribute::NonNull ) );

	const llvm::Function* bar= module->getFunction( "_Z3Barv" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::StructRet ) );
	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !bar->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( bar->getFunctionType()->getNumParams() == 1 );
	U_TEST_ASSERT( bar->getFunctionType()->getParamType(0)->isPointerTy() );
	U_TEST_ASSERT( bar->getReturnType()->isVoidTy() );
	U_TEST_ASSERT( !bar->hasAttribute( llvm::AttributeList::ReturnIndex, llvm::Attribute::NonNull ) );
}

U_TEST( LLVMFunctionAttrsTest_CompositeTypeReturnReferenceAttrs )
{
	// For functions, returning references to composite types, return just pointer.
	static const char c_program_text[]=
	R"(
		fn Foo() : [ f32, 16 ] &mut;
		fn Bar() : tup[ bool, [ i32, 2 ], f32 ] &imut;
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->getFunctionType()->getNumParams() == 0 );
	U_TEST_ASSERT( foo->getReturnType()->isPointerTy() );
	U_TEST_ASSERT( foo->hasAttribute( llvm::AttributeList::ReturnIndex, llvm::Attribute::NonNull ) );

	const llvm::Function* bar= module->getFunction( "_Z3Barv" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->getFunctionType()->getNumParams() == 0 );
	U_TEST_ASSERT( bar->getReturnType()->isPointerTy() );
	U_TEST_ASSERT( bar->hasAttribute( llvm::AttributeList::ReturnIndex, llvm::Attribute::NonNull ) );
}

U_TEST( LLVMFunctionAttrsTest_RawPointerTypeValueParamsAttrs )
{
	// No special attributes must be set for raw pointer value params.
	// Raw pointers may alias and may be null.
	static const char c_program_text[]=
	R"(
		fn Foo( $(i32) x, $(f32) mut y, $(bool) z );
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const function= module->getFunction( "_Z3FooPiPfPb" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );

	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::ReadOnly ) );

	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::FirstArgIndex + 2, llvm::Attribute::ReadOnly ) );
}

U_TEST( LLVMFunctionAttrsTest_RawPointerReturnValueAttrs )
{
	// Raw pointer may be null. It should not have non-null attribute.
	static const char c_program_text[]=
	R"(
		fn Foo() : $(i32);
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const function= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->getReturnType()->isPointerTy() );
	U_TEST_ASSERT( !function->hasAttribute( llvm::AttributeList::ReturnIndex, llvm::Attribute::NonNull ) );
}

U_TEST( LLVMFunctionAttrsTest_GeneratedMethodsAttrsTest )
{
	static const char c_program_text[]=
	R"(
		// Default constructor, copy constructor, copy-assignment operator, destructor should be generated.
		struct S{}
	)";

	const auto module= BuildProgram( c_program_text );

	// "this" as mutable reference param should be marked with "nonnull" and "noalias".
	// "src" (for copy methods) should be marked only as "nonnull" and "readonly", as any other immutable reference param.

	{
		const llvm::Function* const default_constructor= module->getFunction( "_ZN1S11constructorERS_" );
		U_TEST_ASSERT( default_constructor != nullptr );

		U_TEST_ASSERT( default_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( default_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( !default_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );
	}
	{
		const llvm::Function* const copy_constructor= module->getFunction( "_ZN1S11constructorERS_RKS_" );
		U_TEST_ASSERT( copy_constructor != nullptr );

		U_TEST_ASSERT( copy_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( copy_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( !copy_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );

		U_TEST_ASSERT( copy_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( !copy_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( copy_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::ReadOnly ) );
	}
	{
		const llvm::Function* const copy_assignment_operator= module->getFunction( "_ZN1SaSERS_RKS_" );
		U_TEST_ASSERT( copy_assignment_operator != nullptr );

		U_TEST_ASSERT( copy_assignment_operator->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( copy_assignment_operator->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( !copy_assignment_operator->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );

		U_TEST_ASSERT( copy_assignment_operator->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( !copy_assignment_operator->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( copy_assignment_operator->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::ReadOnly ) );
	}
	{
		const llvm::Function* const destructor= module->getFunction( "_ZN1S10destructorERS_" );
		U_TEST_ASSERT( destructor != nullptr );

		U_TEST_ASSERT( destructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( destructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( !destructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );
	}
}

U_TEST( LLVMFunctionAttrsTest_GeneratedDefaultMethodsAttrsTest )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn constructor()= default;
			fn constructor( S &imut other )= default;
			op=( mut this, S &imut other )= default;
		}
	)";

	const auto module= BuildProgram( c_program_text );

	// "this" as mutable reference param should be marked with "nonnull" and "noalias".
	// "src" (for copy methods) should be marked only as "nonnull" and "readonly", as any other immutable reference param.

	{
		const llvm::Function* const default_constructor= module->getFunction( "_ZN1S11constructorERS_" );
		U_TEST_ASSERT( default_constructor != nullptr );

		U_TEST_ASSERT( default_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( default_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( !default_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );
	}
	{
		const llvm::Function* const copy_constructor= module->getFunction( "_ZN1S11constructorERS_RKS_" );
		U_TEST_ASSERT( copy_constructor != nullptr );

		U_TEST_ASSERT( copy_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( copy_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( !copy_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );

		U_TEST_ASSERT( copy_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( !copy_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( copy_constructor->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::ReadOnly ) );
	}
	{
		const llvm::Function* const copy_assignment_operator= module->getFunction( "_ZN1SaSERS_RKS_" );
		U_TEST_ASSERT( copy_assignment_operator != nullptr );

		U_TEST_ASSERT( copy_assignment_operator->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( copy_assignment_operator->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( !copy_assignment_operator->hasAttribute( llvm::AttributeList::FirstArgIndex + 0, llvm::Attribute::ReadOnly ) );

		U_TEST_ASSERT( copy_assignment_operator->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( !copy_assignment_operator->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( copy_assignment_operator->hasAttribute( llvm::AttributeList::FirstArgIndex + 1, llvm::Attribute::ReadOnly ) );
	}
}

} // namespace U
