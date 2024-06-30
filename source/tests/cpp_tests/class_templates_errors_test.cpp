#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( InvalidValueAsTemplateArgumentTest0 )
{
	static const char c_program_text[]=
	R"(
		fn Bar(){}
		template</ i32 x /> struct S</ x />{}
		fn Foo()
		{
			var S</ Bar /> s; // Overloaded functions set as template argument not allowed.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::InvalidValueAsTemplateArgument, 6u ) );
}

U_TEST( InvalidValueAsTemplateArgumentTest1 )
{
	static const char c_program_text[]=
	R"(
		fn Bar(){}
		template</  /> struct S</ Bar />{}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	bool have_error= false;
	for( const auto& e : build_result.errors )
		have_error|= e.code == CodeBuilderErrorCode::InvalidValueAsTemplateArgument;
	U_TEST_ASSERT( have_error );
}

U_TEST( InvalidTypeOfTemplateVariableArgumentTest0 )
{
	static const char c_program_text[]=
	R"(
		template</ i32 x /> struct S</ x />{}
		fn Foo()
		{
			var S</ 0.5f /> s; // float type template arguments forbidden.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::InvalidTypeOfTemplateVariableArgument, 5u ) );
}

U_TEST( InvalidTypeOfTemplateVariableArgumentTest1 )
{
	static const char c_program_text[]=
	R"(
		template</ f32 x /> struct S{}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::InvalidTypeOfTemplateVariableArgument, 2u ) );
}

U_TEST( InvalidTypeOfTemplateVariableArgumentTest2 )
{
	static const char c_program_text[]=
	R"(
		template<//> struct S</ 0.25 />{}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::InvalidTypeOfTemplateVariableArgument, 2u ) );
}

U_TEST( InvalidTypeOfTemplateVariableArgumentTest3 )
{
	static const char c_program_text[]=
	R"(
		template</type T, T Val/> struct S</ Val />{}
		type X= S</ 13.5f />; // Pass invalid type variable in template instantiation.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::InvalidTypeOfTemplateVariableArgument, 3u ) );
}

U_TEST( NameNotFound_ForClassTemplateSingatureArguments_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</  /> class CC</ SSSS /> {} // Name in signature argument not known yet.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::NameNotFound , 2u ) );
}

U_TEST( NameNotFound_ForClassTemplateArguments_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ SSS xx /> class CC</ xx /> {} // Name in type of value-argument not known yet.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::NameNotFound, 2u ) );
}

U_TEST( NameNotFound_ForClassTemplateDefaultSignatureArguments_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> class CC</ T= SSS /> {} // Name of default signature argument not known here yet.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::NameNotFound , 2u ) );
}

U_TEST( NameNotFound_ForClassTemplateArgLookup0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		class C{}

		type SomeT= C</i32/>::T; // Can't access "T" here, because template args are located outside class namespace.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::NameNotFound , 5u ) );
}

U_TEST( NameNotFound_ForClassTemplateArgLookup1 )
{
	static const char c_program_text[]=
	R"(
		template</ size_type S />
		struct C</S/>{}

		auto SomeVar= C</78s/>::S; // Can't access "T" here, because template args are located outside class namespace.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::NameNotFound , 5u ) );
}

U_TEST( ValueIsNotTemplateTest0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			auto this_is_not_template= 0;
			var this_is_not_template</ i32 /> some_var;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ValueIsNotTemplate, 5u ) );
}

U_TEST( TemplateInstantiationRequiredTest0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Box</ T /> { struct Tag{} T t; }

		fn Foo()
		{
			var Box::Tag x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateInstantiationRequired, 6u ) );
}

U_TEST( TemplateInstantiationRequiredTest1 )
{
	static const char c_program_text[]=
	R"(
		template</ type X /> struct A{}
		template</ type T /> struct Box</ A /> {}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateInstantiationRequired, 3u ) );
}

