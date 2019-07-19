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
