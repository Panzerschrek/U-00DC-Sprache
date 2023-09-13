#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( LLVMFunctionAttrsTest_TemplateFunctionsLinkage )
{
	// All template functions and functions inside template classes must have "private" linkage.
	static const char c_program_text[]=
	R"(
		template</type T/> fn GetZero() : T { return T(0); }

		template</type T/> struct S { fn StaticMethod(){} }

		fn Foo()
		{
			GetZero</f32/>();
			S</u32/>::StaticMethod();
		}
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const get_zero= module->getFunction( "_Z7GetZeroIfEvv" );
	U_TEST_ASSERT( get_zero != nullptr );
	U_TEST_ASSERT( get_zero->getLinkage() == llvm::GlobalVariable::PrivateLinkage );

	const llvm::Function* const template_static_method= module->getFunction( "_ZN1SIjE12StaticMethodEv" );
	U_TEST_ASSERT( template_static_method != nullptr );
	U_TEST_ASSERT( template_static_method->getLinkage() == llvm::GlobalVariable::PrivateLinkage );
}

U_TEST( LLVMFunctionAttrsTest_FundamentalTypeValueParamsAttrs )
{
	// No special attributes must be set for fundamental types value params.

	static const char c_program_text[]=
	R"(
		fn Foo( i32 x, f32 mut y, bool z ) { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3Fooifb" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );

	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 2, llvm::Attribute::Dereferenceable ) );

	U_TEST_ASSERT( !function->hasParamAttribute( 2, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 2, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 2, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 2, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 2, llvm::Attribute::Dereferenceable ) );
}