U_TEST( CouldNotOverloadFunction_ForClassTemplates_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct FuncsStroage</ T />
		{
			fn Foo( T t );
			fn Foo( T &imut t );
		}

		fn Foo()
		{
			var FuncsStroage</ i32 /> fs;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TemplateContext );
	U_TEST_ASSERT( error.template_context != nullptr );
	U_TEST_ASSERT( error.template_context->errors.front().code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	const auto line= error.template_context->errors.front().src_loc.GetLine();
	U_TEST_ASSERT( line == 5u || line == 6u );
}

U_TEST( CouldNotOverloadFunction_ForClassTemplates_Test1 )
{
	static const char c_program_text[]=
	R"(
		template</ type T, type U />
		struct FuncsStroage</ T, U />
		{
			fn Foo( T t ){}
			fn Foo( U &imut u ){} // Generates error, if U == T
			fn Foo( i32 i ){} // Generates error, if T or U is i32

			fn Baz( T       t, U &imut u ){}
			fn Baz( U &imut u, T       t ){} // Generates error, if T and U is same
		}

		fn Foo()
		{
			var FuncsStroage</ i32, i32 /> fs;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( ! build_result.errors.empty() );
	U_TEST_ASSERT( build_result.errors.front().template_context != nullptr );
	const auto& errors= build_result.errors.front().template_context->errors;

	U_TEST_ASSERT( errors.size() >= 3u );

	U_TEST_ASSERT( errors[0].code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_TEST_ASSERT( errors[0].src_loc.GetLine() == 5u || errors[0].src_loc.GetLine() == 6u );
	U_TEST_ASSERT( errors[1].code == CodeBuilderErrorCode::FunctionBodyDuplication );
	U_TEST_ASSERT( errors[1].src_loc.GetLine() == 5u || errors[1].src_loc.GetLine() == 7u );
	U_TEST_ASSERT( errors[2].code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_TEST_ASSERT( errors[2].src_loc.GetLine() == 9u || errors[2].src_loc.GetLine() == 10u );
}

U_TEST( MandatoryTemplateSignatureArgumentAfterOptionalArgument_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T, type V /> struct Box</ T= i32, V /> { }
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::MandatoryTemplateSignatureArgumentAfterOptionalArgument );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( TemplateArgumentIsNotDeducedYet_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T, type V />
		struct Box</ T= V, V= i32 /> { }

		fn Foo()
		{
			var Box</ /> box;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateArgumentIsNotDeducedYet , 3u ) );
}

U_TEST( TemplateArgumentIsNotDeducedYet_Test1 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Wrapper</ T /> { T t; }

		template</ type T, type V />
		struct Box</ T= Wrapper</ V />, V= i32 /> { }

		fn Foo()
		{
			var Box</ /> box;
		}
	)";
	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateArgumentIsNotDeducedYet, 5u ) );
}

