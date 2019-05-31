#include "tests.hpp"

namespace U
{

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

	// Here is difference between Ü and C++. In Ü "this" processed as regular argument.
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN10SomeStruct3FooERKS_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN10SomeStruct3BarERS_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN10SomeStruct3BazEv" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN10SomeStructC1ERS_" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN10SomeStructC1ERS_if" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN10SomeStructD0ERS_" ) != nullptr );
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
			}
		}

		struct Ball
		{
			auto constexpr rrtt= -8547;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "I", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "FF", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "DoubleVariable", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZN8SSS_Pace3tttE", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZN8SSS_Pace2piE", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZN8SSS_Pace9pi_doubleE", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZN8SSS_Pace3Bar7abcdefgE", true ) != nullptr );
	U_TEST_ASSERT( engine->FindGlobalVariableNamed( "_ZN4Ball4rrttE", true ) != nullptr );
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
			op!=( Box &imut a, Box &imut b ) : bool
			{
				return a.x != b.x;
			}
			op>( Box &imut a, Box &imut b ) : bool
			{
				return a.x > b.x;
			}
			op>=( Box &imut a, Box &imut b ) : bool
			{
				return a.x >= b.x;
			}
			op<( Box &imut a, Box &imut b ) : bool
			{
				return a.x < b.x;
			}
			op<=( Box &imut a, Box &imut b ) : bool
			{
				return a.x <= b.x;
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
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxneERKS_S1_" ) != nullptr ); // !=
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxgtERKS_S1_" ) != nullptr ); // >
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxgeERKS_S1_" ) != nullptr ); // >=
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxltERKS_S1_" ) != nullptr ); // <
	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxleERKS_S1_" ) != nullptr ); // <=

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

	U_TEST_ASSERT( engine->FindFunctionNamed( "_ZN3BoxixERKS_j" ) != nullptr ); // []
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
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3FooPFvvE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3BarPFivE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z3BazPFviE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z6BinaryPFvffE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z9BinaryRetPFbffE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4RetSPF1SvE" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "_Z4PassPF11OtherStructRKS_E" ) != nullptr );
}

} // namespace U
