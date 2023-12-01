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


def EqualityOperatorGeneration_Test3():
	c_program_text= """
	struct S
	{
		i32 x;
		op==(S& l, S& r) : bool = default; // Explicitly request operator generation.
	}

	fn Foo()
	{
		// Use "mut" to prevent "constexpr" calls.
		var S mut a{ .x= 0 };
		var S mut b{ .x= 0 };
		var S mut c{ .x= 1 };

		halt if( a != a );
		halt if( b != a );
		halt if( !( b == a ) );
		halt if( c == a );
		halt if( c == b );
		halt if( !( c != b ) );
	}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def EqualityOperatorGeneration_Test4():
	c_program_text= """
	class S
	{
		i32 x;
		fn constructor(i32 in_x) (x= in_x){}
		op==(S& l, S& r) : bool = default; // Explicitly request operator generation for class. Normally "==" operator generation fro classes is disabled.
	}

	fn Foo()
	{
		// Use "mut" to prevent "constexpr" calls.
		var S mut a(0);
		var S mut b(0);
		var S mut c(1);

		halt if( a != a );
		halt if( b != a );
		halt if( !( b == a ) );
		halt if( c == a );
		halt if( c == b );
		halt if( !( c != b ) );
	}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def EqualityOperatorGeneration_Test5():
	c_program_text= """
	struct S
	{
		i32 x;
		op==(S& l, S& r) : bool
		{
			return l.x == r.x;
		}
	}
	struct T { S s; } // "==" operator should be generated, which calls "==" for "S" field.
	fn Foo()
	{
		// Use "mut" to prevent "constexpr" calls.
		var T mut a{ .s{ .x= 5 } };
		var T mut b{ .s{ .x= 5 } };
		var T mut c{ .s{ .x= 123 } };
		halt if( a != a );
		halt if( a != b );
		halt if( a == c );
		halt if( c != c );
	}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def EqualityOperatorGeneration_Test6():
	c_program_text= """
	struct S
	{
		$(i32) x;
		// Generated "==" should compare raw pointer field.
	}
	fn Foo()
	{
		var [ i32, 3 ] mut arr= zero_init;
		var S a{ .x= $<(arr[0]) };
		var S a_copy{ .x= $<(arr[0]) };
		var S b{ .x= $<(arr[1]) };
		var S c{ .x= $<(arr[2]) };
		var S c_copy{ .x= $<(arr[2]) };

		halt if( a != a );
		halt if( a != a_copy );
		halt if( a == b );
		halt if( a == c );
		halt if( b == c );
		halt if( c != c_copy );
	}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def EqualityOperatorGeneration_Test7():
	c_program_text= """
		enum E{ A, B, C }
		struct S
		{
			E e;
			// Generated "==" should compare enum field.
		}

		var S a{ .e= E::A };
		auto a_copy= a;
		var S b{ .e= E::B };
		var S c{ .e= E::C };
		var S c_copy{ .e= E::C };

		static_assert( a == a );
		static_assert( a == a_copy );
		static_assert( a != b );
		static_assert( a != c );
		static_assert( b != c );
		static_assert( c == c_copy );
	"""
	tests_lib.build_program( c_program_text )


def EqualityOperatorGeneration_Test8():
	c_program_text= """
		struct S
		{
			i32 x;
			op constexpr ==(S& l, i32 x) : bool { return l.x == x; }
			op constexpr ==(i32 x, S& r) : bool { return x == r.x; }
			// Request generation of "==" because implicit "==" generation is disabled because of other "==" operations.
			op==(S& l, S& r) : bool = default;
		}

		fn Foo()
		{
			// Use "mut" to prevent "constexpr"
			var S mut a{ .x=11 };
			var S mut a_copy{ .x= 11 };
			var S mut b{ .x= 22 };
			var S mut c{ .x= 33 };
			halt if( a != a );
			halt if( a != 11 );
			halt if( 11 != a );
			halt if( a != a_copy );
			halt if( a == b );
			halt if( a == 22 );
			halt if( b == c );
			halt if( 33 == b );
			halt if( 33 != c );
		}

		var S a{ .x=11 };
		var S a_copy{ .x= 11 };
		var S b{ .x= 22 };
		var S c{ .x= 33 };
		static_assert( a == a );
		static_assert( a == 11 );
		static_assert( 11 == a );
		static_assert( a == a_copy );
		static_assert( a != b );
		static_assert( a != 22 );
		static_assert( b != c );
		static_assert( 33 != b );
		static_assert( 33 == c );
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def EqualityOperatorGeneration_Test9():
	c_program_text= """
		// "==" for struct with function pointer inside.
		struct S
		{
			( fn () ) ptr;
		}

		fn Bar(){}
		fn Baz(){}

		fn Foo()
		{
			// Use "mut" to prevent "constexpr"
			var S mut a{ .ptr(Bar) };
			var S mut a_copy{ .ptr(Bar) };
			var S mut b{ .ptr(Baz) };
			halt if( a != a );
			halt if( ! ( a == a ) );
			halt if( a != a_copy );
			halt if( ! ( a_copy == a ) );
			halt if( a == b );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def EqualityOperatorGeneration_Test10():
	c_program_text= """
		// "==" for empty struct.
		struct S{}

		fn Foo()
		{
			var S mut a{};
			var S mut b{};
			halt if( a != a );
			halt if( a != b );
			halt if( !( b == a ) );
		}

		var S a{};
		var S b{};
		static_assert( a == a );
		static_assert( a == b );
		static_assert( !( b != a ) );
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def EqualityOperatorGeneration_Test11():
	c_program_text= """
		// "==" for struct with "void" field.
		struct S{ void v; }

		fn Foo()
		{
			var S mut a;
			var S mut b;
			halt if( a != a );
			halt if( a != b );
			halt if( !( b == a ) );
		}

		var S a{};
		var S b{};
		static_assert( a == a );
		static_assert( a == b );
		static_assert( !( b != a ) );
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def EqualityOperatorIsNotGenerated_Test0():
	c_program_text= """
		struct S
		{
			i32& x;
			// "==" operator is not generated because of reference field.
		}
		fn Foo(S& a, S& b)
		{
			a == b;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 9 ) )


def EqualityOperatorIsNotGenerated_Test1():
	c_program_text= """
		class C
		{
			// "==" operator is not generated for classes by-default.
		}
		fn Foo(C& a, C& b)
		{
			a == b;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 8 ) )


