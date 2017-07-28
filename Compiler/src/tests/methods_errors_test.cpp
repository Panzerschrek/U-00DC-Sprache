#include "tests.hpp"

namespace U
{

U_TEST( BindingConstReferenceToNonconstReference_InThisCallTest0 )
{
	// Call of nonstatic "thiscall" method with mutable this, using immutable object.
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Do( mut this ){}
		}
		fn Foo()
		{
			var S imut s{};
			s.Do();
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.file_pos.line == 9u );
}


U_TEST( BindingConstReferenceToNonconstReference_InThisCallTest1 )
{
	// Call of nonstatic "thiscall" method with mutable this, from method, where "this" is immutable
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn DoImpl( mut this ){}
			fn Do( imut this )
			{
				DoImpl();
			}
		}
		fn Foo()
		{
			var S s{};
			s.Do();
		}
	)";

	const CodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BindingConstReferenceToNonconstReference );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

} // namespace U
