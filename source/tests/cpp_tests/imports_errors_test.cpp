#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( ImportedNamespaceShouldNotBeModified_Test0 )
{
	static const char c_program_text_a[]=
	R"(
		namespace XX{}
	)";

	static const char c_program_text_b[]=
	R"(
		import "a"
		namespace XX // Extend imported namespace. Imported namespace must not be affected.
		{
			fn Foo() {}
		}
	)";

	static const char c_program_text_c[]=
	R"(
		import "a"
		namespace XX
		{
			fn Baz() { Foo(); } // "Foo" from file "b" should not be visible.
		}
	)";

	static const char c_program_text_root[]=
	R"(
		import "b"
		import "c"
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "c", c_program_text_c },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( result.errors[0u].src_loc.GetLine() == 5u );
}

U_TEST( ImportedNamespaceShouldNotBeModified_Test1 )
{
	static const char c_program_text_a[]=
	R"(
		namespace YY{ namespace XX{} }
	)";

	static const char c_program_text_b[]=
	R"(
		import "a"
		namespace YY
		{
			namespace XX // Extend imported inner namespace. Imported namespace must not be affected.
			{
				fn Foo() {}
			}
		}
	)";

	static const char c_program_text_c[]=
	R"(
		import "a"
		namespace YY
		{
			namespace XX
			{
				fn Baz() { Foo(); } // "Foo" from file "b" should not be visible.
			}
		}
	)";

	static const char c_program_text_root[]=
	R"(
		import "b"
		import "c"
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "c", c_program_text_c },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( result.errors[0u].src_loc.GetLine() == 7u );
}

U_TEST( FunctionPrototypeDuplication_ForImports_Test0 )
{
	// Function prototype defined in different files.

	static const char c_program_text_a[]=
	R"(
		fn Foo();
	)";

	static const char c_program_text_b[]=
	R"(
		fn Foo();
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		import "b"
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::FunctionPrototypeDuplication );
	U_TEST_ASSERT( result.errors[0u].src_loc.GetLine() == 2u );
}

U_TEST( FunctionPrototypeDuplication_ForImports_Test1 )
{
	// File with function body does not imports file with prototype.

	static const char c_program_text_a[]=
	R"(
		fn Foo();
	)";

	static const char c_program_text_b[]=
	R"(
		fn Foo(){}
	)";

	static const char c_program_text_root[]=
	R"(
		import "a" // Prototype
		import "b" // than function with body
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::FunctionPrototypeDuplication );
	U_TEST_ASSERT( result.errors[0u].src_loc.GetLine() == 2u );
}

U_TEST( FunctionPrototypeDuplication_ForImports_Test2 )
{
	// File with function body does not imports file with prototype.

	static const char c_program_text_a[]=
	R"(
		fn Foo();
	)";

	static const char c_program_text_b[]=
	R"(
		fn Foo(){}
	)";

	static const char c_program_text_root[]=
	R"(
		import "b" // Function with body
		import "a" // than prototype
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::FunctionPrototypeDuplication );
	U_TEST_ASSERT( result.errors[0u].src_loc.GetLine() == 2u );
}

U_TEST( FunctionBodyDuplication_ForImports_Test0 )
{
	// Body of function defined in different files.

	static const char c_program_text_a[]=
	R"(
		fn Foo();
	)";

	static const char c_program_text_b[]=
	R"(
		import "a"
		fn Foo(){}
	)";

	static const char c_program_text_c[]=
	R"(
		import "a"
		fn Foo(){}
	)";

	static const char c_program_text_root[]=
	R"(
		import "b"
		import "c"
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "c", c_program_text_c },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::FunctionBodyDuplication );
	U_TEST_ASSERT( result.errors[0u].src_loc.GetLine() == 3u );
}

U_TEST( CouldNotOverloadFunction_ForImports_Test0 )
{
	// Function overloading fails during imports merge.
	static const char c_program_text_a[]=
	R"(
		fn Foo( i32  mut x );
	)";

	static const char c_program_text_b[]=
	R"(
		fn Foo( i32 imut x );
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		import "b"
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::FunctionPrototypeDuplication );
	U_TEST_ASSERT( result.errors[0u].src_loc.GetLine() == 2u );
}

U_TEST( Redefineition_ForImports_Test0 )
{
	// Redefinition of variable.

	static const char c_program_text_a[]=
	R"(
		auto constexpr s= 64;
	)";

	static const char c_program_text_b[]=
	R"(
		var i32 constexpr s= 0;
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		import "b"
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( result.errors[0u].src_loc.GetLine() == 2u );
}

