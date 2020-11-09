from py_tests_common import *


def ArraysAreCopyConstructible_Test0():
	c_program_text= """
		fn Foo() : f32
		{
			var [ f32, 2 ] mut a0[ 45.0f, 5.0f ];
			var[ f32, 2 ] mut a1(a0);

			return a1[0] / a1[1];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 45.0 / 5.0 )


def ArraysAreCopyConstructible_Test1():
	c_program_text= """
		fn Foo() : i32
		{
			var [ i32, 3 ] mut a0[ 72, 12, 3 ];
			var[ i32, 3 ] mut a1= a0;

			return ( a1[0] - a1[1] ) * a1[2];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == (72 - 12) * 3 )


def ArraysAreCopyConstructible_Test2():
	c_program_text= """
		fn Foo() : f64
		{
			var [ f64, 2 ] mut a[ 37.0, 22.0 ];
			var tup[ bool, [ f64, 2 ] ] mut t[ false, (a) ]; // Call copy constructor for array tuple element

			return t[1][0] - t[1][1];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 37.0 - 22.0 )


def ArraysAreCopyConstructible_Test3():
	c_program_text= """
		fn Foo() : u32
		{
			var [ u32, 4 ] mut a[ 4u, 8u, 15u, 16u ];
			var [ [ u32, 4 ], 2 ] mut aa[ a, zero_init ]; // Call copy constructor for array tuple element

			return aa[0][0] * aa[0][1] - aa[0][3] / aa[0][2];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 4 * 8 - int(16 / 15) )


def ArrayAsValueArgument_Test0():
	c_program_text= """
		fn Dot( [ f32, 2 ] a, [ f32, 2 ] b ) : f32
		{
			return a[0] * b[0] + a[1] * b[1];
		}
		fn Foo() : f32
		{
			var [ f32, 2 ] mut a[ 3.5f, 0.25f ], mut b[ 4.0f, 10.0f ];

			return Dot( a, b );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 3.5 * 4.0 + 0.25 * 10.0 )
