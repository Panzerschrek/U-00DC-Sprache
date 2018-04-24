from py_tests_common import *


def MoveOperatorDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			move(x);
		}
	"""
	tests_lib.build_program( c_program_text )



def MoveOperatorDeclaration_Test1():
	c_program_text= """
		fn Foo( i32 mut x )
		{
			move(x);
		}
	"""
	tests_lib.build_program( c_program_text )


def MoveOperatorDeclaration_Test2():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			-( move(x) + 42 );
		}
	"""
	tests_lib.build_program( c_program_text )


def MoveOperatorTest0():
	c_program_text= """
		struct S
		{
			i32 &mut r;
			fn constructor( this'a', i32 &'b mut in_r ) ' a <- mut b '
			( r= in_r ){}
			fn destructor()
			{
				r= 666;
			}
		}

		fn Foo() : i32
		{
			var i32 mut x= 0;
			var S mut s( x );
			move(s); // 's' should be destroyed here. 'x' now contains 666
			auto x_copy= x;
			return x_copy;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 666 )


def MoveOperatorTest1():
	c_program_text= """
		struct S
		{
			i32 &mut r;
			fn constructor( this'a', i32 &'b mut in_r ) ' a <- mut b '
			( r= in_r ){}
		}

		fn Foo() : i32
		{
			var i32 mut x= 0;
			var S mut s( x );
			var S s_copy= move(s); // 's' contains mutable reference. After move 's' contains no references, but 's_copy' contains reference.
			s_copy.r= 999;
			return x;
		}
	"""
	tests_lib.build_program( c_program_text, True )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 999 )
