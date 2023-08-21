#include "../code_builder_lib/code_builder.hpp"
#include "../lex_synt_lib/lex_utils.hpp"
#include "cpp_tests_launcher.hpp"
#include <iostream>

namespace U
{

namespace
{

SrcLoc CorrectSrcLoc( const Lexems& lexems, const uint32_t line, const uint32_t column )
{
	auto res= GetLexemSrcLocForPosition( line, column, lexems );
	U_TEST_ASSERT( res != std::nullopt );
	return *res;
}

std::optional<SrcLoc> GetDefinition( const Lexems& lexems, CodeBuilder& code_builder, const uint32_t line, const uint32_t column )
{
	return code_builder.GetDefinition( CorrectSrcLoc( lexems, line, column ) );
}

U_TEST( GoToDefinition_Test0 )
{
	// Simple names in global namespace. Ugly formatting is intentional.
	static const char c_program_text[]=
	R"(

		type B = A;

		type A = i32;
		type LongNameType= f32;

		var LongNameType some= zero_init;

		var SomeStruct ss{};


					struct SomeStruct{}
	)";

	const auto code_builder= BuildProgramForGoToDefinitionTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 3, 11 ) == SrcLoc( 0, 5, 7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 8, 12 ) == SrcLoc( 0, 6, 7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 10, 16 ) == SrcLoc( 0, 13, 12 ) );
}

U_TEST( GoToDefinition_Test1 )
{
	// Use different scopes.
	static const char c_program_text[]=
	R"(

		type Abc= i32;

		namespace Qwe
		{
			type Abc= f32;

			namespace asd
			{
				type Abc= tup[];

				type GlobalAbc= ::Abc;
				type QweAbc= ::Qwe::Abc;
				type AnotherQweAbc= Qwe::Abc;
				type asdAbc0= Abc;
				type asdAbc1= asd::Abc;
				type asdAbc2= Qwe::asd::Abc;
				type asdAbc3= ::Qwe::asd::Abc;
				type SomeStructAbc0= SomeStruct::Abc;
				type SomeStructAbc1= Qwe::SomeStruct::Abc;
				type SomeStructAbc2= ::Qwe::SomeStruct::Abc;
			}

			type GlobalAbc= ::Abc;
			type QweAbc= Abc;
			type SomeStructAbc= SomeStruct::Abc;

			struct SomeStruct
			{
				type Abc= [ f32, 4 ];
			}
		}

		type GlobalAbc0= Abc;
		type GlobalAbc1= ::Abc;
		type QweAbc= Qwe::Abc;
		type asdAbc= Qwe::asd::Abc;
		type another_asdAbc= ::Qwe::asd::Abc;
		type SomeStructAbc= Qwe::SomeStruct::Abc;
		type SomeStruct= Qwe::SomeStruct;
	)";

	const auto code_builder= BuildProgramForGoToDefinitionTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	// Type aliases.
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 13, 23 ) == SrcLoc( 0,  3,  7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 14, 26 ) == SrcLoc( 0,  7,  8 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 15, 30 ) == SrcLoc( 0,  7,  8 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 16, 19 ) == SrcLoc( 0, 11,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 17, 24 ) == SrcLoc( 0, 11,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 18, 30 ) == SrcLoc( 0, 11,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 19, 32 ) == SrcLoc( 0, 11,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 20, 39 ) == SrcLoc( 0, 31,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 21, 44 ) == SrcLoc( 0, 31,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 22, 46 ) == SrcLoc( 0, 31,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 25, 22 ) == SrcLoc( 0,  3,  7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 26, 18 ) == SrcLoc( 0,  7,  8 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 27, 37 ) == SrcLoc( 0, 31,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 35, 21 ) == SrcLoc( 0,  3,  7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 36, 23 ) == SrcLoc( 0,  3,  7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 37, 21 ) == SrcLoc( 0,  7,  8 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 38, 26 ) == SrcLoc( 0, 11,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 39, 36 ) == SrcLoc( 0, 11,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 40, 40 ) == SrcLoc( 0, 31,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 41, 27 ) == SrcLoc( 0, 29, 10 ) );

	// Components of some complex names.
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 14, 20 ) == SrcLoc( 0,  5, 12 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 18, 25 ) == SrcLoc( 0,  9, 13 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 40, 29 ) == SrcLoc( 0, 29, 10 ) );
}

} // namespace

} // namespace U
