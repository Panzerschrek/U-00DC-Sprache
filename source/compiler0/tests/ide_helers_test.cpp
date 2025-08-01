#include "../code_builder_lib/code_builder.hpp"
#include "../lex_synt_lib/lex_utils.hpp"
#include "cpp_tests_launcher.hpp"

namespace U
{

namespace
{

SrcLoc CorrectSrcLoc( const Lexems& lexems, const uint32_t line, const uint32_t column )
{
	const Lexem* const lexem= GetLexemForPosition( line, column, lexems );
	U_TEST_ASSERT( lexem != nullptr );
	return lexem->src_loc;
}

std::optional<SrcLoc> GetDefinition( const Lexems& lexems, CodeBuilder& code_builder, const uint32_t line, const uint32_t column )
{
	return code_builder.GetDefinition( CorrectSrcLoc( lexems, line, column ) );
}

std::vector<SrcLoc> GetAllOccurrences( const Lexems& lexems, CodeBuilder& code_builder, const uint32_t line, const uint32_t column )
{
	return code_builder.GetAllOccurrences( CorrectSrcLoc( lexems, line, column ) );
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

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
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

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
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

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
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

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
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
				( ccc( 42 ) )
			{}
		}
		fn Foo()
		{
			var S s{ .x= 0, .y= 1.0f };
			var T t{ .s= s, .y= false };
		}
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
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

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
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

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
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

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
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

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
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

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 5,  3 ) == SrcLoc( 0,  2, 17 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 7, 10 ) == SrcLoc( 0,  3,  9 ) );
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

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 2, 17 ) == SrcLoc( 0, 3, 8 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 2, 21 ) == SrcLoc( 0, 6, 8 ) );
}

U_TEST( GoToDefinition_Test11 )
{
	static const char c_program_text[]=
	R"(
		namespace Abc
		{
			namespace Def
			{
				class Qwerty
				{
					fn Foo();
				}
			}
		}

		fn Abc::Def::Qwerty::Foo() {}
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 13,  5 ) == SrcLoc( 0,  2, 12 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 13, 10 ) == SrcLoc( 0,  4, 13 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 13, 15 ) == SrcLoc( 0,  6, 10 ) );
}

U_TEST( GoToDefinition_Test12 )
{
	// SrcLoc of template thing is src_loc of its name.
	static const char c_program_text[]=
	R"(
		template</type T/>
		class Box{}

		template</type T/>
		type Vec3= [ T, 3 ];

		template</type T/>
		fn GetZero() : T { return T(0); }

		type IntBox= Box</i32/>;
		type FloatVec= Vec3</f32/>;
		auto double_zero= GetZero</f64/>();
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 11, 15 ) == SrcLoc( 0, 3, 8 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 12, 17 ) == SrcLoc( 0, 6, 7 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 13, 20 ) == SrcLoc( 0, 9, 5 ) );
}

U_TEST( GoToDefinition_Test13 )
{
	// Should return none for fundamental type names.
	static const char c_program_text[]=
	R"(
		type Int= i32;
		var f32 x(0.0f);
		struct S{ bool b; }
		fn Foo() : void;
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 2, 13 ) == std::nullopt );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 3,  6 ) == std::nullopt );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 4, 14 ) == std::nullopt );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 5, 14 ) == std::nullopt );
}

U_TEST( GoToDefinition_Test14 )
{
	// Should select proper definition point for overloaded function call.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x );
		fn Bar( f32 x );
		fn Bar();
		fn Bar( bool b, char8 c );
		fn Foo()
		{
			Bar( false, '7' );
			Bar();
			Bar( 67.5f );
			Bar( 777 );
			Bar</ i64 />( 77u64 );
		}
		template</ type T /> fn Bar( u64 x ) : T { return T(x); }
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  8, 3 ) == SrcLoc( 0, 5, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  9, 3 ) == SrcLoc( 0, 4, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 10, 3 ) == SrcLoc( 0, 3, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 11, 3 ) == SrcLoc( 0, 2, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 12, 3 ) == SrcLoc( 0, 14, 26 ) );
}

U_TEST( GoToDefinition_Test15 )
{
	// Should select proper definition point for overloaded method call.
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Bar( this, i32 x );
			fn Bar( this, f32 x );
			fn Bar( this );
			fn Bar( this, bool b, char8 c );
			template</ type T /> fn Bar( this, u64 x ) : T { return T(x); }
		}
		fn Foo()
		{
			var S s;
			s.Bar( false, '7' );
			s.Bar();
			s.Bar( 67.5f );
			s.Bar( 777 );
			s.Bar</ i64 />( 77u64 );
		}
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 13, 5 ) == SrcLoc( 0, 7, 6 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 14, 5 ) == SrcLoc( 0, 6, 6 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 15, 5 ) == SrcLoc( 0, 5, 6 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 16, 5 ) == SrcLoc( 0, 4, 6 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 17, 5 ) == SrcLoc( 0, 8, 27 ) );
}

