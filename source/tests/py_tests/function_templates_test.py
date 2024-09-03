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


def TemplateMethod_Test5():
	c_program_text= """
		struct Num
		{
			f64 n;

			template</ type T />
			fn Set( Num &mut self, T t )
			{
				self.n= f64(t);
			}
		}

		fn Foo() : f64
		{
			var Num mut num{ .n= 0.0 };
			Num::Set</ i32 />( num, 99654 ); // Directly set template paremeter.
			return num.n;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 99654 )


def TemplateMethod_Test6():
	c_program_text= """
		struct Num
		{
			f64 n;

			template</ type T />
			fn Set( mut this, T t )
			{
				n= f64(t);
			}
		}

		fn Foo() : f64
		{
			var Num mut num{ .n= 0.0 };
			num.Set</ i32 />( 885214 ); // Directly set template paremeter.
			return num.n;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 885214 )


def TemplateMethod_Test7():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			template</ i32 mul/> fn GetXMul(this) : i32 { return x * mul; }
		}

		class B : A
		{
			fn constructor( i32 in_x ) ( base(in_x) ) {}
		}

		fn Foo() : i32
		{
			var B b(66521);
			return b.GetXMul</23/>(); // Should call here template template method of base class.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 66521 * 23 )


def TemplateMethod_Test8():
	c_program_text= """
		struct Num
		{
			f64 n;

			template</ type T />
			fn Set( mut this, T t )
			{
				n= f64(t);
			}

			fn Set5( mut this )
			{
				Set( 5 ); // Call this-call template method, type is deduced.
			}
		}

		fn Foo() : f64
		{
			var Num mut num{ .n= 0.0 };
			num.Set5();
			return num.n;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5 )


def TemplateMethod_Test9():
	c_program_text= """
		struct Num
		{
			f64 n;

			template</ type T />
			fn Set( mut this, T t )
			{
				n= f64(t);
			}

			fn Set11( mut this )
			{
				Set</u32/>( 11u ); // Set type explicitly
			}
		}

		fn Foo() : f64
		{
			var Num mut num{ .n= 0.0 };
			num.Set11();
			return num.n;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 11 )


def TemplateOperator_Test0():
	c_program_text= """
		struct Num
		{
			f64 n;

			template</ type T />
			op*( this, T t ) : Num
			{
				var Num r { .n= n * f64(t) };
				return r;
			}
		}

		fn Foo() : f64
		{
			var Num mut num{ .n= 25.25 };
			return ( num * 2u ).n;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 50.5 )


def TemplateOperator_Test1():
	return
	c_program_text= """
		template</ type T, u64 size />
		struct ArrayWrapper
		{
			[ T, size ] arr;

			fn constructor()
			( arr= zero_init ){}

			template</ type Index />
			op[]( this, Index& index ) : T&
			{
				return arr[u64(index)];
			}

			template</ type Index />
			op[]( mut this, Index& index ) : T&mut
			{
				return arr[u64(index)];
			}
		}

		fn Foo() : i32
		{
			var ArrayWrapper</ i32, 3u64 /> mut arr;
			arr[0.1f]= 5;
			arr[1]= 7;
			arr[2u8]= 9;
			return arr[0] * arr[1] - arr[2];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5 * 7 - 9 )


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


def NoShadowingInsideTemplateFunction_Test0():
	c_program_text= """
		template</ type T />
		fn DoubleIt( T x ) : T
		{
			static_if( same_type</T, f64/> )
			{
				return x * 2.0;
			}
			else
			{
				// Should access here template function itself to instantiate it, rather than "DoubleIt" for current "T".
				return T( DoubleIt( f64(x) ) );
			}
		}
		fn Foo( u32 x ) : u32
		{
			return DoubleIt( x );
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Fooj", 67 ) == 67 * 2 )
	assert( tests_lib.run_function( "_Z3Fooj", 122 ) == 122 * 2 )


def NoShadowingInsideTemplateFunction_Test1():
	c_program_text= """
		template</ type T />
		fn TripleIt( T x ) : T
		{
			// Should access here the whole functions set named "TripleIt", not only currently instantiated function "TripleIt".
			return T( TripleIt( f64(x) ) );
		}
		fn TripleIt(f64 x) : f64 { return x * 3.0; }
		fn Foo( i32 x ) : i32
		{
			return TripleIt( x );
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Fooi", 12 ) == 12 * 3 )
	assert( tests_lib.run_function( "_Z3Fooi", 578 ) == 578 * 3 )


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
			return Bar(0); // Should select Bar( i32 & t ), concrete type is more specialized, than template parameter.
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
	assert( HasError( errors_list, "TooManySuitableOverloadedFunctions", 13 ) )


def Specialization_Test8():
	c_program_text= """
		template</ type T, size_type size />
		fn Bar( [ T, size ]& arr ) : i32 { return 8855; }

		template</ type T />
		fn Bar( [ T, size_type(3) ]& arr3 ) : i32 { return 3333; }

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
			return Bar(arr); // Should select Bar( [ i32, size ] & t ), because array element type as type is better, than array element type as template parameter.
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
		fn Bar( [ i32, size_type(3) ] & v ) : i32 { return 222; }

		fn Foo() : i32
		{
			var [ i32, 3 ] arr= zero_init;
			return Bar(arr); // Error, both functions have same specialization of parameter (concrete type).
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TooManySuitableOverloadedFunctions", 13 ) )


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
	assert( HasError( errors_list, "TooManySuitableOverloadedFunctions", 14 ) )


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
	assert( HasError( errors_list, "TooManySuitableOverloadedFunctions", 14 ) )


def Specialization_Test13():
	c_program_text= """
		template</ type T /> struct Box{}

		template</  />
		fn Bar( Box</ void /> & b ) : i32 { return 666; }

		fn Foo() : i32
		{
			var Box</i32/> box;
			return Bar(box); // Must not select Box</ void />, even if i32 is convertible to void.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 10 ) )


def Specialization_Test14():
	c_program_text= """
		template</ type T />
		fn Bar( T t ) : i32 { return 666; }

		template</ type T />
		fn Bar( ( fn(): T ) fn_t ) : i32 { return 999; }

		fn Foo() : i32
		{
			var( fn() : i32 ) ptr= Foo;
			return Bar( ptr );  // Must select function, specialized for function pointers.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 999 )


def Specialization_Test15():
	c_program_text= """
		template</ T val, type T/>
		fn Square() : T { return val * val; }
		// Deduce arg #1 based on type of value-arg #0.
		static_assert( Square</ 7 />() == 49 );
		static_assert( Square</ 734u />() == 734u * 734u );
		static_assert( Square</ -58875i64 />() == 58875i64 * 58875i64 );
	"""
	tests_lib.build_program( c_program_text )


def Specialization_Test16():
	c_program_text= """
		template</type A, type B/>
		struct Some</A, B= A/>{}

		// Skip default signature parameter of type template in function signature.
		template</type A/>
		fn Bar( Some</A/>& some ){}

		fn Foo()
		{
			var Some</ i32 /> some;
			Bar(some);
		}
	"""
	tests_lib.build_program( c_program_text )


def DirectFunctionTemplateParametersSet_Test0():
	c_program_text= """
		template</ type T />
		fn Cast( f32 x ) : T
		{
			return T(x);
		}

		fn Foo() : i32
		{
			return Cast</ i32 />(45.7f);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 45 )


def DirectFunctionTemplateParametersSet_Test1():
	c_program_text= """
		template</ i32 value, size_type size />
		fn FillArray( [ i32, size ] &mut arr )
		{
			var size_type mut i(0);
			while( i < size )
			{
				arr[i]= value;
				++i;
			}
		}

		fn Foo()
		{
			var [ i32, 42u ] mut arr= zero_init;
			FillArray</ 13, size_type(42) />( arr );
			auto mut i= 0u;
			while( i < 42u )
			{
				halt if( arr[i] != 13 );
				++i;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def DirectFunctionTemplateParametersSet_Test2():
	c_program_text= """
		class A polymorph{}
		class B : A {}
		template</ type T />
		fn Bar( T& a, T& b ) : i32 { return 888541; }

		fn Foo() : i32
		{
			return Bar</ A />( A(), B() );  // Ok, T is concrete type "A", and reference conversions works correctly.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 888541 )


def DirectFunctionTemplateParametersSet_Test3():
	c_program_text= """
		template</ type T />
		fn Bar() : i32 { return 55541; }

		template</ i32 X />
		fn Bar() : i32 { return X; }

		fn Foo() : i32
		{
			return Bar</ 1110 />(); // First template instantiation failed, but second successed.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 1110 )


def DirectFunctionTemplateParametersSet_Test4():
	c_program_text= """
		template</ type T />
		fn Bar() : i32 { return 55541; }

		template</ i32 X />
		fn Bar() : i32 { return X; }

		fn Foo() : i32
		{
			return Bar</ i32 />(); // Second template instantiation failed, but first successed.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 55541 )


def PreResolve_Test0():
	c_program_text= """
		type I= i32;
		namespace N
		{
			type I= i64;
			template</ type T />
			fn ToI( T& t ) : I { return I(t); }
		}

		fn Foo() : i64
		{
			return N::ToI( 45.3f ); // Must return i64 result
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 45 )


def PreResolve_Test1():
	c_program_text= """
		template</ type T /> struct Box{}
		namespace N
		{
			template</ type T /> struct Box{ T boxed; }

			template</ type T />
			fn Unbox( Box</ T /> & box ) : T& { return box.boxed; }  // N::Box should be visible here, not ::Box
		}

		fn Foo() : i32
		{
			var N::Box</ i32 /> box{ .boxed= 666541 };
			return N::Unbox(box);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 666541 )


def PreResolve_Test2():
	c_program_text= """
		template</ type T /> struct Box{}
		namespace N
		{
			template</ type T /> struct Box{ T boxed; }

			template</ type T />
			fn Unbox( Box</ T /> & box ) : T& { return box.boxed; }  // N::Box should be visible here, not ::Box
		}

		fn Foo() : i32
		{
			var Box</ i32 /> box{};
			return N::Unbox(box); // Error, epected N::Box</T/>, got ::Box</T/>
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 14 ) )


def PreResolve_Test4():
	c_program_text= """
		type I= i32;
		namespace N
		{
			type I= i64;
			template</ I val, type T />
			fn SetVal( T&mut t ) { t= val; }
		}

		fn Foo() : i64
		{
			var i64 mut x= zero_init;
			N::SetVal</ 874i64 />( x );  // N::I must be visible here as type of value-argument of template function.
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 874 )


def TemplateDependentFunctionTemplateArguments_Test0():
	c_program_text= """
		template</ type T />
		class Vector
		{
			template</ type F />
			fn emplace_back( F& f ){}

			fn emplace_back( T& t )
			{
				emplace_back( t );
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def TemplateDependentFunctionTemplateArguments_Test1():
	c_program_text= """
		template</ type T />
		class Vector
		{
			template</ type F />
			fn emplace_back( F& f ){}

			fn emplace_back( T& t )
			{
				emplace_back</T/>( t );
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def TemplateFunction_versus_TypesConversion_Test0():
	c_program_text= """
		struct A
		{
			fn conversion_constructor( i32 x ){}
		}
		template</ />
		fn Bar( i32 x ) : i32 { return 666; }
		fn Bar( A a ) : i32 { return 1111; }
		fn Foo()
		{
			halt if( Bar( 42 ) != 666 ); // Should call template function here, because for template functon call conversion not needed.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TemplateFunction_versus_TypesConversion_Test1():
	c_program_text= """
		struct StringView
		{
			template</ size_type S />
			fn conversion_constructor( [ char8, S ]& arr ) {}
		}

		template</ size_type S />
		fn Bar( [ char8, S ]& arr ) : i32 { return 147; }
		fn Bar( StringView sv ) : i32 { return 369; }

		fn Foo()
		{
			halt if( Bar( "wtf" ) != 147 ); // Should call template function here, because for template functon call conversion not needed.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TemplateFunction_versus_TypesConversion_Test2():
	c_program_text= """
		template</ size_type S />
		fn Bar( [ char8, S ]& arr ) : i32 { return 88885; }
		fn Bar( [ char8, 5 ]& arr ) : i32 { return 55558; }
		fn Foo()
		{
			halt if( Bar( "01234" ) != 55558 ); // Should call non-template function here, because it is more specialized.
			halt if( Bar( "210" ) != 88885 ); // Should call template function here.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TemplateFunction_UseLocalVariableForTemplateArgumentUsedAsReference():
	c_program_text= """
	fn Baz(i32& x){}
	template</i32 x/> fn Bar()
	{
		Baz(x); // Pass reference to argument "x". "x" must be global variable.
	}
	fn Foo()
	{
		auto some_local= 666;
		Bar</some_local/>();
	}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TemplateFunctionBuildTriggeredOnlyIfItIsSelected_Test0():
	c_program_text= """
		template</type T/> fn Bar( T & mut t ) {}
		template</type T/> fn Bar( T &imut t ) { static_assert( false, "Should not trigger build of this function!" ); }
		fn Foo()
		{
			auto mut x= 0;
			Bar(x); // Two versions may be called - with "imut" and "mut" argument. But only version with "mut" param is selected and its build dtriggered - since it is more specialized.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TemplateFunctionBuildTriggeredOnlyIfItIsSelected_Test1():
	c_program_text= """
		template</type T/> fn Bar( T & t ) { static_assert( false, "Should not trigger build of this function!" ); }
		template</type T, size_type S/> fn Bar( [ T, S ] & t ) {}
		fn Foo()
		{
			var [ i32, 16 ] arr= zero_init;
			Bar(arr);  // Two versions may be called, but only one is selected and its build is triggered - more specialized one.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TemplateFunctionBuildTriggeredOnlyIfItIsSelected_Test2():
	c_program_text= """
		struct A
		{
			template</size_type S/> fn conversion_constructor( [char8, S]& str ) { static_assert( false, "Should not trigger build of this function!" ); }
		}
		struct B
		{
			op+=( mut this, A a ) { halt; }
			template</size_type S/> op+=( mut this, [char8, S]& str ) {}
		}
		fn Foo()
		{
			var B mut b;
			b += "lol"; // Should select second variant of "+=" operator since it requires no type conversion. Conversion constructor build for "A" should not be triggered.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def FunctionTemplate_WithZeroTemplateParams_Test0():
	c_program_text= """
		// Function template with zero params still must be instantiated in order to be compiled.
		template<//> fn Foo()
		{
			CallUnknownFunction(); // Should not get error here, because it is not compiled.
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTemplate_WithZeroTemplateParams_Test1():
	c_program_text= """
		template<//> fn Bar() : i32
		{
			return 654;
		}
		fn Foo() : i32
		{
			return Bar(); // Instantiate function template with zero template params by calling it.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_res= tests_lib.run_function( "_Z3Foov" )
	assert( call_res == 654 )


def FunctionTemplate_WithZeroTemplateParams_Test2():
	c_program_text= """
		template<//> fn Bar( i32 x ) : i32
		{
			return x - 66;
		}
		fn Foo() : i32
		{
			return Bar( 766 ); // Instantiate function template with zero template params and non-zero function params by calling it.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_res= tests_lib.run_function( "_Z3Foov" )
	assert( call_res == 700 )


def FunctionTemplate_WithZeroTemplateParams_Test3():
	c_program_text= """
		template<//> fn Bar() : i32
		{
			return 354;
		}
		fn Foo() : i32
		{
			return Bar<//>(); // Instantiate function template with zero template params by providing template args.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_res= tests_lib.run_function( "_Z3Foov" )
	assert( call_res == 354 )


def FunctionTemplate_WithZeroTemplateParams_Test4():
	c_program_text= """
		template<//> fn Bar( i32 x ) : i32
		{
			return x - 77;
		}
		fn Foo() : i32
		{
			return Bar<//>( 377 ); // Instantiate function template with zero template params and non-zero function params by providing template args.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_res= tests_lib.run_function( "_Z3Foov" )
	assert( call_res == 300 )


def FunctionTemplate_WithZeroTemplateParams_Test5():
	c_program_text= """
		template<//> fn Bar() : i32
		{
			return 776655;
		}
		fn Foo() : i32
		{
			var (fn() : i32) ptr= Bar; // Instantiate function template with zero template params by initialization of function pointer.
			return ptr();
		}
	"""
	tests_lib.build_program( c_program_text )
	call_res= tests_lib.run_function( "_Z3Foov" )
	assert( call_res == 776655 )


def FunctionTemplate_WithZeroTemplateParams_Test6():
	c_program_text= """
		template<//> fn Bar( i32 x ) : i32
		{
			return x - 44;
		}
		fn Foo() : i32
		{
			var (fn( i32 x ) : i32 ) ptr( Bar<//> );
			return ptr( 944 ); // Instantiate function template with zero template params by initialization of function pointer.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_res= tests_lib.run_function( "_Z3Foov" )
	assert( call_res == 900 )


def FunctionTemplate_WithZeroTemplateParams_Test7():
	c_program_text= """
		template</type T/>
		struct Box
		{
			T t;

			type ThisType= Box</T/>;

			// This function causes infinite recurion of this template.
			// Make it lazy via zero-params template in order to make this function lazy and prevent recurison.
			template<//> fn MakeBoxBox(this) : Box</ ThisType />
			{
				var Box</ ThisType /> b{ .t= this };
				return b;
			}
		}
		type IntBox= Box</i32/>;
		fn BoxBox( IntBox& b ) : Box</IntBox/>
		{
			return b.MakeBoxBox();
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTemplate_WithZeroTemplateParams_Test8():
	c_program_text= """
		template<//> fn Foo() : i32 { return 1323; }
		fn Bar() { Foo(); } // Instantiate zero-params template function in order to generate its code.
	"""
	tests_lib.build_program( c_program_text )
	call_res= tests_lib.run_function( "_Z3FooIEvv" ) # Call template function itself.
	assert( call_res == 1323 )
