#include "tests.hpp"

namespace U
{

U_TEST( ReferenceClassFiledDeclaration )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 value_field;
			i32 mut mut_value_field;
			i32 imut imut_value_field;

			f32 & ref_field;
			f32 &mut mut_ref_field;
			f32 &imut imut_ref_field;
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( BasicReferenceInsideClassUsage )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut r;
		}

		fn Foo() : i32
		{
			auto mut x= 42;
			var S s{ .r= x };
			return s.r;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

} // namespace U
