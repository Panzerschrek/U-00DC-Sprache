#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( CheckTestsWithSameNameInDifferentModules ){}

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

		fn FooByte8Bar( byte8 x ){}
		fn FooByte16Bar( byte16 x ){}
		fn FooByte32Bar( byte32 x ){}
		fn FooByte64Bar( byte64 x ){}
		fn FooByte128Bar( byte128 x ){}
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

	U_TEST_ASSERT( engine->FindFunctionNamed( "?FooByte8Bar@@YAXUbyte8@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?FooByte16Bar@@YAXUbyte16@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?FooByte32Bar@@YAXUbyte32@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?FooByte64Bar@@YAXUbyte64@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?FooByte128Bar@@YAXUbyte128@@@Z" ) != nullptr );
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

U_TEST( ParamsBackReferencesTest )
{
	static const char c_program_text[]=
	R"(
		fn IntArgs( i32 a, i32 b, i32 c ){} // Simple fundamental type params should not be compressed
		fn ConstRefFundamentalArgs( i32 a, i32& b, i32& c, f32 d, f32& f, f32& g ){} // Simple fundamental type params doesn't count, but reference params should use backreferences

		struct SomeStruct{}
		fn SameStructArg(SomeStruct a, SomeStruct b){} // Should use back-reference for type
		fn SameStructRefArg(SomeStruct& a, SomeStruct& b) {} // Should use back-reference for type including reference prefix
		fn SameStructValueAndRefArg(SomeStruct a, SomeStruct& b){} // Should use here only name backreference
		fn SameStructRefAndValueArg(SomeStruct& a, SomeStruct b){} // Should use here only name backreference
		fn SameStructMutAndImutRefArg(SomeStruct &mut a, SomeStruct &imut b){} // Should use here only name backreference
		fn SameStructImutAndMutRefArg(SomeStruct &imut a, SomeStruct &mut b){} // Should use here only name backreference
		fn ValueArgDifferentMutability(SomeStruct mut a, SomeStruct imut b){} // Should use back-reference since value arg mutability doesn't matter

		// Should not use params backreference, only name component backreference
		namespace Abc { struct FF{} }
		namespace Def { struct FF{} }
		fn TwoFF( Abc::FF abc, Def::FF def ){}

		// Should use params backreferences together with name component backreferences
		fn FourFF( Abc::FF abc0, Def::FF def0, Abc::FF abc1, Def::FF def1 ) {}

		// Should not use backreference for return type
		fn Pass( SomeStruct mut s ) : SomeStruct  { return move(s); }
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?IntArgs@@YAXHHH@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?ConstRefFundamentalArgs@@YAXHAEBH0MAEBM1@Z" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?SameStructArg@@YAXUSomeStruct@@0@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?SameStructRefArg@@YAXAEBUSomeStruct@@0@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?SameStructValueAndRefArg@@YAXUSomeStruct@@AEBU1@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?SameStructRefAndValueArg@@YAXAEBUSomeStruct@@U1@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?SameStructMutAndImutRefArg@@YAXAEAUSomeStruct@@AEBU1@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?SameStructImutAndMutRefArg@@YAXAEBUSomeStruct@@AEAU1@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?ValueArgDifferentMutability@@YAXUSomeStruct@@0@Z" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?TwoFF@@YAXUFF@Abc@@U1Def@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?FourFF@@YAXUFF@Abc@@U1Def@@01@Z" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?Pass@@YA?AUSomeStruct@@U1@@Z" ) != nullptr );
}