U_TEST( GoToDefinition_Test16 )
{
	// Should return proper definition point for overloaded functions with prototypes.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x );
		fn Bar( f32 x );
		fn Bar();
		fn Bar( bool b, char8 c );

		fn Bar( i32 x ) {}
		fn Bar( f32 x ) {}
		fn Bar() {}
		fn Bar( bool b, char8 c ){}

		fn Foo()
		{
			Bar( false, '7' );
			Bar();
			Bar( 67.5f );
			Bar( 777 );
		}
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 14, 3 ) == SrcLoc( 0, 10, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 15, 3 ) == SrcLoc( 0,  9, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 16, 3 ) == SrcLoc( 0,  8, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 17, 3 ) == SrcLoc( 0,  7, 5 ) );

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  2, 5 ) == SrcLoc( 0,  7, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  3, 5 ) == SrcLoc( 0,  8, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  4, 5 ) == SrcLoc( 0,  9, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  5, 5 ) == SrcLoc( 0, 10, 5 ) );
}

U_TEST( GoToDefinition_Test17 )
{
	// Should select proper definition point for function pointer initialization.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x );
		fn Bar( f32 x );
		fn Bar();
		fn Bar( bool b, char8 c );
		template</ type T /> fn Bar( u64 x ) : T { return T(x); }
		fn Foo()
		{
			var ( fn( i32 x ) ) ptr0( Bar );
			var ( fn( f32 x ) ) ptr1( Bar );
			var ( fn() ) ptr2( Bar );
			var ( fn( bool b, char8 c ) ) ptr3( Bar );
			var ( fn( u64 x ) : i64 ) ptr4( Bar</i64/> );
		}
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  9, 29 ) == SrcLoc( 0, 2, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 10, 29 ) == SrcLoc( 0, 3, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 11, 22 ) == SrcLoc( 0, 4, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 12, 39 ) == SrcLoc( 0, 5, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 13, 35 ) == SrcLoc( 0, 6, 26 ) );
}

U_TEST( GoToDefinition_Test18 )
{
	// Should select proper mut/imut overloaded method.
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Foo( mut this );
			fn Foo( imut this );
		}
		fn Foo()
		{
			var S mut s_mut, imut s_imut;
			s_mut.Foo();
			s_imut.Foo();
		}
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 10,  9 ) == SrcLoc( 0, 4, 6 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 11, 10 ) == SrcLoc( 0, 5, 6 ) );
}

U_TEST( GoToDefinition_Test19 )
{
	// Should found definitions inside non-instantiated class template.
	static const char c_program_text[]=
	R"(
		template</ type SomeT />
		struct Box
		{
			SomeT field;
			fn Foo( this )
			{
				Bar();
				auto& ref= field;
				auto& ref_to_unknown= unknown; // Errors should be ignored.
			}
			type ThisType= Box</ SomeT />;
		}
		fn Bar();
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  5,  3 ) == SrcLoc( 0,  2, 18 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  8,  4 ) == SrcLoc( 0, 14,  5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  9, 15 ) == SrcLoc( 0,  5,  9 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 12, 18 ) == SrcLoc( 0,  3,  9 ) );
}

U_TEST( GoToDefinition_Test20 )
{
	// Should found definitions inside non-instantiated function template.
	static const char c_program_text[]=
	R"(
		template</ type SomeT />
		fn Foo( SomeT& arg )
		{
			var SomeT& ref= arg;
			Bar();
		}
		fn Bar();
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 3, 10 ) == SrcLoc( 0, 2, 18 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 5, 19 ) == SrcLoc( 0, 3, 17 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 6,  3 ) == SrcLoc( 0, 8,  5 ) );
}

U_TEST( GoToDefinition_Test21 )
{
	// Should found definitions inside non-instantiated function template inside non-instantiated class template.
	static const char c_program_text[]=
	R"(
		template</ type OuterT />
		struct Box
		{
			template</ type InnerT />
			fn Foo()
			{
				var InnerT inner= zero_init;
				var OuterT outer= zero_init;
				Bar();
			}
		}
		fn Bar();
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  8, 8 ) == SrcLoc( 0,  5, 19 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  9, 8 ) == SrcLoc( 0,  2, 18 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 10, 4 ) == SrcLoc( 0, 13,  5 ) );
}

