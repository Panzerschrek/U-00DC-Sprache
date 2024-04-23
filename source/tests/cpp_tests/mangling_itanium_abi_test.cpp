#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( CheckTestsWithSameNameInDifferentModules ){}

U_TEST( NamespacesManglingTest )
{
	static const char c_program_text[]=
	R"(
		fn Foo(){}
		namespace BlaBla
		{
			fn Foo(){}

			namespace Goblin
			{
				fn Tupichok(){}
			}
		}

		struct StructIsNamespace
		{
			fn Foo() {}

			struct InnerStruct
			{
				fn Ratatatata(){}
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Foov" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN6BlaBla3FooEv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN6BlaBla6Goblin8TupichokEv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN17StructIsNamespace3FooEv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN17StructIsNamespace11InnerStruct10RatatatataEv" ) != nullptr );
}

U_TEST( FunctionsParametersManglingTest )
{
	static const char c_program_text[]=
	R"(
		fn Foo(){}
		fn Foo( u32 x ) {}
		fn Foo( i32 x, i32 y ) {}
		fn Foo( bool b ) {}
		fn Foo( f32 & mut x ) {}
		fn Foo( f32 &imut x ) {}
		fn Foo( i64  mut x, f32 f ) {}
		fn Foo( u64 imut x, f64 d ) {}

		struct CustomType{}
		type ArrayType= [ i32, 55 ];

		fn Foo( CustomType t ) {}
		fn Foo( CustomType &mut t ) {}
		fn Foo( ArrayType &mut a ) {}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Foov" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Fooj" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Fooii" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Foob" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooRf" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooRKf" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Fooxf" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Fooyd" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Foo10CustomType" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooR10CustomType" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooRA55_i" ) != nullptr );
}

U_TEST( VoidParamManglingTest )
{
	static const char c_program_text[]=
	R"(
		fn NoArgs(){}
		fn SingleVoidArg(void v){}
		fn TwoVoidArgs(void v0, void v1){}
		fn VoidAndNonVoidArgs(i32 &mut x, f32& y, bool z, void v0, char8 c, char16& cc, void v1 ){}
		fn VoidRefArg( void& v ){}
		fn VoidMutRefArg( void &mut v ){}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z6NoArgsv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z13SingleVoidArgKv" ) != nullptr ); // Should add "const" prefix
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z11TwoVoidArgsKvS_" ) != nullptr ); // Should add "const" prefix and create substitution
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z18VoidAndNonVoidArgsRiRKfbKvcRKDsS2_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z10VoidRefArgRKv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z13VoidMutRefArgRv" ) != nullptr );
}

U_TEST( ClassmethodsManglingTest )
{
	static const char c_program_text[]=
	R"(
		struct SomeStruct
		{
			fn Foo( this ) {}
			fn Bar( mut this ) {}
			fn Baz() {}
			fn constructor(){}
			fn constructor( i32 x, f32 y ){}
			fn destructor(){}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	// Here is difference between Ü and C++:
	// In Ü "this" processed as regular argument.
	// Constructors and destructors named as "constructor" and "destructor", but not "C1" and "D0".
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN10SomeStruct3FooERKS_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN10SomeStruct3BarERS_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN10SomeStruct3BazEv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN10SomeStruct11constructorERS_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN10SomeStruct11constructorERS_if" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN10SomeStruct10destructorERS_" ) != nullptr );
}