U_TEST( ReturnValueManglingTest )
{
	static const char c_program_text[]=
	R"(
		auto g_zero= 0;
		fn IntRet() : i32 { return g_zero; }
		fn IntRefRet() : i32& { return g_zero; }
		fn IntMutRefRet() : i32 &mut { unsafe{ return cast_mut(g_zero); } }
		fn IntPtrRet() : $(i32) { unsafe{ return $<(IntMutRefRet()); } }

		struct KpssSs{}
		var KpssSs g_kpss_ss= zero_init;
		fn StructRet() : KpssSs { return g_kpss_ss; }
		fn StructRefRet() : KpssSs& { return g_kpss_ss; }
		fn StructMutRefRet() : KpssSs &mut { unsafe{ return cast_mut(StructRefRet()); } }
		fn StructPtrRet() : $(KpssSs) { unsafe{ return $<(StructMutRefRet()); } }
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?IntRet@@YAHXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?IntRefRet@@YAAEBHXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?IntMutRefRet@@YAAEAHXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?IntPtrRet@@YAPEAHXZ" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?StructRet@@YA?AUKpssSs@@XZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?StructRefRet@@YAAEBUKpssSs@@XZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?StructMutRefRet@@YAAEAUKpssSs@@XZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?StructPtrRet@@YAPEAUKpssSs@@XZ" ) != nullptr );
}

