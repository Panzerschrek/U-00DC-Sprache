from py_tests_common import *

def ByValThis_Declaration_Test0():
	c_program_text= """
		struct S
		{
			fn Foo( byval this ){}
		}
	"""
	tests_lib.build_program( c_program_text )


def ByValThis_Declaration_Test1():
	c_program_text= """
		struct S
		{
			fn Foo( byval imut this ){}
		}
	"""
	tests_lib.build_program( c_program_text )


def ByValThis_Declaration_Test2():
	c_program_text= """
		struct S
		{
			fn Foo( byval mut this ){}
		}
	"""
	tests_lib.build_program( c_program_text )


def ByValThis_Test0():
	c_program_text= """
		struct S
		{
			fn Foo( byval this ) : i32 { return x; }
			i32 x;
		}
		fn Foo() : i32
		{
			var S s{ .x= 765 };
			return s.Foo();
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 765 )


def ByValThis_Test1():
	c_program_text= """
		struct S
		{
			fn Foo( byval mut this ) { x= 0; }
			i32 x;
		}
		fn Foo() : i32
		{
			var S mut s{ .x= 123 };
			s.Foo(); // Modification doesn't affect source, since "this" is passed by value.
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 123 )


def ByValThis_Test2():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			fn Foo( byval this ) : i32 { return x; }
			i32 x;
		}
		static_assert( !typeinfo</S/>.is_copy_constructible );
		fn Foo() : i32
		{
			var S mut s{ .x= 88776655 };
			return move(s).Foo(); // Call by-value method with immediate value.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 88776655 )


def ByValThis_Test3():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			fn Foo( byval this ) : i32 { return x; }
			i32 x;
		}
		static_assert( !typeinfo</S/>.is_copy_constructible );
		fn Foo() : i32
		{
			var S s{ .x= 88776655 };
			return s.Foo(); // Error - trying to copy-construct value in byval-this method call.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "CopyConstructValueOfNoncopyableType", 12 ) )


def ByValThis_Test4():
	c_program_text= """
		struct Vec
		{
			f32 x; f32 y;
			fn constructor( f32 in_x, f32 in_y ) ( x(in_x), y(in_y) ) {}
			fn constructor( mut this, Vec& other )= delete;
			fn scale( byval mut this, f32 s ) : Vec { x *= s; y *= s; return move(this); }
			fn reverse( byval mut this ) : Vec { x= -x; y= -y; return move(this); }
			fn swap_components( byval mut this ) : Vec { auto tmp= x; x= y; y=tmp; return move(this); }
		}
		static_assert( !typeinfo</Vec/>.is_copy_constructible );
		fn Foo()
		{
			// Use chain of byval thiscall methods, starting with temp variable construction.
			auto v= Vec( 3.0f, 5.0f ).scale( 2.0f ).reverse().swap_components();
			halt if( v.x != -10.0f );
			halt if( v.y !=  -6.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ByValThis_Test5():
	c_program_text= """
		struct S
		{
			i32 x;
			fn Foo( byval this ) : i32
			{
				return Bar(); // Call reference-thiscall method using value this.
			}
			fn Bar( this ) : i32 { return x * 11; }
		}
		fn Foo() : i32
		{
			var S s{ .x= 7 };
			return s.Foo();
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 77 )


def ByValThis_Test6():
	c_program_text= """
		struct S
		{
			i32 x;
			fn Foo( byval mut this ) : i32
			{
				Reverse(); // Call mutable reference-thiscall method using value this.
				return x;
			}
			fn Reverse( mut this ) { x= -x; }
		}
		fn Foo() : i32
		{
			var S s{ .x= -765 };
			return s.Foo();
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 765 )


def ByValThis_Test7():
	c_program_text= """
		struct S
		{
			i32 x;
			fn Foo( byval mut this ) : i32
			{
				Reserve( this ); // Call mutable reference function using value this.
				return x;
			}
		}
		fn Reserve( S &mut s ) { s.x= -s.x; }
		fn Foo() : i32
		{
			var S s{ .x= -88877 };
			return s.Foo();
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 88877 )


def ByValThis_Test8():
	c_program_text= """
		class Base polymorph
		{
			fn Foo( byval this );
			fn constructor( mut this, Base& other )= default;
		}
		class Derived : Base {}
		fn Foo()
		{
			// This is equivalent to someting like Base::Foo( Derived() );
			// Move here inpossible since types are different, so, copy constructor is called instead,
			Derived().Foo();
		}
	"""
	tests_lib.build_program( c_program_text )


def ByValThis_Test9():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x(in_x) ) {}
			op+( byval this, S other ) : S // byval this overloaded binary operator.
			{
				return S( x + other.x );
			}
		}
		fn Foo() : i32
		{
			return ( S( 17 ) + S( 55 ) ).x;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 17 + 55 )


