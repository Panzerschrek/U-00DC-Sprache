#include "tests.hpp"

namespace U
{

U_TEST( BasicFunctionManglingTest )
{
	static const char c_program_text[]=
	R"(
		fn Foo(){} // void return, void args
		fn Bar() : i32 { return 0; } // non-void return, void args
		fn Baz(i32 x) {} // void return, non-empty args
		fn Lol(i32 x) : f32 { return f32(x); } // non-void return, non-empty args
		fn SeveralArgs(u64 x, f32 y, bool z){} // void return, several args
		fn ArgsAndRet(u32 x, i16 y, char8 z) : f64 { return 0.0; }

		fn RefArg(i32& x) {} // imut reference arg
		fn MutRefArg(f32 &mut x){} // mut reference arg

		fn RefRet( f64& x ) : f64& { return x; } // imut return reference
		fn MutRefRet( i32 &mut x ) : i32 &mut { return x; } // mut return reference

		fn MutValueArg(i32 mut x ){}
		fn ImutValueArg(i32 imut x){}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?Foo@@YAXXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Bar@@YAHXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Baz@@YAXH@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Lol@@YAMH@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?SeveralArgs@@YAX_KM_N@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?ArgsAndRet@@YANIFD@Z" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?RefArg@@YAXAEBH@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?MutRefArg@@YAXAEAM@Z" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?RefRet@@YAAEBNAEBN@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?MutRefRet@@YAAEAHAEAH@Z" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?MutValueArg@@YAXH@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?ImutValueArg@@YAXH@Z" ) != nullptr );
}

U_TEST( BasicGlobalVariablesManglingTest )
{
	static const char c_program_text[]=
	R"(
		var i32 IntGlobalVar= 0;
		var i32 mut MutGlobalVar = 0;

		var f32 FloatVar= 0.0f;
		var f64 DoubleVar = 0.0;

		auto some_auto_var= 666u64;
		auto mut mutable_auto_var= "$"c16;

		struct SomeStruct{}
		var SomeStruct struct_type_var= zero_init;
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "?IntGlobalVar@@3HB", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "?MutGlobalVar@@3HA", true ) != nullptr );

	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "?FloatVar@@3MB", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "?DoubleVar@@3NB", true ) != nullptr );

	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "?some_auto_var@@3_KB", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "?mutable_auto_var@@3_SA", true ) != nullptr );

	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "?struct_type_var@@3USomeStruct@@B", true ) != nullptr );
}

U_TEST( BasicNestedNamesTest )
{
	static const char c_program_text[]=
	R"(
		namespace Qwerty
		{
			fn Foo(){}
			auto blab= 0;

			namespace Baz
			{
				fn Bar(){}
				var f32 wer_hat_angst= 0.0f;
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?Foo@Qwerty@@YAXXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "?blab@Qwerty@@3HB", true ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?Bar@Baz@Qwerty@@YAXXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "?wer_hat_angst@Baz@Qwerty@@3MB", true ) != nullptr );
}

U_TEST( NameBackReferencesTest )
{
	static const char c_program_text[]=
	R"(
		namespace Foo
		{
			fn Foo(){} // Should compress namespace name 'Foo' to '0'
		}

		namespace Bar
		{
			namespace Bar
			{
				fn Third(){} // Should compress second usage of 'Bar' to '1'
			}
		}

		namespace Lol
		{
			namespace Wat
			{
				fn Lol(){} // Should compress second usage of 'Lol' to '0'
			}
		}

		namespace Same
		{
			namespace Same
			{
				namespace Same
				{
					fn WTF(){} // Should compress all usage of 'Same' to '1'
					fn Same(){} // Should compress all usage of 'Same' to '0'
				}
			}
		}

		namespace Qwerty
		{
			struct Abc{}
			struct Def{}
			fn Ghi(Abc abc, Def def){} // Should reuse 'Qwerty' for type names
		}

		fn ExternalGhi(Qwerty::Abc abc, Qwerty::Def def) {} // Should reuse 'Qwerty' for type names

		namespace MitDemLebenKommtDerTod
		{
			namespace DieSonneSchlucktDasMorgenrot
			{
				struct MitDemLebenKommtDerTod {}
				fn EinHerzVerliertDieLetzteSchlacht( MitDemLebenKommtDerTod m ){} // Should reuse namespace name for type name.
			}
		}

		namespace Fisting
		{
			namespace Is
			{
				namespace ThreeHundred
				{
					namespace Bucks
					{
						struct Cum{}
						fn RipOffOurPants(Cum cum){} // Should create series of backreferences for param type name, like '1234'
					}
				}
			}
		}

		namespace Ten { // Can't use backreference here - limit is reached
		namespace Five { // duplicate - should back-reference it
		namespace Ten {
		namespace Nine {
		namespace Eight {
		namespace Seven {
		namespace Six {
		namespace Five {
		namespace Four {
		namespace Three {
		namespace Two {
		namespace One {
		fn Zero(){}
		} } } } } } } } } } } }
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?Foo@0@YAXXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Third@Bar@1@YAXXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Lol@Wat@0@YAXXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?WTF@Same@11@YAXXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Same@000@YAXXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Ghi@Qwerty@@YAXUAbc@1@UDef@1@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?ExternalGhi@@YAXUAbc@Qwerty@@UDef@2@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?EinHerzVerliertDieLetzteSchlacht@DieSonneSchlucktDasMorgenrot@MitDemLebenKommtDerTod@@YAXU212@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?RipOffOurPants@Bucks@ThreeHundred@Is@Fisting@@YAXUCum@1234@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Zero@One@Two@Three@Four@Five@Six@Seven@Eight@Nine@Ten@5Ten@@YAXXZ" ) != nullptr );
}

} // namespace U
