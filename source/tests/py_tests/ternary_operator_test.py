from py_tests_common import *


def TernaryOperatorParsing_Test0():
	c_program_text= """
		fn Foo()
		{
			select( true ? 0 : 1 );
		}
	"""
	tests_lib.build_program( c_program_text )


def TernaryOperatorParsing_Test1():
	c_program_text= """
		fn Foo( i32 x )
		{
			2 + select( x > 0 ? x : -x ) * 2;
		}
	"""
	tests_lib.build_program( c_program_text )


def TernaryOperator_Test0():
	c_program_text= """
		fn Foo( bool b, i32 x, i32 y ) : i32
		{
			return select( b ? x : y ); // Both branches result is const reference.
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foobii", True , 55, 11 ) == 55 )
	assert( tests_lib.run_function( "_Z3Foobii", False, 55, 11 ) == 11 )


def TernaryOperator_Test1():
	c_program_text= """
		fn Foo( bool b, i32 x, i32 y ) : i32
		{
			return select( b ? x * 5 : y * 7 ); // Both branches result is value.
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foobii", True , 1, 1 ) == 5 )
	assert( tests_lib.run_function( "_Z3Foobii", False, 1, 1 ) == 7 )


def TernaryOperator_Test2():
	c_program_text= """
		struct S{ i32 x; }
		fn GetS( i32 x ) : S { var S s{ .x= x }; return s; }
		fn Foo( bool b ) : i32
		{
			return select( b ? GetS(412) : GetS(632) ).x; // "select" for value-structs.
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foob", True  ) == 412 )
	assert( tests_lib.run_function( "_Z3Foob", False ) == 632 )


def TernaryOperator_ForReferenceValue_Test0():
	c_program_text= """
		fn Foo( bool b ) : i32
		{
			var i32 mut x= 100, mut y= 5;
			select( b ? x : y )*= 3; // Both values are reference, we can modify it.
			return x / y;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foob", True  ) == 60 )
	assert( tests_lib.run_function( "_Z3Foob", False ) == 6 )


def TernaryOperator_ForReferenceValue_Test1():
	c_program_text= """
		fn Mul5( i32 &mut x )
		{
			x*= 5;
		}
		fn Foo( bool b ) : i32
		{
			var i32 mut x= 1000, mut y= 5;
			Mul5( select( b ? x : y ) ); // Both values are reference, we can modify it.
			return x / y;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foob", True  ) == 1000 )
	assert( tests_lib.run_function( "_Z3Foob", False ) == 40 )


def TernaryOperatorIsLazy_Test0():
	c_program_text= """
		fn Mul5( i32 &mut x )
		{
			x*= 5;
		}
		fn Foo( bool b ) : i32
		{
			var i32 mut x= 666, mut y= 33;
			select( b ? Mul5(x) : Mul5(y) ); // Only one of expressions evaluated.
			return x / y;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foob", True  ) == 100 )
	assert( tests_lib.run_function( "_Z3Foob", False ) == 4 )


def TernaryOperatorIsLazy_Test1():
	c_program_text= """
		fn Foo( bool b ) : i32
		{
			var [ i32, 1 ] arr[ 666 ];
			auto mut invalid_index= 99999u;
			return select( b ? arr[0u] : arr[invalid_index] ); // Programm not halted on invalid array index, if argument is "true"
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foob", True ) == 666 )


def DestructorsCall_ForTernaryOperatorBranches_Test0():
	c_program_text= """
		struct S
		{
			i32 x;
			i32 &mut r;
			fn constructor( this'f', i32 in_x, i32 &'g mut in_r ) ' f <- g '
			( x(in_x), r(in_r) )
			{ ++r; }

			fn destructor() { --r; x= 0; }
		}
		fn Foo( bool b ) : i32
		{
			var i32 mut x= 10, mut y= 10;
			auto r= select( b ? i32(S( 5, x ).x) : i32(S( 3, y ).x) ); // Constructors must increase variable value, destructors must decrease it.
			halt if( x != 10 );
			halt if( y != 10 );
			return r;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foob", True  ) == 5 )
	assert( tests_lib.run_function( "_Z3Foob", False ) == 3 )


def DestructorsCall_ForTernaryOperatorResult_Test0():
	c_program_text= """
		struct S
		{
			i32 &mut x;
			fn destructor() { ++x; }
		}
		fn GetS( i32 &'a mut x ) : S'a'
		{
			var S mut s{ .x= x };
			return move(s);
		}
		fn Foo( bool b ) : i32
		{
			var i32 mut x= 0, mut y= 0;
			{
				auto res= select( b ? GetS(x) : GetS(y) ); // Structure, that refers to "x" or to "y"
				// Called destructor, that increments "x" or "y"
			}
			return (y << 4u) | x;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foob", True  ) ==  1 )
	assert( tests_lib.run_function( "_Z3Foob", False ) == 16 )


def TernaryOperator_VariablesStateMerge_Test0():
	c_program_text= """
		fn Foo( bool b )
		{
			auto mut x= 0;
			auto moved= select( b ? move(x) : move(x) ); // Ok, "x" moved in all branches.
		}
	"""
	tests_lib.build_program( c_program_text )


def TernaryOperator_Constexpr_Test0():
	c_program_text= """
		static_assert( select( true ? 5.0f : -14.3f ) == 5.0f );
	"""
	tests_lib.build_program( c_program_text )


def TernaryOperator_Constexpr_Test1():
	c_program_text= """
		fn Foo()
		{
			var u32 constexpr x(985), constexpr y(521);
			static_assert( select( false ? x : y ) == 521u );
		}
	"""
	tests_lib.build_program( c_program_text )


def TernaryOperator_Constexpr_Test2():
	c_program_text= """
		fn Foo()
		{
			var i32 constexpr x(222), mut y(111);
			static_assert( select( true ? x : y ) == 222 ); // result is constexpr, even if not selected branch is not constexpr.
		}
	"""
	tests_lib.build_program( c_program_text )


def TernaryOperator_Constexpr_Test3():
	c_program_text= """
		fn Foo()
		{
			var f64 constexpr pi(3.1415926535), constexpr e(2.718281828);
			var f64 &constexpr ref= select( false ? pi : e );
			static_assert( ref == e );
		}
	"""
	tests_lib.build_program( c_program_text )


def TernaryOperator_Constexpr_Test4():
	c_program_text= """
		static_assert( select( true ? "Ab" : "bA" )[0u] == "A"c8 );
	"""
	tests_lib.build_program( c_program_text )


def TernaryOperator_Constexpr_Test5():
	c_program_text= """
		fn Foo()
		{
			var i32 constexpr x= 0, mut y= 0;
			auto &constexpr ref= select( false ? x : y );  // Selected non-constexpr branch.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VariableInitializerIsNotConstantExpression" )
	assert( errors_list[0].src_loc.line == 5 )


def TernaryOperator_Constexpr_Test6():
	c_program_text= """
		fn Foo( bool b )
		{
			var i32 constexpr x= 0, constexpr y= 1;
			auto &constexpr ref= select( b ? x : y );  // Condition is not constexpr, result will not be constexpr.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VariableInitializerIsNotConstantExpression" )
	assert( errors_list[0].src_loc.line == 5 )


def TernaryOperator_Constexpr_Test7():
	c_program_text= """
		struct S{ i32 x; }
		var S constexpr s0{ .x= 42 }, s1{ .x= 24 };
		static_assert( select( true ? s0 : s1 ).x == 42 ); // Constexpr select for reference structs.
	"""
	tests_lib.build_program( c_program_text )
