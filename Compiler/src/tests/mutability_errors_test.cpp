#include "tests.hpp"

namespace U
{

U_TEST(BindingConstReferenceToNonconstReferenceTest0)
{
	// Initialize reference using value-object.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 imut a= 42;
			var i32 &a_ref= a;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST(BindingConstReferenceToNonconstReferenceTest1)
{
	// Initialize reference using value-object.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x ){}
		fn Foo()
		{
			var i32 imut x = 0;
			Bar( x );
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST(BindingConstReferenceToNonconstReferenceTest2)
{
	// Return reference, when return value is const reference.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &imut x ) : i32 &mut
		{
			return x;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST(BindingConstReferenceToNonconstReferenceTest3)
{
	// Binding "imut this" to mutable reference.
	static const char c_program_text[]=
	R"(
		struct A
		{
			fn MutateThis( imut this )
			{
				var A& mut this_mut= this;
			}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST(BindingConstReferenceToNonconstReferenceTest4)
{
	// Call "mutable this" method from method with "immutable this".
	static const char c_program_text[]=
	R"(
		struct A
		{
			fn Bar( mut this )
			{}
			fn Foo( imut this )
			{
				Bar();
			}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.file_pos.line == 8u );
}

U_TEST(BindingConstReferenceToNonconstReferenceTest5)
{
	// Try mutate member.
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			fn Foo( imut this )
			{
				var i32& mut x_mut= x;
			}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST(BindingConstReferenceToNonconstReferenceTest6)
{
	// Try mutate array member.
	static const char c_program_text[]=
	R"(
		struct A
		{
			[ i32, 5 ] x;
			fn Foo( imut this )
			{
				var i32& mut x_mut= x[0u];
			}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST(BindingConstReferenceToNonconstReferenceTest7)
{
	// Try mutate member of member member.
	static const char c_program_text[]=
	R"(
		struct BB{ i32 x; }
		struct A
		{
			BB bb;
			fn Foo( imut this )
			{
				var i32& mut x_mut= bb.x;
			}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.file_pos.line == 8u );
}

U_TEST(BindingConstReferenceToNonconstReferenceTest8)
{
	// Try call method with "mutable this" for field, inside "immutable this" method.
	static const char c_program_text[]=
	R"(
		struct B{ fn Mutate( mut this ){} }
		struct A
		{
			B b;
			fn Foo( imut this )
			{
				b.Mutate();
			}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.file_pos.line == 8u );
}

} // namespace U