def EqualityOperatorIsNotGenerated_Test2():
	c_program_text= """
		class C
		{
			// "==" operator is not generated for classes by-default.
		}
		struct S
		{
			i32 x;
			C c;
			f32 z;
			// "==" operator is not generated because one of fields is is not equality-comparable.
		}
		fn Foo(S& a, S& b)
		{
			a == b;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 15 ) )


def EqualityOperatorIsNotGenerated_Test3():
	c_program_text= """
		struct S
		{
			i32 x;
			op==(S& l, S& r) : bool = delete; // Disable "==" generation.
		}
		fn Foo(S& a, S& b)
		{
			a == b;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "AccessingDeletedMethod", 9 ) )


def EqualityOperatorIsNotGenerated_Test4():
	c_program_text= """
		class C
		{
			i32 x;
			op==(C& l, C& r) : bool = delete; // Disable "==" generation (unneeded for classes).
		}
		fn Foo(C& a, C& b)
		{
			a == b;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "AccessingDeletedMethod", 9 ) )


def EqualityOperatorIsNotGenerated_Test5():
	c_program_text= """
		struct S
		{
			i32 x;
			op==(S& a, i32 b) : bool; // Not an actual "==" operator.
			// Real "==" is not generated because of another "==".
		}
		static_assert( !typeinfo</S/>.is_equality_comparable );
	"""
	tests_lib.build_program( c_program_text )


def EqualityOperatorIsNotGenerated_Test6():
	c_program_text= """
		struct S
		{
			i32 x;

			template</type T/> op==(S& a, T& b) : bool { return false; } // Not an actual "==" operator.
			// Real "==" is not generated because of another "==".
		}
		static_assert( !typeinfo</S/>.is_equality_comparable );
	"""
	tests_lib.build_program( c_program_text )


def EqualityOperatorIsNotGenerated_Test7():
	c_program_text= """
		class C {}
		struct S
		{
			op==( S& l, S& r ) : bool = default; // Can't generate generator - one of fileds is not equality-comparable.
			C c;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "MethodBodyGenerationFailed", 5 ) )


