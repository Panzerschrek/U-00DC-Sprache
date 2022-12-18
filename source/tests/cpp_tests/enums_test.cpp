#include "tests.hpp"

namespace U
{

namespace
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
			var ColorComponent mut cc= zero_init;
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

U_TEST( EnumValueAsArgument )
{
	static const char c_program_text[]=
	R"(
		enum ColorComponent{ r, g, b }
		fn ConvertComponent( ColorComponent c ) : ColorComponent
		{
			if( c == ColorComponent::r ) { return ColorComponent::b; }
			if( c == ColorComponent::g ) { return ColorComponent::g; }
			if( c == ColorComponent::b ) { return ColorComponent::r; }
			halt;
		}

		fn Foo()
		{
			halt if( ConvertComponent( ColorComponent::r ) != ColorComponent::b );
			halt if( ConvertComponent( ColorComponent::g ) != ColorComponent::g );
			halt if( ConvertComponent( ColorComponent::b ) != ColorComponent::r );
		}
	)";

	BuildProgram( c_program_text );
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

U_TEST( EnumsOrderCompareTest0 )
{
	// Compare with unsinged underlaying type.
	static const char c_program_text[]=
	R"(
		enum ColorComponent : u8 { r, g, b }

		static_assert(    ColorComponent::r < ColorComponent::g   );
		static_assert( !( ColorComponent::g < ColorComponent::g ) );
		static_assert( !( ColorComponent::b < ColorComponent::g ) );

		static_assert(    ColorComponent::r <= ColorComponent::g   );
		static_assert(    ColorComponent::g <= ColorComponent::g   );
		static_assert( !( ColorComponent::b <= ColorComponent::g ) );

		static_assert( !( ColorComponent::r > ColorComponent::g ) );
		static_assert( !( ColorComponent::g > ColorComponent::g ) );
		static_assert(    ColorComponent::b > ColorComponent::g   );

		static_assert( !( ColorComponent::r >= ColorComponent::g ) );
		static_assert(    ColorComponent::g >= ColorComponent::g   );
		static_assert(    ColorComponent::b >= ColorComponent::g   );
	)";

	BuildProgram( c_program_text );
}

U_TEST( EnumsOrderCompareTest1 )
{
	// Compare with singed underlaying type.
	static const char c_program_text[]=
	R"(
		enum ColorComponent : i8 { r, g, b }

		static_assert(    ColorComponent::r < ColorComponent::g   );
		static_assert( !( ColorComponent::g < ColorComponent::g ) );
		static_assert( !( ColorComponent::b < ColorComponent::g ) );

		static_assert(    ColorComponent::r <= ColorComponent::g   );
		static_assert(    ColorComponent::g <= ColorComponent::g   );
		static_assert( !( ColorComponent::b <= ColorComponent::g ) );

		static_assert( !( ColorComponent::r > ColorComponent::g ) );
		static_assert( !( ColorComponent::g > ColorComponent::g ) );
		static_assert(    ColorComponent::b > ColorComponent::g   );

		static_assert( !( ColorComponent::r >= ColorComponent::g ) );
		static_assert(    ColorComponent::g >= ColorComponent::g   );
		static_assert(    ColorComponent::b >= ColorComponent::g   );
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

U_TEST( EnumToIntConversionTest )
{
	static const char c_program_text[]=
	R"(
		enum Months{ January, February, March, April, May, June, July, August, September, Ocotber, November, December }

		fn Foo() : u32
		{
			var u32 march( Months::March );
			return u32(u16(Months::Ocotber)) - march;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 9 - 2 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( EnumAsClassField )
{
	static const char c_program_text[]=
	R"(
		struct Color
		{
			enum NamedColor{ Red, Green, Blue, Black, White }
			NamedColor named_color;
		}

		fn Foo() : i32
		{
			var Color color{ .named_color= Color::NamedColor::Blue };
			var Color color_copy( color );
			return i32( color_copy.named_color );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 2 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( UnderlayingTypeForEnumTest0 )
{
	static const char c_program_text[]=
	R"(
		enum CompactEnum : u8
		{ A, B, C, }

		enum LargeEnum: i64
		{ A, B, C, }

		enum LargeUnsignedEnum: u64
		{ A, B, C, }

		enum SuperLargeSignedEnum: i128
		{ A, B, C, }

		enum SuperLargeUnsignedEnum: u128
		{ A, B, C, }

		static_assert( typeinfo</CompactEnum/>.size_of == typeinfo</u8/>.size_of );
		static_assert( typeinfo</LargeEnum/>.size_of == typeinfo</i64/>.size_of );
		static_assert( typeinfo</LargeUnsignedEnum/>.size_of == typeinfo</u64/>.size_of );
		static_assert( typeinfo</SuperLargeSignedEnum/>.size_of == typeinfo</i128/>.size_of );
		static_assert( typeinfo</SuperLargeUnsignedEnum/>.size_of == typeinfo</u128/>.size_of );

	)";

	BuildProgram( c_program_text );
}

} // namespace

} // namespace U