U_TEST( EnumsManglingTest )
{
	static const char c_program_text[]=
	R"(
		enum SomeEnum{ One, Two, Three }
		fn RetEnum() : SomeEnum { return SomeEnum::One; }
		fn TakeEnum(SomeEnum e){}
		fn Pass(SomeEnum e) : SomeEnum { return e; }
		fn EnumCmp(SomeEnum a, SomeEnum b) : bool { return a == b; }

		namespace qwe
		{
			namespace wasd
			{
				enum Another{ Zero, One }
				fn Select(Another a, Another b) : Another { return a; }
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?RetEnum@@YA?AW4SomeEnum@@XZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?TakeEnum@@YAXW4SomeEnum@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Pass@@YA?AW4SomeEnum@@W41@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?EnumCmp@@YA_NW4SomeEnum@@0@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Select@wasd@qwe@@YA?AW4Another@12@W4312@0@Z" ) != nullptr );
}

U_TEST( ClassMembersManglingTest )
{
	static const char c_program_text[]=
	R"(
		class SomeClass
		{
			fn Foo(){}
			fn Bar(this){}
			fn Baz(mut this){}
			fn Compare(SomeClass& a, SomeClass& b){}
		}

		template</type T/>
		struct Box
		{
			fn BoxFunc(){}
		}
		type IntBox= Box</i32/>;

		namespace Qwerty
		{
			template</type A, type B/> struct Pair
			{
				A a; B b;

				fn PairFunc(){}
			}

			struct InnerStruct{}
			type InnerStructPair = Pair</InnerStruct, InnerStruct/>;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?Foo@SomeClass@@YAXXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Bar@SomeClass@@YAXAEBU1@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Baz@SomeClass@@YAXAEAU1@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Compare@SomeClass@@YAXAEBU1@0@Z" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?BoxFunc@?$Box@H@@YAXXZ" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?PairFunc@?$Pair@UInnerStruct@Qwerty@@U12@@Qwerty@@YAXXZ" ) != nullptr );
}

U_TEST( TemplateFunctionsManglingTest )
{
	static const char c_program_text[]=
	R"(
		template<//> fn NoArgs(){}

		template</type T/> fn GetZero() : T { return T(0); }

		namespace Qwerty
		{
			template</type A, type B/> fn PerformCast(A a) : B { return B(a); }
		}

		template</i32 RES/> fn GetSomeConst() : i32 { return RES; }

		template</u64 RES/> fn GetSomeConst64() : u64 { return RES; }

		template</type T/> fn MakeItZero( T &mut t ) : T &mut { return t; }
		struct SomeStruct{}

		namespace Bar
		{
			template</type T/> fn PassMut( T &mut t ) : T &mut { return t; }
			struct Gothminister{}
		}

		fn Foo()
		{
			NoArgs();
			GetZero</i32/>();
			GetZero</f64/>();
			Qwerty::PerformCast</f32, u32/>(0.25f);

			GetSomeConst</3/>();
			GetSomeConst</42/>();
			GetSomeConst</-7/>();
			GetSomeConst</-65784/>();
			GetSomeConst</654328757/>();

			GetSomeConst64</0xFFFFFFFFFFFFFFFFu64/>();
			GetSomeConst64</0x7FFFFFFFFFFFFFFFu64/>();

			var SomeStruct mut s= zero_init;
			MakeItZero(s);

			var Bar::Gothminister mut gm= zero_init;
			Bar::PassMut(gm);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "??$NoArgs@@@YAXXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "??$GetZero@H@@YAHXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "??$GetZero@N@@YANXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "??$PerformCast@MI@Qwerty@@YAIM@Z" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "??$GetSomeConst@H$02@@YAHXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "??$GetSomeConst@H$0CK@@@YAHXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "??$GetSomeConst@H$0?6@@YAHXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "??$GetSomeConst@H$0?BAAPI@@@YAHXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "??$GetSomeConst@H$0CHAAEDLF@@@YAHXZ" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "??$GetSomeConst64@_K$0PPPPPPPPPPPPPPPP@@@YA_KXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "??$GetSomeConst64@_K$0HPPPPPPPPPPPPPPP@@@YA_KXZ" ) != nullptr );

	// Should duplicate "SomeStruct" because separate backreferences table created for template.
	U_TEST_ASSERT( engine->FindFunctionNamed( "??$MakeItZero@USomeStruct@@@@YAAEAUSomeStruct@@AEAU0@@Z" ) != nullptr );
	// Should duplicate both "UGothminister" and "Bar".
	U_TEST_ASSERT( engine->FindFunctionNamed( "??$PassMut@UGothminister@Bar@@@Bar@@YAAEAUGothminister@0@AEAU10@@Z" ) != nullptr );
}

U_TEST( TemplateClassesManglingTest0 )
{
	static const char c_program_text[]=
	R"(
		template</type T/>
		struct Box
		{
			T boxed;
		}

		fn UnboxInt(Box</i32/> b) : i32 { return b.boxed; }

		fn SwapBoxes(Box</char8/> &mut a, Box</char8/> &mut b){}

		namespace Qwerty
		{
			template</type A, type B/>
			struct Pair{ A a; B b; }

			fn ZeroPair( Pair</f32, f64/> &mut p ) {}

			struct InnerStruct{}
			fn ZeroPairOfStructs( Pair</ InnerStruct, InnerStruct /> &mut p ) {}
		}

		fn GetFirst( Qwerty::Pair</u32, u64/> p ) : u32 { return p.a; }

		template</type A, type B, type C/>
		struct MyTuple
		{
			A a; B b; C c;
		}

		struct SomeStruct{}

		fn Foo(MyTuple</SomeStruct, SomeStruct, SomeStruct/> t){}

		template</type T/>
		struct StrangeStruct{ T t; }

		namespace Cvb
		{
			template</type T/>
			struct StrangeStruct{ T t; }

			fn Baz(::StrangeStruct</Cvb::StrangeStruct</f32/>/> arg){}
		}

		namespace JWST
		{
			struct StrangeStruct{}
			fn Lol(::StrangeStruct</JWST::StrangeStruct/> arg){}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?UnboxInt@@YAHU?$Box@H@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?SwapBoxes@@YAXAEAU?$Box@D@@0@Z" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?ZeroPair@Qwerty@@YAXAEAU?$Pair@MN@1@@Z" ) != nullptr );

	// "Qwerty" is duplicated here because separate table of backreferences is used for template name + args.
	U_TEST_ASSERT( engine->FindFunctionNamed( "?ZeroPairOfStructs@Qwerty@@YAXAEAU?$Pair@UInnerStruct@Qwerty@@U12@@1@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?GetFirst@@YAIU?$Pair@I_K@Qwerty@@@Z" ) != nullptr );

	// There is no backreferences for class template args.
	// backreferences table is created separately for template args.
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Foo@@YAXU?$MyTuple@USomeStruct@@U1@U1@@@@Z" ) != nullptr );

	// Should duplicate here "StrangeStruct" since new template context created.
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Baz@Cvb@@YAXU?$StrangeStruct@U?$StrangeStruct@M@Cvb@@@@@Z" ) != nullptr );
	// Should not duplicate here "StrangeStruct" because arg is simple struct and no new template context created.
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Lol@JWST@@YAXU?$StrangeStruct@U0JWST@@@@@Z" ) != nullptr );
}

U_TEST( TemplateClassesManglingTest1 )
{
	static const char c_program_text[]=
	R"(
		namespace ust
		{
			template</type T/> struct vector{ T t; }
		}

		namespace Q9
		{
			struct TemplateBase
			{
				struct Param{}
				struct SignatureParam{}
			}

			struct FunctionTemplate
			{
				fn constructor( ust::vector</TemplateBase::Param/> params, ust::vector</TemplateBase::SignatureParam/> signature_params ){}
			}
		}

		template</type A, type B/> struct MyPair{ A a; B b; }
		type SomePair= MyPair</i16, u16/>;
		fn ProcessPairs( SomePair a, SomePair &imut b, SomePair &mut c ){}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	// Should create backreferences for template types.
	U_TEST_ASSERT( engine->FindFunctionNamed( "?constructor@FunctionTemplate@Q9@@YAXAEAU12@U?$vector@UParam@TemplateBase@Q9@@@ust@@U?$vector@USignatureParam@TemplateBase@Q9@@@4@@Z" ) != nullptr );

	// Should reuse backreference for template class name.
	U_TEST_ASSERT( engine->FindFunctionNamed( "?ProcessPairs@@YAXU?$MyPair@FG@@AEBU1@AEAU1@@Z" ) != nullptr );
}

U_TEST( ArraysManglingTest )
{
	static const char c_program_text[]=
	R"(
		template</type T/> struct Box { T t; }

		fn Box4Int( Box</[i32, 4]/> b ){}
		fn Box100Float(Box</[f32, 100]/> b ) {}
		fn BoxMat4x7x23(Box</[[[bool, 23], 7], 4]/> b ) {}

		fn FooIntVec4( [ i32, 4 ] v ){}
		fn FooFloatVec123456Ref( [f32, 123456 ] & v ){}

		type Mat4= [ [ f64, 4 ], 4 ];
		fn MultiplyMat4( Mat4& a, Mat4& b ) : Mat4 { return a; }

		fn FloatVec3Len( [ f32, 3 ]& v ) : f32 { return 0.0f; }
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?Box4Int@@YAXU?$Box@$$BY03H@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Box100Float@@YAXU?$Box@$$BY0GE@M@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?BoxMat4x7x23@@YAXU?$Box@$$BY236BH@_N@@@Z" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?FooIntVec4@@YAXY03H@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?MultiplyMat4@@YAY133NAEBY133N0@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?FloatVec3Len@@YAMAEBY02M@Z" ) != nullptr );
}