U_TEST( NamesCompressionTest )
{
	static const char c_program_text[]=
	R"(
		struct CustomType{}
		fn Foo( CustomType a, CustomType b ) {}

		namespace NnN
		{
			fn Baz( CustomType a, CustomType b ) {}
		}

		namespace Fr
		{
			struct CustomType{}
			fn Bar( CustomType a, CustomType b ) {}
		}

		fn AstFF( CustomType &mut a, CustomType &mut b ) {}
		fn FFAst( CustomType &mut a, CustomType b ) {}
		fn AstLL( CustomType &imut a, CustomType &imut b ) {}

		namespace AAA
		{
			namespace BBB
			{
				namespace CCC
				{
					struct CustomType{}
					fn Foo( CustomType &imut a, CustomType &imut b, CustomType &imut c, CustomType &mut d ) {}
					fn Bar( CustomType& mut a, CustomType &imut b ) {}
					fn Baz( CustomType a, CustomType &mut b ) {}
				}
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Foo10CustomTypeS_" ) != nullptr ); // "CustomType" is "S_"
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3NnN3BazE10CustomTypeS0_" ) != nullptr ); // "NnN" is "S_", "CustomType" iz "S1_"
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN2Fr3BarENS_10CustomTypeES0_" ) != nullptr ); // "Fr" is "S_", "Fr::CustomType" is "S0_"
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5AstFFR10CustomTypeS0_" ) != nullptr ); // "CustomType" is "S_", "CustomType &mut" is "S0_"
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5FFAstR10CustomTypeS_" ) != nullptr ); // "CustomType" is "S_"
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5AstLLRK10CustomTypeS1_" ) != nullptr ); // "CustomType" is "S_", "CustomType &mut" is "S0_", "CustmType &imut" is "S1_"
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3AAA3BBB3CCC3FooERKNS1_10CustomTypeES4_S4_RS2_" ) != nullptr ); // "AAA" is "S_", "BBB" is "S0_", CCC is "S1_", "CustomType" is "S2_", "CustomType &mut" is "S3_", "CustomType &imut" is "S4_"
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3AAA3BBB3CCC3BarERNS1_10CustomTypeERKS2_" ) != nullptr ); // "AAA" is "S_", "BBB" is "S0_", CCC is "S1_", "CustomType" is "S2_"
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3AAA3BBB3CCC3BazENS1_10CustomTypeERS2_" ) != nullptr ); // "AAA" is "S_", "BBB" is "S0_", CCC is "S1_", "CustomType" is "S2_"
}

U_TEST( GlobalVariablesManglingTest0 )
{
	static const char c_program_text[]=
	R"(
		auto constexpr I= 42;
		auto constexpr FF= 2.54f;
		auto constexpr DoubleVariable= 8.4;

		namespace SSS_Pace
		{
			var [ i32, 2 ] imut ttt[ 42, I ];
			var f32 constexpr pi= 3.1415f;
			var f64 imut pi_double= 3.1415926535;

			struct Bar
			{
				auto constexpr abcdefg= 887;
				var i32 mut BarMut= -123;
			}

			var tup[bool] mut namespace_mut[true];
		}

		struct Ball
		{
			auto constexpr rrtt= -8547;
		}

		template</type T/>
		struct Box
		{
			auto size= 0s;

			template</type U/>
			struct Inner
			{
				auto inner_val= 0.25f;
				var f32 mut inner_mut= inner_val;
			}
		}
		type FBoxInner= Box</f32/>::Inner</u32/>;

		auto mut GlobalMut= false;
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "I", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "FF", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "DoubleVariable", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZN8SSS_Pace3tttE", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZN8SSS_Pace2piE", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZN8SSS_Pace9pi_doubleE", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZN8SSS_Pace3Bar7abcdefgE", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZN8SSS_Pace3Bar6BarMutE", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZN8SSS_Pace13namespace_mutE", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZN4Ball4rrttE", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZN3BoxIfE4sizeE", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZN3BoxIfE5InnerIjE9inner_valE", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZN3BoxIfE5InnerIjE9inner_mutE", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "GlobalMut", true ) != nullptr );
}

U_TEST( OperatorsManglingTest )
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

			op[]( Box &imut this_, u32 ind ) : u32
			{
				return this_.x + ind;
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxplERKS_S1_" ) != nullptr ); // binary +
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxmiERKS_S1_" ) != nullptr ); // binary -
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxmlERKS_S1_" ) != nullptr ); // binary *
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxdvERKS_S1_" ) != nullptr ); // /
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxrmERKS_S1_" ) != nullptr ); // %

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxeqERKS_S1_" ) != nullptr ); // ==
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxssERKS_S1_" ) != nullptr ); // <=>

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxanERKS_S1_" ) != nullptr ); // &
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxorERKS_S1_" ) != nullptr ); // |
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxeoERKS_S1_" ) != nullptr ); // ^

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxlsERKS_S1_" ) != nullptr ); // <<
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxrsERKS_S1_" ) != nullptr ); // >>

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxpLERS_RKS_" ) != nullptr ); // +=
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxmIERS_RKS_" ) != nullptr ); // -=
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxmLERS_RKS_" ) != nullptr ); // *=
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxdVERS_RKS_" ) != nullptr ); // /=
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxrMERS_RKS_" ) != nullptr ); // %=

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxaNERS_RKS_" ) != nullptr ); // &=
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxoRERS_RKS_" ) != nullptr ); // |=
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxeOERS_RKS_" ) != nullptr ); // ^=

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxlSERS_RKS_" ) != nullptr ); // <<=
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxrSERS_RKS_" ) != nullptr ); // >>=

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxntERKS_" ) != nullptr ); // !
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxcoERKS_" ) != nullptr ); // ~

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxaSERS_RKS_" ) != nullptr ); // =
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxppERS_" ) != nullptr ); // ++
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxmmERS_" ) != nullptr ); // --

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxclERKS_" ) != nullptr ); // ()
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxixERKS_j" ) != nullptr ); // []
}