U_TEST( Redefineition_ForImports_Test1 )
{
	// Redefinition of type alias for same type.

	static const char c_program_text_a[]=
	R"(
		type size= u32;
	)";

	static const char c_program_text_b[]=
	R"(
		type size= u32;
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		import "b"
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( result.errors[0u].src_loc.GetLine() == 2u );
}

U_TEST( Redefineition_ForImports_Test2 )
{
	// Redefinition of type alias fo different types.

	static const char c_program_text_a[]=
	R"(
		type size= u32;
	)";

	static const char c_program_text_b[]=
	R"(
		type size= u64;
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		import "b"
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( result.errors[0u].src_loc.GetLine() == 2u );
}

U_TEST( Redefineition_ForImports_Test3 )
{
	// Redefinition - different kinds.

	static const char c_program_text_a[]=
	R"(
		struct PI{}
	)";

	static const char c_program_text_b[]=
	R"(
		auto constexpr PI= 3.1415926535;
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		import "b"
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( result.errors[0u].src_loc.GetLine() == 2u );
}

U_TEST( Redefineition_ForImports_Test4 )
{
	// Redefinition - different kinds.

	static const char c_program_text_a[]=
	R"(
		fn X(){}
	)";

	static const char c_program_text_b[]=
	R"(
		auto constexpr X= 3.1415926535;
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		import "b"
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( result.errors[0u].src_loc.GetLine() == 2u );
}

U_TEST( Redefineition_ForImports_Test5 )
{
	// Redefinition - different types.

	static const char c_program_text_a[]=
	R"(
		struct X{}
	)";

	static const char c_program_text_b[]=
	R"(
		type X= i8;
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		import "b"
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( result.errors[0u].src_loc.GetLine() == 2u );
}

U_TEST( Redefineition_ForImports_Test6 )
{
	// Redefinition - different structs.

	static const char c_program_text_a[]=
	R"(
		struct A{}
	)";

	static const char c_program_text_b[]=
	R"(
		class A{}
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		import "b"
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( result.errors[0u].src_loc.GetLine() == 2u );
}

U_TEST( TypeTemplateRedefinition_ForImports_Test0 )
{
	// Redefinition - same type template.

	static const char c_program_text_a[]=
	R"(
		template</type U/> struct S{}
	)";

	static const char c_program_text_b[]=
	R"(
		template</type V/> struct S{}
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		import "b"
	)";

	const ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( HaveError( result.errors, CodeBuilderErrorCode::TypeTemplateRedefinition, 2u ) );
}

U_TEST( TypeTemplateRedefinition_ForImports_Test1 )
{
	static const char c_program_text_a[]=
	R"(
		template</type U/> struct S{}
	)";

	static const char c_program_text_b[]=
	R"(
		import "a"
	)";

	static const char c_program_text_c[]=
	R"(
		import "a"
	)";

	static const char c_program_text_root[]=
	R"(
		// Import same type template from "a" via "b" and "c"
		import "b"
		import "c"
	)";

	BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "b", c_program_text_b },
			{ "c", c_program_text_c },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( TypeTemplateRedefinition_ForImports_Test2 )
{
	static const char c_program_text_a[]=
	R"(
		template</type U/> struct S{}
	)";

	static const char c_program_text_b[]=
	R"(
		template</type U, type V/> struct S{}
	)";

	static const char c_program_text_root[]=
	R"(
		// Ok - take two diffetent type templates from imporded files.
		import "a"
		import "b"
		type T0= S</i32/>;
		type T1= S</f32, i8/>;
	)";

	BuildMultisourceProgram(
		{
			{ "a", c_program_text_a },
			{ "b", c_program_text_b },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( DefineBodyForFunction_UsingChildClassName_Test0 )
{
	static const char c_program_text_a[]=
	R"(
		class A polymorph
		{
			fn Foo();
		}
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"

		class B : A {}

		fn B::Foo(){} // Error, no native function named "Foo" in "B".
	)";

	const ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( HaveError( result.errors, CodeBuilderErrorCode::NameNotFound, 6u ) );
}

U_TEST( DefineBodyForFunction_UsingChildClassName_Test1 )
{
	static const char c_program_text_a[]=
	R"(
		class A polymorph
		{
			fn Foo();
		}
	)";

	static const char c_program_text_b[]=
	R"(
		import "a"
		class B : A
		{
			fn Foo(i32 x);
		}
	)";

	static const char c_program_text_root[]=
	R"(
		import "b"

		fn B::Foo(){} // Error, no native function Foo() in B, only Foo(i32 x)
	)";

	const ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a", c_program_text_a },
				{ "b", c_program_text_b },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( HaveError( result.errors, CodeBuilderErrorCode::FunctionDeclarationOutsideItsScope, 4u ) );
}

} // namespace

} // namespace U
