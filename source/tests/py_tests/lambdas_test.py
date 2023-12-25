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


def NonCaptureLambda_Test5():
	c_program_text= """
		template</type Func/>
		fn CallFunc( Func func ) : i32
		{
			return func();
		}
		fn Foo()
		{
			// Use lambda as argument of template function.
			var i32 res= CallFunc( lambda() : i32 { return 678; } );
			halt if( res != 678 );
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


def LambdaCaptureAllByValue_Test3():
	c_program_text= """
		template</type Func/>
		fn CallFunc( Func func ) : f64
		{
			return func();
		}
		fn Foo()
		{
			// Use lambda as argument of template function.
			var f64 some= 123.4;
			var f64 res= CallFunc( lambda[=]() : f64 { return some; } );
			halt if( res != 123.4 );
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


def LambdaCaptureAllByReference_Test3():
	c_program_text= """
		fn Foo()
		{
			var u64 x(123123123);
			// Return reference to captured by reference variable.
			auto f= lambda [&] () : u64& { return x; };
			static_assert( typeinfo</ typeof(f) />.reference_tag_count == 1s );
			auto& x_ref= f();
			halt if( x_ref != 123123123u64 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaCaptureAllByReference_Test4():
	c_program_text= """
		struct R{ i32& mut x; }
		fn Foo()
		{
			var i32 mut x= 676767;
			{
				// Return reference to captured by reference variable inside a variable.
				auto f=
					lambda [&] () : R
					{
						var R r{ .x= x };
						return r;
					};
				static_assert( typeinfo</ typeof(f) />.reference_tag_count == 1s );
				auto r= f();
				r.x /= 4;
			}
			halt if( x != 676767 / 4 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaCaptureAllByReference_Test5():
	c_program_text= """
		template</type T/>
		struct Box
		{
			T t;
		}

		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0a" ] ];
		template</type T/>
		fn MakeBox( T mut t ) : Box</T/> @(return_inner_references)
		{
			var Box</T/> mut b{ .t= move(t) };
			return move(b);
		}
		fn Foo()
		{
			var i32 mut x= 15;
			{
				auto boxed_lambda= MakeBox( lambda[&](){ x -= 3; } );
				boxed_lambda.t();
			}
			halt if( x != 12 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaMayBeCopyable_Test0():
	c_program_text= """
		fn Foo()
		{
			// Non-capture lambda is copyable.
			auto f= lambda( i32 x, i32 y ) : i32 { return x - y; };
			static_assert( typeinfo</ typeof(f) />.is_copy_constructible );
			auto f_copy0= f;
			var typeof(f) f_copy1(f_copy0);
			halt if( f_copy1( 67, 34 ) != 67 - 34 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaMayBeCopyable_Test1():
	c_program_text= """
		struct S
		{
			f32 x;
		}
		fn Foo()
		{
			var i32 x= 67;
			var S s{ .x= 3.5f };
			// This lambda is copyable because it captures by value only copy-constructible types.
			auto f= lambda[=]() : f32 { return f32(x) * s.x; };
			static_assert( typeinfo</ typeof(f) />.is_copy_constructible );
			auto f_copy0= f;
			var typeof(f) f_copy1(f_copy0);
			halt if( f_copy1( ) != 234.5f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaMayBeCopyable_Test2():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 17, y= 3;
			{
				// This lambda is copyable because it captures only references.
				auto f= lambda[&]() { x *= y; };
				static_assert( typeinfo</ typeof(f) />.is_copy_constructible );
				auto f_copy0= f;
				var typeof(f) f_copy1(f_copy0);
				f_copy1();
			}
			halt if( x != 17 * 3 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaMayBeCopyable_Test3():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 13;
			var f32 y= 22.5f;
			auto mut f= lambda[=]() : f32 { return f32(x) * y; };
			auto f_copy= f;
			f= f_copy; // Copy assignment operator may be generated for lambdas with capturing by value.
			halt if( f() != 13.0f * 22.5f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaTypeinfo_Test0():
	c_program_text= """
		template</ size_type size0, size_type size1 />
		fn constexpr StringEquals( [ char8, size0 ]& s0, [ char8, size1 ]& s1 ) : bool
		{
			if( size0 != size1 ) { return false; }
			var size_type mut i(0);
			while( i < size0 )
			{
				if( s0[i] != s1[i] ) { return false; }
				++i;
			}
			return true;
		}

		template</ type T, size_type name_size />
		fn constexpr IsPrivate( T& list, [ char8, name_size ]& name ) : bool
		{
			for( & list_element : list )
			{
				if( StringEquals( list_element.name, name ) )
				{
					return list_element.is_private;
				}
			}
			halt;
		}

		template</ type T, size_type name_size />
		fn constexpr IsReferenceField( T& fields_list, [ char8, name_size ]& name ) : bool
		{
			for( & list_element : fields_list )
			{
				if( StringEquals( list_element.name, name ) )
				{
					return list_element.is_reference;
				}
			}
			halt;
		}

		fn Foo()
		{
			var i32 x= 0;
			var f32 y= 0.0f;
			auto f= lambda[=]() : f32 { return f32(x) * y; };
			auto& ti= typeinfo</ typeof(f) />;
			// Fields created for captured by value variables are private.
			static_assert( !IsReferenceField( ti.fields_list, "x" ) );
			static_assert( IsPrivate( ti.fields_list, "x" ) );
			static_assert( !IsReferenceField( ti.fields_list, "y" ) );
			static_assert( IsPrivate( ti.fields_list, "y" ) );
		}

		fn Bar()
		{
			var i32 x= 0;
			var f32 y= 0.0f;
			auto f= lambda[&]() : f32 { return f32(x) * y; };
			auto& ti= typeinfo</ typeof(f) />;
			// Fields created for captured by reference variables are private.
			static_assert( IsReferenceField( ti.fields_list, "x" ) );
			static_assert( IsPrivate( ti.fields_list, "x" ) );
			static_assert( IsReferenceField( ti.fields_list, "y" ) );
			static_assert( IsPrivate( ti.fields_list, "y" ) );
		}
	"""
	tests_lib.build_program( c_program_text )


def LambdaTypeinfo_Test1():
	c_program_text= """
		fn Foo()
		{
			// Capture nothing.
			auto f= lambda( i32 x ) : i32 { return x * 2; };
			auto& ti= typeinfo</ typeof(f) />;
			static_assert( ti.is_class );
			static_assert( !ti.is_struct );
			static_assert( !ti.is_polymorph );
			static_assert( ti.is_final );
			static_assert( !ti.is_abstract );
			static_assert( !ti.is_interface );
			static_assert( !ti.is_coroutine );
			static_assert( ti.is_lambda );
			static_assert( !ti.is_default_constructible );
			static_assert( !ti.is_equality_comparable );
			static_assert( ti.field_count == 0s );
			static_assert( ti.reference_tag_count == 0s );
		}
	"""
	tests_lib.build_program( c_program_text )


def LambdaTypeinfo_Test2():
	c_program_text= """
		fn Foo()
		{
			// Capture all by value is specified, but really nothing captured.
			auto f= lambda[=]( i32 x ) : i32 { return x * 2; };
			auto& ti= typeinfo</ typeof(f) />;
			static_assert( ti.is_class );
			static_assert( !ti.is_struct );
			static_assert( ti.is_lambda );
			static_assert( !ti.is_default_constructible );
			static_assert( !ti.is_equality_comparable );
			static_assert( ti.field_count == 0s );
			static_assert( ti.reference_tag_count == 0s );
		}
	"""
	tests_lib.build_program( c_program_text )


def LambdaTypeinfo_Test3():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 1;
			var f32 y= 3.5f;
			var bool b= false;
			// Capture 3 variables by value.
			auto f= lambda[=]() : f32 { return select( b ? y : f32(x) ); };
			auto& ti= typeinfo</ typeof(f) />;
			static_assert( ti.is_class );
			static_assert( !ti.is_struct );
			static_assert( ti.is_lambda );
			static_assert( !ti.is_default_constructible );
			static_assert( !ti.is_equality_comparable );
			static_assert( ti.field_count == 3s );
			static_assert( ti.reference_tag_count == 0s );
		}
	"""
	tests_lib.build_program( c_program_text )


def LambdaTypeinfo_Test4():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 1;
			var f32 y= 3.5f;
			var bool b= false;
			// Capture 3 variables by reference.
			auto f= lambda[&]() : f32 { return select( b ? y : f32(x) ); };
			auto& ti= typeinfo</ typeof(f) />;
			static_assert( ti.is_class );
			static_assert( !ti.is_struct );
			static_assert( ti.is_lambda );
			static_assert( !ti.is_default_constructible );
			static_assert( !ti.is_equality_comparable );
			static_assert( ti.field_count == 3s );
			static_assert( ti.reference_tag_count == 3s );
		}
	"""
	tests_lib.build_program( c_program_text )


def LambdaReferencePoillution_Test0():
	c_program_text= """
		struct R{ i32& x; }
		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( R &mut r, i32& x ) @(pollution) {}
		fn Foo()
		{
			var i32 x= 0, y= 0;
			var R mut r{ .x= x };
			// Should allow performing reference pollution for lambda params.
			auto f= lambda( R &mut r, i32& arg ) { MakePollution( r, arg ); };
			f( r, y );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaReferencePoillution_Test1():
	c_program_text= """
		struct R{ i32& x; }
		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( R &mut r, i32& x ) @(pollution) {}
		fn Foo()
		{
			var i32 x= 0, y= 0;
			// Should allow performing reference pollution for lambda param by captured by value variable.
			auto f= lambda[=]( R &mut r ) { MakePollution( r, y ); };
			var R mut r{ .x= x };
			f(r);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaReferencePoillution_Test2():
	c_program_text= """
		struct R{ i32& x; }
		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( R &mut r, i32& x ) @(pollution) {}
		fn Foo()
		{
			var i32 x= 0, y= 0;
			// Should allow performing reference pollution for lambda param by captured by reference variable.
			auto f= lambda[&]( R &mut r ) { MakePollution( r, y ); };
			var R mut r{ .x= x };
			f(r);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaReferencePoillution_Test3():
	c_program_text= """
		struct R{ i32& x; }
		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( R &mut r, i32& x ) @(pollution) {}
		fn Foo()
		{
			var i32 x= 0, y= 0, z= 0;
			var R r0{ .x= z };
			// Should allow performing reference pollution for lambda param by inner reference of captured by value variable.
			auto f= lambda[=]( R &mut r ) { MakePollution( r, r0.x ); };
			var R mut r{ .x= x };
			f(r);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