U_TEST( FundamentalTypesManglingTest )
{
	static const char c_program_text[]=
	R"(
		fn Foo00(   i8 x ){}
		fn Foo01(   u8 x ){}
		fn Foo02(  i16 x ){}
		fn Foo03(  u16 x ){}
		fn Foo04(  i32 x ){}
		fn Foo05(  u32 x ){}
		fn Foo06(  i64 x ){}
		fn Foo07(  u64 x ){}
		fn Foo08( i128 x ){}
		fn Foo09( u128 x ){}
		fn Foo0A( f32 x ){}
		fn Foo0B( f64 x ){}
		fn Foo0C(  char8 x ){}
		fn Foo0D( char16 x ){}
		fn Foo0E( char32 x ){}
		fn Foo0F( bool x ){}
		fn Foo10(){}
		fn FooByte8Bar( byte8 x ){}
		fn FooByte16Bar( byte16 x ){}
		fn FooByte32Bar( byte32 x ){}
		fn FooByte64Bar( byte64 x ){}
		fn FooByte128Bar( byte128 x ){}
		fn SizeTypeFunc( size_type s ) {}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo00a" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo01h" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo02s" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo03t" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo04i" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo05j" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo06x" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo07y" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo08n" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo09o" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo0Af" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo0Bd" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo0Cc" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo0DDs" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo0EDi" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo0Fb" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z5Foo10v" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z11FooByte8Baru5byte8" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z12FooByte16Baru6byte16" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z12FooByte32Baru6byte32" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z12FooByte64Baru6byte64" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z13FooByte128Baru7byte128" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z12SizeTypeFuncm" ) != nullptr );
}

U_TEST( FunctionTypesMangling_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S{}
		struct OtherStruct{}

		fn Foo( (fn()) void_fn ){}
		fn Bar( (fn() : i32 ) ptr ){}
		fn Baz( (fn( i32 x ) ) ptr ){}
		fn Binary( (fn( f32 x, f32 y ) ) ptr ){}
		fn BinaryRet( (fn( f32 x, f32 y ) : bool ) ptr ){}
		fn RetS( (fn() : S ) ptr ){}
		fn Pass( (fn( OtherStruct& o ) : OtherStruct ) ptr ){}
		fn Cold( (fn() call_conv("cold") ) void_fn ){}
		fn Fast( (fn() call_conv("fast") ) void_fn ){}
		fn Unsafe( (fn() unsafe) ptr ) {}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooPFvvE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3BarPFivE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3BazPFviE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z6BinaryPFvffE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z9BinaryRetPFbffE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4RetSPF1SvE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4PassPF11OtherStructRKS_E" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4ColdPU4coldFvvE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4FastPU4fastFvvE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z6UnsafePFvv6unsafeE" ) != nullptr );
}

U_TEST( FunctionTypesMangling_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32& x; }
		var [ [ [ char8, 2 ], 2 ], 2 ] pollution[ [ "0a", "1_" ], [ "0a", "2_" ] ];
		fn Foo( ( fn( S &mut s, i32& x, i32& y ) @(pollution) ) ptr ) {}

		var[ [ char8, 2 ], 3 ] return_references[ "0_", "1_", "1a" ];
		fn Bar( ( fn( i32& x, S& s ) : i32 & @(return_references) ) ptr ) {}

		var tup[ [ [char8, 2], 1 ], [ [char8, 2], 2 ] ] return_inner_references[ [ "0_" ], [ "1_", "1a" ] ];
		fn Baz( ( fn( i32& x, S& s ) : S @(return_inner_references) ) ptr ) {}

		var[ [ char8, 2 ], 1 ] generator_return_references[ "0a" ];
		fn Lol( ( generator'imut' : i32 & @(generator_return_references) ) gen ) {}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooPFvR1SRKiS2_3_RPIXilililLc48ELc97EEilLc49ELc95EEEililLc48ELc97EEilLc50ELc95EEEEEEE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3BarPFRKiS0_RK1S3_RRIXililLc48ELc95EEilLc49ELc97EEilLc49ELc95EEEEEE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3BazPF1SRKiRKS_4_RIRIXilililLc48ELc95EEEililLc49ELc97EEilLc49ELc95EEEEEEE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Lol9generatorIRKiLj0E3_RRIXililLc48ELc97EEEEEE" ) != nullptr );
}