U_TEST( TemplateArgumentNotUsedInSignature_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Box</ /> {}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TemplateArgumentNotUsedInSignature );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( TemplateParametersDeductionFailed_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ /> struct Box</ i32 /> {}

		fn Foo( Box</ f32 /> &imut b ) {}  // Template requires i32, but f32 given
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 4u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct X{}
		template</ /> struct Box</ X /> {}

		namespace N
		{
			struct X{}
			fn Foo( Box</ X /> &imut b ) {}  // Template requires ::X, but N::X given.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 8u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test2 )
{
	static const char c_program_text[]=
	R"(
		namespace N
		{
			struct X{}
			template</ /> struct Box</ X /> {}
		}

		struct X{}
		fn Foo( N::Box</ X /> &imut b ) {}  // Template requires N::X, but ::X given.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 9u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test3 )
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			struct B{}
		}
		template</ /> struct Box</ A />{}

		fn Foo( Box</ A::B /> &imut b ) {}  // Template requires A, but A::B given.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 8u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test4 )
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			struct B{}
		}
		template</ /> struct Box</ A::B />{}

		fn Foo( Box</ A /> &imut b ) {}  // Template requires A::B, but A given.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 8u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test5 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Ball</ T />{}
		template</ type T /> struct Box</ T />{}

		template</ type T /> struct Baz</ Ball</ T /> /> {}

		fn Foo( Baz</ Box</ f32 /> /> &imut b ) {}  // Template requires Ball</T/>, but Box</T/> given.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 7u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test6 )
{
	static const char c_program_text[]=
	R"(
		struct Ball{ template</ type T /> struct FFF</ T />{} }
		struct  Box{ template</ type T /> struct FFF</ T />{} }

		template</ type T /> struct Baz</ Ball::FFF</ T /> /> {}

		fn Foo( Baz</ Box::FFF</ f32 /> /> &imut b ) {}  // Template requires Ball::FFF</T/>, but Box::FFF</T/> given.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 7u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test7 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct FakePair</ T, T />{}

		fn Foo( FakePair</ f32, i32 /> &imut b ) {}  // Conflicted deduced types.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 5u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test8 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Box{ T t; }

		template</ type T />
		struct FakePair</ T, Box</T/> />{}

		fn Foo( FakePair</ f32, Box</ bool /> /> &imut b ) {}  // Conflicted deduced types.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 7u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test9 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct FFF</ T />
		{
			struct Ball{}
		}

		template</ type T /> struct Baz</ FFF</ T /> /> {}

		fn Foo( Baz</ FFF</ T />::Ball /> &imut b ) {}  // Template requires FFF</T/>, but FFF</T/>::Ball given.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 10u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test10 )
{
	static const char c_program_text[]=
	R"(
		template</ />
		struct FFF</ 42 /> {}

		fn Foo()
		{
			var FFF</ 34 /> ff{}; // signature number and actual number mismatch.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 7u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test11 )
{
	static const char c_program_text[]=
	R"(
		template</ i32 default_val />
		struct Box
		{
			i32 x;
			fn constructor() ( x= default_val ){}
		}

		template</ />
		struct ZeroBoxVec</ Box</ 0 /> />   // numeric constant is inside template signature arg.
		{
			Box</ 0 /> x;
			Box</ 0 /> y;
		}

		fn Foo()
		{
			var ZeroBoxVec</ Box</ 42 /> /> v;   // given number does not match to number in signature parameter.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 18u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test12 )
{
	static const char c_program_text[]=
	R"(
		template</ i32 x /> struct Box {}

		type T= Box</ 0u />; // Value type mismatch - required "i32" used "u32"
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 4u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test13 )
{
	static const char c_program_text[]=
	R"(
		template</ type Y /> struct Box {}

		type ZeroBox= Box</ 0 />; // Expected type, value given
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 4u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test14 )
{
	static const char c_program_text[]=
	R"(
		template</ u32 S /> struct Box {}

		type U32Box= Box</ u32 />; // Expected value, type given
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 4u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test15 )
{
	static const char c_program_text[]=
	R"(
		template</ type A, type B /> struct Box {}

		type IntBox= Box</ i32 />; // Expected 2 params, given 1 arg
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 4u ) );
}

U_TEST( TemplateParametersDeductionFailed_Test16 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Box {}

		type IntDoubleBox= Box</ i32, f64 />; // Expected 1 param, given 2 args
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TemplateParametersDeductionFailed, 4u ) );
}

U_TEST( ExpectedConstantExpression_InTemplateSignatureArgument_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn GetNum() : i32 { return 42; }

		template</ />
		struct FFF</ GetNum() /> {}  // result of function call is not constant
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedConstantExpression );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( ExpectedConstantExpression_InTemplateSignatureArgument_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn GetNum() : i32 { return 42; }

		template</ i32 x />
		struct FFF{}

		type K= FFF</ GetNum() />;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedConstantExpression, 7u ) );
}

U_TEST( UsingKeywordAsName_ForTypeTemplate_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct while {}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::UsingKeywordAsName, 2u ) );
}

U_TEST( UsingKeywordAsName_ForTypeTemplate_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			template</ type T /> type constexpr= [ T, 4 ];
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::UsingKeywordAsName, 4u ) );
}

} // namespace

} // namespace U
