from py_tests_common import *


def ExpectedReferenceValue_ForMove_Test0():
	c_program_text= """
		fn Foo()
		{
			auto imut x= 0;
			move(x); // Expected mutable variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedReferenceValue" )
	assert( errors_list[0].src_loc.line == 5 )


def ExpectedReferenceValue_ForMove_Test1():
	c_program_text= """
		fn Foo( i32 imut x )
		{
			move(x); // Expected mutable variable, got immutable argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedReferenceValue" )
	assert( errors_list[0].src_loc.line == 4 )


def ExpectedReferenceValue_ForMove_Test2():
	c_program_text= """
		auto constexpr pi= 3.141592535;
		fn Foo()
		{
			move(pi); // Expected mutable variable, got global constant.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedVariable" )
	assert( errors_list[0].src_loc.line == 5 )


def ExpectedVariable_ForMove_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			auto &mut r= x;
			move(r); // Expected variable, got reference
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedVariable" )
	assert( errors_list[0].src_loc.line == 6 )


def ExpectedVariable_ForMove_Test1():
	c_program_text= """
		struct S
		{
			i32 x;
			fn Foo( mut this )
			{
				move(x); // Error, moving field
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedVariable" )
	assert( errors_list[0].src_loc.line == 7 )


def ExpectedVariable_ForMove_Test2():
	c_program_text= """
		fn Foo( i32 &mut x )
		{
			move(x);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedVariable" )
	assert( errors_list[0].src_loc.line == 4 )


def AccessingMovedVariable_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			move(x);
			++x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingMovedVariable" )
	assert( errors_list[0].src_loc.line == 6 )


def AccessingMovedVariable_Test1():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			move(x);
			move(x);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingMovedVariable" )
	assert( errors_list[0].src_loc.line == 6 )


def AccessingMovedVariable_Test2():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			auto y= move(x) + x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingMovedVariable" )
	assert( errors_list[0].src_loc.line == 5 )


def AccessingMovedVariable_Test3():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			move(x);
			x= 42; // Currently, event can not assign value to moved variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingMovedVariable" )
	assert( errors_list[0].src_loc.line == 6 )


def AccessingMovedVariable_Test4():
	c_program_text= """
		fn Foo( i32 mut x )
		{
			move(x);
			--x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingMovedVariable" )
	assert( errors_list[0].src_loc.line == 5 )


def AccessingMovedVariable_Test5():
	c_program_text= """
		fn Foo()
		{
			auto mut b= true;
			auto res= move(b) && b;  // In lazy ligical operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingMovedVariable" )
	assert( errors_list[0].src_loc.line == 5 )


def AccessingMovedVariable_Test6():
	c_program_text= """
		fn Foo()
		{
			auto mut b= true;
			if( move(b) ) { b= false; }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingMovedVariable" )
	assert( errors_list[0].src_loc.line == 5 )


def AccessingMovedVariable_InTupleFor_Test0():
	c_program_text= """
		fn Foo()
		{
			var bool mut b= false;
			var tup[ i32, f32 ] t= zero_init;
			for( el : t )
			{
				move(b); // On second iteration thi variable is already moved.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingMovedVariable" )
	assert( errors_list[0].src_loc.line == 8 )


def AccessingMovedVariable_InTupleFor_Test1():
	c_program_text= """
		fn Foo()
		{
			var bool mut b= false;
			var tup[ i32 ] t= zero_init;
			for( el : t )
			{
				move(b); // Ok, move 1 time, because loop have 1 iteration.
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingMovedVariable_InTupleFor_Test2():
	c_program_text= """
		fn Foo()
		{
			var bool mut b= false;
			var tup[ ] t= zero_init;
			for( el : t ) // Loop have zero iterations.
			{
				move(b);
			}
			b= true; // ok, 'b' is not moved.
		}
	"""
	tests_lib.build_program( c_program_text )


def OuterVariableMoveInsideLoop_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			while( false ){ move(x); }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OuterVariableMoveInsideLoop" )
	assert( errors_list[0].src_loc.line == 5 )


def OuterVariableMoveInsideLoop_Test1():
	c_program_text= """
		fn Foo( i32 mut x )
		{
			while( false ){ move(x); }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OuterVariableMoveInsideLoop" )
	assert( errors_list[0].src_loc.line == 4 )


def OuterVariableMoveInsideLoop_Test2():
	c_program_text= """
		fn Foo( i32 mut x )
		{
			while( false )
			{
				if( false ) { move(x); }
				else { move(x); }
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OuterVariableMoveInsideLoop" )
	assert( errors_list[0].src_loc.line == 8 )


def ConditionalMove_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			if( false ) { move(x); }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConditionalMove" )
	assert( errors_list[0].src_loc.line == 5 )


def ConditionalMove_Test1():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			if( false ) { move(x); }
			else if( false ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConditionalMove" )
	assert( errors_list[0].src_loc.line == 6 )


def ConditionalMove_Test2():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			if( false ) { move(x); }
			else if( false ) {}
			else { move(x); }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConditionalMove" )
	assert( errors_list[0].src_loc.line == 7 )


def ConditionalMove_Test3():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			if( false ) { }   // Not moved in first branch
			else { move(x); }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConditionalMove" )
	assert( errors_list[0].src_loc.line == 6 )


def ConditionalMove_InTupleFor_Test0():
	c_program_text= """
		fn Cond() : bool;
		fn Foo()
		{
			var bool mut b= false;
			var tup[ i32, f32 ] t= zero_init;
			for( el : t )
			{
				if( Cond() )
				{
					move(b);
					break;
				}
			} // Error, 'b' moved not in all branches.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConditionalMove" )
	assert( errors_list[0].src_loc.line == 14 )


def ConditionalMove_ForLazyLogicalOperators_Test0():
	c_program_text= """
		fn Foo() : bool
		{
			auto mut b= false;
			return true && move(b); // Second part of lazy logical operator is conditional.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConditionalMove" )
	assert( errors_list[0].src_loc.line == 5 )


def ConditionalMove_ForLazyLogicalOperators_Test1():
	c_program_text= """
		fn Foo() : bool
		{
			auto mut b= false;
			return false || move(b); // Second part of lazy logical operator is conditional.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConditionalMove" )
	assert( errors_list[0].src_loc.line == 5 )


def ConditionalMove_ForLazyLogicalOperators_Test2():
	c_program_text= """
		fn Foo() : bool
		{
			auto mut b= false;
			return false || ( move(b) || true ); // Move inside right part of top-level lazy logical operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConditionalMove" )
	assert( errors_list[0].src_loc.line == 5 )


def MovedVariableHasReferences_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 3.5;
			auto &ref= x;
			move(x);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MovedVariableHasReferences" )
	assert( errors_list[0].src_loc.line == 6 )


def MovedVariableHasReferences_Test1():
	c_program_text= """
		fn Foo( i16 mut short )
		{
			auto &mut ref= short;
			move(short);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MovedVariableHasReferences" )
	assert( errors_list[0].src_loc.line == 5 )


def MovedVariableHasReferences_Test2():
	c_program_text= """
		struct S
		{
			i32& r;
		}
		fn Foo()
		{
			var i32 mut x= 45;
			var S s{ .r= x };
			move(x);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MovedVariableHasReferences" )
	assert( errors_list[0].src_loc.line == 10 )


def MovedVariableHasReferences_Test3():
	c_program_text= """
		struct S
		{
			i32& r;
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			fn constructor( this, i32& x ) @(pollution)
			( r= x ) {}
		}
		fn Bar( S s, i32 x ){}
		fn Foo()
		{
			var i32 mut x= 45;
			Bar( S(x), move(x) );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "MovedVariableHasReferences", 13 ) )


def MoveConstexprIsNotPreserved_Test0():
	c_program_text= """
		fn constexpr Foo() : i32
		{
			var i32 mut x= 42;
			x= 34;
			return move(x); // Should return "34" here.
		}
		static_assert( Foo() == 34 );
	"""
	tests_lib.build_program( c_program_text )


def MoveConstexprIsNotPreserved_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 42;
			x= 34;
			static_assert( move(x) == 42 );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "StaticAssertExpressionIsNotConstant", 6 ) )


def ReturnAutoMoveIsDisabled_Test0():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		struct T
		{
			S s;
			fn Foo( mut this ) : S
			{
				return s; // Auto-move for "return" isn't possible - returning struct field.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 12 ) )


def ReturnAutoMoveIsDisabled_Test1():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		fn Foo() : S
		{
			var S mut s= zero_init;
			auto& s_ref= s;
			return s; // Auto-move for "return" isn't possible - returned variable has a reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 11 ) )


def ReturnAutoMoveIsDisabled_Test2():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		fn Foo() : S
		{
			var S mut s= zero_init;
			auto &mut s_ref= s;
			return s; // Auto-move for "return" isn't possible - returned variable has a mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 11 ) )


def ReturnAutoMoveIsDisabled_Test3():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		fn Foo() : S
		{
			var S mut s= zero_init;
			move(s);
			return s; // Auto-move for "return" isn't possible - returned variable is already moved.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingMovedVariable", 11 ) )


def ReturnAutoMoveIsDisabled_Test4():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		fn Foo() : S
		{
			var S mut s= zero_init;
			auto &imut s_ref= s;
			return s_ref; // Auto-move for "return" isn't possible - returning a reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 11 ) )


def ReturnAutoMoveIsDisabled_Test5():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		fn Foo() : S
		{
			var S mut s= zero_init;
			auto &mut s_ref= s;
			return s_ref; // Auto-move for "return" isn't possible - returning a mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 11 ) )


def ReturnAutoMoveIsDisabled_Test6():
	c_program_text= """
		struct S
		{
			i32 x;
		}
		var S mut global_s{ .x= 42 };
		fn GetS() : S
		{
			unsafe{  return global_s;  } // Auto-move for "return" isn't possible - given variable is global. Copy is taken instead.
		}
		fn Foo()
		{
			auto s= GetS();
			unsafe{  global_s.x= 0;  }
			halt if( s.x != 42 ); // Local copy should not be changed.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ReturnAutoMoveIsDisabled_Test7():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		fn Foo() : S
		{
			var S mut s= zero_init;
			unsafe{  return cast_mut(s);  } // Auto-move for "return" isn't possible - returning "cast_mut" result instead of bare variable name.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 10 ) )


def ReturnAutoMoveIsDisabled_Test8():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		fn Foo() : S
		{
			var S mut s= zero_init;
			return cast_imut(s); // Auto-move for "return" isn't possible - returning "cast_imut" result instead of bare variable name.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 10 ) )


def ReturnAutoMoveIsDisabled_Test9():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		fn Foo() : S
		{
			var S mut s= zero_init;
			return safe(s); // Auto-move for "return" isn't possible - returning "safe" expression result instead of bare variable name.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 10 ) )


def ReturnAutoMoveIsDisabled_Test10():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		fn Foo() : S
		{
			var S mut s= zero_init;
			return unsafe(s); // Auto-move for "return" isn't possible - returning "unsafe" expression result instead of bare variable name.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 10 ) )


def ReturnAutoMoveIsDisabled_Test11():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		fn Foo() : S
		{
			var S mut s= zero_init;
			return PassS(s); // Auto-move for "return" isn't possible - returning not a variable itself, but using it in a function call.
		}
		fn PassS(S mut s) : S;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 10 ) )


def ReturnAutoMoveIsDisabled_Test12():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		fn Foo() : S
		{
			var S s= zero_init;
			auto& s_ref= s;
			return s; // Auto-move for "return" isn't possible - returned variable still has local reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 11 ) )


def ReturnAutoMoveIsDisabled_Test13():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		fn Foo(S& s) : S
		{
			return s; // Auto-move for "return" isn't possible - given name is reference arg.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 9 ) )


def ReturnAutoMoveIsDisabled_Test14():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		fn Foo(S &mut s) : S
		{
			return s; // Auto-move for "return" isn't possible - given name is mutable reference arg.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 9 ) )


def ReturnAutoMoveIsDisabled_Test15():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
			fn Foo(this) : S
			{
				return this; // Auto-move for "return" isn't possible - given name is reference arg "this".
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 8 ) )


def ReturnAutoMoveIsDisabled_Test16():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
			fn Foo(mut this) : S
			{
				return this; // Auto-move for "return" isn't possible - given name is mutable reference arg "this".
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 8 ) )


def ReturnAutoMoveIsDisabled_Test17():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			i32 x;
		}
		struct T
		{
			fn conversion_constructor( S mut in_s ) ( s(move(in_s)) ) {}
			S s;
		}
		fn MakeT( S s ) : T
		{
			return T(s); // Auto-move in "return" doesn't work here - conversion constructor is executed explicitly.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 14 ) )