U_TEST( TupleTypesManglengTest )
{
	static const char c_program_text[]=
	R"(
		struct SomeStruct{}

		fn Foo( tup[] t ) {}
		fn Bar( tup[ bool ] t ) {}
		fn Baz( tup[ i32, f32, char32 ] t ) {}
		fn Lol( tup[ u16, u16, u16, u16 ] t ) {}
		fn Double( tup[ f32 , f64 ] t0, tup[ f32, f64 ] t1 ) {}
		fn SSS( tup[ SomeStruct, i32 ] t0, SomeStruct t1 ) {}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Foo3tupIE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Bar3tupIbE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Baz3tupIifDiE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Lol3tupIttttE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z6Double3tupIfdES0_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3SSS3tupI10SomeStructiES0_" ) != nullptr );
}

U_TEST( RawPointerTypeManglingTest )
{
	static const char c_program_text[]=
	R"(
		struct SomeStruct{}
		fn FooA( $(i32) x ){}
		fn FooB( $(bool) b ){}
		fn FooC( $(f32)& c ){}
		fn FooD( $(u64) &mut i ){}
		fn FooE( $($(u32)) ptr_to_ptr ) {}
		fn FooF( $(SomeStruct) s ){}
		fn FooG( $(fn(i32 x, f32& y, bool& mut z) : SomeStruct) ptr_to_function_ptr, [ i32, 4 ] arr ) {}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4FooAPi" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4FooBPb" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4FooCRKPf" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4FooDRPy" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4FooEPPj" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4FooFP10SomeStruct" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4FooGPPF10SomeStructiRKfRbEA4_i" ) != nullptr );
}

U_TEST( ClassTemplatesMangling_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct Box
		{
			T t;
			fn Foo(){}
		}

		type IntBox= Box</i32/>;
		type FloatBox= Box</f32/>;
		type IntBoxBox= Box</ Box</ i32 /> />;
		type IntBoxBoxBox= Box</ Box</ Box</ i32 /> /> />;
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxIiE3FooEv" ) != nullptr ); // Box</i32/>::Foo()
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxIfE3FooEv" ) != nullptr ); // Box</f32/>::Foo()
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxIS_IiEE3FooEv" ) != nullptr ); // Box</ Box</ i32 /> />::Foo(), "S_" = "Box"
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxIS_IS_IiEEE3FooEv" ) != nullptr ); // Box</ Box</ Box</ i32 /> /> />::Foo(), "S_" = "Box"
}

U_TEST( ClassTemplatesMangling_Test1 )
{
	static const char c_program_text[]=
	R"(
		enum E{ A, B, C }
		template</ i32 x /> struct A{ fn FunA(){} }
		template</ i32 x, u64 y /> struct B{ fn FunB(){} }
		template</ E e /> struct C{ fn FunC(){} }
		template</ byte32 b /> struct D{ fn FuncD(){} }

		type A66= A</ 66 />;
		type B_minus_5_plus_666= B</ -5, 666u64 />;
		type C_C= C</ E::C />;
		type D_D= D</ byte32(768u) />;
	)";
	auto module= BuildProgram( c_program_text );
	U_TEST_ASSERT( llvm::StructType::getTypeByName( module->getContext(), "1AILi66EE" ) != nullptr ); // A</ 66 />
	U_TEST_ASSERT( llvm::StructType::getTypeByName( module->getContext(), "1BILin5ELy666EE" ) != nullptr ); // B</ -5, 666u64 />
	U_TEST_ASSERT( llvm::StructType::getTypeByName( module->getContext(), "1CIL1E2EE" ) != nullptr ); // C</ E::C />
	U_TEST_ASSERT( llvm::StructType::getTypeByName( module->getContext(), "1DILu6byte32768EE" ) != nullptr ); // D</ byte32(768u) />

	const EnginePtr engine= CreateEngine( std::move(module) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN1AILi66EE4FunAEv" ) != nullptr ); // A</ 66 />::FunA()
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN1BILin5ELy666EE4FunBEv" ) != nullptr ); // B</ -5, 666u64 />::FunB()
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN1CIL1E2EE4FunCEv" ) != nullptr ); // C</ E::C />::FunC()
	U_TEST_ASSERT(engine->FindFunctionNamed( "_ZN1DILu6byte32768EE5FuncDEv" ) != nullptr ); // D</ byte32(768u) />::FunD()
}

