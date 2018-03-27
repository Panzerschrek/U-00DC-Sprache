#include "tests.hpp"

namespace U
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InvalidValueAsTemplateArgument );
	U_TEST_ASSERT( error.file_pos.line == 6u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InvalidTypeOfTemplateVariableArgument );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

// TODO - InvalidTypeOfTemplateVariableArgument for arrays, structs.

U_TEST( ClassPrepass_ErrorsTest0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		class CC</ T />
		{
			fn Foo()
			{
				var i32 x= 0.25f; // all components are not template dependnent - generate error.
			}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST( ClassPrepass_ErrorsTest1 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		class CC</ T />
		{
			fn Foo()
			{
				var [ i32, 42 ] s{}; // struct initializer for non-struct.
			}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::StructInitializerForNonStruct );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST( ClassPrepass_ErrorsTest2 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		class CC</ T />
		{
			fn Foo()
			{
				var i32 mut something_mutable= 34;
				auto constexpr x= something_mutable;
			}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::VariableInitializerIsNotConstantExpression );
	U_TEST_ASSERT( error.file_pos.line == 8u );
}

U_TEST( DeclarationShadowsTemplateArgument_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		class CC</ T />
		{
			i32 T; // class field
		}

		template</ type T />
		class DD</ T />
		{
			fn T(); // function
			fn Bar()
			{
				{ var i32 T= 0; } // variable
				{ auto T= 0; } // auto-variable
			}
			fn Foo( f64 T ){} // function aargument
		}

		template</ type T />
		class EE</ T />
		{
			class T{} // class
		}

		template</ type T />
		class FF</ T />
		{
			// TODO - turn on this, when class templates inside classes will be supported.
			//template</ T size /> class T</ size />{} // class template argument
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= /* 7u */ 6u );

	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::DeclarationShadowsTemplateArgument );
	U_TEST_ASSERT( build_result.errors[0].file_pos.line == 5u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::DeclarationShadowsTemplateArgument );
	U_TEST_ASSERT( build_result.errors[1].file_pos.line == 11u );
	U_TEST_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::DeclarationShadowsTemplateArgument );
	U_TEST_ASSERT( build_result.errors[2].file_pos.line == 14u );
	U_TEST_ASSERT( build_result.errors[3].code == CodeBuilderErrorCode::DeclarationShadowsTemplateArgument );
	U_TEST_ASSERT( build_result.errors[3].file_pos.line == 15u );
	U_TEST_ASSERT( build_result.errors[4].code == CodeBuilderErrorCode::DeclarationShadowsTemplateArgument );
	U_TEST_ASSERT( build_result.errors[4].file_pos.line == 17u );
	U_TEST_ASSERT( build_result.errors[5].code == CodeBuilderErrorCode::DeclarationShadowsTemplateArgument );
	U_TEST_ASSERT( build_result.errors[5].file_pos.line == 23u );
	//U_TEST_ASSERT( build_result.errors[6].code == CodeBuilderErrorCode::DeclarationShadowsTemplateArgument );
	//U_TEST_ASSERT( build_result.errors[6].file_pos.line == 29u );
}

U_TEST( NameNotFound_ForClassTemplateSingatureArguments_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</  /> class CC</ SSSS /> {} // Name in signature argument not known yet.

		struct SSSS{}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );


	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST( NameNotFound_ForClassTemplateArguments_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ SSS xx /> class CC</ xx /> {} // Name in type of value-argument not known yet.

		struct SSS{}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );


	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST( NameNotFound_ForClassTemplateDefaultSignatureArguments_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> class CC</ T= SSS /> {} // Name of default signature argument not known here yet.

		struct SSS{}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );


	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 2u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );


	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ValueIsNotTemplate );
	U_TEST_ASSERT( error.file_pos.line == 5u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );


	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TemplateInstantiationRequired );
	U_TEST_ASSERT( error.file_pos.line == 6u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );


	U_TEST_ASSERT( !build_result.errors.empty() );

	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_TEST_ASSERT( error.file_pos.line == 6u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= 3u );

	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_TEST_ASSERT( build_result.errors[0].file_pos.line == 6u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_TEST_ASSERT( build_result.errors[1].file_pos.line == 10u );
	U_TEST_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::FunctionBodyDuplication );
	U_TEST_ASSERT( build_result.errors[2].file_pos.line == 7u );
}

U_TEST( MandatoryTemplateSignatureArgumentAfterOptionalArgument_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T, type V /> struct Box</ T= i32, V /> { }
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::MandatoryTemplateSignatureArgumentAfterOptionalArgument );
	U_TEST_ASSERT( error.file_pos.line == 2u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TemplateArgumentIsNotDeducedYet );
	U_TEST_ASSERT( error.file_pos.line == 3u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TemplateArgumentIsNotDeducedYet );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( TemplateArgumentNotUsedInSignature_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Box</ /> {}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TemplateArgumentNotUsedInSignature );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST( IncompleteMemberOfClassTemplate_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Box</ T />
		{
			fn A(); // <- incomplete

			fn B();
			fn B(){}

			struct C; // <- incomplete

			struct D
			{
				struct E; // <- incomplete
			}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.size() >= 3u );

	// TODO - correct line numbers.
	// We must write correct file_pos of incomplete member.
	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::IncompleteMemberOfClassTemplate );
	U_TEST_ASSERT( build_result.errors[0].file_pos.line == 2u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::IncompleteMemberOfClassTemplate );
	U_TEST_ASSERT( build_result.errors[1].file_pos.line == 2u );
	U_TEST_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::IncompleteMemberOfClassTemplate );
	U_TEST_ASSERT( build_result.errors[2].file_pos.line == 2u );
}

U_TEST( TemplateParametersDeductionFailed_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ /> struct Box</ i32 /> {}

		fn Foo( Box</ f32 /> &imut b ) {}  // Template requires i32, but f32 given
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 4u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 8u );
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
		fn Foo( Box</ X /> &imut b ) {}  // Template requires N::X, but ::X given.
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 9u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 8u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 8u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 7u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST( TemplateParametersDeductionFailed_Test7 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct FakePair</ T, T />{}

		fn Foo( FakePair</ f32, i32 /> &imut b ) {}  // Conflicted deduced types.
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 5u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 7u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 10u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST( ExpectedConstantExpression_InTemplateSignatureArgument_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn GetNum() : i32 { return 42; }

		template</ />
		struct FFF</ GetNum() /> {}  // result of function call is not constant
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedConstantExpression );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

} // namespace U
