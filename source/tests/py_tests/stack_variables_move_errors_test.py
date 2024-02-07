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
