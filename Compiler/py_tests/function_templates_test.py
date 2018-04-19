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


def Specialization_Test0():
	c_program_text= """
		template</ type T /> struct Box{}

		template</ type T />
		fn Bar( T& t ) : i32 { return 666; }

		template</ type T />
		fn Bar( Box</ T />& t ) : i32 { return 999; }

		fn Foo() : i32
		{
			var Box</ i32 /> box;
			return Bar(box); // Should select  fn Bar( Box</ T />& t ), because it is more specialized.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 999 )


def Specialization_Test1():
	c_program_text= """
		template</ type T /> struct Box{}
		template</ type T /> struct Vec{}

		template</ type T />
		fn Bar( T& t ) : i32 { return 666; }

		template</ type T />
		fn Bar( Box</ T />& t ) : i32 { return 999; }

		template</ type T />
		fn Bar( Vec</ Box</ T /> />& t ) : i32 { return 111; }

		fn Foo() : i32
		{
			var Box</ i32 /> box;
			return Bar(box); // Should select Bar( Box</ T />& t ), because it is more specialized. Bar( Vec</ Box</ T /> />& t ) does not match.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 999 )


def Specialization_Test2():
	c_program_text= """
		template</ type T /> struct Box{}
		template</ type T /> struct Vec{}

		template</ type T />
		fn Bar( Vec</ T />& t ) : i32 { return 999; }

		template</ type T />
		fn Bar( Vec</ Box</ T /> />& t ) : i32 { return 111; }

		fn Foo() : i32
		{
			var Vec</ Box</ i32 /> /> vec_box;
			return Bar(vec_box); // Should select  Bar( Vec</ Box</ T /> />& t ), because it is more specialized.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 111 )


def Specialization_Test3():
	c_program_text= """
		template</ type T, type size_type, size_type size />
		fn Bar( [ T, size ] & t ) : i32 { return 9965; }

		template</ type T />
		fn Bar( T& t ) : i32 { return 8521; }

		fn Foo() : i32
		{
			var [ i32, 16 ] arr= zero_init;
			return Bar(arr); // Should select Bar( [ T, size ] & t ), array is more specialized.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 9965 )


def Specialization_Test4():
	c_program_text= """
		template</ type T />
		fn Bar( T& t ) : i32 { return 8521; }

		template</  />
		fn Bar( i32& t ) : i32 { return 8888; }

		fn Foo() : i32
		{
			return Bar(0); // Should select Bar( i32 & t ), concrete type is more specialized, then template parameter.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 8888 )


def Specialization_Test5():
	c_program_text= """
		template</ type T /> struct Box{}

		template</ type T />
		fn Bar( T& t0, T& t1 ) : i32 { return 666; }

		template</ type T0, type T1 />
		fn Bar( Box</ T0 />& t0, T1& t1 ) : i32 { return 999; }

		fn Foo() : i32
		{
			var Box</ i32 /> box;
			return Bar( box, box ); // Should select Bar( Box</ T />& t0, T& t1 ), because first parameter is more specialized.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 999 )


def Specialization_Test6():
	c_program_text= """
		template</ type T, type Diff /> struct Point{}

		template</ type T />
		fn Bar( Point</ T, i32 />& point ) : i32 { return 111; }

		template</ type T, type Diff />
		fn Bar( Point</ T, Diff />& point ) : i32 { return 666; }

		fn Foo() : i32
		{
			var Point</ i32, i32 /> point;
			return Bar( point ); // Should select ar( Point</ T, i32 />& point ), because second argument it template is concrete type, not template parameter.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 111 )


def Specialization_Test7():
	c_program_text= """
		template</ type T, type Diff /> struct Point{}

		template</ type    T />
		fn Bar( Point</   T,  i32 />& point ) : i32 { return 111; }

		template</ type Diff />
		fn Bar( Point</ i32, Diff />& point ) : i32 { return 666; }

		fn Foo() : i32
		{
			var Point</ i32, i32 /> point;
			return Bar( point ); // Error, can not select, specializations incomparable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TooManySuitableOverloadedFunctions" )
	assert( errors_list[0].file_pos.line == 13 )


def Specialization_Test8():
	c_program_text= """
		template</ type T, u32 size />
		fn Bar( [ T, size ]& arr ) : i32 { return 8855; }

		template</ type T />
		fn Bar( [ T, 3u ]& arr3 ) : i32 { return 3333; }

		fn Foo() : i32
		{
			var [ i32, 3 ] arr= zero_init;
			return Bar( arr ); // Should select Bar( [ T, 3u ]& arr3 ), because array with constant size is better, than array with template parameter size.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 3333 )


def Specialization_Test9():
	c_program_text= """
		template</ type T, type size_type, size_type size />
		fn Bar( [ T, size ] & t ) : i32 { return 9965; }

		template</ type size_type, size_type size />
		fn Bar( [ i32, size ] & t ) : i32 { return 111114; }

		template</ type T />
		fn Bar( T& t ) : i32 { return 8521; }

		fn Foo() : i32
		{
			var [ i32, 8 ] arr= zero_init;
			return Bar(arr); // Should select Bar( [ i32, size ] & t ), because array element type as type is better, then array element type as template parameter.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 111114 )


def Specialization_Test10():
	c_program_text= """
		type IVec3= [ i32, 3u ];

		template</  />
		fn Bar( IVec3& v ) : i32 { return 666; }

		template</  />
		fn Bar( [ i32, 3u ] & v ) : i32 { return 222; }

		fn Foo() : i32
		{
			var [ i32, 3 ] arr= zero_init;
			return Bar(arr); // Error, both functions have same specialization of parameter (concrete type).
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TooManySuitableOverloadedFunctions" )
	assert( errors_list[0].file_pos.line == 13 )


def Specialization_Test11():
	c_program_text= """
		template</ type T /> struct Box{}
		type IBox= Box</ i32 />;

		template</  />
		fn Bar( IBox& v ) : i32 { return 666; }

		template</  />
		fn Bar( Box</ i32 /> & v ) : i32 { return 222; }

		fn Foo() : i32
		{
			var IBox box;
			return Bar(box); // Error, both functions have same specialization of parameter (concrete type).
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TooManySuitableOverloadedFunctions" )
	assert( errors_list[0].file_pos.line == 14 )


def Specialization_Test12():
	c_program_text= """
		template</ type T, u32 size /> struct ArrayWrapper{}
		type IVec4= ArrayWrapper</ i32, 4u />;

		template</  />
		fn Bar( IVec4& v ) : i32 { return 666; }

		template</  />
		fn Bar( ArrayWrapper</ i32, 4u /> & v ) : i32 { return 222; }

		fn Foo() : i32
		{
			var IVec4 vec;
			return Bar(vec); // Error, both functions have same specialization of parameter (concrete type).
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TooManySuitableOverloadedFunctions" )
	assert( errors_list[0].file_pos.line == 14 )
