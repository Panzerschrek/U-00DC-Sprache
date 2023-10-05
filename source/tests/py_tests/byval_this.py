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
				Reserve(); // Call mutable reference-thiscall method using value this.
				return x;
			}
			fn Reserve( mut this ) { x= -x; }
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
