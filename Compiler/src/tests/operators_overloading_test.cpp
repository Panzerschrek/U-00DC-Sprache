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

} // namespace U
