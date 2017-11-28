#include "tests.hpp"

namespace U
{

U_TEST( OperatorsOverloadingTest0 )
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

		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( OperatorsOverloadingTest1 )
{
	// Basic overloaded binary operator.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			op/( S &imut a, S &imut b ) : S
			{
				var S r{ .x= a.x / b.x };
				return r;
			}
		}

		fn Foo() : i32
		{
			var S a{ .x= 58 }, b{ .x= 3 };
			return ( a / b ).x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 58 / 3 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( OperatorsOverloadingTest2 )
{
	// Basic overloaded unary operator.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			op-( S &imut a ) : S
			{
				var S r{ .x= -a.x };
				return r;
			}
		}

		fn Foo() : i32
		{
			var S a{ .x= -4645651 };
			return (-a).x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 4645651 ) == result_value.IntVal.getLimitedValue() );
}

} // namespace U