U_TEST( ClassTemplatesMangling_Test2 )
{
	static const char c_program_text[]=
	R"(
		template</ i32 x /> struct A{ fn FunA(){} }
		fn Baz(A</13/> arg0, A</13/> arg1){}
		fn Tatata(A</55/> arg0, A</77/> arg1){}

		template</ type T /> struct Box{ T t; }
		template</ type T /> struct Void{ }

		namespace Abc{ namespace Def{ struct HH{} } }

		fn Lol( Box</ Abc::Def::HH /> arg0,  Void</ Abc::Def::HH /> arg1 ){}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Baz1AILi13EES0_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z6Tatata1AILi55EES_ILi77EE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Lol3BoxIN3Abc3Def2HHEE4VoidIS2_E" ) != nullptr );
}

U_TEST( ClassTemplatesMangling_Test3 )
{
	static const char c_program_text[]=
	R"(
		template<//> struct Box</i32/>
		{
			fn FunA(){}
		}

		template<//> struct NumBox</0u/>
		{
			fn FunZero(){}
		}

		template</type T/> struct Box
		{
			fn FunRegular(){}
		}

		type IntBox= Box</i32/>;
		type ZeroBox= NumBox</0u/>;
		type FloatBox= Box</f32/>;
		type Bool4Box= Box</ [ bool, 4s ] />;
	)";

	// Mangling uses signature parameters.

	auto module= BuildProgram( c_program_text );
	U_TEST_ASSERT( llvm::StructType::getTypeByName( module->getContext(), "3BoxIiE" ) != nullptr );
	U_TEST_ASSERT( llvm::StructType::getTypeByName( module->getContext(), "6NumBoxILj0EE" ) != nullptr );
	U_TEST_ASSERT( llvm::StructType::getTypeByName( module->getContext(), "3BoxIfE" ) != nullptr );
	U_TEST_ASSERT( llvm::StructType::getTypeByName( module->getContext(), "3BoxIA4_bE" ) != nullptr );

	const EnginePtr engine= CreateEngine( std::move(module) );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxIiE4FunAEv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN6NumBoxILj0EE7FunZeroEv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxIfE10FunRegularEv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxIA4_bE10FunRegularEv" ) != nullptr );
}

U_TEST( ClassTemplatesMangling_Test4 )
{
	static const char c_program_text[]=
	R"(
		namespace Abc
		{
			template</type T/> struct shared_ptr_mut{}

			template</type T/>
			struct vector
			{
				fn Foo(){}
			}

			template</ type A, type B />
			struct pair
			{
				fn Baz(){}
			}
		}

		type VV= Abc::vector</ Abc::shared_ptr_mut</f32/> />;
		type PP= Abc::pair</ Abc::shared_ptr_mut</i64/>, Abc::shared_ptr_mut</i64/> />;
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3Abc6vectorINS_14shared_ptr_mutIfEEE3FooEv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3Abc4pairINS_14shared_ptr_mutIxEES2_E3BazEv" ) != nullptr );
}