def EqualityOperatorForCompositeValue_Test0():
	c_program_text= """
		fn Foo()
		{
			// Use "mut" to prevent "constexpr"
			var [ i32, 3 ] mut a[5, 6, 7], mut a_copy[5, 6, 7], mut b[0, 6, 7], mut c[5, 0, 7], mut d[5, 6, 0];
			halt if( a != a );
			halt if( !( a == a ) );
			halt if( a != a_copy );
			halt if( !( a_copy == a ) );
			halt if( a == b );
			halt if( c == a );
			halt if( a == d );
			halt if( ! ( c != d ) );
		}

		var [ i32, 3 ] a[5, 6, 7], a_copy[5, 6, 7], b[0, 6, 7], c[5, 0, 7], d[5, 6, 0];
		static_assert( a == a );
		static_assert( !( a != a ) );
		static_assert( a == a_copy );
		static_assert( !( a_copy != a ) );
		static_assert( a != b );
		static_assert( c != a );
		static_assert( a != d );
		static_assert( ! ( c == d ) );

		// Now we can compare string literals!
		static_assert( "qhy66  !" == "qhy66  !" );
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def EqualityOperatorForCompositeValue_Test1():
	c_program_text= """
		fn Foo()
		{
			// Use "mut" to prevent "constexpr"
			var tup[ f32, bool, i32 ] mut a[ 1.5f, true, 66 ], mut a_copy[ 1.5f, true, 66 ], mut b [ 0.0f, true, 66 ], mut c[ 1.5f, false, 66 ], mut d[ 1.5f, true, 0 ];
			halt if( a != a );
			halt if( !( a == a ) );
			halt if( a != a_copy );
			halt if( !( a_copy == a ) );
			halt if( a == b );
			halt if( c == a );
			halt if( a == d );
			halt if( ! ( c != d ) );
		}

		var tup[ f32, bool, i32 ] a[ 1.5f, true, 66 ], a_copy[ 1.5f, true, 66 ], b [ 0.0f, true, 66 ], c[ 1.5f, false, 66 ], d[ 1.5f, true, 0 ];
		static_assert( a == a );
		static_assert( !( a != a ) );
		static_assert( a == a_copy );
		static_assert( !( a_copy != a ) );
		static_assert( a != b );
		static_assert( c != a );
		static_assert( a != d );
		static_assert( ! ( c == d ) );
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def EqualityOperatorForCompositeValue_Test2():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constexpr conversion_constructor(i32 in_x) ( x= in_x ) {}
		}

		fn Foo()
		{
			// Use "mut" to prevent "constexpr"
			var tup[ S, [ S, 2 ] ] mut a[ 7, [ 8, 9 ] ], mut a_copy[ 7, [ 8, 9 ] ], mut b[ 0, [ 8, 9 ] ], mut c[ 7, [ 0, 9 ] ], mut d[ 7, [ 8, 0 ] ];
			halt if( a != a );
			halt if( !( a == a ) );
			halt if( a != a_copy );
			halt if( !( a_copy == a ) );
			halt if( a == b );
			halt if( c == a );
			halt if( a == d );
			halt if( ! ( c != d ) );
		}

		fn constexpr Q(i32 x) : S { return x; }

		var tup[ S, [ S, 2 ] ] a[ Q(7), [ Q(8), Q(9) ] ], a_copy[ Q(7), [ Q(8), Q(9) ] ], b[ Q(0), [ Q(8), Q(9) ] ], c[ Q(7), [ Q(0), Q(9) ] ], d[ Q(7), [ Q(8), Q(0) ] ];
		static_assert( a == a );
		static_assert( !( a != a ) );
		static_assert( a == a_copy );
		static_assert( !( a_copy != a ) );
		static_assert( a != b );
		static_assert( c != a );
		static_assert( a != d );
		static_assert( ! ( c == d ) );
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def EqualityOperatorForCompositeValue_Test3():
	c_program_text= """
		fn Foo()
		{
			// Use "mut" to prevent "constexpr"
			var [ f32, 2 ] mut a[ 1.0f, 2.0f ], mut a_copy[ 1.0f, 2.0f ], mut b[ 0.0f, 2.0f ], mut c[ 1.0f, 0.0f ], mut plus_zero[ +0.0f, +0.0f ], mut minus_zero[ -0.0f, 0.0f ], mut nan [ 0.0f / 0.0f, 0.0f / 0.0f ];
			halt if( a != a );
			halt if( !( a == a ) );
			halt if( a != a_copy );
			halt if( !( a_copy == a ) );
			halt if( a == b );
			halt if( c == a );
			halt if( plus_zero != minus_zero );
			halt if( !( minus_zero == plus_zero ) );
			halt if( nan == a );
			halt if( plus_zero == nan );
			halt if( nan == nan );
			halt if( !( nan != nan ) );
		}

		var [ f32, 2 ] a[ 1.0f, 2.0f ], a_copy[ 1.0f, 2.0f ], b[ 0.0f, 2.0f ], c[ 1.0f, 0.0f ], plus_zero[ +0.0f, +0.0f ], minus_zero[ -0.0f, 0.0f ], nan [ 0.0f / 0.0f, 0.0f / 0.0f ];
		static_assert( a == a );
		static_assert( !( a != a ) );
		static_assert( a == a_copy );
		static_assert( !( a_copy != a ) );
		static_assert( a != b );
		static_assert( c != a );
		static_assert( plus_zero == minus_zero );
		static_assert( !( minus_zero != plus_zero ) );
		static_assert( nan != a );
		static_assert( plus_zero != nan );
		static_assert( nan != nan );
		static_assert( !( nan == nan ) );
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def EqualityOperatorForCompositeValue_Test4():
	c_program_text= """
		fn Foo()
		{
			// Use "mut" to prevent "constexpr"
			var [ i32, 0 ] mut empty_arr[], mut empty_arr_copy[];
			halt if( empty_arr != empty_arr );
			halt if( empty_arr != empty_arr_copy );
			halt if( !( empty_arr_copy == empty_arr ) );

			var tup[] mut empty_tup[], mut empty_tup_copy[];
			halt if( empty_tup != empty_tup );
			halt if( empty_tup != empty_tup_copy );
			halt if( !( empty_tup_copy == empty_tup ) );
		}

		var [ i32, 0 ] empty_arr[], empty_arr_copy[];
		static_assert( empty_arr == empty_arr );
		static_assert( empty_arr == empty_arr_copy );
		static_assert( !( empty_arr_copy != empty_arr ) );

		var tup[] empty_tup[], empty_tup_copy[];
		static_assert( empty_tup == empty_tup );
		static_assert( empty_tup == empty_tup_copy );
		static_assert( !( empty_tup_copy != empty_tup ) );
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
