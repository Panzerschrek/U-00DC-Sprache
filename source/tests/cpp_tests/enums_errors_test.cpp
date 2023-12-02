#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( UsingKeywordAsName_ForEnum_Test0 )
{
	static const char c_program_text[]=
	R"(
		enum i128 { A }
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::UsingKeywordAsName, 2u ) );
}

U_TEST( UsingKeywordAsName_ForEnum_Test1 )
{
	static const char c_program_text[]=
	R"(
		enum E
		{
			async,
			fn,
			nomangle,
			call_conv,
			struct,
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::UsingKeywordAsName, 4u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::UsingKeywordAsName, 5u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::UsingKeywordAsName, 6u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::UsingKeywordAsName, 7u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::UsingKeywordAsName, 8u ) );
}

U_TEST( Redefinition_ForEnums )
{
	static const char c_program_text[]=
	R"(
		struct S{}
		enum S{ A }
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( Redefinition_ForEnumMembers )
{
	static const char c_program_text[]=
	R"(
		enum E
		{
			A, B, C,
			B,
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( EnumsRestrictionsTest )
{
	static const char c_program_text[]=
	R"(
		enum E{ a, b }
		fn Foo()
		{
			E::a + E::b; // basic arithmetic operations for enums forbidden
			-E::a; // unary minus for enums forbidden
			var i32 x= E::a; // Implicit conversion to int forbidden
			var E mut y= E::b;
			y+= E::a; // additive assignment operations forbidden
			E::a | E::b; // Logical operations forbidden
			E::a && E::b; // Lazy logical operations forbidden
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.size() >= 6u );

	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[0].src_loc.GetLine() == 5u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[1].src_loc.GetLine() == 6u );
	U_TEST_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( build_result.errors[2].src_loc.GetLine() == 7u );
	U_TEST_ASSERT( build_result.errors[3].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[3].src_loc.GetLine() == 9u );
	U_TEST_ASSERT( build_result.errors[4].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[4].src_loc.GetLine() == 10u );
	U_TEST_ASSERT( build_result.errors[5].code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( build_result.errors[5].src_loc.GetLine() == 11u );
}

U_TEST( NameNotFound_ForUnderlyingEnumType_Test )
{
	static const char c_program_text[]=
	R"(
		enum E : wtf_where_is_type_name
		{
			A, B, C,
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::NameNotFound, 2u ) );
}

U_TEST( NameNotFound_ForEnumElement )
{
	static const char c_program_text[]=
	R"(
		enum E{ A, B, C }
		fn Foo(){ var E x= E::D; }
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::NameNotFound, 3u ) );
}

U_TEST( NameIsNotTypeName_ForUnderlyingEnumType_Test )
{
	static const char c_program_text[]=
	R"(
		fn Foo(){}
		enum E : Foo
		{
			A, B, C,
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::NameIsNotTypeName, 3u ) );
}

U_TEST( TypesMismatch_ForUnderlyingEnumType_Test0 )
{
	// Float as underlying type
	static const char c_program_text[]=
	R"(
		enum E : f32
		{
			A, B, C,
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( TypesMismatch_ForUnderlyingEnumType_Test1 )
{
	// Bool as underlying type
	static const char c_program_text[]=
	R"(
		enum E : bool
		{
			A, B, C,
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( TypesMismatch_ForUnderlyingEnumType_Test2 )
{
	// struct as underlying type
	static const char c_program_text[]=
	R"(
		struct S{}
		enum E : S
		{
			A, B, C,
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( UnderlyingTypeIsTooSmall_Test )
{
	static const char c_program_text[]=
	R"(
		enum E : i8
		{
			V00, V01, V02, V03, V04, V05, V06, V07, V08, V09, V0A, V0B, V0C, V0D, V0E, V0F,
			V10, V11, V12, V13, V14, V15, V16, V17, V18, V19, V1A, V1B, V1C, V1D, V1E, V1F,
			V20, V21, V22, V23, V24, V25, V26, V27, V28, V29, V2A, V2B, V2C, V2D, V2E, V2F,
			V30, V31, V32, V33, V34, V35, V36, V37, V38, V39, V3A, V3B, V3C, V3D, V3E, V3F,
			V40, V41, V42, V43, V44, V45, V46, V47, V48, V49, V4A, V4B, V4C, V4D, V4E, V4F,
			V50, V51, V52, V53, V54, V55, V56, V57, V58, V59, V5A, V5B, V5C, V5D, V5E, V5F,
			V60, V61, V62, V63, V64, V65, V66, V67, V68, V69, V6A, V6B, V6C, V6D, V6E, V6F,
			V70, V71, V72, V73, V74, V75, V76, V77, V78, V79, V7A, V7B, V7C, V7D, V7E, V7F,
			Too, Much, Values, In, This, Enum,
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnderlyingTypeForEnumIsTooSmall );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

} // namespace

} // namespace U
