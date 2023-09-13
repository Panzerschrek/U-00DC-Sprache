#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( OperatorsOverloadingTest0_NonThisCallOperators )
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

			// TODO - is this legal to declare non-this-call additive-assignment operators?

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

			op++( Box &mut a )
			{
				++a.x;
			}
			op--( Box &mut a )
			{
				--a.x;
			}

			// TODO - is this legal to declare non-this-call assignment operator?
			op=( Box &mut a, Box &imut b )
			{
				a.x= b.x;
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( OperatorsOverloadingTest_ThisCallOperators )
{
	static const char c_program_text[]=
	R"(
		struct Box
		{
			u32 x;

			op+( this ) : Box
			{
				return this;
			}
			op-( this ) : Box
			{
				var Box r{ .x= -x };
				return r;
			}

			op+( this, Box &imut b ) : Box
			{
				var Box r{ .x= x + b.x };
				return r;
			}
			op-( this, Box &imut b ) : Box
			{
				var Box r{ .x= x - b.x };
				return r;
			}
			op*( this, Box &imut b ) : Box
			{
				var Box r{ .x= x * b.x };
				return r;
			}
			op/( this, Box &imut b ) : Box
			{
				var Box r{ .x= x / b.x };
				return r;
			}

			op&( this, Box &imut b ) : Box
			{
				var Box r{ .x= x & b.x };
				return r;
			}
			op|( this, Box &imut b ) : Box
			{
				var Box r{ .x= x | b.x };
				return r;
			}
			op^( this, Box &imut b ) : Box
			{
				var Box r{ .x= x ^ b.x };
				return r;
			}

			op<<( this, Box &imut b ) : Box
			{
				var Box r{ .x= x << b.x };
				return r;
			}
			op>>( this, Box &imut b ) : Box
			{
				var Box r{ .x= x >> b.x };
				return r;
			}

			op+=( mut this, Box &imut b )
			{
				x+= b.x;
			}
			op-=( mut this, Box &imut b )
			{
				x-= b.x;
			}
			op*=( mut this, Box &imut b )
			{
				x*= b.x;
			}
			op/=( mut this, Box &imut b )
			{
				x/= b.x;
			}
			op%=( mut this, Box &imut b )
			{
				x%= b.x;
			}

			op&=( mut this, Box &imut b )
			{
				x&= b.x;
			}
			op|=( mut this, Box &imut b )
			{
				x|= b.x;
			}
			op^=( mut this, Box &imut b )
			{
				x^= b.x;
			}

			op<<=( mut this, Box &imut b )
			{
				x <<= b.x;
			}
			op>>=( mut this, Box &imut b )
			{
				x >>= b.x;
			}

			op++( mut this )
			{
				++x;
			}
			op--( mut this )
			{
				--x;
			}

			op=( mut this, Box &imut b )
			{
				x= b.x;
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( OperatorsOverloadingTest_ValueArgumentsOperators )
{
	static const char c_program_text[]=
	R"(
		struct Box
		{
			u32 x;

			op+( Box b ) : Box
			{
				return b;
			}
			op-( Box b ) : Box
			{
				var Box r{ .x= -b.x };
				return r;
			}

			op+( Box a, Box b ) : Box
			{
				var Box r{ .x= a.x + b.x };
				return r;
			}
			op-( Box a, Box b ) : Box
			{
				var Box r{ .x= a.x - b.x };
				return r;
			}
			op*( Box a, Box b ) : Box
			{
				var Box r{ .x= a.x * b.x };
				return r;
			}
			op/( Box a, Box b ) : Box
			{
				var Box r{ .x= a.x / b.x };
				return r;
			}

			op&( Box a, Box b ) : Box
			{
				var Box r{ .x= a.x & b.x };
				return r;
			}
			op|( Box a, Box b ) : Box
			{
				var Box r{ .x= a.x | b.x };
				return r;
			}
			op^( Box a, Box b ) : Box
			{
				var Box r{ .x= a.x ^ b.x };
				return r;
			}

			op<<( Box a, Box b ) : Box
			{
				var Box r{ .x= a.x << b.x };
				return r;
			}
			op>>( Box a, Box b ) : Box
			{
				var Box r{ .x= a.x >> b.x };
				return r;
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
			var S mut a{ .x= 5845 };
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
			var S mut a{ .x= 55414 }, b{ .x= 332 };
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
			var S mut a{ .x= 55414 }, b{ .x= 332 };
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

U_TEST( OperatorsOverloadingTest6 )
{
	// Overloaded operator takes value arguments and returns value result.
	static const char c_program_text[]=
	R"(
		struct MyInt
		{
			i32 x;
			op-( MyInt a, MyInt b ) : MyInt
			{
				var MyInt r{ .x= a.x - b.x };
				return r;
			}
		}

		fn Foo() : i32
		{
			var MyInt a{ .x= 55114 }, b{ .x= 537 };
			return ( a - b ).x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 55114 - 537 ) == result_value.IntVal.getLimitedValue() );
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
			var [ S, 1 ] mut arr0= zero_init;
			var [ S, 1 ] arr1= zero_init;
			var i32 mut fff= 4;

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
			var [ S, 1 ] mut arr0= zero_init;
			var [ S, 1 ] arr1= zero_init;
			var i32 mut fff= 4;

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
			var i32 mut fff= 4;

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
			op<=>( MyInt &imut a, MyInt &imut b ) : i32
			{
				return a.x <=> b.x;
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
			halt if( a <=> b != +1 );
			halt if( b <=> a != -1 );
			halt if( 0 != a <=> a );
			halt if( 0 != b <=> b );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

U_TEST( OperatorsOverloadingTest_EqualityOperatorsConstexpr )
{
	static const char c_program_text[]=
	R"(
		struct MyInt
		{
			i32 x;
			op==( MyInt& a, MyInt& b ) : bool = default;
			op constexpr <=>( MyInt& a, MyInt& b ) : i32
			{
				return a.x <=> b.x;
			}
		}

		var MyInt a{ .x= 584 }, b{ .x= 11 };
		static_assert( a == a );
		static_assert( b == b );
		static_assert( a != b );
		static_assert( b != a );
		static_assert( a > b );
		static_assert( a >= b );
		static_assert( ! ( a < b ) );
		static_assert( ! ( a <= b ) );
		static_assert( b < a );
		static_assert( b <= a );
		static_assert( ! ( b > a ) );
		static_assert( ! ( b >= a ) );
		static_assert( a <=> b == +1 );
		static_assert( b <=> a == -1 );
		static_assert( +1 == a <=> b );
		static_assert( -1 == b <=> a );
		static_assert( a <=> a == 0 );
		static_assert( 0 == b <=> b );
	)";

	BuildProgram( c_program_text );
}

U_TEST( OperatorsOverloadingTest_IndexationOperator0 )
{
	static const char c_program_text[]=
	R"(
		struct MyIntVec
		{
			[ i32, 4 ] x;

			op[]( mut this, i32 index ) : i32 &mut
			{
				halt if( index < 0 | index >= 4 );
				return x[u32(index)];
			}
			op[]( imut this, i32 index ) : i32 &imut
			{
				halt if( index < 0 | index >= 4 );
				return x[u32(index)];
			}
		}

		fn Foo() : i32
		{
			var MyIntVec mut a= zero_init, imut b{ .x[ 58, 451, 654, 11254 ] };
			a[1]= b[2];
			return a.x[1u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 654 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( OperatorsOverloadingTest_IndexationOperator1 )
{
	static const char c_program_text[]=
	R"(
		struct MyIntVec
		{
			[ i32, 4 ] x;

			op[]( MyIntVec v, i32 index ) : i32 // Non-this-call operator[]
			{
				return v.x[u32(index)];
			}
		}

		fn Foo() : i32
		{
			var MyIntVec a{ .x[ 3, 75, 41, 98 ] };
			return a.x[2u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, {} );
	U_TEST_ASSERT( result_value.IntVal.getLimitedValue() == uint64_t(41) );
}

U_TEST( OperatorsOverloadingTest_CallOperator0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			op()( mut this ) : i32
			{
				++x;
				return x;
			}
		}
		fn Foo() : i32
		{
			var S mut s{ .x= 425 };
			s();
			s();
			s();
			return s();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 425 + 4 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( OperatorsOverloadingTest_CallOperator1 )
{
	// Multiple () operators in same class.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			op()( mut this ) : i32
			{
				++x;
				return x;
			}
			op()( mut this, i32 y ) : i32
			{
				x+= y;
				return x;
			}
		}
		fn Foo() : i32
		{
			var S mut s{ .x= 0 };
			s();
			s( 77454 );
			return s.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 1 + 77454 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( OperatorsOverloadingTest_CallOperator2 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			// Non-this-call operator
			op()( S mut s ) : i32
			{
				++s.x;
				return s.x * s.x;
			}
		}
		fn Foo() : i32
		{
			var S mut s{ .x= 37 };
			return s();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, {} );
	U_TEST_ASSERT( result_value.IntVal.getLimitedValue() == uint64_t(38 * 38) );
}

U_TEST( GeneratedCopyAssignmentOperatorTest0 )
{
	static const char c_program_text[]=
	R"(
		struct Box{}
		struct BoxBox
		{
			Box b;
			bool bbb;
		}
		struct S
		{
			[ i32, 4 ] arr;
			f32 x;
			Box b;
			BoxBox bb;

			op==( S &imut a, S& imut b ) : bool
			{
				return
					a.arr[0u] == b.arr[0u] && a.arr[1u] == b.arr[1u] && a.arr[2u] == b.arr[2u] && a.arr[3u] == b.arr[3u] &&
					a.x == b.x &&
					a.bb.bbb == b.bb.bbb;
			}
		}

		fn Foo()
		{
			var S
				src{ .arr[ 584, 654125, -57, 8547 ], .x= 3.14f, .bb{ .bbb= true } },
				mut dst=zero_init;
			dst= src;
			halt if( !( dst == src ) );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
}

U_TEST( GeneratedCopyAssignmentOperatorTest1 )
{
	// Generated operator calls to implicit operator
	static const char c_program_text[]=
	R"(
		struct A
		{
			i32 x;
			op=( A &mut dst, A &imut src )
			{
				dst.x= 8888745;
			}
		}
		struct B
		{
			A a;
			f32 ch;
		}
		fn Foo() : i32
		{
			var B
				imut src{ .a= zero_init, .ch= 35.54f },
				mut dst=zero_init;
			dst= src;
			halt if( dst.ch != src.ch );
			return dst.a.x; // Should return "wrong" value.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 8888745 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( GeneratedCopyAssignmentOperatorTest2 )
{
	static const char c_program_text[]=
	R"(
		struct A  // Copy-assignment operator not generated, because class contains immutable fields.
		{
			i32 imut x;
			fn constructor() ( x= 0 ){}
		}
		fn Foo()
		{
			var A mut x, mut y;
			x= y; // Error, no = operator
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( error.src_loc.GetLine() == 10u );
}

U_TEST( GeneratedCopyAssignmentOperatorTest3 )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32& x; } // Struct with reference, have no operator=.
		struct A{ [ S, 0 ] s; } // Have generated operator= initilizer, because all fields have operator=.
		fn Foo()
		{
			var A mut a, mut b;
			a= b; // Ok, have generated operator=.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( OperatorBodyOutsideClass )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			op/( S &imut a, S &imut b ) : S;
			op+( S &imut a, S &imut b ) : S;
		}

		op S::/( S &imut a, S &imut b ) : S // relative name
		{
			var S r{ .x= a.x / b.x };
			return r;
		}
		op ::S::+( S &imut a, S &imut b ) : S // global name
		{
			var S r{ .x= a.x + b.x };
			return r;
		}
		fn Foo() : i32
		{
			var S imut a{ .x= 584147 }, imut b{ .x= 55 };
			return ( a / b ).x + ( a + b ).x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 584147 / 55 + ( 584147 + 55 ) ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( UseInheritedOverloadedOperator_Test0 )
{
	static const char c_program_text[]=
	R"(
		class A polymorph
		{
			op()(this) : i32 { return 657; }
		}

		class B : A{}

		fn Call(B& b) : i32
		{
			return b();
		}

		fn Foo() : i32
		{
			var B b;
			return Call(b);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 657 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( UseInheritedOverloadedOperator_Test1 )
{
	static const char c_program_text[]=
	R"(
		class A polymorph
		{
			op+(A& x, B& y) : i32 { return 99991; }
		}

		class B : A{}

		fn Add(B& x, B& y) : i32
		{
			return x + y;
		}

		fn Foo() : i32
		{
			var B x, y;
			return Add(x, y);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 99991 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( UseInheritedOverloadedOperator_Test2 )
{
	static const char c_program_text[]=
	R"(
		class A polymorph
		{
			op<=>(A& x, A& y) : i32 { return 0; }
		}

		class B : A{}

		fn Compare(B& x, B& y)
		{
			x <=> y; // <=> not inherited
		}

	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( error.src_loc.GetLine() == 11u );
}

U_TEST( UseInheritedOverloadedOperator_Test3 )
{
	static const char c_program_text[]=
	R"(
		class A polymorph
		{
			op==(A& x, A& y) : bool = default;
		}

		class B : A{}

		fn Compare(B& x, B& y)
		{
			x == y; // == not inherited
		}

	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( error.src_loc.GetLine() == 11u );
}

} // namespace

} // namespace U