U_TEST( RawPointersManglingTest )
{
	static const char c_program_text[]=
	R"(
		template</type T/> struct Box { T t; }

		fn BoxIntPtr( Box</$(i32)/> b ){}
		fn BoxIntPtrPtr( Box</$($(i32))/> b ){}
		fn BoxVec4IntPtr( Box</$([i32, 4])/> b ){}

		struct SomeStruct{}
		fn BoxSomeStructPtr(Box</$(SomeStruct)/> b) {}

		fn SomeStructPtr($(SomeStruct) ptr){}
		fn SomeStructPtrPtrx2($($(SomeStruct)) ptr0, $($(SomeStruct)) ptr1){}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?BoxIntPtr@@YAXU?$Box@PEAH@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?BoxIntPtrPtr@@YAXU?$Box@PEAPEAH@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?BoxVec4IntPtr@@YAXU?$Box@PEAY03H@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?BoxSomeStructPtr@@YAXU?$Box@PEAUSomeStruct@@@@@Z" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?SomeStructPtr@@YAXPEAUSomeStruct@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?SomeStructPtrPtrx2@@YAXPEAPEAUSomeStruct@@0@Z" ) != nullptr );
}

U_TEST( TuplesManglingTest )
{
	static const char c_program_text[]=
	R"(
		template</type T/> fn AllocT() { var T t= zero_init; }

		fn Foo()
		{
			AllocT</ tup[ i32, f64, bool ] />();
		}

		fn PassTuple( tup[ f32, u64 ] t ){}
		fn PassTupleRef( tup[ bool, f64, i32 ]& t ){}
		fn TupleRet() : tup[bool, char8] { var tup[bool, char8] t= zero_init; return t; }

		type Qwerty= tup[ f64, f32, bool, i64, char8 ];
		fn QwertyFunc( Qwerty a, Qwerty &mut b, Qwerty &imut c ){}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "??$AllocT@U?$tup@HN_N@@@@YAXXZ" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?PassTuple@@YAXU?$tup@M_K@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?PassTupleRef@@YAXAEBU?$tup@_NNH@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?TupleRet@@YA?AU?$tup@_ND@@XZ" ) != nullptr );

	// Should compress tuple type like regular template type.
	U_TEST_ASSERT( engine->FindFunctionNamed( "?QwertyFunc@@YAXU?$tup@NM_N_JD@@AEAU1@AEBU1@@Z" ) != nullptr );
}