U_TEST( FunctionTemplatesMangling_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> fn Foo(){}

		struct Abc{}
		fn Main()
		{
			Foo</i32/>();
			Foo</f32/>();
			Foo</Abc/>();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooIiEvv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooIfEvv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooI3AbcEvv" ) != nullptr );
}

U_TEST( FunctionTemplatesMangling_Test1 )
{
	static const char c_program_text[]=
	R"(
		template</ type A, type B, type C /> fn Foo(){}

		struct Abc{}
		fn Main()
		{
			Foo</i32, i32, i32/>();
			Foo</i32, f32, i32/>();
			Foo</bool, u16, u32/>();
			Foo</Abc, Abc, Abc/>();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooIiiiEvv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooIifiEvv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooIbtjEvv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooI3AbcS0_S0_Evv" ) != nullptr );
}

U_TEST( FunctionTemplatesMangling_Test2 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> fn Foo(T t){}

		struct Abc{}
		fn Main()
		{
			Foo(66);
			Foo(0.25f);
			Foo(Abc());
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooIiEvi" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooIfEvf" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooI3AbcEvS0_" ) != nullptr );
}

U_TEST( FunctionTemplatesMangling_Test3 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> fn Foo(T t){}

		namespace Abc
		{
			struct default_hasher
			{
				template</type T/> fn hash( T& t ) {}
			}

			struct InnerType{}
		}

		struct OuterType{}

		fn Main()
		{
			Abc::default_hasher::hash(0u);
			Abc::default_hasher::hash(Abc::InnerType());
			Abc::default_hasher::hash(OuterType());
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3Abc14default_hasher4hashIjEEvRKj" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3Abc14default_hasher4hashINS_9InnerTypeEEEvRKS2_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3Abc14default_hasher4hashI9OuterTypeEEvRKS2_" ) != nullptr );
}

U_TEST( CoroutinesMangling_Test0 )
{
	// Coroutine type is encoded like template with two params - extended return type and inner reference kind, encoded as variable param of type u32.
	// 0 - means no references inside, 1 - immutable references inside, 2 - mutable references inside.

	static const char c_program_text[]=
	R"(
		type Gen= generator : i32;
		fn Foo( Gen gen ) {}
		fn Bar( f32 x, Gen gen, u32 z ) {}

		type ImutRefGen= generator'imut' : f64;
		fn Baz( ImutRefGen gen ) {}

		type MutRefRetAsnycFunc= async'mut, imut' : char8 &mut;
		fn Lol( MutRefRetAsnycFunc f ) {}

		type NonSyncGen = generator non_sync : [i32, 4];
		fn Kek( NonSyncGen gen ) {}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	// Functions with coroutine param.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Foo9generatorIiE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Barf9generatorIiEj" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Baz9generatorIdLj0EE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Lol5asyncIRcLj1ELj0EE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3Kek9generatorIA4_iLb1EE" ) != nullptr );

	// Generated coroutine type destructors.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN9generatorIiE10destructorERS0_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN9generatorIdLj0EE10destructorERS0_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN5asyncIRcLj1ELj0EE10destructorERS1_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN9generatorIA4_iLb1EE10destructorERS1_" ) != nullptr );
}

U_TEST( VirtualTableMangling_Test0 )
{
	static const char c_program_text[]=
	R"(
		class SukaBlat polymorph{}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZTV8SukaBlat", true ) != nullptr );
}

U_TEST( VirtualTableMangling_Test1 )
{
	static const char c_program_text[]=
	R"(
		namespace Lol
		{
			class WTF polymorph{}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZTVN3Lol3WTFE", true ) != nullptr );
}

U_TEST( LambdasMangling_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			auto f= lambda( i32 x, f32& y ) {};
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_37389c86aec1c171f5a5ea1c99fa3ab2_4_11_clERKS_iRKf" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_37389c86aec1c171f5a5ea1c99fa3ab2_4_11_10destructorERS_" ) != nullptr ); // Destructor.
}

U_TEST( LambdasMangling_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn Bar()
		{
			var i32 x= 0;
			// These lambdas are practically identical, but still are different classes.
			auto f0= lambda[=]() : i32 { return x; };
			auto f1= lambda[=]() : i32 { return x; };
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_10dc947b6ebd99491b21a0054987f108_6_12_clERKS_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_10dc947b6ebd99491b21a0054987f108_6_12_10destructorERS_" ) != nullptr ); // Destructor.

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_10dc947b6ebd99491b21a0054987f108_7_12_clERKS_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_10dc947b6ebd99491b21a0054987f108_7_12_10destructorERS_" ) != nullptr ); // Destructor.
}

U_TEST( LambdasMangling_Test2 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			// Lambda with by-reference capturing - nothing special.
			auto f0= lambda[&]( i32 y ) : i32 { return y * x; };
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_326fb0be30230539be75b457a531cd60_6_12_clERKS_i" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_326fb0be30230539be75b457a531cd60_6_12_10destructorERS_" ) != nullptr ); // Destructor.
}

