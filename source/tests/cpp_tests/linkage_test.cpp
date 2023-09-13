#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( FunctionLinkage_Test0 )
{
	static const char c_program_text_a[]= R"()";

	static const char c_program_text_root[]=
	R"(
		// Function defined in main file without prototype in external file should be private.
		fn Foo(){}
		namespace NN{ fn Baz(){} }
		struct S
		{
			// Methods defined in main file without prototype in external file should be private.
			fn Bar(this){}
			fn StaticMethod(){}
		}
	)";

	const auto module= BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" );

	const llvm::Function* const foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );
	U_TEST_ASSERT( foo->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !foo->hasComdat() );

	const llvm::Function* const baz= module->getFunction( "_ZN2NN3BazEv" );
	U_TEST_ASSERT( baz != nullptr );
	U_TEST_ASSERT( baz->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !baz->hasComdat() );

	const llvm::Function* const bar= module->getFunction( "_ZN1S3BarERKS_" );
	U_TEST_ASSERT( bar != nullptr );
	U_TEST_ASSERT( bar->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !bar->hasComdat() );

	const llvm::Function* const static_method= module->getFunction( "_ZN1S12StaticMethodEv" );
	U_TEST_ASSERT( static_method != nullptr );
	U_TEST_ASSERT( static_method->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !static_method->hasComdat() );
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

	const auto module= BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" );

	const llvm::Function* const foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );
	U_TEST_ASSERT( foo->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !foo->hasComdat() );

	const llvm::Function* const baz= module->getFunction( "_ZN2NN3BazEv" );
	U_TEST_ASSERT( baz != nullptr );
	U_TEST_ASSERT( baz->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !baz->hasComdat() );

	const llvm::Function* const bar= module->getFunction( "_ZN1S3BarERKS_" );
	U_TEST_ASSERT( bar != nullptr );
	U_TEST_ASSERT( bar->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !bar->hasComdat() );

	const llvm::Function* const static_method= module->getFunction( "_ZN1S12StaticMethodEv" );
	U_TEST_ASSERT( static_method != nullptr );
	U_TEST_ASSERT( static_method->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !static_method->hasComdat() );

	const llvm::Function* const lol= module->getFunction( "_Z3Lolv" );
	U_TEST_ASSERT( lol != nullptr );
	U_TEST_ASSERT( lol->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !lol->hasComdat() );

	const llvm::Function* const kek= module->getFunction( "_ZN1S3KekERS_" );
	U_TEST_ASSERT( kek != nullptr );
	U_TEST_ASSERT( kek->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !kek->hasComdat() );
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
		// Functions with prototype in imported file should have external linkage.
		fn Foo(){}
		fn NN::Baz(){}
		fn S::Bar(this){}
		fn S::StaticMethod(){}
	)";

	const auto module= BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" );

	const llvm::Function* const foo= module->getFunction( "_Z3Foov" );
	U_TEST_ASSERT( foo != nullptr );
	U_TEST_ASSERT( foo->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( !foo->hasComdat() );

	const llvm::Function* const baz= module->getFunction( "_ZN2NN3BazEv" );
	U_TEST_ASSERT( baz != nullptr );
	U_TEST_ASSERT( baz->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( !baz->hasComdat() );

	const llvm::Function* const bar= module->getFunction( "_ZN1S3BarERKS_" );
	U_TEST_ASSERT( bar != nullptr );
	U_TEST_ASSERT( bar->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( !bar->hasComdat() );

	const llvm::Function* const static_method= module->getFunction( "_ZN1S12StaticMethodEv" );
	U_TEST_ASSERT( static_method != nullptr );
	U_TEST_ASSERT( static_method->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( !static_method->hasComdat() );
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

	const auto module= BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" );

	const llvm::Function* const foo= module->getFunction( "Foo" );
	U_TEST_ASSERT( foo != nullptr );
	U_TEST_ASSERT( foo->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !foo->hasComdat() );

	const llvm::Function* const bar= module->getFunction( "Bar" );
	U_TEST_ASSERT( bar != nullptr );
	U_TEST_ASSERT( bar->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( !bar->hasComdat() );
}

U_TEST( VariableLinkage_Test0 )
{
	// All constant global variables should have private linkage.
	static const char c_program_text_a[]= " var i32 x = 0; auto y = false; ";
	static const char c_program_text_root[]= "import \"a\" var f32 z= 0.0f; auto w= \"lol\"; ";

	const auto engine= CreateEngine( BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" ) );

	const llvm::GlobalVariable* const x= engine->FindGlobalVariableNamed( "x", true );
	U_TEST_ASSERT( x != nullptr );
	U_TEST_ASSERT( x->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !x->hasComdat() );

	const llvm::GlobalVariable* const y= engine->FindGlobalVariableNamed( "y", true );
	U_TEST_ASSERT( y != nullptr );
	U_TEST_ASSERT( y->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !y->hasComdat() );

	const llvm::GlobalVariable* const z= engine->FindGlobalVariableNamed( "z", true );
	U_TEST_ASSERT( z != nullptr );
	U_TEST_ASSERT( z->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !z->hasComdat() );

	const llvm::GlobalVariable* const w= engine->FindGlobalVariableNamed( "w", true );
	U_TEST_ASSERT( w != nullptr );
	U_TEST_ASSERT( w->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !w->hasComdat() );
}

U_TEST( VariableLinkage_Test1 )
{
	// Mutable global variables, that are defined in imported files, should have external linkage.
	// Additionaly comdat must be present in order to merge identical mutable variables in different modules.
	static const char c_program_text_a[]= " var i32 mut x = 0; auto mut y = false; ";
	// Global mutable variables, defined in main files, should have private linkage.
	static const char c_program_text_root[]= "import \"a\" var f32 mut z= 0.0f; auto mut w= \"lol\"; ";

	const auto engine= CreateEngine( BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" ) );

	const llvm::GlobalVariable* const x= engine->FindGlobalVariableNamed( "x", true );
	U_TEST_ASSERT( x != nullptr );
	U_TEST_ASSERT( x->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( x->hasComdat() );

	const llvm::GlobalVariable* const y= engine->FindGlobalVariableNamed( "y", true );
	U_TEST_ASSERT( y != nullptr );
	U_TEST_ASSERT( y->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( y->hasComdat() );

	const llvm::GlobalVariable* const z= engine->FindGlobalVariableNamed( "z", true );
	U_TEST_ASSERT( z != nullptr );
	U_TEST_ASSERT( z->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !z->hasComdat() );

	const llvm::GlobalVariable* const w= engine->FindGlobalVariableNamed( "w", true );
	U_TEST_ASSERT( w != nullptr );
	U_TEST_ASSERT( w->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !w->hasComdat() );
}

U_TEST( PolymorphClassesDataLinkage_Test0 )
{
	// Virtaul functions table should have private linkage.
	static const char c_program_text[]=
	R"(
		class C polymorph {}
	)";

	const auto engine= CreateEngine( BuildProgram( c_program_text ) );

	const llvm::GlobalVariable* const vtable= engine->FindGlobalVariableNamed( "_ZTV1C", true );
	U_TEST_ASSERT( vtable != nullptr );
	U_TEST_ASSERT( vtable->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !vtable->hasComdat() );
}

U_TEST( PolymorphClassesDataLinkage_Test1 )
{
	// Virtaul functions table should have private linkage event for class, declared in imported file.
	static const char c_program_text_a[]= " class C polymorph {} ";
	static const char c_program_text_root[]= "import \"a\" ";

	const auto engine= CreateEngine( BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" ) );

	const llvm::GlobalVariable* const vtable= engine->FindGlobalVariableNamed( "_ZTV1C", true );
	U_TEST_ASSERT( vtable != nullptr );
	U_TEST_ASSERT( vtable->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !vtable->hasComdat() );
}

U_TEST( PolymorphClassesDataLinkage_Test2 )
{
	// Type id table of class, declared in main file, should have private linkage.
	static const char c_program_text[]=
	R"(
		class C polymorph {}
	)";

	const auto engine= CreateEngine( BuildProgram( c_program_text ) );

	const llvm::GlobalVariable* const type_id_table= engine->FindGlobalVariableNamed( "_type_id_for_1C", true );
	U_TEST_ASSERT( type_id_table != nullptr );
	U_TEST_ASSERT( type_id_table->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !type_id_table->hasComdat() );
}

U_TEST( PolymorphClassesDataLinkage_Test3 )
{
	// Type id table of class, declared in imported file, should have external linkage and comdat.
	static const char c_program_text_a[]= " class C polymorph {} ";
	static const char c_program_text_root[]= "import \"a\" ";

	const auto engine= CreateEngine( BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" ) );

	const llvm::GlobalVariable* const type_id_table= engine->FindGlobalVariableNamed( "_type_id_for_1C" );
	U_TEST_ASSERT( type_id_table != nullptr );
	U_TEST_ASSERT( type_id_table->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( type_id_table->hasComdat() );
}

} // namespace

} // namespace U