U_TEST( FunctionPointersManglingTest )
{
	static const char c_program_text[]=
	R"(
		fn VoidFunc( (fn()) ptr ){}
		fn IntArgFunc( (fn(i32 x)) ptr ){}
		fn IntRetFunc( (fn() : i32) ptr ){}
		fn IntArgAndRetFunc( (fn(i32 x) : i32) ptr ){}
		fn RefArgFunc( (fn(f32& x)) ptr ){}
		fn MutRefArgFunc( (fn(f32 &mut x)) ptr ){}
		fn RefRetFunc( (fn() : char16&) ptr ){}
		fn MutRefRetFunc( (fn() : char16 &mut) ptr ){}
		fn TwoRefArgsFunc( (fn( u32& x, u32& y ) ) ptr ){}
		fn PassRefFunc( (fn( u8& x ) : u8& ) ptr ){}

		struct SomeStruct{}
		fn PassStructRefFunc( (fn( SomeStruct& x ) : SomeStruct& ) ptr ){}
		fn TwoStructMutRefArgsFunc( (fn( SomeStruct &mut x, SomeStruct &mut y ) ) ptr ){}
		fn StructRetFunc( (fn() : SomeStruct ) ptr ){}

		fn TwoFuncsArgs( (fn()) ptr0, (fn()) ptr1 ) {}

		template</type A, type B/> struct Box</ fn(B b) : A /> {}
		fn Foo( Box</ fn(i32 arg) : i32 /> b ){}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?VoidFunc@@YAXP6AXXZ@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?IntArgFunc@@YAXP6AXH@Z@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?IntRetFunc@@YAXP6AHXZ@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?IntArgAndRetFunc@@YAXP6AHH@Z@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?RefArgFunc@@YAXP6AXAEBM@Z@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?MutRefArgFunc@@YAXP6AXAEAM@Z@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?RefRetFunc@@YAXP6AAEB_SXZ@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?MutRefRetFunc@@YAXP6AAEA_SXZ@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?TwoRefArgsFunc@@YAXP6AXAEBI0@Z@Z" ) != nullptr ); // Should use params backreferences here
	U_TEST_ASSERT( engine->FindFunctionNamed( "?PassRefFunc@@YAXP6AAEBEAEBEU?$_RR@$0A@$0PP@@@@Z@Z" ) != nullptr ); // Should not use backreference - return value doesn't count

	U_TEST_ASSERT( engine->FindFunctionNamed( "?PassStructRefFunc@@YAXP6AAEBUSomeStruct@@AEBU1@U?$_RR@$0A@$0PP@@@@Z@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?TwoStructMutRefArgsFunc@@YAXP6AXAEAUSomeStruct@@0@Z@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?StructRetFunc@@YAXP6A?AUSomeStruct@@XZ@Z" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?TwoFuncsArgs@@YAXP6AXXZ0@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Foo@@YAXU?$Box@P6AHH@Z@@@Z" ) != nullptr );
}

U_TEST( OperatorsMangling_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct Box
		{
			u32 x;

			op+( Box &imut b ) : Box
			{
				return b;
			}
			op-( Box &imut b ) : Box
			{
				var Box r{ .x= -b.x };
				return r;
			}

			op+( Box &imut a, Box &imut b ) : Box
			{
				var Box r{ .x= a.x + b.x };
				return r;
			}
			op-( Box &imut a, Box &imut b ) : Box
			{
				var Box r{ .x= a.x - b.x };
				return r;
			}
			op*( Box &imut a, Box &imut b ) : Box
			{
				var Box r{ .x= a.x * b.x };
				return r;
			}
			op/( Box &imut a, Box &imut b ) : Box
			{
				var Box r{ .x= a.x / b.x };
				return r;
			}
			op%( Box &imut a, Box &imut b ) : Box
			{
				var Box r{ .x= a.x % b.x };
				return r;
			}

			op==( Box &imut a, Box &imut b ) : bool
			{
				return a.x == b.x;
			}
			op<=>( Box &imut a, Box &imut b ) : i32
			{
				return a.x <=> b.x;
			}

			op&( Box &imut a, Box &imut b ) : Box
			{
				var Box r{ .x= a.x & b.x };
				return r;
			}
			op|( Box &imut a, Box &imut b ) : Box
			{
				var Box r{ .x= a.x | b.x };
				return r;
			}
			op^( Box &imut a, Box &imut b ) : Box
			{
				var Box r{ .x= a.x ^ b.x };
				return r;
			}

			op<<( Box &imut a, Box &imut b ) : Box
			{
				var Box r{ .x= a.x << b.x };
				return r;
			}
			op>>( Box &imut a, Box &imut b ) : Box
			{
				var Box r{ .x= a.x >> b.x };
				return r;
			}

			op+=( Box &mut a, Box &imut b )
			{
				a.x+= b.x;
			}
			op-=( Box &mut a, Box &imut b )
			{
				a.x-= b.x;
			}
			op*=( Box &mut a, Box &imut b )
			{
				a.x*= b.x;
			}
			op/=( Box &mut a, Box &imut b )
			{
				a.x/= b.x;
			}
			op%=( Box &mut a, Box &imut b )
			{
				a.x%= b.x;
			}

			op&=( Box &mut a, Box &imut b )
			{
				a.x&= b.x;
			}
			op|=( Box &mut a, Box &imut b )
			{
				a.x|= b.x;
			}
			op^=( Box &mut a, Box &imut b )
			{
				a.x^= b.x;
			}

			op<<=( Box &mut a, Box &imut b )
			{
				a.x <<= b.x;
			}
			op>>=( Box &mut a, Box &imut b )
			{
				a.x >>= b.x;
			}

			op!( Box &imut b ) : bool
			{
				return b.x != 0u;
			}
			op~( Box &imut b ) : Box
			{
				var Box r{ .x= ~b.x };
				return r;
			}

			op++( Box &mut a )
			{
				++a.x;
			}
			op--( Box &mut a )
			{
				--a.x;
			}

			op=( Box &mut a, Box &imut b )
			{
				a.x= b.x;
			}

			op()( Box &imut this_ )
			{
			}

			template</type T/>
			op()( Box &imut this_, T t )
			{
			}

			op[]( Box &imut this_, u32 ind ) : u32
			{
				return this_.x + ind;
			}
		}

		fn Foo()
		{
			var Box box{ .x= 0u };
			box(14.2f);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "??HBox@@YA?AU0@AEBU0@@Z" ) != nullptr ); // unary +
	U_TEST_ASSERT( engine->FindFunctionNamed( "??GBox@@YA?AU0@AEBU0@@Z" ) != nullptr ); // unary -

	U_TEST_ASSERT( engine->FindFunctionNamed( "??HBox@@YA?AU0@AEBU0@0@Z" ) != nullptr ); // binary +
	U_TEST_ASSERT( engine->FindFunctionNamed( "??GBox@@YA?AU0@AEBU0@0@Z" ) != nullptr ); // binary -
	U_TEST_ASSERT( engine->FindFunctionNamed( "??DBox@@YA?AU0@AEBU0@0@Z" ) != nullptr ); // *
	U_TEST_ASSERT( engine->FindFunctionNamed( "??KBox@@YA?AU0@AEBU0@0@Z" ) != nullptr ); // /
	U_TEST_ASSERT( engine->FindFunctionNamed( "??LBox@@YA?AU0@AEBU0@0@Z" ) != nullptr ); // %

	U_TEST_ASSERT( engine->FindFunctionNamed( "??8Box@@YA_NAEBU0@0@Z" ) != nullptr ); // ==
	U_TEST_ASSERT( engine->FindFunctionNamed( "??__MBox@@YAHAEBU0@0@Z" ) != nullptr ); // <=>

	U_TEST_ASSERT( engine->FindFunctionNamed( "??IBox@@YA?AU0@AEBU0@0@Z" ) != nullptr ); // &
	U_TEST_ASSERT( engine->FindFunctionNamed( "??UBox@@YA?AU0@AEBU0@0@Z" ) != nullptr ); // |
	U_TEST_ASSERT( engine->FindFunctionNamed( "??TBox@@YA?AU0@AEBU0@0@Z" ) != nullptr ); // ^

	U_TEST_ASSERT( engine->FindFunctionNamed( "??6Box@@YA?AU0@AEBU0@0@Z" ) != nullptr ); // <<
	U_TEST_ASSERT( engine->FindFunctionNamed( "??5Box@@YA?AU0@AEBU0@0@Z" ) != nullptr ); // >>

	U_TEST_ASSERT( engine->FindFunctionNamed( "??YBox@@YAXAEAU0@AEBU0@@Z" ) != nullptr ); // +=
	U_TEST_ASSERT( engine->FindFunctionNamed( "??ZBox@@YAXAEAU0@AEBU0@@Z" ) != nullptr ); // -=
	U_TEST_ASSERT( engine->FindFunctionNamed( "??XBox@@YAXAEAU0@AEBU0@@Z" ) != nullptr ); // *=
	U_TEST_ASSERT( engine->FindFunctionNamed( "??_0Box@@YAXAEAU0@AEBU0@@Z" ) != nullptr ); // /=
	U_TEST_ASSERT( engine->FindFunctionNamed( "??_1Box@@YAXAEAU0@AEBU0@@Z" ) != nullptr ); // %=

	U_TEST_ASSERT( engine->FindFunctionNamed( "??_4Box@@YAXAEAU0@AEBU0@@Z" ) != nullptr ); // &=
	U_TEST_ASSERT( engine->FindFunctionNamed( "??_5Box@@YAXAEAU0@AEBU0@@Z" ) != nullptr ); // |=
	U_TEST_ASSERT( engine->FindFunctionNamed( "??_6Box@@YAXAEAU0@AEBU0@@Z" ) != nullptr ); // ^=

	U_TEST_ASSERT( engine->FindFunctionNamed( "??_3Box@@YAXAEAU0@AEBU0@@Z" ) != nullptr ); // <<=
	U_TEST_ASSERT( engine->FindFunctionNamed( "??_2Box@@YAXAEAU0@AEBU0@@Z" ) != nullptr ); // >>=

	U_TEST_ASSERT( engine->FindFunctionNamed( "??7Box@@YA_NAEBU0@@Z" ) != nullptr ); // !
	U_TEST_ASSERT( engine->FindFunctionNamed( "??SBox@@YA?AU0@AEBU0@@Z" ) != nullptr ); // !

	U_TEST_ASSERT( engine->FindFunctionNamed( "??EBox@@YAXAEAU0@@Z" ) != nullptr ); // ++
	U_TEST_ASSERT( engine->FindFunctionNamed( "??FBox@@YAXAEAU0@@Z" ) != nullptr ); // --

	U_TEST_ASSERT( engine->FindFunctionNamed( "??4Box@@YAXAEAU0@AEBU0@@Z" ) != nullptr ); // =
	U_TEST_ASSERT( engine->FindFunctionNamed( "??RBox@@YAXAEBU0@@Z" ) != nullptr ); // ()
	U_TEST_ASSERT( engine->FindFunctionNamed( "??ABox@@YAIAEBU0@I@Z" ) != nullptr ); // []

	U_TEST_ASSERT( engine->FindFunctionNamed( "??$?RM@Box@@YAXAEBU0@M@Z" ) != nullptr ); // Template op()</f32/>
}

U_TEST( GeneratorsMangling_Test0 )
{
	// Generator type is encoded like template with two params - extended return type and inner reference kind, encoded as variable param of type u32.
	// 0 - means no references inside, 1 - immutable references inside, 2 - mutable references inside.

	static const char c_program_text[]=
	R"(
		type Gen= generator : i32;
		fn Foo( Gen gen ) {}
		fn Bar( f32 x, Gen gen, u32 z ) {}

		type ImutRefGen= generator'imut' : f64;
		fn Baz( ImutRefGen gen ) {}

		type MutRefRetGen= generator'mut, imut' : char8 &mut;
		fn Lol( MutRefRetGen gen ) {}

		type NonSyncGen = generator non_sync : u16;
		fn Kek( NonSyncGen gen ) {}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	// Functions with generator param.
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Foo@@YAXU?$generator@H@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Bar@@YAXMU?$generator@H@@I@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Baz@@YAXU?$generator@NI$0A@@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Lol@@YAXU?$generator@AEADI$00I$0A@@@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Kek@@YAXU?$generator@G_N$00@@@Z" ) != nullptr );

	// Generated generator type destructors.
	U_TEST_ASSERT( engine->FindFunctionNamed( "?destructor@?$generator@H@@YAXAEAU1@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?destructor@?$generator@NI$0A@@@YAXAEAU1@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?destructor@?$generator@AEADI$00I$0A@@@YAXAEAU1@@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?destructor@?$generator@G_N$00@@YAXAEAU1@@Z" ) != nullptr );
}

U_TEST( GeneratorsMangling_Test1 )
{
	// In MSVC mangling return type is also encoded in function name.
	// For generator function return type is generator type.
	static const char c_program_text[]=
	R"(
		fn generator Foo() : i32 {}
		fn Bar() : (generator : i32) { halt; } // Type of this function is identical to previous.
		fn generator nomangle NoMangleGenerator() : f32 {}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Foo@@YA?AU?$generator@H@@XZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Bar@@YA?AU?$generator@H@@XZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "NoMangleGenerator" ) != nullptr );
}

