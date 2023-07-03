#include "tests.hpp"

namespace U
{

namespace
{

U_TEST( FunctionLinkage_Test0 )
{
	static const char c_program_text_a[]= R"()";

	static const char c_program_text_root[]=
	R"(
		// Function defined in main file without prototypes in external file should be private.
		fn Foo(){}
		namespace NN{ fn Baz(){} }
		struct S
		{
			// Methods defined in main file without prototypes in external file should be private.
			fn Bar(this){}
			fn StaticMethod(){}
		}
	)";

	auto module= BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" );

	const llvm::Function* const foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );
	U_TEST_ASSERT( foo->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const baz= module->getFunction( "_ZN2NN3BazEv" );
	U_TEST_ASSERT( baz != nullptr );
	U_TEST_ASSERT( baz->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const bar= module->getFunction( "_ZN1S3BarERKS_" );
	U_TEST_ASSERT( bar != nullptr );
	U_TEST_ASSERT( bar->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const static_method= module->getFunction( "_ZN1S12StaticMethodEv" );
	U_TEST_ASSERT( static_method != nullptr );
	U_TEST_ASSERT( static_method->getLinkage() == llvm::GlobalValue::PrivateLinkage );
}

U_TEST( FunctionLinkage_Test1 )
{
	static const char c_program_text_a[]= R"(
		// Function defined in imported file should be private.
		fn Foo(){}
		namespace NN{ fn Baz(){} }
		struct S
		{
			// Methods defined in imported file should be private.
			fn Bar(this){}
			fn StaticMethod(){}
			fn Kek(mut this);
		}
		// Even function with prototype still should be private if it is defined in imported file.
		fn Lol();
		fn Lol(){}
		// Even method with prototype still should be private if it is defined in imported file.
		fn S::Kek(mut this){}
	)";

	static const char c_program_text_root[]= R"( import "a" )";

	auto module= BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" );

	const llvm::Function* const foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );
	U_TEST_ASSERT( foo->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const baz= module->getFunction( "_ZN2NN3BazEv" );
	U_TEST_ASSERT( baz != nullptr );
	U_TEST_ASSERT( baz->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const bar= module->getFunction( "_ZN1S3BarERKS_" );
	U_TEST_ASSERT( bar != nullptr );
	U_TEST_ASSERT( bar->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const static_method= module->getFunction( "_ZN1S12StaticMethodEv" );
	U_TEST_ASSERT( static_method != nullptr );
	U_TEST_ASSERT( static_method->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const lol= module->getFunction( "_Z3Lolv" );
	U_TEST_ASSERT( lol != nullptr );
	U_TEST_ASSERT( lol->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const kek= module->getFunction( "_ZN1S3KekERS_" );
	U_TEST_ASSERT( kek != nullptr );
	U_TEST_ASSERT( kek->getLinkage() == llvm::GlobalValue::PrivateLinkage );
}

U_TEST( FunctionLinkage_Test2 )
{
	static const char c_program_text_a[]= R"(
		fn Foo();
		namespace NN{ fn Baz(); }
		struct S
		{
			fn Bar(this);
			fn StaticMethod();
		}
	)";

	static const char c_program_text_root[]= R"(
		import "a"
		// Functions with prototypes in imported file should have external linkage.
		fn Foo(){}
		fn NN::Baz(){}
		fn S::Bar(this){}
		fn S::StaticMethod(){}
	)";

	auto module= BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" );

	const llvm::Function* const foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );
	U_TEST_ASSERT( foo->getLinkage() == llvm::GlobalValue::ExternalLinkage );

	const llvm::Function* const baz= module->getFunction( "_ZN2NN3BazEv" );
	U_TEST_ASSERT( baz != nullptr );
	U_TEST_ASSERT( baz->getLinkage() == llvm::GlobalValue::ExternalLinkage );

	const llvm::Function* const bar= module->getFunction( "_ZN1S3BarERKS_" );
	U_TEST_ASSERT( bar != nullptr );
	U_TEST_ASSERT( bar->getLinkage() == llvm::GlobalValue::ExternalLinkage );

	const llvm::Function* const static_method= module->getFunction( "_ZN1S12StaticMethodEv" );
	U_TEST_ASSERT( static_method != nullptr );
	U_TEST_ASSERT( static_method->getLinkage() == llvm::GlobalValue::ExternalLinkage );
}

U_TEST( FunctionLinkage_Test3 )
{
	static const char c_program_text_a[]= R"(
		// Nomangle function defined in imported file should still have private linkage.
		fn nomangle Foo(){}
		)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		// Nomangle function in main file should have external linkage.
		fn nomangle Bar(){}
	)";

	auto module= BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" );

	const llvm::Function* const foo= module->getFunction( "Foo" );
	U_TEST_ASSERT( foo != nullptr );
	U_TEST_ASSERT( foo->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const bar= module->getFunction( "Bar" );
	U_TEST_ASSERT( bar != nullptr );
	U_TEST_ASSERT( bar->getLinkage() == llvm::GlobalValue::ExternalLinkage );
}

} // namespace

} // namespace U
