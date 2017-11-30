#include "tests.hpp"

namespace U
{

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

} // namespace U
