from py_tests_common import *


def EqualityOperatorGeneration_Test0():
	c_program_text= """
	struct S
	{
		i32 a;
		[ char8, 4 ] b;
		f32 c;
		tup[ bool, void, u8 ] d;

		// Operator "==" should be generated.
	}
	fn Foo()
	{
		// Use "mut" to prevent "constexpr" calls.
		var S mut q{ .a=42, .b= "lolQ", .c= 3.1451926535f, .d[ true, void(), 77u8 ] };
		var S mut w= q;
		halt if( w != q );
		halt if( !( w== q ) );
		halt if( q != q );
		halt if( !( w == w ) );

		auto mut e= w;
		e.d[0]= false;

		halt if( !( e == e ) );
		halt if( e == w );
		halt if( !( e != q ) );

		var S mut r{ .a=42, .b= "lolQ", .c= 3.1451926535f, .d[ false, void(), 77u8 ] };
		halt if( r != e );
		halt if( !( e == r ) );

		r.b[1]= "%"c8;
		halt if( e == r );
	}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def EqualityOperatorGeneration_Test1():
	c_program_text= """
	struct S
	{
		i32 a;
		f32 b;
		u32 c;
		[ i32, 2 ] d;
		// Constexpr operator == should be generated.
	}

	var S q{ .a= 42, .b= 66.1f, .c= 789u, .d[ 76, -33 ] };
	var S w{ .a= 42, .b= 66.1f, .c= 789u, .d[ 76, -33 ] };
	var S e{ .a= 24, .b= 66.1f, .c= 789u, .d[ 76, -33 ] };
	var S r{ .a= 42, .b= 66.1f, .c= 789u, .d[  0, -33 ] };
	var S t{ .a= 42, .b= 66.1f, .c= -89u, .d[ 76, -33 ] };
	var S y{ .a= 42, .b= 66.1f, .c= 789u, .d[ 76, +33 ] };

	static_assert( q == q );
	static_assert( q == w );
	static_assert( w == q );
	static_assert( !( q != w ) );
	static_assert( q != e );
	static_assert( r != w );
	static_assert( t != r );
	static_assert( q != t );
	static_assert( t == t );
	static_assert( y != q );
	"""
	tests_lib.build_program( c_program_text )


def EqualityOperatorGeneration_Test2():
	c_program_text= """
	struct Vec
	{
		f32 x;
		f32 y;
	}

	fn Foo()
	{
		// Use "mut" to prevent "constexpr" calls.
		var Vec mut normal_vec{ .x= 77.4f, .y= -32.0f };
		var Vec mut normal_vec_copy= normal_vec;
		var Vec mut another_normal_vec{ .x= 65.4f, .y= 999999999.0f };

		var Vec mut plus_zero_vec { .x= +0.0f, .y= +0.0f };
		var Vec mut minus_zero_vec{ .x= -0.0f, .y= -0.0f };

		var Vec mut nan_vec0{ .x= 77.0f, .y= 0.0f / 0.0f };
		var Vec mut nan_vec1{ .x= 0.0f / 0.0f, .y= 66.1f };

		halt if( normal_vec != normal_vec );
		halt if( normal_vec != normal_vec_copy );
		halt if( normal_vec == another_normal_vec );
		halt if( normal_vec_copy == another_normal_vec );

		halt if( plus_zero_vec  != plus_zero_vec  );
		halt if( minus_zero_vec != minus_zero_vec );
		halt if( plus_zero_vec != minus_zero_vec );

		halt if( nan_vec0 == nan_vec0 );
		halt if( nan_vec1 == nan_vec1 );
		halt if( nan_vec0 == nan_vec1 );
		halt if( nan_vec0 == normal_vec );
		halt if( nan_vec1 == normal_vec );
	}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
