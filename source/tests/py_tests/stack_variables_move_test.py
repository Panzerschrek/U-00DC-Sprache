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
			{
				var S s_copy= move(s); // 's' contains mutable reference. After move 's' contains no references, but 's_copy' contains reference.
				s_copy.r= 999;
			}
			return x; // Here we have no references to 'x'.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 999 )


def MoveOperatorTest2():
	c_program_text= """
		struct S
		{
			i32 &mut r;
			fn constructor( this'a', i32 &'b mut in_r ) ' a <- mut b '
			( r= in_r ){}
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var S mut s( x ); // 's' contains mutable reference to 'x' now.
			move(s); // After move 's' lost contained references.
			++x; // Ok, can change, because have no references.
		}
	"""
	tests_lib.build_program( c_program_text )


def MoveOperatorTest3():
	c_program_text= """
		struct S
		{
			i32 &mut r;
			fn constructor( this'a', i32 &'b mut in_r ) ' a <- mut b '
			( r= in_r ){}
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var S mut s0( x ); // 's' contains mutable reference to 'x' now.
			auto s1= move(s0); // After move 's' transfers contained references to 's1'.
			++x; // Error, accessing variable, that have mutalbe reference inside 's1'.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingVariableThatHaveMutableReference" )
	assert( errors_list[0].file_pos.line == 14 )


def MoveInsideIf_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			if( false ) { move(x); }
			else { move(x); }   // Ok, move variable in all if-else branches.
		}
	"""
	tests_lib.build_program( c_program_text )


def MoveInsideIf_Test1():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			if( false ) { move(x); }
			else if( false ) { move(x); }
			else if( true ) { move(x); }
			else { move(x); }   // Ok, move variable in all if-else branches.
		}
	"""
	tests_lib.build_program( c_program_text )


def MoveInsideIf_Test2():
	c_program_text= """
		fn Foo()
		{
			auto mut b= false;
			if(  move(b) ) { }
		}
	"""
	tests_lib.build_program( c_program_text )


def MoveInsideIf_Test3():
	c_program_text= """
		fn Foo()
		{
			auto mut b= false;
			if( false ) { move(b); }
			else if( move(b) ){}
			else {}  //  Ok, 'b" moved in brach and later condition. Here is no conditional move.
		}
	"""
	tests_lib.build_program( c_program_text )


def MoveBeforeIf_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			move(x);
			if( false ){}
			else {}
		}
	"""
	tests_lib.build_program( c_program_text )


def MoveBeforeLoop_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			move(x);
			while(false){}
		}
	"""
	tests_lib.build_program( c_program_text )


def Move_InLazyLogicalOperator_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut b= false;
			move(b) || true;  // Ok, move in first part, which is unconditional.
		}
	"""
	tests_lib.build_program( c_program_text )


def Move_InLazyLogicalOperator_Test1():
	c_program_text= """
		fn Foo()
		{
			auto mut b= true;
			move(b) && false;  // Ok, move in first part, which is unconditional.
		}
	"""
	tests_lib.build_program( c_program_text )
