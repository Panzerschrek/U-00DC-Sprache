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
			fn constructor() ( x= 0, y= 0 ) {}
		}
		fn constexpr Bar( i32 x ) : i32
		{
			var Vec mut vec; // Call default constructor here
			vec.x = x * 2;
			vec.y=  x * 7;
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
			fn Dot( this ) : i32 { return x * y; }
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