U_TEST( GoToDefinition_Test22 )
{
	// Should suggest some funciton even if selection is failed.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x );
		fn Bar()
		{
			Foo(); // Too few args.
			Foo( 42, 24 ); // Too many args.
			Foo( "lol" ); // Invalid arg type.
			Foo( false ); // Invalid arg type.
			Foo( 0.25f ); // Invalid arg type.
			Foo( 66 ); // Correct arg type.
		}
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text, true );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  5, 3 ) == SrcLoc( 0,  2, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  6, 3 ) == SrcLoc( 0,  2, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  7, 3 ) == SrcLoc( 0,  2, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  8, 3 ) == SrcLoc( 0,  2, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder,  9, 3 ) == SrcLoc( 0,  2, 5 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 10, 3 ) == SrcLoc( 0,  2, 5 ) );
}

U_TEST( GoToDefinition_Test23 )
{
	// Should collect usage for an external variable in lambda body.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 local_var= 0;
			auto f=
				lambda[=]() : i32
				{
					return local_var * 2;
				};
		}
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text, true );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 8, 13 ) == SrcLoc( 0, 4, 11 ) );
}

U_TEST( GoToDefinition_Test24 )
{
	// Should collect usage for captured variable in lambda capture list and in lambda body itself.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 local_arg )
		{
			auto f=
				lambda[local_arg]() : i32
				{
					return local_arg * 2;
				};
		}
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text, true );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 5, 11 ) == SrcLoc( 0, 2, 14 ) );
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 7, 12 ) == SrcLoc( 0, 2, 14 ) );
}

U_TEST( GoToDefinition_Test25 )
{
	// Should go to definition of "alloca" variable.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			alloca f32 arr[ 16s ];
			{
				var i32 y= 0;
				auto ptr= arr;
			}
		}
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text, true );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;
	U_TEST_ASSERT( GetDefinition( lexems, *code_builder, 8, 14 ) == SrcLoc( 0, 5, 14 ) );
}

U_TEST( GetAllOccurrences_Test0 )
{
	static const char c_program_text[]=
	R"(
		type T= i32;
		var T x= 0;
		fn Foo( T x );
		fn Bar( i32 y ); // "i32" as synonym for "T", but should not be listed.
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	const std::vector<SrcLoc> expected_result{ SrcLoc( 0, 2, 7 ), SrcLoc( 0, 3, 6 ), SrcLoc( 0, 4, 10 ) };

	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 2,  7 ) == expected_result );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 3,  6 ) == expected_result );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 4, 10 ) == expected_result );
}

U_TEST( GetAllOccurrences_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x )
		{
			auto y= x;
			{
				auto x= 42; // Shadow variable
				auto z= x; // This reffers to other "x".
			}
		}
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	const std::vector<SrcLoc> expected_result{ SrcLoc( 0, 2, 14 ), SrcLoc( 0, 4, 11 ) };
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 2, 14 ) == expected_result );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 4, 11 ) == expected_result );

	const std::vector<SrcLoc> expected_result_shadowed{ SrcLoc( 0, 6, 9 ), SrcLoc( 0, 7, 12 ) };
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 6,  9 ) == expected_result_shadowed );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 7, 12 ) == expected_result_shadowed );
}

U_TEST( GetAllOccurrences_Test2 )
{
	static const char c_program_text[]=
	R"(
		type Abc= u16;
		var Abc abc0= zero_init;
		var Bar::Abc abc1= zero_init;
		var Baz::Abc abc2= zero_init;
		namespace Bar
		{
			type Abc= u32;
			var Abc abc0= zero_init;
			var ::Abc abc1= zero_init;
			var Baz::Abc abc2= zero_init;
		}
		namespace Baz
		{
			type Abc= char8;
			var Abc abc0= zero_init;
			var ::Abc abc1= zero_init;
			var Bar::Abc abc2= zero_init;
		}
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	const std::vector<SrcLoc> abc_global{ SrcLoc( 0,  2,  7 ), SrcLoc( 0,  3,  6 ), SrcLoc( 0, 10,  9 ), SrcLoc( 0, 17,  9 ) };
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  2,  7 ) == abc_global );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  3,  6 ) == abc_global );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 10,  9 ) == abc_global );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 17,  9 ) == abc_global );

	const std::vector<SrcLoc> abc_bar   { SrcLoc( 0,  4, 11 ), SrcLoc( 0,  8,  8 ), SrcLoc( 0,   9,  7 ), SrcLoc( 0, 18, 12 ) };
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  4, 11 ) == abc_bar );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  8,  8 ) == abc_bar );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  9,  7 ) == abc_bar );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 18, 12 ) == abc_bar );

	const std::vector<SrcLoc> abc_baz   { SrcLoc( 0,  5, 11 ), SrcLoc( 0, 11, 12 ), SrcLoc( 0,  15,  8 ), SrcLoc( 0, 16,  7 ) };
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  5, 11 ) == abc_baz );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 11, 12 ) == abc_baz );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 15,  8 ) == abc_baz );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 16,  7 ) == abc_baz );
}

