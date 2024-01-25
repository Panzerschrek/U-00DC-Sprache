from py_tests_common import *

def ConstexprCall_ResultTypeIs_u16():
	c_program_text= """
		fn constexpr Pass( u16 x ) : u16
		{
			return x;
		}
		fn Foo()
		{
			static_assert( Pass(65521u16) == 65521u16 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprCall_ResultTypeIs_i32():
	c_program_text= """
		fn constexpr Pass( i32 x ) : i32
		{
			return x;
		}
		fn Foo()
		{
			static_assert( Pass(42) == 42 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprCall_ResultTypeIs_i64():
	c_program_text= """
		fn constexpr Pass( i64 x ) : i64
		{
			return x;
		}
		fn Foo()
		{
			static_assert( Pass(80816996193i64) == 80816996193i64 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprCall_ResultTypeIs_f32():
	c_program_text= """
		fn constexpr Pass( f32 x ) : f32
		{
			return x;
		}
		fn Foo()
		{
			static_assert( Pass(3.1415926535f) == 3.1415926535f );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprCall_ResultTypeIs_f64():
	c_program_text= """
		fn constexpr Pass( f64 x ) : f64
		{
			return x;
		}
		fn Foo()
		{
			static_assert( Pass(3.1415926535) == 3.1415926535 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_Add():
	c_program_text= """
		fn constexpr Bar( i32 x, i32 y ) : i32
		{
			return x + y;
		}
		fn Foo()
		{
			static_assert( Bar(85, 14) == 85 + 14 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_Sub():
	c_program_text= """
		fn constexpr Bar( i64 x, i64 y ) : i64
		{
			return x - y;
		}
		fn Foo()
		{
			static_assert( Bar(14i64, 85i64) == 14i64 - 85i64 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_Mul():
	c_program_text= """
		fn constexpr Bar( i32 x, i32 y ) : i32
		{
			return x * y;
		}
		fn Foo()
		{
			static_assert( Bar(14, 85) == 14 * 85 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_Div():
	c_program_text= """
		fn constexpr Bar( i32 x, i32 y ) : i32
		{
			return x / y;
		}
		fn Foo()
		{
			static_assert( Bar(85, 14) == 85 / 14 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_FAdd():
	c_program_text= """
		fn constexpr Bar( f32 x, f32 y ) : f32
		{
			return x + y;
		}
		fn Foo()
		{
			static_assert( Bar(85.5f, 14.1f) == 85.5f + 14.1f );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_FSub():
	c_program_text= """
		fn constexpr Bar( f32 x, f32 y ) : f32
		{
			return x - y;
		}
		fn Foo()
		{
			static_assert( Bar(85.5f, 14.1f) == 85.5f - 14.1f );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_FMul():
	c_program_text= """
		fn constexpr Bar( f32 x, f32 y ) : f32
		{
			return x * y;
		}
		fn Foo()
		{
			static_assert( Bar(85.5f, 14.1f) == 85.5f * 14.1f );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_FDiv():
	c_program_text= """
		fn constexpr Bar( f64 x, f64 y ) : f64
		{
			return x / y;
		}
		fn Foo()
		{
			static_assert( Bar(85.5, 14.1) == 85.5 / 14.1 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionArithmeticOperatorsTest_BitOperators():
	c_program_text= """
		fn constexpr Bar( i32 x, i32 y, i32 z, i32 w ) : i32
		{
			return ( x & y ) | ( z ^ w );
		}
		fn Foo()
		{
			static_assert( Bar( 255, 254, 542, 86254 ) == ( (255&254) | (542^86254) ) );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionControlFlow_Test0():
	c_program_text= """
		fn constexpr Select( bool first, i32 x, i32 y ) : i32
		{
			if( first ) { return x; }
			return y;
		}
		fn Foo()
		{
			static_assert( Select( true, 85, 865 ) == 85 );
			static_assert( Select( false, 888, 999 ) == 999 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionControlFlow_Test1():
	c_program_text= """
		fn constexpr Sum( i32 mut from, i32 to ) : i32
		{
			var i32 mut res= 0;
			while( from < to )
			{
				res += from;
				++from;
			}
			return res;
		}
		fn Foo()
		{
			static_assert( Sum( 7, 10 ) == 7 + 8 + 9 );
			static_assert( Sum( 5, 13 ) == 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionControlFlow_Test2():
	c_program_text= """
	fn constexpr And( bool first, bool second ) : bool
	{
		return first && second;
	}
	fn constexpr Or ( bool first, bool second ) : bool
	{
		return first || second;
	}
	fn Foo()
	{
		static_assert( And( false, false ) == false );
		static_assert( And( false,  true ) == false );
		static_assert( And(  true, false ) == false );
		static_assert( And(  true,  true ) ==  true );
		static_assert( Or ( false, false ) == false );
		static_assert( Or ( false,  true ) ==  true );
		static_assert( Or (  true, false ) ==  true );
		static_assert( Or (  true,  true ) ==  true );
	}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionAccessGlobalVariable_Test0():
	c_program_text= """
		auto constexpr g_x= 666;

		fn constexpr Mul( i32& x, i32& y ) : i32
		{
			return x * y;
		}

		fn constexpr GetX( i32 mul ) : i32
		{
			return Mul( g_x, mul );
		}

		static_assert( GetX( 999 ) == 999 * 666 );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionAccessGlobalVariable_Test1():
	c_program_text= """
		struct S{ i32 x; }
		var S constexpr g_s{ .x= 42 };

		fn constexpr GetXImpl( S& s, i32 mul ) : i32
		{
			return s.x * mul;
		}

		fn constexpr GetX( i32 mul ) : i32
		{
			return GetXImpl( g_s, mul );
		}

		static_assert( GetX( 5 ) == 42 * 5 );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprStructGeneratedMethodsAreConstexpr_Test0():
	c_program_text= """
		struct S{}
		fn constexpr Foo()
		{
			// Call default constructor here, it must be constexpr.
			var S s0;
			var S s1();
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprStructGeneratedMethodsAreConstexpr_Test1():
	c_program_text= """
		struct S{ i32 x; }
		fn constexpr Foo()
		{
			var S s0{ .x= 555 };
			var S s1(s0); // Call copy constructor here, it must be constexpr.
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprStructGeneratedMethodsAreConstexpr_Test2():
	c_program_text= """
		struct S{ i32 x; }
		fn constexpr Foo()
		{
			var S mut s0{ .x= 555 }, mut s1{ .x=666 };
			s0= s1; // Call copy assignment operator  here, it must be constexpr.
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionWithMutableArguments_Test0():
	c_program_text= """
		fn constexpr SetZero( i32&mut x ) { x= 0; }

		fn constexpr Sum( i32 x, i32 y ) : i32
		{
			var i32 mut r= zero_init;
			SetZero(r); // Call here constexpr function with mutable-reference argument.
			r= x + y;
			return r;
		}

		static_assert( Sum( 85, 74 ) == 85 + 74 );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionWithMutableArguments_Test1():
	c_program_text= """
		struct Box
		{
			i32 x;
			fn constexpr constructor( i32 in_x )
			( x= in_x ) {}
		}

		fn constexpr Sum( i32 x, i32 y ) : i32
		{
			var Box mut r(0); // Call here constexpr constructor.
			r.x= x + y;
			return r.x;
		}

		static_assert( Sum( 85, 74 ) == 85 + 74 );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionWithMutableArguments_Test2():
	c_program_text= """
		struct Box
		{
			i32 x;
			fn constexpr constructor()
			( x= 0 ) {}
		}

		fn constexpr Div( i32 x, i32 y ) : i32
		{
			var Box mut r; // Call here constexpr constructor.
			r.x= x / y;
			return r.x;
		}

		static_assert( Div( 98547, 74 ) == 98547 / 74 );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunction_ReturningReference_Test0():
	c_program_text= """
		fn constexpr MaxImpl( i32& x, i32& y ) : i32&
		{
			if( x > y ) { return x; }
			return y;
		}
		fn constexpr Max( i32 x, i32 y ) : i32
		{
			return MaxImpl( x, y );  // Call here function, returning reference.
		}

		static_assert( Max( -5, 85 ) == 85 );
		static_assert( Max( -658, 14 ) == 14 );
		static_assert( Max( 8854, 55 ) == 8854 );
		static_assert( Max( 55, -985 ) == 55 );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionInternalArray_Test0():
	c_program_text= """
		fn constexpr Bar( i32 x ) : i32
		{
			var [ i32, 3 ] mut arr= zero_init;
			arr[1u]= x;
			return arr[1u] + arr[2u];
		}
		fn Foo()
		{
			static_assert( Bar( 65854 ) == 65854 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionInternalArray_Test1():
	c_program_text= """
		fn constexpr Bar() : i32
		{
			var [ i32, 3 ] constexpr arr[ 85, 69, 845 ];
			return arr[2u];
		}
		fn Foo()
		{
			static_assert( Bar() == 845 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionInternalArray_Test2():
	c_program_text= """
		fn constexpr Bar( i32 x ) : i32
		{
			var [ i32, 3 ] mut arr[ x - 1, x, x + 1 ];

			auto mut sum= 0;
			auto mut counter= 0u;
			while( counter < 3u )
			{
				sum+= arr[counter];
				++counter;
			}
			return sum;
		}
		fn Foo()
		{
			static_assert( Bar( 85 ) == 85 * 3 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionInternalStruct_Test0():
	c_program_text= """
		struct Vec{ i32 x; i32 y; }
		fn constexpr Bar( i32 x ) : i32
		{
			var Vec mut vec= zero_init;
			vec.x=  x;
			vec.y= -x;
			return vec.x * vec.y;
		}
		fn Foo()
		{
			static_assert( Bar( 85 ) == -85 * 85 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionInternalStruct_Test1():
	c_program_text= """
		struct Vec
		{
			i32 x; i32 y;
		}
		fn constexpr Bar( i32 x ) : i32
		{
			var Vec mut vec{ .x= 0, .y= 0 }; // Call default constructor here
			vec.x= x * 2;
			vec.y= x * 7;
			return vec.x - vec.y;
		}
		fn Foo()
		{
			static_assert( Bar( 658 ) == -5 * 658 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionCallOtherFunction_Test0():
	c_program_text= """
		fn constexpr Add( i32 x, i32 y, i32 z ) : i32
		{
			return x + y + z;
		}
		fn constexpr Bar( i32 x ) : i32
		{
			return Add( x, x, x ); // Call multiple args function.
		}
		fn Foo()
		{
			static_assert( Bar( 41 ) == 41 * 3 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionCallOtherFunction_Test1():
	c_program_text= """
		fn constexpr Get() : i32
		{
			return 666;
		}
		fn constexpr Pass() : i32
		{
			return Get();  // Call zero-arg function.
		}
		fn Foo()
		{
			static_assert( Pass() == 666 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionCallOtherFunction_Test2():
	c_program_text= """
		struct Vec
		{
			i32 x;
			i32 y;
			fn constexpr Dot( this ) : i32 { return x * y; }
		}
		fn constexpr Bar( i32 x, i32 y ) : i32
		{
			var Vec vec{ .x= x, .y= y };
			return vec.Dot(); // Call method.
		}
		fn Foo()
		{
			static_assert( Bar( 451, 654 ) == 451 * 654 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionCallOtherFunction_Test3():
	c_program_text= """
		fn constexpr DoNothing(){}
		fn constexpr Get() : i32
		{
			DoNothing(); // Call function with void type result.
			return 2018;
		}
		fn Foo()
		{
			static_assert( Get() == 2018 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionCallOtherFunction_Test4():
	c_program_text= """
		fn constexpr Pass( i32 x ) : i32 { return x; }
		fn constexpr Get() : i32
		{
			auto mut i= 0u;
			auto mut res= 0;
			while( i < 4096u ) // Call function in loop. We must reset stack position after each call.
			{
				auto mut one= 1; // Mutable, because we needs to prevent constexpr call of 'Pass' function.
				res+= Pass(one);
				++i;
			}

			return res;
		}
		fn Foo()
		{
			static_assert( Get() == 4096 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionCallOtherFunction_Test5():
	c_program_text= """
		fn constexpr Pass( i32& x ) : i32 { return x; }
		fn constexpr DoubleIt( i32 x ) : i32
		{
			return Pass(x) * 2; // Call function with reference arg.
		}
		fn Foo()
		{
			static_assert( DoubleIt(985) == 985 * 2 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunctionCallOtherFunction_Test5():
	c_program_text= """
		struct Box{ i32 x; }
		fn constexpr Unbox( Box box ) : i32 { return box.x; }
		fn constexpr Pass( i32 x ) : i32
		{
			var Box box{ .x= x };
			return Unbox(box);   // Call here function with "byval" argument.
		}
		fn Foo()
		{
			static_assert( Pass(114525) == 114525 );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunction_CompositeArgument_Test0():
	c_program_text= """
		struct Vec{ i32 x; i32 y; }
		fn constexpr Dot( Vec& vec ) : i32
		{
			return vec.x * vec.y;
		}
		fn Foo()
		{
			var Vec constexpr vec{ .x= 954, .y= 8854 };
			static_assert( Dot(vec) == 954 * 8854 );   // Pass simple struct by-reference.
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunction_CompositeArgument_Test1():
	c_program_text= """
		struct Vec{ i32 x; i32 y; }
		fn constexpr Sub( Vec vec ) : i32
		{
			return vec.x - vec.y;
		}
		fn Foo()
		{
			var Vec constexpr vec{ .x= 954, .y= 8854 };
			static_assert( Sub(vec) == 954 - 8854 );   // Pass simple struct by-value.
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunction_CompositeArgument_Test2():
	c_program_text= """
		struct Box{ i32& r; }
		fn constexpr Unbox( Box& box ) : i32
		{
			return box.r;
		}

		auto constexpr g_x= 88854;
		var Box constexpr g_box{ .r= g_x };
		static_assert( Unbox( g_box ) == g_x );   // Pass structure with reference inside.
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunction_CompositeArgument_Test3():
	c_program_text= """
		fn constexpr GetArrayElement( [ i32, 4 ]& arr, u32 index ) : i32
		{
			return arr[index];
		}

		var [ i32, 4 ] constexpr arr[ 58, 9545, 652, -85 ];
		// Pass array by reference.
		static_assert( GetArrayElement( arr, 0u ) == arr[0u] );
		static_assert( GetArrayElement( arr, 1u ) == arr[1u] );
		static_assert( GetArrayElement( arr, 2u ) == arr[2u] );
		static_assert( GetArrayElement( arr, 3u ) == arr[3u] );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunction_CompositeArgument_Test4():
	c_program_text= """
		fn constexpr Pass( f32& x ) : f32
		{
			return x;
		}

		static_assert( Pass( 2.718281828f ) == 2.718281828f );   // Pass scalar argument by-reference.
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunction_ReturnStruct_Test0():
	c_program_text= """
		struct S{ i32 x; }
		fn constexpr Foo() : S
		{
			var S s{ .x= 88565 };
			return s;
		}

		static_assert( Foo().x == 88565 );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunction_ReturnStruct_Test1():
	c_program_text= """
		struct S{ i32 x; i32 y; }
		fn constexpr Foo() : S
		{
			var S s{ .x= 9995, .y= -6665 };
			return s;
		}
		auto constexpr g_s= Foo();
		static_assert( g_s.x == 9995 );
		static_assert( g_s.y == -6665 );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunction_ReturnStruct_Test2():
	c_program_text= """
		struct S{ [ i32, 3 ] arr; }
		fn constexpr Foo() : S
		{
			var S mut s= zero_init;
			s.arr[1u]= 77412;
			return s;
		}

		static_assert( Foo().arr[1u] == 77412 );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunction_ReturnStruct_Test3():
	c_program_text= """
		struct F{ [ f64, 8 ] trash; i32 x; }
		struct S{ f32 trash; F f; }
		fn constexpr Foo( i32 x ) : S
		{
			var S mut s= zero_init;
			s.f.x= x;
			return s;
		}

		static_assert( Foo(85258).f.x == 85258 );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunction_ReturnStruct_Test4():
	c_program_text= """
		struct F{ [ f64, 8 ] trash; i32 x; }
		struct S{ f32 trash; F f; }
		fn constexpr Bar( i32 x ) : S
		{
			var S mut s= zero_init;
			s.f.x= x;
			return s;
		}

		fn Foo()
		{
			auto mut s= Bar( 95159 );
			halt if( s.f.x != 95159 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ConstexprFunction_ReturnStruct_Test5():
	c_program_text= """
		struct F{ [ f64, 8 ] trash; i32 x; }
		struct S{ f32 trash; F f; }
		fn constexpr Bar( i32 x ) : S
		{
			var S mut s= zero_init;
			s.f.x= x;
			return s;
		}

		fn constexpr Extract( S& s ) : i32
		{
			return s.f.x;
		}

		static_assert( Extract( Bar( 8547 ) ) == 8547 );  // Call one constexpr struc and pass it`s result by-reference to another.
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunction_ReturnStruct_Test6():
	c_program_text= """
		struct F{ [ f64, 8 ] trash; i32 x; }
		struct S{ f32 trash; F f; }
		fn constexpr Bar( i32 x ) : S
		{
			var S mut s= zero_init;
			s.f.x= x;
			return s;
		}

		fn constexpr Extract( S s ) : i32
		{
			return s.f.x;
		}

		static_assert( Extract( Bar( 8547 ) ) == 8547 );  // Call one constexpr struc and pass it`s result by-value to another.
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunction_ReturnStruct_Test7():
	c_program_text= """
		struct F{ [ f64, 8 ] trash; i32 x; }
		struct S{ f32 trash; F f; }
		fn constexpr Bar( i32 x ) : S
		{
			var S mut s= zero_init;
			s.f.x= x;
			return s;
		}

		fn ExtractAndMul( S& s, i32 x ) : i32
		{
			return s.f.x * x;
		}

		fn Foo()
		{
			auto r= ExtractAndMul( Bar( 95254 ), 21 ); // Pass constexpr result to runtime function call.
			halt if( r != 95254 * 21 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ConstexprFunction_ReturnStruct_Test8():
	c_program_text= """
		struct S{ i32 x; i32 y; }
		fn constexpr Foo() : S
		{
			var S mut s{ .x= 111, .y= -3 };
			// "move" in "constexpr" function
			return move(s);
		}
		auto constexpr g_s= Foo();
		static_assert( g_s.x == 111 );
		static_assert( g_s.y == -3 );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunction_RecursiveCall_Test0():
	c_program_text= """
		fn constexpr Factorial( u32 x ) : u32
		{
			if( x <= 1u ) { return 1u; }
			return x * Factorial( x - 1u );
		}

		static_assert( Factorial( 0u) ==   1u );
		static_assert( Factorial( 1u) ==   1u );
		static_assert( Factorial( 2u) ==   2u );
		static_assert( Factorial( 3u) ==   6u );
		static_assert( Factorial( 4u) ==  24u );
		static_assert( Factorial( 5u) == 120u );
		static_assert( Factorial( 6u) == 720u );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprFunction_RecursiveCall_Test1():
	c_program_text= """
		namespace S
		{

		template</ type T />
		fn constexpr Factorial( T x ) : T
		{
			if( x <= T(1) ) { return T(1); }
			return x * S::Factorial( x - T(1) );
		}

		static_assert( Factorial( 0u) ==   1u );
		static_assert( Factorial( 1u) ==   1u );
		static_assert( Factorial( 2u) ==   2u );
		static_assert( Factorial( 3u) ==   6u );
		static_assert( Factorial( 4u) ==  24u );
		static_assert( Factorial( 5u) == 120u );
		static_assert( Factorial( 6u) == 720u );

		} // namespace S
	"""
	tests_lib.build_program( c_program_text )


def ConstexprStaticMethodBuilding_Test0():
	c_program_text= """
		struct S
		{
			i32 x;
			template</type T/>
			fn Make( T t ) : S
			{
				var S mut s{ .x= i32(t) };
				return s;
			}
		}
		fn Foo()
		{
			// Should access template factory method which built is triggered and triggers class building.
			auto s= S::Make( 34.7f );
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstexprCallLoop_Test0():
	c_program_text= """
		// This example triggers "FooA" constexpr call when its building isn't finished yet.
		auto x= FooA();
		fn constexpr FooA() : i32
		{
			if( false )
			{
				static_assert( FooB() == 33 );
			}
			return 33;
		}
		fn constexpr FooB() : i32
		{
			return FooA();
		}
	"""
	tests_lib.build_program( c_program_text )
