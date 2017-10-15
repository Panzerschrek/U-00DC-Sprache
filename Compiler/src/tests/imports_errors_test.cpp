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

} // namespace U