def ByValThis_Test10():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x(in_x) ) {}
			op()( byval this ) : i32 // byval this overloaded call operator.
			{
				return x * 5;
			}
		}
		fn Foo() : i32
		{
			return S( 6 )();
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 30 )


def ByValThis_Test11():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x(in_x) ) {}
			// Overloading with byval this.
			fn Foo( mut this ) : i32 { return x * 5; }
			fn Foo( byval this ) : i32 { return x * 3; }
		}
		fn Foo()
		{
			var S mut s_mut( 10 ), imut s_imut( 20 );
			halt if( s_mut.Foo() != 50 );
			halt if( s_imut.Foo() != 60 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ByValThis_Test12():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x(in_x) ) {}
			template</ type T /> fn CastScale( this, T scale ) : T { return T(x) * scale; }
		}
		fn Foo() : u32
		{
			var S s(7);
			return s.CastScale( 11u );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 77 )


def ByValThis_Test13():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constexpr Foo( byval this ) : i32 { return x * 7; }
		}
		var S constexpr s{ .x= 15 };
		static_assert( s.Foo() == 15 * 7 ); // "byval" "this" method may be constexpr.
	"""
	tests_lib.build_program( c_program_text )


def ByValThis_Test14():
	c_program_text= """
		struct S
		{
			i32 x;
			fn Foo( byval this ) : i32 { return x * 5; }
		}
		fn Foo() : i32
		{
			var (fn( S s ) : i32 ) ptr= S::Foo; // Assign "byval" "this" method to pointer.
			var S s{ .x= 9 };
			return ptr(s); // Call method via pointer.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 45 )


def ByValThisErrors_Test0():
	c_program_text= """
		struct S
		{
			i32 x;
			fn Foo( byval this )
			{
				x= 42; // error - 'this' is not mutable.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ExpectedReferenceValue", 7 ) )


def ByValThisErrors_Test1():
	c_program_text= """
		struct S
		{
			i32 x;
			fn Foo( byval this )
			{
				move( this ); // error - 'this' is not mutable.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ExpectedReferenceValue", 7 ) )


def ByValThisErrors_Test2():
	c_program_text= """
		struct S
		{
			i32 x;
			fn Foo( byval this )
			{
				Bar(); // Error - 'this' is not mutable, but mutable method is called for it.
			}
			fn Bar( mut this );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "CouldNotSelectOverloadedFunction", 7 ) )


def ByValThisErrors_Test3():
	c_program_text= """
		struct S
		{
			i32 x;
			fn Foo( byval this )
			{
				Bar(this); // Error - 'this' is not mutable, but mutable function is called for it.
			}
		}
		fn Bar( S &mut s );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "CouldNotSelectOverloadedFunction", 7 ) )


def ByValThisErrors_Test4():
	c_program_text= """
		class Base polymorph
		{
			fn Foo( byval this );
		}
		class Derived : Base {}
		fn Foo()
		{
			// This is equivalent to someting like Base::Foo( Derived() );
			// But such call is impossible - move is disabled, since types are different and copying is impossible since 'Base' is non-copyable.
			Derived().Foo();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "CopyConstructValueOfNoncopyableType", 11 ) )


def ByValThisErrors_Test5():
	c_program_text= """
		struct S
		{
			// For now type completeness required for all value args of generators.
			// Because of that byval this generator methods are not possible.
			fn generator Gen( byval this );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "GlobalsLoopDetected", 2 ) )


def ByValThisErrors_Test6():
	c_program_text= """
		fn Foo( byval this ); // byval this outside class.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisInNonclassFunction", 2 ) )


def ByValThisErrors_Test7():
	c_program_text= """
		struct S
		{
			// Can't overload function, since mutability modifier of value parametr doesn't affect function type.
			fn Foo( byval mut this );
			fn Foo( byval imut this );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "FunctionPrototypeDuplication", 5 ) or HaveError( errors_list, "FunctionPrototypeDuplication", 6 ) )


