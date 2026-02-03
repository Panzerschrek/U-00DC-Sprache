from py_tests_common import *


def ArrayFillerInitializer_Test0():
	c_program_text= """
		fn Foo()
		{
			// Only one element filled.
			var [ i32, 3 ] mut arr[ 55, 777, 9999 ... ];
			halt if( arr[0] != 55 );
			halt if( arr[1] != 777 );
			halt if( arr[2] != 9999 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializer_Test1():
	c_program_text= """
		fn Foo()
		{
			// Fill three elements.
			var [ i32, 5 ] mut arr[ 2, 45, 78 ... ];
			halt if( arr[0] != 2 );
			halt if( arr[1] != 45 );
			halt if( arr[2] != 78 );
			halt if( arr[3] != 78 );
			halt if( arr[4] != 78 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializer_Test2():
	c_program_text= """
		fn Foo()
		{
			// Apply constructor initializer (with type conversion).
			var [ f32, 6 ] mut arr[ (-5), (42), (1272) ... ];
			halt if( arr[0] != -5.0f );
			halt if( arr[1] != 42.0f );
			halt if( arr[2] != 1272.0f );
			halt if( arr[3] != 1272.0f );
			halt if( arr[4] != 1272.0f );
			halt if( arr[5] != 1272.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializer_Test3():
	c_program_text= """
		fn Foo()
		{
			// The whole array is filled.
			var [ char8, 4 ] mut arr[ 'q' ... ];
			halt if( arr[0] != 'q' );
			halt if( arr[1] != 'q' );
			halt if( arr[2] != 'q' );
			halt if( arr[3] != 'q' );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializer_Test4():
	c_program_text= """
		fn Foo()
		{
			// Apply struct initializer.
			var [ S, 3 ] mut arr[ { .x= 56, .y= -23.5f } ... ];
			halt if( arr[0].x != 56 ); halt if( arr[0].y != -23.5f );
			halt if( arr[1].x != 56 ); halt if( arr[1].y != -23.5f );
			halt if( arr[2].x != 56 ); halt if( arr[2].y != -23.5f );
		}
		struct S{ i32 x; f32 y; }
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializer_Test5():
	c_program_text= """
		fn Foo()
		{
			// Apply array initializer.
			var [ [ i32, 2 ], 3 ] mut arr[ [ 7, 90 ] ... ];
			halt if( arr[0][0] != 7 ); halt if( arr[0][1] != 90 );
			halt if( arr[1][0] != 7 ); halt if( arr[1][1] != 90 );
			halt if( arr[2][0] != 7 ); halt if( arr[2][1] != 90 );
		}
		struct S{ i32 x; f32 y; }
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializer_Test6():
	c_program_text= """
		fn Foo()
		{
			// Array filler within array filler.
			var [ [ u16, 3 ], 4 ] mut arr[ [ 567u16 ... ] ... ];
			halt if( arr[0][0] != 567u16 );
			halt if( arr[0][1] != 567u16 );
			halt if( arr[0][2] != 567u16 );
			halt if( arr[1][0] != 567u16 );
			halt if( arr[1][1] != 567u16 );
			halt if( arr[1][2] != 567u16 );
			halt if( arr[2][0] != 567u16 );
			halt if( arr[2][1] != 567u16 );
			halt if( arr[2][2] != 567u16 );
			halt if( arr[3][0] != 567u16 );
			halt if( arr[3][1] != 567u16 );
			halt if( arr[3][2] != 567u16 );
		}
		struct S{ i32 x; f32 y; }
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializer_Test7():
	c_program_text= """
		fn Foo()
		{
			// Initializer mutates external state.
			var u32 mut x= 7u;
			var [ u32, 5 ] mut arr[ Next(x) ... ];
			halt if( arr[0] != 7u );
			halt if( arr[1] != 8u );
			halt if( arr[2] != 9u );
			halt if( arr[3] != 10u );
			halt if( arr[4] != 11u );
		}
		fn Next( u32 &mut x ) : u32
		{
			var u32 res= x;
			++x;
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializer_Test8():
	c_program_text= """
		fn Foo()
		{
			// Array filler within array filler with mutate state usage.
			var u32 mut x= 0u;
			var [ [ u32, 3 ], 2 ] mut arr[ [ NextSquare(x) ... ] ... ];
			halt if( arr[0][0] !=  0u );
			halt if( arr[0][1] !=  1u );
			halt if( arr[0][2] !=  4u );
			halt if( arr[1][0] !=  9u );
			halt if( arr[1][1] != 16u );
			halt if( arr[1][2] != 25u );
		}
		fn NextSquare( u32 &mut x ) : u32
		{
			var u32 res= x * x;
			++x;
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializer_Test9():
	c_program_text= """
		struct S
		{
			fn conversion_constructor( i32 in_x )
				( x= in_x )
			{}

			i32 x;
		}
		fn Foo()
		{
			// Perform type conversion in array filler initializer.
			var [ S, 5 ] mut arr[ 67, 123 ... ];
			halt if( arr[0].x != 67 );
			halt if( arr[1].x != 123 );
			halt if( arr[2].x != 123 );
			halt if( arr[3].x != 123 );
			halt if( arr[4].x != 123 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializer_Test10():
	c_program_text= """
		// zero_init works for array filler initializer.
		var [ char8, 6 ] constexpr arr[ 'r', '!', zero_init ... ];
		static_assert( arr[0] == 'r' );
		static_assert( arr[1] == '!' );
		static_assert( arr[2] == '\\0' );
		static_assert( arr[3] == '\\0' );
		static_assert( arr[4] == '\\0' );
		static_assert( arr[5] == '\\0' );
	"""
	tests_lib.build_program( c_program_text )


def ArrayFillerInitializer_Test11():
	c_program_text= """
		fn Foo()
		{
			unsafe
			{
				// "uninitialized" for filler initializer.
				var[ u32, 1024 ] mut arr[ 13u, 76u, 677u, uninitialized ... ];
				halt if( arr[0] != 13u );
				halt if( arr[1] != 76u );
				halt if( arr[2] != 677u );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerConstexpr_Test0():
	c_program_text= """
		fn Foo()
		{
			// Fill the whole array with single constant value.
			var [ i32, 4 ] constexpr arr[ 56 ... ];
			static_assert( arr[0] == 56 );
			static_assert( arr[1] == 56 );
			static_assert( arr[2] == 56 );
			static_assert( arr[3] == 56 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerConstexpr_Test1():
	c_program_text= """
		fn Foo()
		{
			// First several values are specified one by one, the remaining tail is filled.
			var [ u32, 8 ] constexpr arr[ 17u, 56u, 901u, 7u ... ];
			static_assert( arr[0] == 17u );
			static_assert( arr[1] == 56u );
			static_assert( arr[2] == 901u );
			static_assert( arr[3] == 7u );
			static_assert( arr[4] == 7u );
			static_assert( arr[5] == 7u );
			static_assert( arr[6] == 7u );
			static_assert( arr[7] == 7u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerConstexpr_Test2():
	c_program_text= """
		fn Foo()
		{
			// Fill array of constexpr structs.
			var [ S, 3 ] constexpr arr [ { .x= 'T', .y= -67.5 } ... ];
			static_assert( arr[0].x == 'T' ); static_assert( arr[0].y == -67.5 );
			static_assert( arr[1].x == 'T' ); static_assert( arr[1].y == -67.5 );
			static_assert( arr[2].x == 'T' ); static_assert( arr[2].y == -67.5 );
		}
		struct S{ char8 x; f64 y; }
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerConstexpr_Test3():
	c_program_text= """
		fn Foo()
		{
			// Fill array of constexpr structs, but specify different first element.
			var [ S, 4 ] constexpr arr[ { .x= '~', .y= -1.8 }, { .x= 'n', .y= 124.25 } ... ];
			static_assert( arr[0].x == '~' ); static_assert( arr[0].y == -1.8 );
			static_assert( arr[1].x == 'n' ); static_assert( arr[1].y == 124.25 );
			static_assert( arr[2].x == 'n' ); static_assert( arr[2].y == 124.25 );
			static_assert( arr[3].x == 'n' ); static_assert( arr[3].y == 124.25 );
		}
		struct S{ char8 x; f64 y; }
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerConstexpr_Test4():
	c_program_text= """
		fn Foo()
		{
			// Fill large constant array.
			var [ u64, 1024 * 32 ] constexpr arr[ 783567835673u64 ... ];
			static_assert( arr[     0 ] == 783567835673u64 );
			static_assert( arr[    67 ] == 783567835673u64 );
			static_assert( arr[   230 ] == 783567835673u64 );
			static_assert( arr[   437 ] == 783567835673u64 );
			static_assert( arr[ 15435 ] == 783567835673u64 );
			static_assert( arr[ 32767 ] == 783567835673u64 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerConstexpr_Test5():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			// Error - filler doesn't produce constant expression.
			var [ i32, 4 ] constexpr arr[ x ... ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 6 ) )


def ArrayFillerInitializerConstexpr_Test6():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			// Filler is constant, but head element isn't.
			var [ i32, 4 ] constexpr arr[ x, 0 ... ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 6 ) )


def ArrayFillerInitializerConstexpr_Test7():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			// Error - filler doesn't produce constant expression, since filler function call mutates an external variable.
			var [ i32, 4 ] constexpr arr[ Next(x) ... ];
		}
		fn constexpr Next( u32 &mut x )
		{
			var u32 res= x;
			++x;
			return res;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 6 ) )


def ArrayFillerInitializerConstexpr_Test8():
	c_program_text= """
		// Fill the whole array with single constant value.
		var [ i32, 4 ] constexpr arr[ 56 ... ];
		static_assert( arr[0] == 56 );
		static_assert( arr[1] == 56 );
		static_assert( arr[2] == 56 );
		static_assert( arr[3] == 56 );
	"""
	tests_lib.build_program( c_program_text )


def ArrayFillerInitializerConstexpr_Test9():
	c_program_text= """
		// First several values are specified one by one, the remaining tail is filled.
		var [ u32, 8 ] constexpr arr[ 17u, 56u, 901u, 7u ... ];
		static_assert( arr[0] == 17u );
		static_assert( arr[1] == 56u );
		static_assert( arr[2] == 901u );
		static_assert( arr[3] == 7u );
		static_assert( arr[4] == 7u );
		static_assert( arr[5] == 7u );
		static_assert( arr[6] == 7u );
		static_assert( arr[7] == 7u );
	"""
	tests_lib.build_program( c_program_text )


def ArrayFillerInitializerConstexpr_Test10():
	c_program_text= """
		// Fill array of constexpr structs.
		var [ S, 3 ] constexpr arr [ { .x= 'T', .y= -67.5 } ... ];
		static_assert( arr[0].x == 'T' ); static_assert( arr[0].y == -67.5 );
		static_assert( arr[1].x == 'T' ); static_assert( arr[1].y == -67.5 );
		static_assert( arr[2].x == 'T' ); static_assert( arr[2].y == -67.5 );
		struct S{ char8 x; f64 y; }
	"""
	tests_lib.build_program( c_program_text )


def ArrayFillerInitializerConstexpr_Test11():
	c_program_text= """
		// Fill array of constexpr structs, but specify different first element.
		var [ S, 4 ] constexpr arr[ { .x= '~', .y= -1.8 }, { .x= 'n', .y= 124.25 } ... ];
		static_assert( arr[0].x == '~' ); static_assert( arr[0].y == -1.8 );
		static_assert( arr[1].x == 'n' ); static_assert( arr[1].y == 124.25 );
		static_assert( arr[2].x == 'n' ); static_assert( arr[2].y == 124.25 );
		static_assert( arr[3].x == 'n' ); static_assert( arr[3].y == 124.25 );
		struct S{ char8 x; f64 y; }
	"""
	tests_lib.build_program( c_program_text )


def ArrayFillerInitializerConstexpr_Test12():
	c_program_text= """
		// Fill large constant array.
		var [ u64, 1024 * 32 ] constexpr arr[ 783567835673u64 ... ];
		static_assert( arr[     0 ] == 783567835673u64 );
		static_assert( arr[    67 ] == 783567835673u64 );
		static_assert( arr[   230 ] == 783567835673u64 );
		static_assert( arr[   437 ] == 783567835673u64 );
		static_assert( arr[ 15435 ] == 783567835673u64 );
		static_assert( arr[ 32767 ] == 783567835673u64 );
	"""
	tests_lib.build_program( c_program_text )


def ArrayFillerInitializerForGlobalMutableVariable_Test0():
	c_program_text= """
		// Fill the whole array with single constant value.
		var [ i32, 4 ] mut arr[ 78431 ... ];
		fn Foo()
		{
			unsafe
			{
				halt if( arr[0] != 78431 );
				halt if( arr[1] != 78431 );
				halt if( arr[2] != 78431 );
				halt if( arr[3] != 78431 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerForGlobalMutableVariable_Test1():
	c_program_text= """
		// First several values are specified one by one, the remaining tail is filled.
		var [ u32, 8 ] mut arr[ 17u, 56u, 901u, 78423u ... ];
		fn Foo()
		{
			unsafe
			{
				halt if( arr[0] != 17u );
				halt if( arr[1] != 56u );
				halt if( arr[2] != 901u );
				halt if( arr[3] != 78423u );
				halt if( arr[4] != 78423u );
				halt if( arr[5] != 78423u );
				halt if( arr[6] != 78423u );
				halt if( arr[7] != 78423u );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerForGlobalMutableVariable_Test2():
	c_program_text= """
		struct S{ char8 x; f64 y; }
		// Fill array of structs.
		var [ S, 3 ] mut arr[ { .x= 'k', .y= 744.3 } ... ];
		fn Foo()
		{
			unsafe
			{
				halt if( arr[0].x != 'k' ); halt if( arr[0].y != 744.3 );
				halt if( arr[1].x != 'k' ); halt if( arr[1].y != 744.3 );
				halt if( arr[2].x != 'k' ); halt if( arr[2].y != 744.3 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerForGlobalMutableVariable_Test3():
	c_program_text= """
		struct S{ char8 x; f64 y; }
		// Fill array of structs, but specify different first element.
		var [ S, 4 ] mut arr[ { .x= '~', .y= -1.8 }, { .x= '@', .y= 6124.25 } ... ];
		fn Foo()
		{
			unsafe
			{
				halt if( arr[0].x != '~' ); halt if( arr[0].y != -1.8 );
				halt if( arr[1].x != '@' ); halt if( arr[1].y != 6124.25 );
				halt if( arr[2].x != '@' ); halt if( arr[2].y != 6124.25 );
				halt if( arr[3].x != '@' ); halt if( arr[3].y != 6124.25 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerForGlobalMutableVariable_Test4():
	c_program_text= """
		// Fill large constant array.
		var [ i64, 1024 * 8 ] mut arr[ 85588654653456 ... ];
		fn Foo()
		{
			unsafe
			{
				halt if( arr[    0 ] != 85588654653456 );
				halt if( arr[   67 ] != 85588654653456 );
				halt if( arr[  230 ] != 85588654653456 );
				halt if( arr[  437 ] != 85588654653456 );
				halt if( arr[ 2673 ] != 85588654653456 );
				halt if( arr[ 8191 ] != 85588654653456 );
				for( auto mut i= 0s; i < 1024s * 8s; ++i )
				{
					halt if( arr[i] != 85588654653456 );
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerForFieldOwnInitializer_Test0():
	c_program_text= """
		struct S
		{
			[ i32, 4 ] arr[ 776 ... ];
		}
		fn Foo()
		{
			var S mut s{}; // Own initializer containing array filler initializer should be called within {}.
			halt if( s.arr[0] != 776 );
			halt if( s.arr[1] != 776 );
			halt if( s.arr[2] != 776 );
			halt if( s.arr[3] != 776 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerForFieldOwnInitializer_Test1():
	c_program_text= """
		struct S
		{
			[ f32, 4 ] arr[ 13.5f ... ];
		}
		fn Foo()
		{
			var S mut s; // Own initializer containing array filler initializer should be called within default constructor.
			halt if( s.arr[0] != 13.5f );
			halt if( s.arr[1] != 13.5f );
			halt if( s.arr[2] != 13.5f );
			halt if( s.arr[3] != 13.5f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerForFieldOwnInitializer_Test2():
	c_program_text= """
		struct S
		{
			[ u32, 3 ] arr[ Bar() ... ];
		}
		fn Bar() : u32 { return 56712u; }
		fn Foo()
		{
			var S mut s(); // Own initializer containing array filler initializer should be called within called constructor.
			halt if( s.arr[0] != 56712u );
			halt if( s.arr[1] != 56712u );
			halt if( s.arr[2] != 56712u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerForFieldOwnInitializer_Test3():
	c_program_text= """
		struct S
		{
			// "arr" filler initializer is called here.
			fn constructor()
			{}

			[ char8, 5 ] arr[ '/' ... ];
		}
		fn Foo()
		{
			var S mut s;
			halt if( s.arr[0] != '/' );
			halt if( s.arr[1] != '/' );
			halt if( s.arr[2] != '/' );
			halt if( s.arr[3] != '/' );
			halt if( s.arr[4] != '/' );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerForFieldOwnInitializer_Test4():
	c_program_text= """
		struct S
		{
			// "arr" filler initializer is called here.
			fn constructor()
			()
			{}

			[ u8, 3 ] arr[ 0u8, 255u8 ... ];
		}
		fn Foo()
		{
			var S mut s;
			halt if( s.arr[0] != 0u8 );
			halt if( s.arr[1] != 255u8 );
			halt if( s.arr[2] != 255u8 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerForFieldOwnInitializer_Test5():
	c_program_text= """
		struct S
		{
			[ i32, 4 ] arr;
		}
		struct T
		{
			S a{ .arr[ 6 ... ] };
			S b{ .arr[ 9 ... ] };
		}
		fn Foo()
		{
			var T mut t; // Own initializers for fields "a" and "b" are called here, which include array filler initializers.
			halt if( t.a.arr[0] != 6 );
			halt if( t.a.arr[1] != 6 );
			halt if( t.a.arr[2] != 6 );
			halt if( t.a.arr[3] != 6 );
			halt if( t.b.arr[0] != 9 );
			halt if( t.b.arr[1] != 9 );
			halt if( t.b.arr[2] != 9 );
			halt if( t.b.arr[3] != 9 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerForFieldOwnInitializer_Test6():
	c_program_text= """
		struct S
		{
			[ i32, 4 ] arr;
		}
		struct T
		{
			S a{ .arr[ 66 ... ] };
			S b{ .arr[ 99 ... ] };
		}
		fn Foo()
		{
			var T mut t{ .b{ .arr[ 1, 2, 3, 4 ] } }; // Own initializers for field "a" is called here. Initializer for field "b" is overriden.
			halt if( t.a.arr[0] != 66 );
			halt if( t.a.arr[1] != 66 );
			halt if( t.a.arr[2] != 66 );
			halt if( t.a.arr[3] != 66 );
			halt if( t.b.arr[0] != 1 );
			halt if( t.b.arr[1] != 2 );
			halt if( t.b.arr[2] != 3 );
			halt if( t.b.arr[3] != 4 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerForFieldOwnInitializer_Test7():
	c_program_text= """
		struct S
		{
			[ u32, 3 ] arr[ Bar() ... ];
		}
		var u32 mut g_x= 100u;
		fn Bar() : u32
		{
			unsafe
			{
				var u32 res= g_x;
				++g_x;
				return res;
			}
		}
		fn Foo()
		{
			var S mut s; // Own initializer of "arr" field is called here - via generated default constructor call.
			halt if( s.arr[0] != 100u );
			halt if( s.arr[1] != 101u );
			halt if( s.arr[2] != 102u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerForFieldOwnInitializer_Test8():
	c_program_text= """
		struct S
		{
			[ i32, 5 ] arr[ Bar() ... ];
		}
		var i32 mut g_x= -3;
		fn Bar() : i32
		{
			unsafe
			{
				var i32 res= g_x * g_x * g_x;
				++g_x;
				return res;
			}
		}
		fn Foo()
		{
			var S mut s{}; // Own initializer of "arr" field is called here.
			halt if( s.arr[0] != -3 * 3 * 3 );
			halt if( s.arr[1] != -2 * 2 * 2 );
			halt if( s.arr[2] != -1 * 1 * 1 );
			halt if( s.arr[3] !=  0 * 0 * 0 );
			halt if( s.arr[4] !=  1 * 1 * 1 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerInConstructorInitializer_Test0():
	c_program_text= """
		struct S
		{
			fn constructor()
				( arr[ 1.0f, -1.0f ... ] )
			{}

			[ f32, 4 ] arr;
		}

		fn Foo()
		{
			var S mut s;
			halt if( s.arr[0] != 1.0f );
			halt if( s.arr[1] != -1.0f );
			halt if( s.arr[2] != -1.0f );
			halt if( s.arr[3] != -1.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerInConstructorInitializer_Test1():
	c_program_text= """
		struct S
		{
			fn constructor()
				( arr[ Bar() ... ] )
			{}

			[ i32, 5 ] arr;
		}
		var i32 mut g_x= -3;
		fn Bar() : i32
		{
			unsafe
			{
				var i32 res= g_x * g_x * g_x;
				++g_x;
				return res;
			}
		}
		fn Foo()
		{
			var S mut s;
			halt if( s.arr[0] != -3 * 3 * 3 );
			halt if( s.arr[1] != -2 * 2 * 2 );
			halt if( s.arr[2] != -1 * 1 * 1 );
			halt if( s.arr[3] !=  0 * 0 * 0 );
			halt if( s.arr[4] !=  1 * 1 * 1 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ArrayFillerInitializerInConstructorInitializer_Test2():
	c_program_text= """
		struct S
		{
			fn constructor( u32 mut x )
				( arr[ Next( x ) ... ] )
			{}

			[ u32, 3 ] arr;
		}
		fn Next( u32 &mut x ) : u32
		{
			var u32 res= x * x;
			++x;
			return res;
		}
		fn Foo()
		{
			var S mut s( 3u );
			halt if( s.arr[0] != 3u * 3u );
			halt if( s.arr[1] != 4u * 4u );
			halt if( s.arr[2] != 5u * 5u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
