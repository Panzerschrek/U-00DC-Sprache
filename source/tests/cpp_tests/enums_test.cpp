#include "cpp_tests.hpp"

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
	// Compare with unsinged underlying type.
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
	// Compare with singed underlying type.
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

U_TEST( EnumsOrderCompareTest2 )
{
	// Compare with unsinged underlying type.
	static const char c_program_text[]=
	R"(
		enum ColorComponent : u8 { r, g, b }

		static_assert( ColorComponent::r <=> ColorComponent::g == -1 );
		static_assert( ColorComponent::g <=> ColorComponent::g ==  0 );
		static_assert( ColorComponent::b <=> ColorComponent::g ==  1 );
	)";

	BuildProgram( c_program_text );
}

U_TEST( EnumsOrderCompareTest3 )
{
	// Compare with singed underlying type.
	static const char c_program_text[]=
	R"(
		enum ColorComponent : i8 { r, g, b }

		static_assert( ColorComponent::r <=> ColorComponent::g == -1 );
		static_assert( ColorComponent::g <=> ColorComponent::g ==  0 );
		static_assert( ColorComponent::b <=> ColorComponent::g ==  1 );
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

U_TEST( UnderlyingTypeForEnumTest0 )
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

U_TEST( UnderlyingTypeForEnumTest1 )
{
	static const char c_program_text[]=
	R"(
		// Less than 256 values
		enum CompactEnum
		{ A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P}

		static_assert( typeinfo</CompactEnum/>.size_of == typeinfo</u8/>.size_of );
		static_assert( typeinfo</CompactEnum/>.underlying_type.is_unsigned_integer );

		// More, than 256 values.
		enum LargeEnum
		{
			V00, V01, V02, V03, V04, V05, V06, V07, V08, V09, V0A, V0B, V0C, V0D, V0E, V0F,
			V10, V11, V12, V13, V14, V15, V16, V17, V18, V19, V1A, V1B, V1C, V1D, V1E, V1F,
			V20, V21, V22, V23, V24, V25, V26, V27, V28, V29, V2A, V2B, V2C, V2D, V2E, V2F,
			V30, V31, V32, V33, V34, V35, V36, V37, V38, V39, V3A, V3B, V3C, V3D, V3E, V3F,
			V40, V41, V42, V43, V44, V45, V46, V47, V48, V49, V4A, V4B, V4C, V4D, V4E, V4F,
			V50, V51, V52, V53, V54, V55, V56, V57, V58, V59, V5A, V5B, V5C, V5D, V5E, V5F,
			V60, V61, V62, V63, V64, V65, V66, V67, V68, V69, V6A, V6B, V6C, V6D, V6E, V6F,
			V70, V71, V72, V73, V74, V75, V76, V77, V78, V79, V7A, V7B, V7C, V7D, V7E, V7F,
			V80, V81, V82, V83, V84, V85, V86, V87, V88, V89, V8A, V8B, V8C, V8D, V8E, V8F,
			V90, V91, V92, V93, V94, V95, V96, V97, V98, V99, V9A, V9B, V9C, V9D, V9E, V9F,
			VA0, VA1, VA2, VA3, VA4, VA5, VA6, VA7, VA8, VA9, VAA, VAB, VAC, VAD, VAE, VAF,
			VB0, VB1, VB2, VB3, VB4, VB5, VB6, VB7, VB8, VB9, VBA, VBB, VBC, VBD, VBE, VBF,
			VC0, VC1, VC2, VC3, VC4, VC5, VC6, VC7, VC8, VC9, VCA, VCB, VCC, VCD, VCE, VCF,
			VD0, VD1, VD2, VD3, VD4, VD5, VD6, VD7, VD8, VD9, VDA, VDB, VDC, VDD, VDE, VDF,
			VE0, VE1, VE2, VE3, VE4, VE5, VE6, VE7, VE8, VE9, VEA, VEB, VEC, VED, VEE, VEF,
			VF0, VF1, VF2, VF3, VF4, VF5, VF6, VF7, VF8, VF9, VFA, VFB, VFC, VFD, VFE, VFF,
			Too, Much, Values, In, This, Enum,
		}

		static_assert( typeinfo</LargeEnum/>.size_of == typeinfo</u16/>.size_of );
		static_assert( typeinfo</LargeEnum/>.underlying_type.is_unsigned_integer );
	)";

	CreateEngine( BuildProgram( c_program_text ) );
}

} // namespace

} // namespace U
