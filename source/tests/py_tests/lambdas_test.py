from py_tests_common import *


def SimpleLambda_Test0():
	c_program_text= """
		fn Foo()
		{
			// Lambda without params, return value and with empty body.
			auto f= lambda(){};
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SimpleLambda_Test1():
	c_program_text= """
		fn Foo()
		{
			// Lambda with params and empty body.
			auto f= lambda( i32 x, f32& y ){ };
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SimpleLambda_Test2():
	c_program_text= """
		fn Foo()
		{
			// Lambda with return value and non-empty body.
			auto f= lambda() : i32 & { halt; };
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def NonCaptureLambda_Test0():
	c_program_text= """
		fn Foo()
		{
			auto f= lambda() : i32 { return 16; };
			halt if( f() != 16 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def NonCaptureLambda_Test1():
	c_program_text= """
		fn Foo()
		{
			auto f= lambda( f32 x ) : f32 { return x * 2.0f; };
			halt if( f( 17.0f ) != 34.0f );
			halt if( f( -3.5f ) != -7.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def NonCaptureLambda_Test2():
	c_program_text= """
		fn Foo()
		{
			// Lambda returns passed reference.
			auto f= lambda( i32& x ) : i32& { return x; };
			var i32 arg= 765;
			var i32& ref= f(arg);
			halt if( ref != 765 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def NonCaptureLambda_Test3():
	c_program_text= """
		struct S{ i32 &mut r; }
		fn Foo()
		{
			// Lambda returns mutable reference passed inside value arg.
			auto f= lambda( S s ) : i32 &mut { return s.r; };
			var i32 mut q= 654;
			{
				var S s{ .r= q };
				var i32 &mut q_ref= f( s );
				q_ref= -863;
			}
			halt if( q != -863 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def NonCaptureLambda_Test4():
	c_program_text= """
		struct R{ i32& r; }
		fn Foo()
		{
			// Lambda returns passed reference inside a variable.
			auto f=
				lambda( i32& x ) : R
				{
					var R r{ .r= x };
					return r;
				};

			var i32 arg= 9890;
			var R r= f(arg);
			halt if( r.r != 9890 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaCaptureAllByValue_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 543;
			auto f= lambda [=] () : i32 { return x; };
			halt if( f() != 543 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaCaptureAllByValue_Test1():
	c_program_text= """
		fn Foo()
		{
			var f32 x= 3.0f, mut y= 16.0f;
			auto f= lambda [=] ( f32 i ) : f32 { return i * x + y; };
			halt if( f( 4.0f ) != 28.0f );
			halt if( f( -2.5f ) != 8.5f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaCaptureAllByValue_Test2():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S s{ .x= 42, .y= 0.25f };
			var u64 x(1234567);
			// Capture struct and scalar.
			auto f= lambda [=] () : f64 { return f64(s.x) * f64(s.y) + f64(x); };
			halt if( f() != 1234577.5 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def Lambda_ReturnReferenceToCapturedVariable_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 12345;
			auto f= lambda[=]() : i32& { return x; };
			auto& ref= f();
			halt if( ref != 12345 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def Lambda_ReturnReferenceToCapturedVariable_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 777;
			auto f= lambda[=]() : i32& { return x; };
			auto& ref= f();
			x= 0; // Since lambda captures by value, we can modify source variable without affecting captured inside lambda value.
			halt if( ref != 777 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def Lambda_ReturnReferenceToCapturedVariable_Test2():
	c_program_text= """
		struct R
		{
			f64 &mut r;
		}
		fn Foo()
		{
			var f64 mut x= 1234.5;
			{
				var R r { .r= x };
				// Return inner reference of captured variable.
				auto f= lambda[=]() : f64 &mut { return r.r; };
				static_assert( typeinfo</ typeof(f) />.reference_tag_count == 1s );
				auto &mut ref= f();
				halt if( ref != 1234.5 );
				ref= 5.25;
			}
			halt if( x != 5.25 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def Lambda_ReturnReferenceToCapturedVariable_Test3():
	c_program_text= """
		struct R
		{
			i32& r;
		}
		fn Foo()
		{
			var i32 mut x= 6543;
			auto f=
				lambda[=]() : R
				{
					var R r{ .r= x };
					return r; // Return reference to captured variable "x" inside "R".
				};
			static_assert( typeinfo</ typeof(f) />.reference_tag_count == 0s );
			var R r= f();
			halt if( r.r != 6543 );
			x= 77; // Change source variable, but captured in lambda by value variable should not be changed.
			halt if( r.r != 6543 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaCaptureAllByReference_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 676;
			auto f= lambda [&] () : i32 { return x; };
			static_assert( typeinfo</ typeof(f) />.reference_tag_count == 1s );
			halt if( f() != 676 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaCaptureAllByReference_Test1():
	c_program_text= """
		fn Foo()
		{
			var f32 mut k= 0.0f;
			// Capture mutable reference.
			auto f= lambda [&] () { k+= 133.5f; };
			static_assert( typeinfo</ typeof(f) />.reference_tag_count == 1s );
			halt if( k != 0.0f );
			f();
			halt if( k != 133.5f );
			f();
			halt if( k != 267.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaCaptureAllByReference_Test2():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 5, mut y= 7;
			// Capture two mutable references.
			auto f= lambda [&] ()
				{
					auto tmp= x;
					x= y;
					y= tmp;
				};
			static_assert( typeinfo</ typeof(f) />.reference_tag_count == 2s );
			halt if( x != 5 );
			halt if( y != 7 );
			f();
			halt if( x != 7 );
			halt if( y != 5 );
			f();
			halt if( x != 5 );
			halt if( y != 7 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