U_TEST( LambdasMangling_Test3 )
{
	static const char c_program_text[]=
	R"(
		namespace Lol
		{
			namespace What
			{
				fn Foo()
				{
					// Lambda class name should contain enclosed namespace prefix.
					auto f= lambda(){};
				}
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3Lol4What46_lambda_dde73c3ed253793bc832a3587be39fed_9_13_clERKS1_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3Lol4What46_lambda_dde73c3ed253793bc832a3587be39fed_9_13_10destructorERS1_" ) != nullptr ); // Destructor.
}

U_TEST( LambdasMangling_Test4 )
{
	static const char c_program_text[]=
	R"(
		class Some
		{
			fn Foo( this )
			{
				// Lambda class name should contain enclosed namespace prefix (here class name).
				auto f= lambda(){};
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN4Some46_lambda_885cf66f6cb9419ca09cc783385a4642_7_12_clERKS0_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN4Some46_lambda_885cf66f6cb9419ca09cc783385a4642_7_12_10destructorERS0_" ) != nullptr ); // Destructor.
}

U_TEST( LambdasMangling_Test5 )
{
	static const char c_program_text[]=
	R"(
		namespace Prefix
		{
			template</ type T, u64 S />
			struct Box
			{
				[ T, S ] contents;
				fn Foo( this )
				{
					// Lambda class name should contain enclosed namespace prefix (here class name including template stuff).
					auto f= lambda(){};
				}
			}
		}
		type IntVec4Box= Prefix::Box</ i32, 4u64 />;
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN6Prefix3BoxIiLy4EE47_lambda_58cffb4e1eb5791bdb38e16b1abbc43d_11_13_clERKS2_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN6Prefix3BoxIiLy4EE47_lambda_58cffb4e1eb5791bdb38e16b1abbc43d_11_13_10destructorERS2_" ) != nullptr ); // Destructor.
}

U_TEST( LambdasMangling_Test6 )
{
	static const char c_program_text[]=
	R"(
		class S
		{
			fn Foo( this, bool cond0, bool cond1 )
			{
				{
					while( cond0 )
					{
						if( cond1 )
						{
							unsafe
							{
								// Scopes of function blocks should be ignored while composing lambda name.
								auto f= lambda(){};
							}
						}
					}
				}
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN1S47_lambda_c9f02c2d45b85f9c1349656a0f11d19d_14_16_clERKS0_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN1S47_lambda_c9f02c2d45b85f9c1349656a0f11d19d_14_16_10destructorERS0_" ) != nullptr ); // Destructor.
}

U_TEST( LambdasMangling_Test7 )
{
	static const char c_program_text[]=
	R"(
		struct spqr
		{
			// Should include struct name as lambda prefix.
			i32 x= lambda() : i32 { return 42; } ();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN4spqr46_lambda_c81936537015db40f92af46cf866ea7c_5_10_clERKS0_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN4spqr46_lambda_c81936537015db40f92af46cf866ea7c_5_10_10destructorERS0_" ) != nullptr ); // Destructor.
}

U_TEST( LambdasMangling_Test8 )
{
	static const char c_program_text[]=
	R"(
		?macro <? DEFINE_LAMBDA:block ?> -> <? auto f = lambda(){}; ?>
		fn Foo()
		{
			// Lambda name should encode macro expansion context.
			DEFINE_LAMBDA
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN83_lambda_ab05617aabb05c0fc242caebbebb2910_2_50_ab05617aabb05c0fc242caebbebb2910_6_3_clERKS_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN83_lambda_ab05617aabb05c0fc242caebbebb2910_2_50_ab05617aabb05c0fc242caebbebb2910_6_3_10destructorERS_" ) != nullptr ); // Destructor.
}

U_TEST( LambdasMangling_Test9 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var tup[ f32, i32, f32 ] t= zero_init;
			for( el : t )
			{
				// Should produce here 3 distinct lambdas - for each tuple-for iteration.
				auto f= lambda[=]() : f64 { return f64(el); };
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN51_lambda_5e2746f59dd72ad91092fde7c587e0bf_8_12_tf_0_clERKS_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN51_lambda_5e2746f59dd72ad91092fde7c587e0bf_8_12_tf_0_10destructorERS_" ) != nullptr ); // Destructor.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN51_lambda_5e2746f59dd72ad91092fde7c587e0bf_8_12_tf_1_clERKS_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN51_lambda_5e2746f59dd72ad91092fde7c587e0bf_8_12_tf_1_10destructorERS_" ) != nullptr ); // Destructor.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN51_lambda_5e2746f59dd72ad91092fde7c587e0bf_8_12_tf_2_clERKS_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN51_lambda_5e2746f59dd72ad91092fde7c587e0bf_8_12_tf_2_10destructorERS_" ) != nullptr ); // Destructor.
}

U_TEST( LambdasMangling_Test10 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var tup[ f32, i32 ] t0= zero_init;
			var tup[ i64, f64, u32 ] t1= zero_init;
			for( el0 : t0 )
			{
				// Should produce here lambdas for each tuple-for combination.
				for( el1 : t1 )
				{
					auto f= lambda[=]() : f64 { return f64(el0) * f64(el1); };
				}
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN54_lambda_aad100a36c92a7d970360fe32a49ebda_11_13_tf_0_0_clERKS_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN54_lambda_aad100a36c92a7d970360fe32a49ebda_11_13_tf_0_1_clERKS_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN54_lambda_aad100a36c92a7d970360fe32a49ebda_11_13_tf_0_2_clERKS_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN54_lambda_aad100a36c92a7d970360fe32a49ebda_11_13_tf_1_0_clERKS_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN54_lambda_aad100a36c92a7d970360fe32a49ebda_11_13_tf_1_1_clERKS_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN54_lambda_aad100a36c92a7d970360fe32a49ebda_11_13_tf_1_2_clERKS_" ) != nullptr );
}

U_TEST( LambdasMangling_Test11 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			// Should properly encode lambda in static variable initialization.
			auto f= lambda() {};
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN1S46_lambda_6b87f26714d940306e3c14c4c7140d1f_5_11_clERKS0_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN1S46_lambda_6b87f26714d940306e3c14c4c7140d1f_5_11_10destructorERS0_" ) != nullptr ); // Destructor.
}

U_TEST( LambdasMangling_Test12 )
{
	static const char c_program_text[]=
	R"(
		template</ u64 S />
		class IVec polymorph
		{
			[ i32, S ] v;
		}
		// Use lambda for calculation of temlate variable argument in class parents list.
		class IVec4 : IVec</ lambda() : u64 { return 16u64; } () />
		{
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_dfc55a700ed21c85b6a53d00e3076c1f_8_23_clERKS_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_dfc55a700ed21c85b6a53d00e3076c1f_8_23_10destructorERS_" ) != nullptr ); // Destructor.
}

U_TEST( LambdasMangling_Test13 )
{
	static const char c_program_text[]=
	R"(
		template</type T/>
		fn Bar()
		{
			auto f= lambda(){};
		}
		fn Foo()
		{
			Bar</ f64 />();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_ff38ed8c4972a1c123a9ae13633e72f4_5_11_IdEclERKS0_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_ff38ed8c4972a1c123a9ae13633e72f4_5_11_IdE10destructorERS0_" ) != nullptr ); // Destructor.
}

U_TEST( LambdasMangling_Test14 )
{
	static const char c_program_text[]=
	R"(
		// Use lambda in non-sync expression of template struct.
		// Should encode template args here.
		template</ type T, u64 S />
		struct Box non_sync( lambda() : bool { return false; } () )
		{
			[ T, S ] arr;
		}
		type FloatBox= Box</f32, 33u64 />;
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_a27f810123558fda7268fd5e43a5137d_5_23_ILy33EfEclERKS0_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_a27f810123558fda7268fd5e43a5137d_5_23_ILy33EfE10destructorERS0_" ) != nullptr ); // Destructor.
}

U_TEST( LambdasMangling_Test15 )
{
	// Should encode template args of type alias.
	static const char c_program_text[]=
	R"(
		template</type T/>
		type Vec4= [ T, lambda() : u64 { return 4u64; } () ];
		type U64Vec4= Vec4</u64/>;
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_5cd35d2582e59b3a7ad332605201dcf1_3_18_IyEclERKS0_" ) != nullptr ); // Call operator itslef.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_5cd35d2582e59b3a7ad332605201dcf1_3_18_IyE10destructorERS0_" ) != nullptr ); // Destructor.
}

U_TEST( LambdasMangling_Test16 )
{
	// Should encode mutable this parameter of lambdas operator().
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			auto f= lambda mut(){};
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN46_lambda_9fb393ef56b4b4cf3ac8c99be9349c03_4_11_clERS_" ) != nullptr ); // Call operator itslef.
}

} // namespace

} // namespace U