U_TEST( LLVMFunctionAttrsTest_FundamentalTypeReturnValueAttrs )
{
	static const char c_program_text[]=
	R"(
		fn Foo() : i32 { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->getFunctionType()->getReturnType()->isIntegerTy() );
	U_TEST_ASSERT( !function->hasRetAttribute( llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
}

U_TEST( LLVMFunctionAttrsTest_FundamentalTypeReturnReferenceAttrs )
{
	// Return reference has nonnull attribute.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32 & mut { halt; }
		fn Bar() : i32 &imut { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->getFunctionType()->getReturnType()->isPointerTy() );
	U_TEST_ASSERT( foo->hasRetAttribute( llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( foo->hasRetAttribute( llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( foo->getAttributeAtIndex( llvm::AttributeList::ReturnIndex, llvm::Attribute::Dereferenceable ).getDereferenceableBytes() == 4 );

	const llvm::Function* bar= module->getFunction( "_Z3Barv" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->getFunctionType()->getReturnType()->isPointerTy() );
	U_TEST_ASSERT( bar->hasRetAttribute( llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( bar->hasRetAttribute( llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( bar->getAttributeAtIndex( llvm::AttributeList::ReturnIndex, llvm::Attribute::Dereferenceable ).getDereferenceableBytes() == 4 );
}

U_TEST( LLVMFunctionAttrsTest_FundamentalTypeImutReferenceParamsAttrs )
{
	// Immutable reference params should have "nonnull", "readonly", "noalias" attrs. "dereferenceable" should be equal to type size.

	static const char c_program_text[]=
	R"(
		fn Foo( i32& x, f32& y, bool& z ) { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3FooRKiRKfRKb" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( function->getParamDereferenceableBytes( 0 ) == 4 );

	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( function->getParamDereferenceableBytes( 1 ) == 4 );

	U_TEST_ASSERT( function->hasParamAttribute( 2, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasParamAttribute( 2, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->hasParamAttribute( 2, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 2, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( function->hasParamAttribute( 2, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( function->getParamDereferenceableBytes( 2 ) == 1 );
}

U_TEST( LLVMFunctionAttrsTest_FundamentalTypeMutReferenceParamsAttrs )
{
	// Mutable reference params should have "nonnull" and "noalias" attrs, but non "readonly". "dereferenceable" should be equal to type size.
	// Add "noalias" because it is forbidden by reference checking to create two mutable references to same data.

	static const char c_program_text[]=
	R"(
		fn Foo( i32 &mut x, f32 &mut y, bool &mut z ) { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3FooRiRfRb" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( function->getParamDereferenceableBytes( 0 ) == 4 );

	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( function->getParamDereferenceableBytes( 1 ) == 4 );

	U_TEST_ASSERT( function->hasParamAttribute( 2, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasParamAttribute( 2, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 2, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 2, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( function->hasParamAttribute( 2, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( function->getParamDereferenceableBytes( 2 ) == 1 );
}

U_TEST( LLVMFunctionAttrsTest_StructTypeValueParamsAttrs )
{
	// Structs (and other composite types) passed by pointer, if they contain more than single scalar inside.
	// This pointer is non-null and actual storage is unique for each arg, so "noalias" must present.
	// "readonly" should not be set since it's possible to mutate arg in its destructor.
	// "dereferenceable" should be equal to type size.
	// "nocapture" should be used since there is no legal way to capture address of passed variable (because of some kind of "ReferenceProtectionError").
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; f32 y; }
		struct E{}
		fn Foo( S  mut s, E  mut e ) { halt; }
		fn Bar( E imut e, S imut s ) { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foo1S1E" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( foo->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !foo->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( foo->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( foo->getFunctionType()->getParamType(0)->isPointerTy() ); // Passed by pointer.
	U_TEST_ASSERT( foo->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( foo->getParamDereferenceableBytes( 0 ) == 8 );

	U_TEST_ASSERT( foo->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( foo->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !foo->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( foo->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( foo->getFunctionType()->getParamType(1)->isPointerTy() ); // Passed by pointer.
	U_TEST_ASSERT( !foo->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) || foo->getParamDereferenceableBytes( 1 ) == 0  );

	const llvm::Function* bar= module->getFunction( "_Z3Bar1E1S" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( bar->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( bar->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( bar->getFunctionType()->getParamType(0)->isPointerTy() ); // Passed by pointer.
	U_TEST_ASSERT( !bar->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) || bar->getParamDereferenceableBytes( 0 ) == 0 );

	U_TEST_ASSERT( bar->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( bar->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( bar->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( bar->getFunctionType()->getParamType(1)->isPointerTy() ); // Passed by pointer.
	U_TEST_ASSERT( bar->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( bar->getParamDereferenceableBytes( 1 ) == 8 );
}

U_TEST( LLVMFunctionAttrsTest_StructTypeSingleScalarsValueParamsAttrs )
{
	// Structs with single scalar inside are passed by value.
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; }
		struct E{ S s; }
		fn Foo( S  mut s, E  mut e ) { halt; }
		fn Bar( E imut e, S imut s ) { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foo1S1E" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( !foo->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !foo->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !foo->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !foo->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( !foo->getFunctionType()->getParamType(0)->isPointerTy() ); // Passed by value.
	U_TEST_ASSERT( !foo->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );

	U_TEST_ASSERT( !foo->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !foo->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !foo->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !foo->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( !foo->getFunctionType()->getParamType(1)->isPointerTy() ); // Passed by value.
	U_TEST_ASSERT( !foo->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) );

	const llvm::Function* bar= module->getFunction( "_Z3Bar1E1S" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( !bar->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( !bar->getFunctionType()->getParamType(0)->isPointerTy() ); // Passed by value.
	U_TEST_ASSERT( !bar->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );

	U_TEST_ASSERT( !bar->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( !bar->getFunctionType()->getParamType(1)->isPointerTy() ); // Passed by value.
	U_TEST_ASSERT( !bar->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) );
}

U_TEST( LLVMFunctionAttrsTest_StructTypeImutReferenceParamsAttrs )
{
	// Immutalbe reference params of struct type marked as "nonnull", "readonly", "noalias". "dereferenceable" should be equal to type size.
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; f32 y; }
		struct E{}
		fn Foo( S &imut s, E &imut e ) { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3FooRK1SRK1E" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(0)->isPointerTy() );
	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( function->getParamDereferenceableBytes( 0 ) == 8 );

	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(1)->isPointerTy() );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) || function->getParamDereferenceableBytes( 1 ) == 0 );
}

U_TEST( LLVMFunctionAttrsTest_StructTypeMutReferenceParamsAttrs )
{
	// Mutable reference params of struct type marked both as "nonnull"and "noalias", but not as "readonly". "dereferenceable" should be equal to type size.
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; f32 y; }
		struct E{}
		fn Foo( S &mut s, E &mut e ) { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* function= module->getFunction( "_Z3FooR1SR1E" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(0)->isPointerTy() );
	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( function->getParamDereferenceableBytes( 0 ) == 8 );

	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(1)->isPointerTy() );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) || function->getParamDereferenceableBytes( 1 ) == 0 );
}

U_TEST( LLVMFunctionAttrsTest_StructTypeReturnValueAttrs )
{
	// For functions, returning struct values, create hidden pointer param, where returned value placed.

	static const char c_program_text[]=
	R"(
		struct S{ i32 x; f32 y; }
		struct E{}
		fn Foo() : S { halt; }
		fn Bar() : E { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->hasParamAttribute( 0, llvm::Attribute::StructRet ) );
	U_TEST_ASSERT( foo->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !foo->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( foo->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( foo->getParamDereferenceableBytes( 0 ) == 8 );
	U_TEST_ASSERT( foo->getFunctionType()->getNumParams() == 1 );
	U_TEST_ASSERT( foo->getFunctionType()->getParamType(0)->isPointerTy() );
	U_TEST_ASSERT( foo->getReturnType()->isVoidTy() );
	U_TEST_ASSERT( !foo->hasRetAttribute( llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !foo->hasRetAttribute( llvm::Attribute::Dereferenceable ) );

	const llvm::Function* bar= module->getFunction( "_Z3Barv" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->hasParamAttribute( 0, llvm::Attribute::StructRet ) );
	U_TEST_ASSERT( bar->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) || bar->getParamDereferenceableBytes( 0 ) == 0 );
	U_TEST_ASSERT( bar->getFunctionType()->getNumParams() == 1 );
	U_TEST_ASSERT( bar->getFunctionType()->getParamType(0)->isPointerTy() );
	U_TEST_ASSERT( bar->getReturnType()->isVoidTy() );
	U_TEST_ASSERT( !bar->hasRetAttribute( llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !bar->hasRetAttribute( llvm::Attribute::Dereferenceable ) );
}

U_TEST( LLVMFunctionAttrsTest_SingleScalarStructTypeReturnValueAttrs )
{
	// If struct contains just single scalar inside - pass it in register.
	static const char c_program_text[]=
	R"(
		struct S{ f32 x; }
		struct E{ S s; }
		fn Foo() : S { halt; }
		fn Bar() : E { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->getReturnType()->isFloatTy() );
	U_TEST_ASSERT( foo->getFunctionType()->getNumParams() == 0 );

	const llvm::Function* bar= module->getFunction( "_Z3Barv" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->getReturnType()->isFloatTy() );
	U_TEST_ASSERT( bar->getFunctionType()->getNumParams() == 0 );
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
	U_TEST_ASSERT( foo->hasRetAttribute( llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( foo->hasRetAttribute( llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( foo->getAttributeAtIndex( llvm::AttributeList::ReturnIndex, llvm::Attribute::Dereferenceable ).getDereferenceableBytes() == 8 );

	const llvm::Function* bar= module->getFunction( "_Z3Barv" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->getFunctionType()->getNumParams() == 0 );
	U_TEST_ASSERT( bar->getReturnType()->isPointerTy() );
	U_TEST_ASSERT( bar->hasRetAttribute( llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( bar->hasRetAttribute( llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( bar->getAttributeAtIndex( llvm::AttributeList::ReturnIndex, llvm::Attribute::Dereferenceable ).getDereferenceableBytes() == 8 );
}

U_TEST( LLVMFunctionAttrsTest_CompositeTypeValueParamsAttrs )
{
	// Composite type value-arguments passed by pointer.
	// This pointer is non-null and actual storage is unique for each arg, so "noalias" must present.
	// "readonly" should not be set since it's possible to mutate arg in its destructor.
	// "dereferenceable" should be equal to type size.
	// "nocapture" should be used since there is no legal way to capture address of passed variable (because of some kind of "ReferenceProtectionError").
	// Composites with single scalar inside are passed in register using this scalar type.
	static const char c_program_text[]=
	R"(
		fn Foo( [ i32, 2 ] mut a, tup[ bool, f64 ] imut b ) { halt; }
		fn Bar( tup[ i32 ] a, [ char8, 1 ] b ) { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const foo= module->getFunction( "_Z3FooA2_i3tupIbdE" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( foo->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !foo->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( foo->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( foo->getFunctionType()->getParamType(0)->isPointerTy() ); // Passed by pointer.
	U_TEST_ASSERT( foo->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( foo->getParamDereferenceableBytes( 0 ) == 8 );

	U_TEST_ASSERT( foo->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( foo->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !foo->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( foo->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( foo->getFunctionType()->getParamType(1)->isPointerTy() ); // Passed by pointer.
	U_TEST_ASSERT( foo->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( foo->getParamDereferenceableBytes( 1 ) == 16 );

	const llvm::Function* const bar= module->getFunction( "_Z3Bar3tupIiEA1_c" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( !bar->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( !bar->getFunctionType()->getParamType(0)->isPointerTy() ); // Passed by value.
	U_TEST_ASSERT( !bar->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );

	U_TEST_ASSERT( !bar->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( !bar->getFunctionType()->getParamType(1)->isPointerTy() ); // Passed by value.
	U_TEST_ASSERT( !bar->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) );
}

U_TEST( LLVMFunctionAttrsTest_CompositeTypeImutReferenceParamsAttrs )
{
	// Immutalbe reference params of composite types marked as "nonnull", "readonly", "noalias". "dereferenceable" should be equal to type size.
	static const char c_program_text[]=
	R"(
		fn Foo( [ i32, 2 ] &imut a, tup[ bool, f64 ] &imut b ) { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const function= module->getFunction( "_Z3FooRKA2_iRK3tupIbdE" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(0)->isPointerTy() ); // Passed by pointer.
	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( function->getParamDereferenceableBytes( 0 ) == 8 );

	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(1)->isPointerTy() ); // Passed by pointer.
	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( function->getParamDereferenceableBytes( 1 ) == 16 );
}

U_TEST( LLVMFunctionAttrsTest_CompositeTypeMutReferenceParamsAttrs )
{
	// Mutable reference params of composite types marked both as "nonnull"and "noalias", but not as "readonly". "dereferenceable" should be equal to type size.
	static const char c_program_text[]=
	R"(
		fn Foo( [ i32, 2 ] &mut a, tup[ bool, f64 ] &mut b ) { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const function= module->getFunction( "_Z3FooRA2_iR3tupIbdE" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(0)->isPointerTy() ); // Passed by pointer.
	U_TEST_ASSERT( function->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( function->getParamDereferenceableBytes( 0 ) == 8 );

	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( function->getFunctionType()->getParamType(1)->isPointerTy() ); // Passed by pointer.
	U_TEST_ASSERT( function->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( function->getParamDereferenceableBytes( 1 ) == 16 );
}

U_TEST( LLVMFunctionAttrsTest_CompositeTypeReturnValueAttrs )
{
	// For functions, returning composite type values, create hidden pointer param, where returned value placed.
	static const char c_program_text[]=
	R"(
		fn Foo() : [ f32, 16 ] { halt; }
		fn Bar() : tup[ bool, [ i32, 2 ], f32 ] { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->hasParamAttribute( 0, llvm::Attribute::StructRet ) );
	U_TEST_ASSERT( foo->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !foo->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( foo->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( foo->getParamDereferenceableBytes( 0 ) == 64 );
	U_TEST_ASSERT( foo->getFunctionType()->getNumParams() == 1 );
	U_TEST_ASSERT( foo->getFunctionType()->getParamType(0)->isPointerTy() );
	U_TEST_ASSERT( foo->getReturnType()->isVoidTy() );
	U_TEST_ASSERT( !foo->hasRetAttribute( llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !foo->hasRetAttribute( llvm::Attribute::Dereferenceable ) );

	const llvm::Function* bar= module->getFunction( "_Z3Barv" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->hasParamAttribute( 0, llvm::Attribute::StructRet ) );
	U_TEST_ASSERT( bar->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !bar->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( bar->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( bar->getParamDereferenceableBytes( 0 ) == 16 );
	U_TEST_ASSERT( bar->getFunctionType()->getNumParams() == 1 );
	U_TEST_ASSERT( bar->getFunctionType()->getParamType(0)->isPointerTy() );
	U_TEST_ASSERT( bar->getReturnType()->isVoidTy() );
	U_TEST_ASSERT( !bar->hasRetAttribute( llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !bar->hasRetAttribute( llvm::Attribute::Dereferenceable ) );
}

U_TEST( LLVMFunctionAttrsTest_SingleScalarCompositeTypeReturnValueAttrs )
{
	// If function returns value of composite type, containing single scalar inside, just use this scalar type as return value.
	static const char c_program_text[]=
	R"(
		fn Foo() : [ f64, 1 ] { halt; }
		fn Bar() : tup[ [ tup[ [ u16, 1 ] ], 1 ] ] { halt; }
		fn Baz() : [ $(i64), 1 ] { halt; }
		fn Lol() : tup[ (fn (i32 x, f32 y): f64) ] { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );
	U_TEST_ASSERT( foo->getReturnType()->isDoubleTy() );
	U_TEST_ASSERT( foo->getFunctionType()->getNumParams() == 0 );

	const llvm::Function* bar= module->getFunction( "_Z3Barv" );
	U_TEST_ASSERT( bar != nullptr );
	U_TEST_ASSERT( bar->getReturnType()->isIntegerTy() );
	U_TEST_ASSERT( bar->getFunctionType()->getNumParams() == 0 );

	const llvm::Function* baz= module->getFunction( "_Z3Bazv" );
	U_TEST_ASSERT( baz != nullptr );
	U_TEST_ASSERT( baz->getReturnType()->isPointerTy() );
	U_TEST_ASSERT( baz->getFunctionType()->getNumParams() == 0 );

	const llvm::Function* lol= module->getFunction( "_Z3Lolv" );
	U_TEST_ASSERT( lol != nullptr );
	U_TEST_ASSERT( lol->getReturnType()->isPointerTy() );
	U_TEST_ASSERT( lol->getFunctionType()->getNumParams() == 0 );
}

U_TEST( LLVMFunctionAttrsTest_CompositeTypeReturnReferenceAttrs )
{
	// For functions, returning references to composite types, return just pointer.
	static const char c_program_text[]=
	R"(
		fn Foo() : [ f32, 16 ] &mut { halt; }
		fn Bar() : tup[ bool, [ i32, 2 ], f32 ] &imut { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );

	U_TEST_ASSERT( foo->getFunctionType()->getNumParams() == 0 );
	U_TEST_ASSERT( foo->getReturnType()->isPointerTy() );
	U_TEST_ASSERT( foo->hasRetAttribute( llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( foo->hasRetAttribute( llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( foo->getAttributeAtIndex( llvm::AttributeList::ReturnIndex, llvm::Attribute::Dereferenceable ).getDereferenceableBytes() == 64 );

	const llvm::Function* bar= module->getFunction( "_Z3Barv" );
	U_TEST_ASSERT( bar != nullptr );

	U_TEST_ASSERT( bar->getFunctionType()->getNumParams() == 0 );
	U_TEST_ASSERT( bar->getReturnType()->isPointerTy() );
	U_TEST_ASSERT( bar->hasRetAttribute( llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( bar->hasRetAttribute( llvm::Attribute::Dereferenceable ) );
	U_TEST_ASSERT( bar->getAttributeAtIndex( llvm::AttributeList::ReturnIndex, llvm::Attribute::Dereferenceable ).getDereferenceableBytes() == 16 );
}

U_TEST( LLVMFunctionAttrsTest_RawPointerTypeValueParamsAttrs )
{
	// No special attributes must be set for raw pointer value params.
	// Raw pointers may alias, may be null, may be captured and pointed values may be changed.
	static const char c_program_text[]=
	R"(
		fn Foo( $(i32) x, $(f32) mut y, $(bool) z ) { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const function= module->getFunction( "_Z3FooPiPfPb" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );

	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) );

	U_TEST_ASSERT( !function->hasParamAttribute( 2, llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 2, llvm::Attribute::NoAlias ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 2, llvm::Attribute::ReadOnly ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 2, llvm::Attribute::NoCapture ) );
	U_TEST_ASSERT( !function->hasParamAttribute( 2, llvm::Attribute::Dereferenceable ) );
}

U_TEST( LLVMFunctionAttrsTest_RawPointerReturnValueAttrs )
{
	// Raw pointer may be null. It should not have non-null attribute.
	static const char c_program_text[]=
	R"(
		fn Foo() : $(i32) { halt; }
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const function= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	U_TEST_ASSERT( function->getReturnType()->isPointerTy() );
	U_TEST_ASSERT( !function->hasRetAttribute( llvm::Attribute::NonNull ) );
	U_TEST_ASSERT( !function->hasRetAttribute( llvm::Attribute::Dereferenceable ) );
}

U_TEST( LLVMFunctionAttrsTest_GeneratedMethodsAttrsTest )
{
	static const char c_program_text[]=
	R"(
		// Default constructor, copy constructor, copy-assignment operator, destructor should be generated.
		struct S{ i16 x= zero_init; }
	)";

	const auto module= BuildProgram( c_program_text );

	// "this" as mutable reference param should be marked with "nonnull" and "noalias".
	// "src" (for copy methods) should be marked as "nonnull", "noalias", "readonly", as any other immutable reference param.
	// "Dereferenceable" should be used for references.
	// All methods must have private linkage because they are generated.

	{
		const llvm::Function* const default_constructor= module->getFunction( "_ZN1S11constructorERS_" );
		U_TEST_ASSERT( default_constructor != nullptr );
		U_TEST_ASSERT( default_constructor->getLinkage() == llvm::GlobalValue::PrivateLinkage );

		U_TEST_ASSERT( default_constructor->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( default_constructor->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( !default_constructor->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
		U_TEST_ASSERT( !default_constructor->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
		U_TEST_ASSERT( default_constructor->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
		U_TEST_ASSERT( default_constructor->getParamDereferenceableBytes( 0 ) == 2 );
	}
	{
		const llvm::Function* const copy_constructor= module->getFunction( "_ZN1S11constructorERS_RKS_" );
		U_TEST_ASSERT( copy_constructor != nullptr );
		U_TEST_ASSERT( copy_constructor->getLinkage() == llvm::GlobalValue::PrivateLinkage );

		U_TEST_ASSERT( copy_constructor->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( copy_constructor->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( !copy_constructor->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
		U_TEST_ASSERT( !copy_constructor->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
		U_TEST_ASSERT( copy_constructor->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
		U_TEST_ASSERT( copy_constructor->getParamDereferenceableBytes( 0 ) == 2 );

		U_TEST_ASSERT( copy_constructor->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( copy_constructor->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( copy_constructor->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
		U_TEST_ASSERT( !copy_constructor->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
		U_TEST_ASSERT( copy_constructor->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) );
		U_TEST_ASSERT( copy_constructor->getParamDereferenceableBytes( 1 ) == 2 );
	}
	{
		const llvm::Function* const copy_assignment_operator= module->getFunction( "_ZN1SaSERS_RKS_" );
		U_TEST_ASSERT( copy_assignment_operator != nullptr );
		U_TEST_ASSERT( copy_assignment_operator->getLinkage() == llvm::GlobalValue::PrivateLinkage );

		U_TEST_ASSERT( copy_assignment_operator->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( copy_assignment_operator->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( !copy_assignment_operator->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
		U_TEST_ASSERT( !copy_assignment_operator->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
		U_TEST_ASSERT( copy_assignment_operator->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
		U_TEST_ASSERT( copy_assignment_operator->getParamDereferenceableBytes( 0 ) == 2 );

		U_TEST_ASSERT( copy_assignment_operator->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( copy_assignment_operator->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( copy_assignment_operator->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
		U_TEST_ASSERT( !copy_assignment_operator->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
		U_TEST_ASSERT( copy_assignment_operator->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) );
		U_TEST_ASSERT( copy_assignment_operator->getParamDereferenceableBytes( 1 ) == 2 );
	}
	{
		const llvm::Function* const destructor= module->getFunction( "_ZN1S10destructorERS_" );
		U_TEST_ASSERT( destructor != nullptr );
		U_TEST_ASSERT( destructor->getLinkage() == llvm::GlobalValue::PrivateLinkage );

		U_TEST_ASSERT( destructor->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( destructor->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( !destructor->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
		U_TEST_ASSERT( !destructor->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
		U_TEST_ASSERT( destructor->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
		U_TEST_ASSERT( destructor->getParamDereferenceableBytes( 0 ) == 2 );
	}
}

U_TEST( LLVMFunctionAttrsTest_GeneratedDefaultMethodsAttrsTest )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			[ f32, 3] arr= zero_init;

			fn constructor()= default;
			fn constructor( S &imut other )= default;
			op=( mut this, S &imut other )= default;
		}
	)";

	const auto module= BuildProgram( c_program_text );

	// "this" as mutable reference param should be marked with "nonnull" and "noalias".
	// "src" (for copy methods) should be marked as "nonnull", "readonly", "noalias", as any other immutable reference param.
	// All methods must have private linkage because they are generated.

	{
		const llvm::Function* const default_constructor= module->getFunction( "_ZN1S11constructorERS_" );
		U_TEST_ASSERT( default_constructor != nullptr );
		U_TEST_ASSERT( default_constructor->getLinkage() == llvm::GlobalValue::PrivateLinkage );

		U_TEST_ASSERT( default_constructor->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( default_constructor->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( !default_constructor->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
		U_TEST_ASSERT( !default_constructor->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
		U_TEST_ASSERT( default_constructor->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
		U_TEST_ASSERT( default_constructor->getParamDereferenceableBytes( 0 ) == 12 );
	}
	{
		const llvm::Function* const copy_constructor= module->getFunction( "_ZN1S11constructorERS_RKS_" );
		U_TEST_ASSERT( copy_constructor != nullptr );
		U_TEST_ASSERT( copy_constructor->getLinkage() == llvm::GlobalValue::PrivateLinkage );

		U_TEST_ASSERT( copy_constructor->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( copy_constructor->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( !copy_constructor->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
		U_TEST_ASSERT( !copy_constructor->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
		U_TEST_ASSERT( copy_constructor->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
		U_TEST_ASSERT( copy_constructor->getParamDereferenceableBytes( 0 ) == 12 );

		U_TEST_ASSERT( copy_constructor->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( copy_constructor->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( copy_constructor->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
		U_TEST_ASSERT( !copy_constructor->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
		U_TEST_ASSERT( copy_constructor->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) );
		U_TEST_ASSERT( copy_constructor->getParamDereferenceableBytes( 1 ) == 12 );
	}
	{
		const llvm::Function* const copy_assignment_operator= module->getFunction( "_ZN1SaSERS_RKS_" );
		U_TEST_ASSERT( copy_assignment_operator != nullptr );
		U_TEST_ASSERT( copy_assignment_operator->getLinkage() == llvm::GlobalValue::PrivateLinkage );

		U_TEST_ASSERT( copy_assignment_operator->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( copy_assignment_operator->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( !copy_assignment_operator->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
		U_TEST_ASSERT( !copy_assignment_operator->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
		U_TEST_ASSERT( copy_assignment_operator->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
		U_TEST_ASSERT( copy_assignment_operator->getParamDereferenceableBytes( 0 ) == 12 );

		U_TEST_ASSERT( copy_assignment_operator->hasParamAttribute( 1, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( copy_assignment_operator->hasParamAttribute( 1, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( copy_assignment_operator->hasParamAttribute( 1, llvm::Attribute::ReadOnly ) );
		U_TEST_ASSERT( !copy_assignment_operator->hasParamAttribute( 1, llvm::Attribute::NoCapture ) );
		U_TEST_ASSERT( copy_assignment_operator->hasParamAttribute( 1, llvm::Attribute::Dereferenceable ) );
		U_TEST_ASSERT( copy_assignment_operator->getParamDereferenceableBytes( 1 ) == 12 );
	}
	{
		const llvm::Function* const destructor= module->getFunction( "_ZN1S10destructorERS_" );
		U_TEST_ASSERT( destructor != nullptr );
		U_TEST_ASSERT( destructor->getLinkage() == llvm::GlobalValue::PrivateLinkage );

		U_TEST_ASSERT( destructor->hasParamAttribute( 0, llvm::Attribute::NonNull ) );
		U_TEST_ASSERT( destructor->hasParamAttribute( 0, llvm::Attribute::NoAlias ) );
		U_TEST_ASSERT( !destructor->hasParamAttribute( 0, llvm::Attribute::ReadOnly ) );
		U_TEST_ASSERT( !destructor->hasParamAttribute( 0, llvm::Attribute::NoCapture ) );
		U_TEST_ASSERT( destructor->hasParamAttribute( 0, llvm::Attribute::Dereferenceable ) );
		U_TEST_ASSERT( destructor->getParamDereferenceableBytes( 0 ) == 12 );
	}
}

} // namespace

} // namespace U
