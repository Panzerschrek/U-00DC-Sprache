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


def ArraysAreCopyConstructible_Test4():
	c_program_text= """
		fn Foo() : i32
		{
			var [ i32, 2 ] mut a0[ 7568, 13 ];
			auto a1= a0; // Copy array in auto variable initialization.

			return a1[0] - a1[1];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 7568 - 13 )


def ArraysAreCopyConstructible_Test5():
	c_program_text= """
		class S {} // Class is noncopyable by-default
		fn Foo()
		{
			var [ S, 4 ] mut a0;
			var [ S, 4 ] mut a1(a0); // Error, array is not copyable, because element is not copyable
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CopyConstructValueOfNoncopyableType" )
	assert( errors_list[0].src_loc.line == 6 )


def ArraysAreCopyConstructible_Test6():
	c_program_text= """
		class S {} // Class is noncopyable by-default
		fn Foo()
		{
			var [ S, 4 ] mut a0;
			var [ S, 4 ] mut a1= a0; // Error, array is not copyable, because element is not copyable
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CopyConstructValueOfNoncopyableType" )
	assert( errors_list[0].src_loc.line == 6 )


def ArraysAreCopyConstructible_Test7():
	c_program_text= """
		class S {} // Class is noncopyable by-default
		fn Foo()
		{
			var [ S, 4 ] mut a0;
			auto mut a1= a0; // Error, array is not copyable, because element is not copyable
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CopyConstructValueOfNoncopyableType" )
	assert( errors_list[0].src_loc.line == 6 )


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


def ArrayAsValueArgument_Test1():
	c_program_text= """
		struct S{ i32 x; }
		fn Sum( [ S, 4 ] arr ) : i32
		{
			return arr[0].x + arr[1].x + arr[2].x + arr[3].x;
		}
		fn Foo() : i32
		{
			var [ S, 4 ] mut a[ { .x= 56 }, { .x= 94 }, { .x=176 }, { .x= 6434 } ];

			return Sum( a );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 56 + 94 + 176 + 6434 )


def ArrayAsValueArgument_Test2():
	c_program_text= """
		class S {} // Class is noncopyable by-default
		fn Bar( [ S, 4 ] arr );
		fn Foo()
		{
			var [ S, 4 ] a;
			return Bar( a ); // Error, array is not copyable
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CopyConstructValueOfNoncopyableType" )
	assert( errors_list[0].src_loc.line == 7 )


def ArrayAsValueArgument_Test3():
	c_program_text= """
		class S
		{
			i32 x;
			// Class is noncopyable by-default
			fn constructor( i32 in_x ) (x= in_x) {}
		}
		fn Product( [ S, 3 ] arr ) : i32
		{
			return arr[0].x * arr[1].x * arr[2].x;
		}
		fn Foo() : i32
		{
			var [ S, 3 ] mut a[ (17), (2), (97) ];

			return Product( move(a) ); // Ok, move, not copy
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 17 * 2 * 97 )


def ArrayAsReturnValue_Test0():
	c_program_text= """
		fn GetArr() : [ i32, 4 ]
		{
			var [ i32, 4 ] mut a[ 78, 12, 156, 8 ];
			return a;
		}
		fn Foo() : i32
		{
			auto a= GetArr();
			return ( a[0] - a[1] ) * a[2] / a[3];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == int( ( 78 - 12 ) * 156 / 8 ) )


def ArrayAsReturnValue_Test1():
	c_program_text= """
		class S {} // Class is noncopyable by-default
		fn GetArr() : [ S, 8 ]
		{
			var [ S, 8 ] a;
			return a; // Error, array is noncopyable
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CopyConstructValueOfNoncopyableType" )
	assert( errors_list[0].src_loc.line == 6 )


def ArrayAsReturnValue_Test2():
	c_program_text= """
		class S
		{
			i32 x;
			// Class is noncopyable by-default
			fn constructor( i32 in_x ) (x= in_x) {}
		}
		fn GetArr() : [ S, 2 ]
		{
			var [ S, 2 ] mut a[ (33), (97) ];
			return move(a); // Ok, move, not copy
		}
		fn Foo() : i32
		{
			auto a= GetArr();
			return a[1].x / a[0].x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == int( 97 / 33 ) )


def ArraysAssignment_Test0():
	c_program_text= """
		fn Foo() : u32
		{
			var [ u32, 3 ] mut a0= zero_init, mut a1[ 45u, 11u, 2u ];
			a0= a1;
			return a0[0] - a0[1] * a0[2];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 45 - 11 * 2 )


def ArraysAssignment_Test1():
	c_program_text= """
		class S {} // Class is noncopyable by-default
		fn Foo()
		{
			var [ S, 8 ] mut a0, mut a1;
			a0= a1;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OperationNotSupportedForThisType" )
	assert( errors_list[0].src_loc.line == 6 )


def ArraysAssignment_Test2():
	c_program_text= """
		class S
		{
			i32 x;
			// Class is noncopyable by-default
			fn constructor( i32 in_x ) (x= in_x) {}
		}
		fn Foo() : i32
		{
			var [ S, 2 ] mut a0[ (0), (0) ], mut a1[ (79), (3) ];
			a0= move(a1); // ok, copy, not move
			return a0[0].x / a0[1].x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == int( 79 / 3 ) )


def StringLiteralAsInitializator_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			var [ char8, 3 ] mut str("kRt"); // constructor initializer
			return i32(str[0]) + 256 * i32(str[1]) + 65536 * i32(str[2]);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('k') + 256 * ord('R') + 65536 * ord('t') )


def StringLiteralAsInitializator_Test1():
	c_program_text= """
		fn Foo() : u32
		{
			var [ char8, 3 ] mut str= "1_S"; // expression initializer
			return u32(str[0]) + 256u * u32(str[1]) + 65536u * u32(str[2]);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('1') + 256 * ord('_') + 65536 * ord('S') )


def StringLiteralAsInitializator_Test2():
	c_program_text= """
		fn Foo() : u64
		{
			auto mut str= "R_4f+"; // auto variable initialization
			var u64 mut r(0);
			for( auto mut i= 0s; i < typeinfo</typeof(str)/>.element_count; ++i )
			{
				r|= u64(str[i]) << (i << 3u);
			}
			return r;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( ( ( call_result >>  0 ) & 255) == ord('R') )
	assert( ( ( call_result >>  8 ) & 255) == ord('_') )
	assert( ( ( call_result >> 16 ) & 255) == ord('4') )
	assert( ( ( call_result >> 24 ) & 255) == ord('f') )
	assert( ( ( call_result >> 32 ) & 255) == ord('+') )


def LargeArrayCopy_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			auto s= 65536s;
			var [ i32, s ] mut a0= zero_init;
			a0[657]= 565;
			a0[3456]= -54;
			a0[312]= 78423;
			a0[22]= 88;
			a0[1274]= 751;
			a0[47231]= 51;
			a0[26754]= -4623;

			var [ i32, s ] mut a1= a0; // Copy here large array.

			auto mut r= 0;
			for( auto mut i= 0s; i < s; ++i ){ r+= a1[i]; }
			return r;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 565 -54 + 78423 + 88 + 751 + 51 -4623 )


def ReferenceProtectionError_ForArrayCopying_Test0():
	c_program_text= """
		type Vec4= [ f32, 4 ];
		fn Foo()
		{
			var Vec4 mut a0= zero_init, mut a1= zero_init;
			auto &mut a1_ref= a1;
			a0= a1;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 7 )


def ReferenceProtectionError_ForArrayCopying_Test1():
	c_program_text= """
		type Vec4= [ f32, 4 ];
		fn Foo()
		{
			var Vec4 mut a0= zero_init, mut a1= zero_init;
			auto &mut a0_ref= a0;
			a0= a1;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 7 )