U_TEST( GetAllOccurrences_Test3 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			s.x= 66;
			auto y= s.y;
		}
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	const std::vector<SrcLoc> result_x{ SrcLoc( 0,  4,  7 ), SrcLoc( 0,  9, 17 ), SrcLoc( 0, 10,  5 ) };
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  4,  7 ) == result_x );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  9, 17 ) == result_x );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 10,  5 ) == result_x );

	const std::vector<SrcLoc> result_y{ SrcLoc( 0,  5,  7 ), SrcLoc( 0,  9, 24 ), SrcLoc( 0, 11, 13 ) };
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  5,  7 ) == result_y );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  9, 24 ) == result_y );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 11, 13 ) == result_y );
}

U_TEST( GetAllOccurrences_Test4 )
{
	static const char c_program_text[]=
	R"(
		namespace Abc
		{
			namespace Def
			{
				class Qwerty
				{
					fn Foo();
				}
			}
		}

		fn Abc::Def::Qwerty::Foo() {}
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	const std::vector<SrcLoc> result_abc   { SrcLoc( 0,  2, 12 ), SrcLoc( 0, 13,  5 ) };
	const std::vector<SrcLoc> result_def   { SrcLoc( 0,  4, 13 ), SrcLoc( 0, 13, 10 ) };
	const std::vector<SrcLoc> result_qwerty{ SrcLoc( 0,  6, 10 ), SrcLoc( 0, 13, 15 ) };
	const std::vector<SrcLoc> result_foo   { SrcLoc( 0,  8,  8 ), SrcLoc( 0, 13, 23 ) };

	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  2, 12 ) == result_abc );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 13,  5 ) == result_abc );

	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  4, 13 ) == result_def );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 13, 10 ) == result_def );

	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  6, 10 ) == result_qwerty );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 13, 15 ) == result_qwerty );

	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  8,  8 ) == result_foo );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 13, 23 ) == result_foo );
}

U_TEST( GetAllOccurrences_Test5 )
{
	// Should return proper definition point for overloaded functions with prototypes.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x );
		fn Bar( f32 x );
		fn Bar();
		fn Bar( bool b, char8 c );

		fn Bar( i32 x ) {}
		fn Bar( f32 x ) {}
		fn Bar() {}
		fn Bar( bool b, char8 c ){}

		fn Foo()
		{
			Bar( false, '7' );
			Bar();
			Bar( 67.5f );
			Bar( 777 );
		}
	)";

	const auto code_builder= BuildProgramForIdeHelpersTest( c_program_text );
	const Lexems lexems= LexicalAnalysis( c_program_text ).lexems;

	const std::vector<SrcLoc> result_0{ SrcLoc( 0,  2,  5 ), SrcLoc( 0,  7,  5 ), SrcLoc( 0, 17,  3 ) };
	const std::vector<SrcLoc> result_1{ SrcLoc( 0,  3,  5 ), SrcLoc( 0,  8,  5 ), SrcLoc( 0, 16,  3 ) };
	const std::vector<SrcLoc> result_2{ SrcLoc( 0,  4,  5 ), SrcLoc( 0,  9,  5 ), SrcLoc( 0, 15,  3 ) };
	const std::vector<SrcLoc> result_3{ SrcLoc( 0,  5,  5 ), SrcLoc( 0, 10,  5 ), SrcLoc( 0, 14,  3 ) };

	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  2,  5 ) == result_0 );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  7,  5 ) == result_0 );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 17,  3 ) == result_0 );

	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  3,  5 ) == result_1 );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  8,  5 ) == result_1 );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 16,  3 ) == result_1 );

	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  4,  5 ) == result_2 );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  9,  5 ) == result_2 );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 15,  3 ) == result_2 );

	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder,  5,  5 ) == result_3 );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 10,  5 ) == result_3 );
	U_TEST_ASSERT( GetAllOccurrences( lexems, *code_builder, 14,  3 ) == result_3 );
}

} // namespace

} // namespace U
