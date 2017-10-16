#include "tests.hpp"

namespace U
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

	ICodeBuilder::BuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a"_SpC, c_program_text_a },
				{ "b"_SpC, c_program_text_b },
				{ "c"_SpC, c_program_text_c },
				{ "root"_SpC, c_program_text_root }
			},
			"root"_SpC );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( result.errors[0u].file_pos.line == 5u );
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

	ICodeBuilder::BuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a"_SpC, c_program_text_a },
				{ "b"_SpC, c_program_text_b },
				{ "c"_SpC, c_program_text_c },
				{ "root"_SpC, c_program_text_root }
			},
			"root"_SpC );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( result.errors[0u].file_pos.line == 7u );
}

U_TEST( ImportedClassShouldNotBeModified_Test0 )
{
	static const char c_program_text_a[]=
	R"(
		struct A;
	)";

	static const char c_program_text_b[]=
	R"(
		import "a"
		struct A{} // Extend imported class. Imported class must not be affected.
	)";

	static const char c_program_text_c[]=
	R"(
		import "a"
		fn Foo()
		{
			var A vvv; // "A" must be incomplete at this point.
		}
	)";

	static const char c_program_text_root[]=
	R"(
		import "b"
		import "c"
	)";

	ICodeBuilder::BuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a"_SpC, c_program_text_a },
				{ "b"_SpC, c_program_text_b },
				{ "c"_SpC, c_program_text_c },
				{ "root"_SpC, c_program_text_root }
			},
			"root"_SpC );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( result.errors[0u].file_pos.line == 5u );
}

U_TEST( ImportedClassShouldNotBeModified_Test1 )
{
	static const char c_program_text_a[]=
	R"(
		struct A{ struct B; }
	)";

	static const char c_program_text_b[]=
	R"(
		import "a"
		struct A::B{} // Extend imported class. Imported class must not be affected.
	)";

	static const char c_program_text_c[]=
	R"(
		import "a"
		fn Foo()
		{
			var A::B vvv; // "A::B" must be incomplete at this point.
		}
	)";

	static const char c_program_text_root[]=
	R"(
		import "b"
		import "c"
	)";

	ICodeBuilder::BuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a"_SpC, c_program_text_a },
				{ "b"_SpC, c_program_text_b },
				{ "c"_SpC, c_program_text_c },
				{ "root"_SpC, c_program_text_root }
			},
			"root"_SpC );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::UsingIncompleteType );
	U_TEST_ASSERT( result.errors[0u].file_pos.line == 5u );
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

	ICodeBuilder::BuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a"_SpC, c_program_text_a },
				{ "b"_SpC, c_program_text_b },
				{ "root"_SpC, c_program_text_root }
			},
			"root"_SpC );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::FunctionPrototypeDuplication );
	U_TEST_ASSERT( result.errors[0u].file_pos.line == 2u );
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

	ICodeBuilder::BuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a"_SpC, c_program_text_a },
				{ "b"_SpC, c_program_text_b },
				{ "root"_SpC, c_program_text_root }
			},
			"root"_SpC );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::FunctionPrototypeDuplication );
	U_TEST_ASSERT( result.errors[0u].file_pos.line == 2u );
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

	ICodeBuilder::BuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a"_SpC, c_program_text_a },
				{ "b"_SpC, c_program_text_b },
				{ "root"_SpC, c_program_text_root }
			},
			"root"_SpC );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::FunctionPrototypeDuplication );
	U_TEST_ASSERT( result.errors[0u].file_pos.line == 2u );
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

	ICodeBuilder::BuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a"_SpC, c_program_text_a },
				{ "b"_SpC, c_program_text_b },
				{ "c"_SpC, c_program_text_c },
				{ "root"_SpC, c_program_text_root }
			},
			"root"_SpC );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::FunctionBodyDuplication );
	U_TEST_ASSERT( result.errors[0u].file_pos.line == 3u );
}

U_TEST( ClassPrototypeDuplication_ForImports_Test0 )
{
	// Class prototype defined in different files.

	static const char c_program_text_a[]=
	R"(
		struct FRT;
	)";

	static const char c_program_text_b[]=
	R"(
		struct FRT;
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		import "b"
	)";

	ICodeBuilder::BuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a"_SpC, c_program_text_a },
				{ "b"_SpC, c_program_text_b },
				{ "root"_SpC, c_program_text_root }
			},
			"root"_SpC );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( result.errors[0u].file_pos.line == 2u );
}

U_TEST( ClassPrototypeDuplication_ForImports_Test1 )
{
	// File with class body does not imports file with prototype.

	static const char c_program_text_a[]=
	R"(
		struct FRT;
	)";

	static const char c_program_text_b[]=
	R"(
		struct FRT{}
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		import "b"
	)";

	ICodeBuilder::BuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a"_SpC, c_program_text_a },
				{ "b"_SpC, c_program_text_b },
				{ "root"_SpC, c_program_text_root }
			},
			"root"_SpC );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( result.errors[0u].file_pos.line == 2u );
}

U_TEST( ClassBodyDuplication_ForImports_Test0 )
{
	// Body of class defined in different files.

	static const char c_program_text_a[]=
	R"(
		struct ODR_BREAKER;
	)";

	static const char c_program_text_b[]=
	R"(
		import "a"
		struct ODR_BREAKER{}
	)";

	static const char c_program_text_c[]=
	R"(
		import "a"
		struct ODR_BREAKER{}
	)";

	static const char c_program_text_root[]=
	R"(
		import "b"
		import "c"
	)";

	ICodeBuilder::BuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "a"_SpC, c_program_text_a },
				{ "b"_SpC, c_program_text_b },
				{ "c"_SpC, c_program_text_c },
				{ "root"_SpC, c_program_text_root }
			},
			"root"_SpC );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( result.errors[0u].code == CodeBuilderErrorCode::ClassBodyDuplication );
	U_TEST_ASSERT( result.errors[0u].file_pos.line == 3u );
}

} // namespace U