def ByValThisErrors_Test8():
	c_program_text= """
		struct S
		{
			fn Foo( byval mut this )
			{
				auto& this_ref= this;
				move(this);
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "MovedVariableHaveReferences", 7 ) )


def ByValThisErrors_Test9():
	c_program_text= """
		struct S
		{
			fn Foo( byval mut this, bool cond )
			{
				if( cond ) { move(this); }
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ConditionalMove", 6 ) )


def ByValThisErrors_Test10():
	c_program_text= """
		struct S
		{
			fn Foo( mut this )
			{
				move(this); // Try to move mutable reference "this".
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ExpectedVariable", 6 ) )


def ByValThisErrors_Test11():
	c_program_text= """
		struct S
		{
			fn Foo( imut this )
			{
				move(this); // Try to move immutable reference "this".
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ExpectedReferenceValue", 6 ) )


def ByvalThisForConstructorOrDestructor_Test0():
	c_program_text= """
		struct S
		{
			fn constructor( byval this );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ByvalThisForConstructorOrDestructor", 4 ) )


def ByvalThisForConstructorOrDestructor_Test1():
	c_program_text= """
		struct S
		{
			fn constructor( byval mut this );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ByvalThisForConstructorOrDestructor", 4 ) )


def ByvalThisForConstructorOrDestructor_Test2():
	c_program_text= """
		struct S
		{
			fn destructor( byval this );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ByvalThisForConstructorOrDestructor", 4 ) )


def VirtualForByvalThisFunction_Test0():
	c_program_text= """
		class C polymorph
		{
			fn virtual Foo( byval imut this );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "VirtualForByvalThisFunction", 4 ) )


def VirtualForByvalThisFunction_Test1():
	c_program_text= """
		class C interface
		{
			fn virtual pure Foo( byval mut this );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "VirtualForByvalThisFunction", 4 ) )


def VirtualForByvalThisFunction_Test2():
	c_program_text= """
		class C abstract
		{
			fn virtual pure Foo( byval this, i32 y );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "VirtualForByvalThisFunction", 4 ) )


def InvalidFirstParamValueTypeForAssignmentLikeOperator_ForByvalThis_Test0():
	c_program_text= """
		struct S
		{
			op=( byval mut this, S& other );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "InvalidFirstParamValueTypeForAssignmentLikeOperator", 4 ) )


def InvalidFirstParamValueTypeForAssignmentLikeOperator_ForByvalThis_Test1():
	c_program_text= """
		struct S
		{
			op*=( byval imut this, S& other );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "InvalidFirstParamValueTypeForAssignmentLikeOperator", 4 ) )


def InvalidFirstParamValueTypeForAssignmentLikeOperator_ForByvalThis_Test2():
	c_program_text= """
		struct S
		{
			op>>=( byval this, i32 shift );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "InvalidFirstParamValueTypeForAssignmentLikeOperator", 4 ) )


def AccessingMovedByvalThis_Test0():
	c_program_text= """
		struct S
		{
			fn Foo( byval mut this )
			{
				move(this);
				move(this); // Accessing moved "this" in move operator.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "AccessingMovedVariable", 7 ) )


def AccessingMovedByvalThis_Test1():
	c_program_text= """
		struct S
		{
			fn Foo( byval mut this )
			{
				move(this);
				auto& this_ref= this; // Accessing moved "this" directly by name.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "AccessingMovedVariable", 7 ) )


def AccessingMovedByvalThis_Test2():
	c_program_text= """
		struct S
		{
			i32 x;
			fn Foo( byval mut this )
			{
				move(this);
				auto x_copy= x; // Access field of moved "this".
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "AccessingMovedVariable", 8 ) )


def AccessingMovedByvalThis_Test3():
	c_program_text= """
		struct S
		{
			fn Foo( byval mut this )
			{
				move(this);
				Bar(); // Can't call thiscall method, since "this" is already move.
			}
			fn Bar(this);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "CouldNotSelectOverloadedFunction", 7 ) )


def AccessingMovedByvalThis_Test4():
	c_program_text= """
		class C polymorph
		{
			fn Foo( byval mut this )
			{
				move(this);
				Bar(); // Can't call thiscall virtual method, since "this" is already move.
			}
			fn virtual Bar(this);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "CouldNotSelectOverloadedFunction", 7 ) )


def AccessingMovedByvalThis_Test5():
	c_program_text= """
		struct S
		{
			fn Foo( byval mut this )
			{
				move(this);
				Bar(); // Ok - call static method, when "this" is alredy moved.
			}
			fn Bar();
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingMovedByvalThis_Test6():
	c_program_text= """
		class Base polymorph {}
		class Derived : Base
		{
			fn Foo( byval mut this )
			{
				move(this);
				auto& b= base; // Access base pointer for moved "this".
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "AccessingMovedVariable", 8 ) )
