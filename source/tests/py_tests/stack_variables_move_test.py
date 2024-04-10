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
			auto y= -( move(x) + 42 );
		}
	"""
	tests_lib.build_program( c_program_text )


def MoveOperatorTest0():
	c_program_text= """
		struct S
		{
			$(i32) r;
			fn constructor( this, i32 & mut in_r )
			( r= $<(in_r) ){}
			fn destructor()
			{
				unsafe{  $>(r)= 666;  }
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
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			fn constructor( this, i32 & mut in_r ) @(pollution)
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
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			fn constructor( this, i32 & mut in_r ) @(pollution)
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
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			fn constructor( this, i32 & mut in_r ) @(pollution)
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
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 15 )


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


def MoveInsideIf_Test4():
	c_program_text= """
		fn Foo( i32 x )
		{
			auto mut b= false;
			if( x == 0 )
			{
				move(b); // Move allowed for branches of if-else with terminal instruction inside.
				return;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def MoveInsideIf_Test5():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			while( x < 100 )
			{
				auto mut b= false;
				if( x == 0 )
				{}
				else if( x == 20 )
				{
					move(b); // Move allowed for branches of if-else with terminal instruction inside.
					break;
				}
				++x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def MoveInsideLoop_Test0():
	c_program_text= """
		fn Cond() : bool;
		fn Foo()
		{
			auto mut b= false;
			while(Cond())
			{
				if( Cond() )
				{
					move(b); // Ok, move outer loop variable in 'return' branch.
					return;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def MoveInsideLoop_Test1():
	c_program_text= """
		fn Cond() : bool;
		fn Foo()
		{
			auto mut b= false;
			for(;;)
			{
				if( Cond() )
				{
					move(b); // Ok, move outer loop variable in 'return' branch.
					return;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def MoveInsideLoop_Test2():
	c_program_text= """
		fn Cond() : bool;
		fn Foo()
		{
			auto mut b= false;
			var tup[ f32, i32, bool ] t= zero_init;
			for( el : t )
			{
				if( Cond() )
				{
					move(b); // Ok, move in 'return' branch.
					return;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def MoveInsideLoop_Test3():
	c_program_text= """
		fn Cond() : bool;
		fn Foo()
		{
			auto mut b= false;
			var tup[  ] t= zero_init;
			for( el : t )
			{
				move(b); // Ok, move not happens, because loop have 0 iterations.
			}
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
			auto x= move(b) || true;  // Ok, move in first part, which is unconditional.
		}
	"""
	tests_lib.build_program( c_program_text )


def Move_InLazyLogicalOperator_Test1():
	c_program_text= """
		fn Foo()
		{
			auto mut b= true;
			auto r= move(b) && false;  // Ok, move in first part, which is unconditional.
		}
	"""
	tests_lib.build_program( c_program_text )


def ReturnAutoMove_Test0():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		static_assert( !typeinfo</S/>.is_copy_constructible );
		fn GetS() : S
		{
			var S mut s{ .x= 67 };
			return s; // Should automatically move local variable "s" in "return" statement.
		}
		fn Foo()
		{
			auto s= GetS();
			halt if( s.x != 67 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ReturnAutoMove_Test1():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		static_assert( !typeinfo</S/>.is_copy_constructible );
		fn GetS() : S
		{
			var S s{ .x= 987 };
			return s; // Should automatically move local immutable variable "s" in "return" statement.
		}
		fn Foo()
		{
			auto s= GetS();
			halt if( s.x != 987 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ReturnAutoMove_Tes2():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		static_assert( !typeinfo</S/>.is_copy_constructible );
		fn PassS( S mut s ) : S
		{
			return s; // Should automatically move argument "s" in "return" statement.
		}
		fn Foo()
		{
			var S mut s_initial{ .x= 765 };
			auto s= PassS( move(s_initial) );
			halt if( s.x != 765 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ReturnAutoMove_Tes3():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		static_assert( !typeinfo</S/>.is_copy_constructible );
		fn PassS( S s ) : S
		{
			return s; // Should automatically move immutable argument "s" in "return" statement.
		}
		fn Foo()
		{
			var S mut s_initial{ .x= 1287 };
			auto s= PassS( move(s_initial) );
			halt if( s.x != 1287 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ReturnAutoMove_Tes4():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
			fn PassThis( byval this ) : S
			{
				return this; // Move value argument "this".
			}
		}
		static_assert( !typeinfo</S/>.is_copy_constructible );
		fn Foo()
		{
			var S mut s{ .x= 1241 };
			auto s_moved= move(s).PassThis();
			halt if( s_moved.x != 1241 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