U_TEST( VirtualTableManglingTest )
{
	static const char c_program_text[]=
	R"(
		class SukaBlat polymorph{}

		namespace Lol
		{
			class WTF polymorph{}

			template</type T/> class Hämatom polymorph { T t; }
			type Musik= Hämatom</bool/>;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "??_7SukaBlat@@6B@", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "??_7WTF@Lol@@6B@", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "??_7?$Hämatom@_N@Lol@@6B@", true ) != nullptr );
}

U_TEST( SpecialFunctionTypeDataManglingTest )
{
	static const char c_program_text[]=
	R"(
		// Should encode unsafe flag
		fn VoidParamUnsafeFunction( ( fn() unsafe ) f ) { }
		fn IntParamUnsafeFunction( ( fn( i32 x ) unsafe ) f ) {}

		fn RefPassFunction( ( fn( i32& x ) : i32& ) f ) {}

		struct S{ i32& x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "1a", "0_" ] ];
		fn RefPollutionFunction( ( fn( i32 & x, S &mut s ) @(pollution) ) f ) {}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?VoidParamUnsafeFunction@@YAXP6AXUunsafe@@@Z@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?IntParamUnsafeFunction@@YAXP6AXHUunsafe@@@Z@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?RefPassFunction@@YAXP6AAEBHAEBHU?$_RR@$0A@$0PP@@@@Z@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?RefPollutionFunction@@YAXP6AXAEBHAEAUS@@U?$_RP@$00$0A@$0A@$0PP@@@@Z@Z" ) != nullptr );
}

U_TEST( TypeinfoClassManglingTest )
{
	static const char c_program_text[]=
	R"(
		fn Foo( typeof(typeinfo</i32/>) tt ){}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?Foo@@YAXU?$_TI@H@@@Z" ) != nullptr );
}

U_TEST( CallingConventionTest )
{
	static const char c_program_text[]=
	R"(
		fn Foo() call_conv("C"){}
		fn Bar() call_conv("fast"){}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?Foo@@YAXXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Bar@@YIXXZ" ) != nullptr );
}

} // namespace

} // namespace U
