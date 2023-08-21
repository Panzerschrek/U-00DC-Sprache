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

U_TEST( GoToDefinition_Test2 )
{
	// Go to definition for local variables.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x, i32 y )
		{
			auto x_copy= x;
			{
				auto x_copy= y; // shadow previous x_copy
				auto x= 42; // shadow previous x
				var i32 kkk= 3 + 7 * x; // access x on previous line
			}
			var i32 ww= w; // Access global variable.
			auto some = y + x; // Access local y, not global y.
		}
		auto y= w;
		var i32 w= 0;
	)";

	const auto code_builder= BuildProgramForGoToDefinitionTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  4, 16 ) == SrcLoc( 0,  2, 14 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  6, 17 ) == SrcLoc( 0,  2, 21 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  8, 25 ) == SrcLoc( 0,  7,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 10, 15 ) == SrcLoc( 0, 14, 10 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 11, 15 ) == SrcLoc( 0,  2, 21 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 11, 19 ) == SrcLoc( 0,  2, 14 ) );
}

U_TEST( GoToDefinition_Test3 )
{
	// Member access operator, field access.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn GetX( this ) : i32 { return x; }
		}
		struct T
		{
			Int x;
			fn SetX( mut this, Int in_x ){ x= in_x; }
		}
		type Int= i32;
		fn Foo( S &mut s, T& t )
		{
			var i32 x= t.x;
			s.x= x;
		}
		struct SS
		{
			S s;
		}
		fn Bar() : SS;
		fn Baz()
		{
			auto x= Bar().s.x;
		}
	)";

	const auto code_builder= BuildProgramForGoToDefinitionTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  5, 34 ) == SrcLoc( 0,  4,  7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  9,  5 ) == SrcLoc( 0, 12,  7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 10, 22 ) == SrcLoc( 0, 12,  7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 10, 34 ) == SrcLoc( 0,  9,  7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 10, 39 ) == SrcLoc( 0, 10, 26 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 13, 10 ) == SrcLoc( 0,  2,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 13, 20 ) == SrcLoc( 0,  7,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 15, 14 ) == SrcLoc( 0, 13, 23 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 15, 16 ) == SrcLoc( 0,  9,  7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 16,  3 ) == SrcLoc( 0, 13, 17 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 16,  5 ) == SrcLoc( 0,  4,  7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 16,  8 ) == SrcLoc( 0, 15, 11 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 20,  3 ) == SrcLoc( 0,  2,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 22, 14 ) == SrcLoc( 0, 18,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 25, 17 ) == SrcLoc( 0, 20,  5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 25, 19 ) == SrcLoc( 0,  4,  7 ) );
}

U_TEST( GoToDefinition_Test4 )
{
	// Struct initializers.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			f32 y;
		}
		struct T
		{
			S s;
			bool y;
		}
		struct Q
		{
			char8 ccc;
			fn constructor()
				( ccc= 42c8 )
			{}
		}
		fn Foo()
		{
			var S s{ .x= 0, .y= 1.0f };
			var T t{ .s= s, .y= false };
		}
	)";

	const auto code_builder= BuildProgramForGoToDefinitionTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  9,  3 ) == SrcLoc( 0,  2,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 16,  7 ) == SrcLoc( 0, 14,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 21,  7 ) == SrcLoc( 0,  2,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 21, 13 ) == SrcLoc( 0,  4,  7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 21, 20 ) == SrcLoc( 0,  5,  7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 22,  7 ) == SrcLoc( 0,  7,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 22, 13 ) == SrcLoc( 0,  9,  5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 22, 20 ) == SrcLoc( 0, 10,  8 ) );
}

U_TEST( GoToDefinition_Test5 )
{
	// Move operator
	static const char c_program_text[]=
	R"(
		fn Foo( i32 mut x )
		{
			move(x);
		}
	)";

	const auto code_builder= BuildProgramForGoToDefinitionTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 4, 8 ) == SrcLoc( 0, 2, 18 ) );
}

U_TEST( GoToDefinition_Test6 )
{
	// Functions.
	// For now we can't distiguish between overloading functions.
	static const char c_program_text[]=
	R"(
		fn Foo(i32 x);
		fn Foo(f32 x);
		fn Bar()
		{
			Foo(7);
			Foo(4.5f);
		}
	)";

	const auto code_builder= BuildProgramForGoToDefinitionTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	const auto foo_definition0= GetDefinition( lexems, *code_builder, 6, 3 );
	const auto foo_definition1= GetDefinition( lexems, *code_builder, 7, 3 );
	U_TEST_ASSERT( foo_definition0 == SrcLoc( 0, 2, 5 ) || foo_definition0 == SrcLoc( 0, 3, 5 ) );
	U_TEST_ASSERT( foo_definition1 == SrcLoc( 0, 2, 5 ) || foo_definition1 == SrcLoc( 0, 3, 5 ) );
}

U_TEST( GoToDefinition_Test7 )
{
	// Should return definition, not prototype.
	static const char c_program_text[]=
	R"(
		fn Foo(i32 x);
		fn Bar()
		{
			Foo(7);
		}
		fn Foo(i32 x){}
	)";

	const auto code_builder= BuildProgramForGoToDefinitionTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 5, 3 ) == SrcLoc( 0, 7, 5 ) );
}

U_TEST( GoToDefinition_Test8 )
{
	// Call different functions in different scopes.
	static const char c_program_text[]=
	R"(
		fn Foo();
		namespace NS
		{
			fn Foo();
		}
		struct S
		{
			fn Foo(this);
		}
		struct T
		{
			fn Foo();
		}
		fn Bar()
		{
			Foo();
			NS::Foo();
			var S s;
			s.Foo();
			T::Foo();
		}
	)";

	const auto code_builder= BuildProgramForGoToDefinitionTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 17,  3 ) == SrcLoc( 0,  2,  5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 18,  3 ) == SrcLoc( 0,  3, 12 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 18,  7 ) == SrcLoc( 0,  5,  6 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 19,  7 ) == SrcLoc( 0,  7,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 20,  3 ) == SrcLoc( 0, 19,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 20,  5 ) == SrcLoc( 0,  9,  6 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 21,  3 ) == SrcLoc( 0, 11,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 21,  6 ) == SrcLoc( 0, 13,  6 ) );
}

U_TEST( GoToDefinition_Test9 )
{
	// Type templates.
	static const char c_program_text[]=
	R"(
		template</type T/>
		struct Box
		{
			T t;
		}
		fn Foo( Box</Float/> b ) : Float
		{
			return b.t;
		}
		type Float= f32;
	)";

	const auto code_builder= BuildProgramForGoToDefinitionTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 5,  3 ) == SrcLoc( 0,  2, 17 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 7, 10 ) == SrcLoc( 0,  2,  2 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 7, 15 ) == SrcLoc( 0, 11,  7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 9, 10 ) == SrcLoc( 0,  7, 23 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 9, 12 ) == SrcLoc( 0,  5,  5 ) );
}

U_TEST( GoToDefinition_Test10 )
{
	// Typeof.
	static const char c_program_text[]=
	R"(
		type K= typeof(s)::K;
		var S s{};
		struct S
		{
			type K= char16;
		}
	)";

	const auto code_builder= BuildProgramForGoToDefinitionTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 2, 17 ) == SrcLoc( 0, 3, 8 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 2, 21 ) == SrcLoc( 0, 6, 8 ) );
}

} // namespace

} // namespace U
