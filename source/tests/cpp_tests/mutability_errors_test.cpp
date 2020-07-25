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
			var i32 &mut a_ref= a;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.file_pos.GetLine() == 5u );
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
	U_TEST_ASSERT( error.file_pos.GetLine() == 4u );
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
	U_TEST_ASSERT( error.file_pos.GetLine() == 6u );
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
	U_TEST_ASSERT( error.file_pos.GetLine() == 7u );
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
	U_TEST_ASSERT( error.file_pos.GetLine() == 7u );
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
	U_TEST_ASSERT( error.file_pos.GetLine() == 8u );
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

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotSelectOverloadedFunction );
	U_TEST_ASSERT( error.file_pos.GetLine() == 8u );
}

U_TEST(ImmutableClassField_Test0)
{
	// Mutating class field via member access operator.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 imut x;
		}
		fn Foo()
		{
			var S mut s {.x= 42 };
			s.x= 34;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.file_pos.GetLine() == 9u );
}

U_TEST(ImmutableClassField_Test1)
{
	// Mutating class field via implicit this.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 imut x;
			fn Foo( mut this )
			{
				x= 34;
			}
		}

	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.file_pos.GetLine() == 7u );
}

U_TEST(ImmutableClassField_Test2)
{
	// Mutating class field via explicit this.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 imut x;
			fn Foo( mut this )
			{
				this.x= 34;
			}
		}

	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.file_pos.GetLine() == 7u );
}


U_TEST(ImmutableClassField_Test3)
{
	// Mutating class field in constructor is forbidden. But initializing must be ok.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 imut x;
			fn constructor( i32 in_x )
			( x= in_x ) // ok
			{
				++x; // error
			}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.file_pos.GetLine() == 8u );
}

U_TEST(ImmutableClassField_Test4)
{
	// Mutating class field in constructor initializer list.
	static const char c_program_text[]=
	R"(
		fn Mutate( i32 &mut x ) : i32
		{
			++x;
			return x;
		}
		struct S
		{
			i32 imut x;
			i32 y;
			fn constructor( i32 in_x )
				(
					x= in_x,
					y( Mutate(x) ) // error, mutating already initialized immutable field.
				)
			{}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::CouldNotSelectOverloadedFunction, 14u ) );
}

} // namespace U
