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
	U_TEST_ASSERT( foo->getVisibility() == llvm::GlobalValue::HiddenVisibility );

	const llvm::Function* const baz= module->getFunction( "_ZN2NN3BazEv" );
	U_TEST_ASSERT( baz != nullptr );
	U_TEST_ASSERT( baz->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( !baz->hasComdat() );
	U_TEST_ASSERT( baz->getVisibility() == llvm::GlobalValue::HiddenVisibility );

	const llvm::Function* const bar= module->getFunction( "_ZN1S3BarERKS_" );
	U_TEST_ASSERT( bar != nullptr );
	U_TEST_ASSERT( bar->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( !bar->hasComdat() );
	U_TEST_ASSERT( bar->getVisibility() == llvm::GlobalValue::HiddenVisibility );

	const llvm::Function* const static_method= module->getFunction( "_ZN1S12StaticMethodEv" );
	U_TEST_ASSERT( static_method != nullptr );
	U_TEST_ASSERT( static_method->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( !static_method->hasComdat() );
	U_TEST_ASSERT( static_method->getVisibility() == llvm::GlobalValue::HiddenVisibility );
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
	U_TEST_ASSERT( bar->getVisibility() == llvm::GlobalValue::HiddenVisibility );
}

U_TEST( FunctionLinkage_Test4 )
{
	static const char c_program_text_a[]= R"(
		?macro <? DECLARE_FOO:namespace ?> -> <? fn Foo(); ?>
		)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		// Function prototype declared via imported macro expansion is stil assumed to be in the file with macro expansion.
		// This prevents this function to be external.
		DECLARE_FOO
		fn Foo(){}
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
}

U_TEST( LambdasLinkage_Test0 )
{
	// All lambda methods should be private.

	static const char c_program_text[]=
	R"(
		fn Foo( i32 x, i32 y ) : i32
		{
			auto f= lambda[=]( i32 a ) : i32 { return x * a; };
			return f( y );
		}
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const call_operator= module->getFunction( "_ZN46_lambda_320db96fe11148d72174af6aff172f7b_4_11_clERKS_i" );
	U_TEST_ASSERT( call_operator != nullptr );
	U_TEST_ASSERT( call_operator->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const destructor= module->getFunction( "_ZN46_lambda_320db96fe11148d72174af6aff172f7b_4_11_10destructorERS_" );
	U_TEST_ASSERT( destructor != nullptr );
	U_TEST_ASSERT( destructor->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const copy_constructor= module->getFunction( "_ZN46_lambda_320db96fe11148d72174af6aff172f7b_4_11_11constructorERS_RKS_" );
	U_TEST_ASSERT( copy_constructor != nullptr );
	U_TEST_ASSERT( copy_constructor->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const copy_assignment_operator= module->getFunction( "_ZN46_lambda_320db96fe11148d72174af6aff172f7b_4_11_aSERS_RKS_" );
	U_TEST_ASSERT( copy_assignment_operator != nullptr );
	U_TEST_ASSERT( copy_assignment_operator->getLinkage() == llvm::GlobalValue::PrivateLinkage );
}

U_TEST( LambdasLinkage_Test1 )
{
	// All lambda methods should be private, even for lambda in global space.
	static const char c_program_text[]=
	R"(
		auto f= lambda(){};
	)";

	const auto module= BuildProgram( c_program_text );

	const llvm::Function* const call_operator= module->getFunction( "_ZN46_lambda_bd6f49d675a76049d075b67bcb1073e2_2_10_clERKS_" );
	U_TEST_ASSERT( call_operator != nullptr );
	U_TEST_ASSERT( call_operator->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const destructor= module->getFunction( "_ZN46_lambda_bd6f49d675a76049d075b67bcb1073e2_2_10_10destructorERS_" );
	U_TEST_ASSERT( destructor != nullptr );
	U_TEST_ASSERT( destructor->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const copy_constructor= module->getFunction( "_ZN46_lambda_bd6f49d675a76049d075b67bcb1073e2_2_10_11constructorERS_RKS_" );
	U_TEST_ASSERT( copy_constructor != nullptr );
	U_TEST_ASSERT( copy_constructor->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const copy_assignment_operator= module->getFunction( "_ZN46_lambda_bd6f49d675a76049d075b67bcb1073e2_2_10_aSERS_RKS_" );
	U_TEST_ASSERT( copy_assignment_operator != nullptr );
	U_TEST_ASSERT( copy_assignment_operator->getLinkage() == llvm::GlobalValue::PrivateLinkage );
}

U_TEST( LambdasLinkage_Test2 )
{
	// All lambda methods should be private, even if lambda is in imported file.

	static const char c_program_text_a[]=
	R"(
		fn Foo( i32 x, i32 y ) : i32
		{
			auto f= lambda[=]( i32 a ) : i32 { return x * a; };
			return f( y );
		}
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
	)";

	const auto module= BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" );

	const llvm::Function* const call_operator= module->getFunction( "_ZN46_lambda_320db96fe11148d72174af6aff172f7b_4_11_clERKS_i" );
	U_TEST_ASSERT( call_operator != nullptr );
	U_TEST_ASSERT( call_operator->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const destructor= module->getFunction( "_ZN46_lambda_320db96fe11148d72174af6aff172f7b_4_11_10destructorERS_" );
	U_TEST_ASSERT( destructor != nullptr );
	U_TEST_ASSERT( destructor->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const copy_constructor= module->getFunction( "_ZN46_lambda_320db96fe11148d72174af6aff172f7b_4_11_11constructorERS_RKS_" );
	U_TEST_ASSERT( copy_constructor != nullptr );
	U_TEST_ASSERT( copy_constructor->getLinkage() == llvm::GlobalValue::PrivateLinkage );

	const llvm::Function* const copy_assignment_operator= module->getFunction( "_ZN46_lambda_320db96fe11148d72174af6aff172f7b_4_11_aSERS_RKS_" );
	U_TEST_ASSERT( copy_assignment_operator != nullptr );
	U_TEST_ASSERT( copy_assignment_operator->getLinkage() == llvm::GlobalValue::PrivateLinkage );
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
	// Global mutable variables defined in main files should also have external linkage, comdat and private visibility.
	// All names of global mutable variables should also contain suffix - file path contents.
	static const char c_program_text_a[]= " var i32 mut x = 0; auto mut y = false; ";
	static const char c_program_text_root[]= "import \"a\" var f32 mut z= 0.0f; auto mut w= \"lol\"; ";

	const auto engine= CreateEngine( BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" ) );

	const llvm::GlobalVariable* const x= engine->FindGlobalVariableNamed( "x.0cc175b9c0f1b6a831c399e269772661", true );
	U_TEST_ASSERT( x != nullptr );
	U_TEST_ASSERT( x->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( x->hasComdat() );
	U_TEST_ASSERT( x->getVisibility() == llvm::GlobalValue::HiddenVisibility );

	const llvm::GlobalVariable* const y= engine->FindGlobalVariableNamed( "y.0cc175b9c0f1b6a831c399e269772661", true );
	U_TEST_ASSERT( y != nullptr );
	U_TEST_ASSERT( y->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( y->hasComdat() );
	U_TEST_ASSERT( y->getVisibility() == llvm::GlobalValue::HiddenVisibility );

	const llvm::GlobalVariable* const z= engine->FindGlobalVariableNamed( "z.63a9f0ea7bb98050796b649e85481845", true );
	U_TEST_ASSERT( z != nullptr );
	U_TEST_ASSERT( z->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( z->hasComdat() );
	U_TEST_ASSERT( z->getVisibility() == llvm::GlobalValue::HiddenVisibility );

	const llvm::GlobalVariable* const w= engine->FindGlobalVariableNamed( "w.63a9f0ea7bb98050796b649e85481845", true );
	U_TEST_ASSERT( w != nullptr );
	U_TEST_ASSERT( w->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( w->hasComdat() );
	U_TEST_ASSERT( w->getVisibility() == llvm::GlobalValue::HiddenVisibility );
}

U_TEST( VariableLinkage_Test2 )
{
	// Mutable variables defined via imported macro expansion should have prefix based on main file,
	// because technically it is defined inside main file.
	static const char c_program_text_a[]= "?macro <? DEFINE_VAR:namespace ?> -> <? auto mut some_var= 0; ?> ";
	static const char c_program_text_root[]= " import \"a\" DEFINE_VAR ";

	const auto engine= CreateEngine( BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root", false ) );

	const llvm::GlobalVariable* const some_var= engine->FindGlobalVariableNamed( "some_var.63a9f0ea7bb98050796b649e85481845", true );
	U_TEST_ASSERT( some_var != nullptr );
	U_TEST_ASSERT( some_var->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( some_var->hasComdat() );
	U_TEST_ASSERT( some_var->getVisibility() == llvm::GlobalValue::HiddenVisibility );
}

U_TEST( VariableLinkage_Test3 )
{
	// Mutable global variables, that are defined in imported files, should have external linkage.
	// Additionaly comdat must be present in order to merge identical mutable variables in different modules.
	// This works also for global mutable variables within templates.
	static const char c_program_text_a[]=
	R"(
		template</type T/> struct Box
		{
			// Has external linkage for all instantiations.
			var T mut global_template_var(444);
		}

		type IntBox= Box</i32/>;
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"

		type DoubleBox= Box</f64/>;

		template</type T/> struct LocalBox
		{
			// Has private linkage for all instantiations.
			var T mut global_template_var(444);
		}

		type LocalU32Box= LocalBox</u32/>;
	)";

	const auto engine= CreateEngine( BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" ) );

	const llvm::GlobalVariable* const x_i32= engine->FindGlobalVariableNamed( "_ZN3BoxIiE19global_template_varE.0cc175b9c0f1b6a831c399e269772661", true );
	U_TEST_ASSERT( x_i32 != nullptr );
	U_TEST_ASSERT( x_i32->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( x_i32->hasComdat() );
	U_TEST_ASSERT( x_i32->getVisibility() == llvm::GlobalValue::HiddenVisibility );

	const llvm::GlobalVariable* const x_f64= engine->FindGlobalVariableNamed( "_ZN3BoxIdE19global_template_varE.0cc175b9c0f1b6a831c399e269772661", true );
	U_TEST_ASSERT( x_f64 != nullptr );
	U_TEST_ASSERT( x_f64->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( x_f64->hasComdat() );
	U_TEST_ASSERT( x_f64->getVisibility() == llvm::GlobalValue::HiddenVisibility );

	const llvm::GlobalVariable* const x_local= engine->FindGlobalVariableNamed( "_ZN8LocalBoxIjE19global_template_varE.63a9f0ea7bb98050796b649e85481845", true );
	U_TEST_ASSERT( x_local != nullptr );
	U_TEST_ASSERT( x_local->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( x_local->hasComdat() );
	U_TEST_ASSERT( x_local->getVisibility() == llvm::GlobalValue::HiddenVisibility );
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
	// Type id table of class, declared in main file, should have external linkage and comdat.
	static const char c_program_text[]=
	R"(
		class C polymorph {}
	)";

	const auto engine= CreateEngine( BuildProgram( c_program_text ) );

	const llvm::GlobalVariable* const type_id_table= engine->FindGlobalVariableNamed( "_type_id_for_1C.b14a7b8059d9c055954c92674ce60032", true );
	U_TEST_ASSERT( type_id_table != nullptr );
	U_TEST_ASSERT( type_id_table->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( type_id_table->hasComdat() );
	U_TEST_ASSERT( type_id_table->getVisibility() == llvm::GlobalValue::HiddenVisibility );
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

	const llvm::GlobalVariable* const type_id_table= engine->FindGlobalVariableNamed( "_type_id_for_1C.0cc175b9c0f1b6a831c399e269772661" );
	U_TEST_ASSERT( type_id_table != nullptr );
	U_TEST_ASSERT( type_id_table->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( type_id_table->hasComdat() );
	U_TEST_ASSERT( type_id_table->getVisibility() == llvm::GlobalValue::HiddenVisibility );
}

U_TEST( PolymorphClassesDataLinkage_Test4 )
{
	// Class defined in imported macro expansion is assumed to be private. But type id table is still external and has comdat.
	static const char c_program_text_a[]= "?macro <? DEFINE_CLASS:namespace ?> -> <? class C polymorph {} ?> ";
	static const char c_program_text_root[]= " import \"a\" DEFINE_CLASS ";

	const auto engine= CreateEngine( BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" ) );

	const llvm::GlobalVariable* const type_id_table= engine->FindGlobalVariableNamed( "_type_id_for_1C.63a9f0ea7bb98050796b649e85481845", true );
	U_TEST_ASSERT( type_id_table != nullptr );
	U_TEST_ASSERT( type_id_table->getLinkage() == llvm::GlobalValue::ExternalLinkage );
	U_TEST_ASSERT( type_id_table->hasComdat() );
	U_TEST_ASSERT( type_id_table->getVisibility() == llvm::GlobalValue::HiddenVisibility );

	// Vtable is always private, since it's contant and deduplication isn't necessary.
	const llvm::GlobalVariable* const vtable= engine->FindGlobalVariableNamed( "_ZTV1C", true );
	U_TEST_ASSERT( vtable != nullptr );
	U_TEST_ASSERT( vtable->getLinkage() == llvm::GlobalValue::PrivateLinkage );
	U_TEST_ASSERT( !vtable->hasComdat() );
}

} // namespace

} // namespace U
