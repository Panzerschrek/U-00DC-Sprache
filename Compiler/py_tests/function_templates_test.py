from py_tests_common import *


def FunctionTemplateDeclaration_Test0():
	c_program_text= """
		template</ type T />
		fn ToVoid( T& t ) : void&
		{
			return t;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTemplateDeclaration_Test1():
	c_program_text= """
		template</ type T, u32 size />
		fn ZeroFill( [ T, size ] &mut arr )
		{
			var u32 mut i= 0u;
			while( i < size )
			{
				arr[i]= T(0);
				++i;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTemplateDeclaration_Test2():
	c_program_text= """
		class C
		{
			template</ type T, type U />
			fn DoNothing( T& t, U& u ) {}
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTemplateBase_Test0():
	c_program_text= """
		template</ type T />
		fn Set42( T &mut x )
		{
			x= T(42);
		}

		fn Foo() : f32
		{
			var f32 mut x= zero_init;
			Set42(x);
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 42.0 )


def FunctionTemplateBase_Test1():
	c_program_text= """
		template</ type T, type size_type, size_type array_size />
		fn SetFirstArrayElementToArraySize( [ T, array_size ] &mut arr )
		{
			static_assert( array_size > size_type(0) );
			arr[0u]= T(array_size);
		}

		fn Foo() : i32
		{
			var [ i32, 93 ] mut arr= zero_init;
			SetFirstArrayElementToArraySize( arr );   // Must deduce all template arguments, both type and value.
			return arr[0u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 93 )


def FunctionTemplateBase_Test2():
	c_program_text= """
		template</ type T />
		struct Box
		{
			T boxed;
		}

		template</ type T />
		fn Unbox( Box</ T />& box ) : T&
		{
			return box.boxed;
		}

		fn Foo() : f64
		{
			var Box</ f64 /> pi_in_box{ .boxed= 3.1415926535 };
			return Unbox( pi_in_box );  // Must deduce complex argument.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 3.1415926535 )


def FunctionTemplateBase_Test3():
	c_program_text= """
		template</ type T />
		fn Div( T a, T b ) : T
		{
			return a / b;
		}

		fn Foo() : f64
		{
			return f64( Div( 48, 6 ) ) + Div( 14.5, 1.0 );  // Must deduce multiple parameters with same template type.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ( 48.0 / 6.0 + 14.5 / 1.0 ) )


def TemplateMethod_Test0():
	c_program_text= """
		struct Num
		{
			f64 n;

			template</ type T />
			fn Set( mut this, T t ) // Template this-call function
			{
				n= f64(t);
			}
		}

		fn Foo() : f64
		{
			var Num mut num{ .n= 0.0 };
			num.Set( 66685 ); // Must generate integer method
			halt if( num.n != 66685.0 );
			num.Set( 5.25f ); // Must generate float methid
			halt if( num.n != 5.25 );
			return num.n;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5.25 )


def TemplateMethod_Test1():
	c_program_text= """
		struct Num
		{
			f64 n;

			template</ type T />
			fn Set( Num &mut self, T t ) // Template static function
			{
				self.n= f64(t);
			}
		}

		fn Foo() : f64
		{
			var Num mut num{ .n= 0.0 };
			Num::Set( num, -856 ); // Must generate integer method
			halt if( num.n != -856.0 );
			Num::Set( num, 3.14 ); // Must generate float methid
			halt if( num.n != 3.14 );
			return num.n;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 3.14 )


def TemplateMethod_Test2():
	c_program_text= """
		struct Num
		{
			f64 n;

			template</ type T />
			fn Set( mut this, T t ) // Template this-call function
			{
				n= f64(t);
			}

			fn Set( mut this, f64 f )
			{
				n= -666.0;
			}
		}

		fn Foo() : f64
		{
			var Num mut num{ .n= 59.5 };
			num.Set( 999.0 ); // Must select non-template function
			return num.n;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == -666.0 )


def TemplateMethod_Test3():
	c_program_text= """
		struct Num
		{
			f64 n;

			template</ type T />
			fn Set( mut this, T t ) // Template this-call function
			{
				n= f64(t);
			}

			fn SetZero( mut this )
			{
				Set(0); // template function must be visible here
			}
		}

		fn Foo() : f64
		{
			var Num mut num{ .n= 59.5 };
			num.SetZero();
			return num.n;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0.0 )


def TemplateMethod_Test4():
	c_program_text= """
		struct Num
		{
			f64 n;

			template</ type T />
			fn Set( Num &mut self, T t ) // Template static function
			{
				self.n= f64(t);
			}
		}

		fn Foo() : f64
		{
			var Num dummy_num{ .n= 0.0 };
			var Num mut num{ .n= 0.0 };
			dummy_num.Set( num, 856 ); // must call static method and dump "this" argument
			return num.n;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 856.0 )


def RecursiveTemplateFunctionCall_Test0():
	c_program_text= """
		template</ type T />
		fn Sum( T from, T to ) : T
		{
			auto diff = to - from;
			if( diff <= T(0) ) { return T(0); }
			return from + Sum( from + T(1), to ); // Must do recursive call here.
		}

		fn Foo() : u32
		{
			return Sum( 9u, 15u );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ( 9 + 10 + 11 + 12 + 13 + 14 ) )
