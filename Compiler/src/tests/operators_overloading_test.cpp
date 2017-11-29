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

U_TEST( OperatorsOverloadingTest3 )
{
	// Basic overloaded unary operator.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			op++( mut this )
			{
				++x;
			}
			op--( mut this )
			{
				--x;
			}
		}

		fn Foo() : i32
		{
			var S a{ .x= 5845 };
			--a;
			--a;
			--a;
			++a;
			--a;
			++a;
			++a;
			++a;
			++a;
			++a;
			++a;
			++a;
			++a;
			++a;
			--a;
			return a.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 5845 - 5 + 10 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( OperatorsOverloadingTest4 )
{
	// Basic overloaded assignment operator.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			op=( S &mut dst, S &imut src )
			{
				dst.x= src.x;
			}
		}

		fn Foo() : i32
		{
			var S a{ .x= 55414 }, b{ .x= 332 };
			a= b;
			return a.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 332 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( OperatorsOverloadingTest5 )
{
	// Basic overloaded additive assignment operator.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			op*=( S &mut dst, S &imut src )
			{
				dst.x*= src.x;
			}
		}

		fn Foo() : i32
		{
			var S a{ .x= 55414 }, b{ .x= 332 };
			a*= b;
			return a.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 55414 * 332 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( AssignmentOperatorArgumentsShouldBeEvaluatedInReverseOrder )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			op=( S &mut dst, S &imut src )
			{
				dst.x= src.x;
			}
		}

		fn Mul5( i32 &mut x ) : u32
		{
			x*= 5;
			return 0u;
		}

		fn Div7( i32 &mut x ) : u32
		{
			x/= 7;
			return 0u;
		}

		fn Foo() : i32
		{
			var [ S, 1 ] arr0= zero_init;
			var [ S, 1 ] arr1= zero_init;
			var i32 fff= 4;

			arr0[ Div7(fff) ]= arr1[ Mul5(fff) ];  // Must first evaluate Mul5, then - Mul7

			return fff;
		}
	)";

	static_assert( 4 * 5 / 7 != 4 / 7 * 5, "test is wrong" );

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 4 * 5 / 7 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( AdditiveAssignmentOperatorArgumentsShouldBeEvaluatedInReverseOrder )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			op+=( S &mut dst, S &imut src )
			{
				dst.x+= src.x;
			}
		}

		fn Mul5( i32 &mut x ) : u32
		{
			x*= 5;
			return 0u;
		}

		fn Div7( i32 &mut x ) : u32
		{
			x/= 7;
			return 0u;
		}

		fn Foo() : i32
		{
			var [ S, 1 ] arr0= zero_init;
			var [ S, 1 ] arr1= zero_init;
			var i32 fff= 4;

			arr0[ Div7(fff) ]+= arr1[ Mul5(fff) ];  // Must first evaluate Mul5, then - Mul7

			return fff;
		}
	)";

	static_assert( 4 * 5 / 7 != 4 / 7 * 5, "test is wrong" );

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 4 * 5 / 7 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( BinaryOperatorArgumentsShouldBeEvaluatedInDirectOrder )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			op+( S &imut a, S &imut b )
			{
				// do nothing
			}
		}

		fn Mul5( i32 &mut x ) : u32
		{
			x*= 5;
			return 0u;
		}

		fn Div7( i32 &mut x ) : u32
		{
			x/= 7;
			return 0u;
		}

		fn Foo() : i32
		{
			var [ S, 1 ] arr= zero_init;
			var i32 fff= 4;

			arr[ Mul5(fff) ] + arr[ Div7(fff) ];  // Must first evaluate Mul5, then - Mul7

			return fff;
		}
	)";

	static_assert( 4 * 5 / 7 != 4 / 7 * 5, "test is wrong" );

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 4 * 5 / 7 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( OperatorsOverloadingTest_InTemplates )
{
	// Overloaded operator must be selected inside template.
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct Math
		{
			fn Sub( T &imut a, T &imut b ) : T
			{
				return a - b;
			}
		}

		struct MyInt
		{
			i32 x;
			op-( MyInt &imut a, MyInt &imut b ) : MyInt
			{
				var MyInt res{ .x= a.x - b.x };
				return res;
			}
		}

		fn Foo() : i32
		{
			var MyInt a{ .x= 584 }, b{ .x= 11 };
			return Math</ MyInt />::Sub( a, b ).x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 584 - 11 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( OperatorsOverloadingTest_EqualityOperators )
{
	static const char c_program_text[]=
	R"(
		struct MyInt
		{
			i32 x;
			op==( MyInt &imut a, MyInt &imut b ) : bool
			{
				return a.x == b.x;
			}
			op!=( MyInt &imut a, MyInt &imut b ) : bool
			{
				return a.x != b.x;
			}
			op> ( MyInt &imut a, MyInt &imut b ) : bool
			{
				return a.x >  b.x;
			}
			op>=( MyInt &imut a, MyInt &imut b ) : bool
			{
				return a.x >= b.x;
			}
			op< ( MyInt &imut a, MyInt &imut b ) : bool
			{
				return a.x <  b.x;
			}
			op<=( MyInt &imut a, MyInt &imut b ) : bool
			{
				return a.x <= b.x;
			}
		}

		fn Foo()
		{
			var MyInt imut a{ .x= 584 }, imut b{ .x= 11 };
			halt if( a == b );
			halt if( a != a );
			halt if( a < b );
			halt if( a <= b );
			halt if( !( a <= a ) );
			halt if( b > a );
			halt if( b >= a );
			halt if( !( b >= b ) );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

} // namespace U
