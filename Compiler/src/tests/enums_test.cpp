#include "tests.hpp"

namespace U
{

U_TEST( EnumsDeclarationTest )
{
	static const char c_program_text[]=
	R"(
		enum GlobalEnum
		{
			A, B, C,
		}

		class C
		{
			enum InClassEnum
			{
				FFF, RRR, TT,
				Lol
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( InitializationOfEnumVariables )
{
	static const char c_program_text[]=
	R"(
		enum Letters { A, B, C, D, E, F }
		fn Foo()
		{
			var Letters b= Letters::B; // Expression initializer
			var Letters e( Letters::E ); // Constructor initializer
			var Letters c= zero_init; // Zero initializer
			auto d= Letters::D; // auto variable of enum type
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( EnumsAssignmentAndReturnTest )
{
	static const char c_program_text[]=
	R"(
		enum ColorComponent{ r, g, b }
		fn Foo() : ColorComponent
		{
			var ColorComponent cc= zero_init;
			cc= ColorComponent::b;
			return cc;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 2 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( EnumsCompareTest )
{
	static const char c_program_text[]=
	R"(
		enum ColorComponent{ r, g, b }
		fn Foo()
		{
			static_assert( ColorComponent::r == ColorComponent::r );
			static_assert( ColorComponent::r != ColorComponent::g );
			var bool eq= ColorComponent::b == ColorComponent::b;
			var bool ne= ColorComponent::b != ColorComponent::b;
			halt if( !eq );
			halt if( ne );
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( EnumValueRefTest )
{
	static const char c_program_text[]=
	R"(
		enum ColorComponent{ r, g, b }
		fn GetGreen() : ColorComponent &imut
		{
			return ColorComponent::g;
		}

		fn Foo() : ColorComponent
		{
			auto &imut green= GetGreen();
			return green;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 1 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( EnumTypeAsTemplateTypeParameter )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct Wrapper
		{
			type Wrapped= T;
		}

		enum Sex{ Male, Female }

		fn Foo() : Sex
		{
			return Wrapper</ Sex />::Wrapped::Female;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 1 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( EnumValueAsTemplateValueParameter )
{
	static const char c_program_text[]=
	R"(
		enum LightingMode{ None, Constant, ColoredConstant, PerVertex, Lightmap }

		template</ LightingMode lighting_mode />
		struct Rasterizer
		{
			fn Do()
			{
				if( lighting_mode == LightingMode::PerVertex ){}
			}
		}

		fn Foo()
		{
			var Rasterizer</ LightingMode::Constant /> rast;
			rast.Do();
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace U
