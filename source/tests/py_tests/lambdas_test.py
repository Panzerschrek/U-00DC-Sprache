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


def SimpleLambda_Test3():
	c_program_text= """
		fn Foo()
		{
			// Each lambda has its own type, even if they have identical bodies.
			auto f0= lambda(){};
			auto f1= lambda(){};
			static_assert( !same_type</ typeof(f0), typeof(f1) /> );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SimpleLambda_Test4():
	c_program_text= """
		fn Foo()
		{
			// Local type alias should be visible inside lambda signature and body.
			type Int= i32;
			auto f=
				lambda ( Int x ) : Int
				{
					var Int res= x * 2;
					return res;
				};
			halt if( f( 67 ) != 67 * 2 );
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


def LambdaCaptureAllByValue_Test4():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 7890;
			auto f0=
				lambda[=]() : i32
				{
					// Capture variable in level0 lambda.
					auto x_copy= x;
					auto f1=
						lambda[=]() : i32
						{
							// Capture variable from level0 lambda in level1 lambda.
							return x_copy;
						};
					return f1();
				};
			halt if( f0() != 7890 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaCaptureAllByValue_Test5():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 543;
			auto f=
				lambda [=] () : i32
				{
					// "typeof" captures value.
					var typeof(x) res= 210;
					return res;
				};
			auto& ti= typeinfo</ typeof(f) />;
			static_assert( ti.field_count == 1s );
			static_assert( ti.size_of == typeinfo</ typeof(x) />.size_of );
			halt if( f() != 210 );
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


def LambdaCaptureAllByReference_Test6():
	c_program_text= """
		struct S
		{
			i32 x;
			fn SomeMethod( mut this )
			{
				// Can capture reference to "this" field by binding it first to a local reference.
				auto &mut x_ref= x;
				auto f= lambda[&]() { x_ref= 112233; };
				f();
			}
		}
		fn Foo()
		{
			var S mut s= zero_init;
			s.SomeMethod();
			halt if( s.x != 112233 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaCaptureAllByReference_Test7():
	c_program_text= """
		struct S
		{
			i32 x;
			i32 y;
			fn SomeMethod( mut this )
			{
				// Can capture reference to different "this" fields by binding them first to local references.
				auto &mut x_ref= x;
				auto &mut y_ref= y;
				auto f=
					lambda[&]()
					{
						auto tmp= x_ref;
						x_ref= y_ref;
						y_ref= tmp;
					};
				f();
			}
		}
		fn Foo()
		{
			var S mut s{ .x= 33, .y= 66 };
			s.SomeMethod();
			halt if( s.x != 66 );
			halt if( s.y != 33 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaCaptureAllByReference_Test8():
	c_program_text= """
		struct S
		{
			i32 x;
			fn SomeMethod( mut this )
			{
				// Can capture reference to "this" by binding it first to a local reference.
				auto &mut self= this;
				auto f= lambda[&]() { self.x= 7777; };
				f();
			}
		}
		fn Foo()
		{
			var S mut s= zero_init;
			s.SomeMethod();
			halt if( s.x != 7777 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaCaptureAllByReference_Test9():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			auto mut f0=
				lambda[&]()
				{
					// Capture variable in level0 lambda.
					auto &mut x_ref= x;
					auto f1=
						lambda[&]()
						{
							// Capture variable from level0 lambda in level1 lambda.
							x_ref= 55665544;
						};
					f1();
				};
			f0();
			move(f0);
			++x;
			halt if( x != 55665545 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaCaptureAllByReference_Test10():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 543;
			auto f=
				lambda [&] () : i32
				{
					// "typeof" captures value.
					var typeof(x) res= 210;
					return res;
				};
			auto& ti= typeinfo</ typeof(f) />;
			static_assert( ti.field_count == 1s );
			static_assert( ti.size_of == typeinfo</ $( typeof(x) ) />.size_of );
			static_assert( ti.reference_tag_count == 1s );
			halt if( f() != 210 );
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


def UnsafeLambda_Test0():
	c_program_text= """
		fn Foo()
		{
			auto f= lambda() unsafe {}; // Ok - define unsafe lambda, but do not call it.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def UnsafeLambda_Test1():
	c_program_text= """
		fn Foo()
		{
			auto f= lambda() unsafe {};
			unsafe( f() ); // Ok - call unsafe lambda in unsafe expression.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def UnsafeLambda_Test2():
	c_program_text= """
		fn Foo()
		{
			auto f= lambda() unsafe {};
			f(); // Error - calling unsafe function outside unsafe block.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnsafeFunctionCallOutsideUnsafeBlock", 5 ) )


def LambdaConstexpr_Test0():
	c_program_text= """
		fn Foo()
		{
			// Constexpr non-capture lambda.
			auto constexpr f= lambda() : i32 { return 5566; };
			var i32 constexpr i= f();
			static_assert( i == 5566 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaConstexpr_Test1():
	c_program_text= """
		fn Foo()
		{
			// Constexpr non-capture lambda with argument.
			auto constexpr f= lambda( u32 x ) : u32 { return x * 5u; };
			var u32 constexpr i= f( 17u );
			static_assert( i == 17u * 5u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaConstexpr_Test2():
	c_program_text= """
		fn Foo()
		{
			// Constexpr lambda with capture by value.
			var i32 x= 4455;
			auto constexpr f= lambda[=]() : i32 { return x - 8; };
			var i32 constexpr i= f();
			static_assert( i == 4455 - 8 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaConstexpr_Test3():
	c_program_text= """
		fn Foo()
		{
			// Constexpr lambda with capture by value and with argument.
			var f32 x= 0.25f, y= 3.5f;
			auto constexpr f= lambda[=]( f32 z ) : f32 { return z * x - y; };
			static_assert( f( 6.0f ) == 6.0f * 0.25f - 3.5f );
			static_assert( f( -2.5f ) == -2.5f * 0.25f - 3.5f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaConstexpr_Test4():
	c_program_text= """
		fn Foo()
		{
			// Constexpr lambda with capture by reference and with argument.
			var f32 x= 0.25f, y= 3.5f;
			auto constexpr f= lambda[&]( f32 z ) : f32 { return z * x - y; };
			static_assert( f( 6.0f ) == 6.0f * 0.25f - 3.5f );
			static_assert( f( -2.5f ) == -2.5f * 0.25f - 3.5f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaConstexpr_Test5():
	c_program_text= """
		fn Foo()
		{
			// Lambda object is still considered to be constexpr, even if lambda constains "unsafe" inside.
			// Later it's impossible to call such lambda in constexpr context.
			auto constexpr f= lambda() { unsafe{} };
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaConstexpr_Test6():
	c_program_text= """
		fn Foo()
		{
			auto constexpr f= lambda() : i32 { unsafe{} return 0; };
			auto constexpr res= f(); // Error, lambda () operator isn't constexpr, because it constains non-constexpr operations inside.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "VariableInitializerIsNotConstantExpression", 5 ) )


def LambdaConstexpr_Test7():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			// The type of the lambda is constexpr, but lambda object isn't constexpr, since captured variable isn't constexpr.
			auto constexpr f= lambda[=]() : i32 { return x; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "VariableInitializerIsNotConstantExpression", 6 ) )


def LambdaConstexpr_Test8():
	c_program_text= """
		struct S{ fn destructor(); } // This struct is not constexpr, since it contains non-trivial destructor.
		fn Foo()
		{
			var S s;
			// Lambda type isn't constexpr, because it captures variable of non-constexpr type.
			auto constexpr f= lambda[=]() : i32 { auto& s_ref= s; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "InvalidTypeForConstantExpressionVariable", 7 ) or HaveError( errors_list, "VariableInitializerIsNotConstantExpression", 7 ) )


def LambdaConstexpr_Test9():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			// Lambda isn't constexpr, since it captures mutable reference.
			auto constexpr f= lambda[&]() : i32 { return x; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "InvalidTypeForConstantExpressionVariable", 6 ) or HaveError( errors_list, "VariableInitializerIsNotConstantExpression", 6 ) )


def LambdaConstexpr_Test10():
	c_program_text= """
		template</type Func/>
		fn Derivative( Func& f, f32 x ) : f32
		{
			return f( x + 0.5f ) - f( x - 0.5f );
		}
		fn Foo()
		{
			auto scale= 3.0f;
			// Pass constexpr lambda with capture into template function and otain constexpr result.
			var f32 constexpr d= Derivative( lambda[=]( f32 x ) : f32 { return scale * x; }, 17.5f );
			static_assert( d == 3.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaConstexpr_Test11():
	c_program_text= """
		fn Foo()
		{
			// Define lambda and call it in array size expression.
			var[ i32, lambda() : size_type { return 4s; } () ] arr= zero_init;
			static_assert( typeinfo</ typeof(arr) />.element_count == 4s );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaConstexpr_Test12():
	c_program_text= """
		fn Foo()
		{
			// Define lambda with capture by reference and call it in array size expression.
			var size_type s(13);
			var[ f64, lambda[&]() : size_type { return s; } () ] arr= zero_init;
			static_assert( typeinfo</ typeof(arr) />.element_count == s );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaGlovalVariableCapture_Test0():
	c_program_text= """
		var i32 global_x= 121212;
		fn Foo()
		{
			// Global variables are not captured in non-capture lambda.
			auto f= lambda() : i32 { return global_x; };
			auto& ti= typeinfo</ typeof(f) />;
			static_assert( ti.size_of == 0s );
			static_assert( ti.field_count == 0s );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaGlovalVariableCapture_Test1():
	c_program_text= """
		struct S
		{
			auto v= 5.125f;
		}
		fn Foo()
		{
			// Global variables are not captured in capture by value lambda.
			auto f= lambda[=]() : f32 { return S::v; };
			auto& ti= typeinfo</ typeof(f) />;
			static_assert( ti.size_of == 0s );
			static_assert( ti.field_count == 0s );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaGlovalVariableCapture_Test2():
	c_program_text= """
		template</type T, size_type S/>
		fn Bar( [ T, S ]& arr )
		{
			// Variable template arguments are not captured in lambda.
			auto f= lambda[&]() : size_type { return S; };
			auto& ti= typeinfo</ typeof(f) />;
			static_assert( ti.size_of == 0s );
			static_assert( ti.field_count == 0s );
		}
		fn Foo()
		{
			var [ i32, 6 ] arr= zero_init;
			Bar( arr );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaInGlobalContext_Test0():
	c_program_text= """
		// Global lambda.
		auto double_it= lambda( i32 x ) : i32 { return x * 2; };
		fn Foo( i32 x ) : i32
		{
			return double_it( x );
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Fooi", 787 ) == 787 * 2 )


def LambdaInGlobalContext_Test1():
	c_program_text= """
		namespace Math
		{
			auto pi= 3.1415926535f;
			// Lambda in a namespace with usage of external variables.
			auto circle_area= lambda( f32 radius ) : f32 { return pi * radius * radius; };
			// Should capture nothing.
			static_assert( typeinfo</ typeof(circle_area) />.size_of == 0s );
		}
		fn Foo()
		{
			static_assert( Math::circle_area( 2.5f ) == Math::pi * 2.5f * 2.5f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaInGlobalContext_Test2():
	c_program_text= """
		struct S
		{
			fn Bar() : u32 { return 7767u; }
			// Lambda in a struct, that is constexpr, but call operator isn't constexpr since it calls non-constexpr function.
			auto f= lambda() : u32 { return Bar(); };
		}
		fn Foo()
		{
			auto x= S::f();
			halt if( x != 7767u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaInGlobalContext_Test3():
	c_program_text= """
		// Use lambda to initialize global variable.
		auto two_powers=
			lambda() : [ u32, 8 ]
			{
				var [ u32, 8 ] mut res= zero_init;
				for( auto mut i= 0u; i < 8u; ++i )
				{
					res[ size_type(i) ]= 1u << i;
				}
				return res;
			} ();

		static_assert( two_powers[0] ==   1u );
		static_assert( two_powers[1] ==   2u );
		static_assert( two_powers[2] ==   4u );
		static_assert( two_powers[3] ==   8u );
		static_assert( two_powers[4] ==  16u );
		static_assert( two_powers[5] ==  32u );
		static_assert( two_powers[6] ==  64u );
		static_assert( two_powers[7] == 128u );
	"""
	tests_lib.build_program( c_program_text )


def LambdaNonSync_Test0():
	c_program_text= """
		fn Foo()
		{
			// Simple lambda with no captures should be not non_sync.
			auto f= lambda(){};
			static_assert( !non_sync</ typeof(f) /> );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaNonSync_Test1():
	c_program_text= """
		struct S non_sync{}
		static_assert( non_sync</ S /> );
		fn Foo()
		{
			// Simple lambda with no captures should be not non_sync, even if it uses non_sync types inside.
			auto f=
				lambda()
				{
					var S s;
				};
			static_assert( !non_sync</ typeof(f) /> );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaNonSync_Test2():
	c_program_text= """
		struct S non_sync{}
		static_assert( non_sync</ S /> );
		fn Foo()
		{
			// Simple lambda with no captures should be not non_sync, even if it has non_sync paramters and return value.
			auto f=
				lambda( S s ) : S
				{
					return s;
				};
			static_assert( !non_sync</ typeof(f) /> );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaNonSync_Test3():
	c_program_text= """
		struct S{ i32 x; }
		static_assert( !non_sync</ S /> );
		fn Foo()
		{
			// A lambda that captures non-non_sync type by value is not non-non_sync.
			var S s= zero_init;
			auto f=
				lambda[=]()
				{
					auto& s_ref= s;
				};
			static_assert( !non_sync</ typeof(f) /> );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaNonSync_Test4():
	c_program_text= """
		struct S{ i32 x; }
		static_assert( !non_sync</ S /> );
		fn Foo()
		{
			// A lambda that captures non-non_sync type by reference is non-non_sync.
			var S s= zero_init;
			auto f=
				lambda[&]()
				{
					auto& s_ref= s;
				};
			static_assert( !non_sync</ typeof(f) /> );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaNonSync_Test5():
	c_program_text= """
		struct S non_sync { i32 x; }
		static_assert( non_sync</ S /> );
		fn Foo()
		{
			// A lambda that captures non_sync type by value is non_sync.
			var S s= zero_init;
			auto f=
				lambda[=]()
				{
					auto& s_ref= s;
				};
			static_assert( non_sync</ typeof(f) /> );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaNonSync_Test6():
	c_program_text= """
		struct S non_sync { i32 x; }
		static_assert( non_sync</ S /> );
		fn Foo()
		{
			// A lambda that captures non_sync type by reference is non_sync.
			var S s= zero_init;
			auto f=
				lambda[&]()
				{
					auto& s_ref= s;
				};
			static_assert( non_sync</ typeof(f) /> );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaMutabilityModifier_Test0():
	c_program_text= """
		fn Foo()
		{
			auto f= lambda imut() {};
		}
	"""
	tests_lib.build_program( c_program_text )


def LambdaMutabilityModifier_Test1():
	c_program_text= """
		fn Foo()
		{
			auto f= lambda mut(){};
		}
	"""
	tests_lib.build_program( c_program_text )


def LambdaMutabilityModifier_Test2():
	c_program_text= """
		fn Foo()
		{
			auto x= 0;
			auto f= lambda [=] mut ( i32 a ) : i32 { return x * a; };
		}
	"""
	tests_lib.build_program( c_program_text )
