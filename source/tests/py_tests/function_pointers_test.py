from py_tests_common import *


def FunctionTypeDeclaration_Test0():
	c_program_text= """
		// As global variable
		var fn( i32 x, i32 y ) constexpr pointer_to_some_function= zero_init;
	"""
	tests_lib.build_program( c_program_text )


def FunctionTypeDeclaration_Test1():
	c_program_text= """
		// As local variable
		fn Foo()
		{
			var fn( i32 x, i32 y ) : i32 pointer_to_some_function= zero_init;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTypeDeclaration_Test2():
	c_program_text= """
		// As local variable
		fn Foo()
		{
			// With brackets
			var ( fn( i32 x, i32 y ) : i32& ) pointer_to_some_function= zero_init;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTypeDeclaration_Test3():
	c_program_text= """
		// In type alias
		type BinaryIntFunction= fn( i32 a, i32 b ) : i32;
		fn Foo( BinaryIntFunction a )
		{}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTypeDeclaration_Test4():
	c_program_text= """
		// As function argument. Also unsafe
		fn Foo( fn() unsafe arg )
		{}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTypeDeclaration_Test5():
	c_program_text= """
		// As field. Also pollution list
		struct F{ i32& r; }
		struct S
		{
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			( fn( F& mut f, i32& x ) @(pollution) ) some_function;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPoinerInitialization_Test0():
	c_program_text= """
		// Zero initializer
		fn Foo()
		{
			var ( fn() ) foo= zero_init;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPoinerInitialization_Test1():
	c_program_text= """
		// Expression initializer and single function
		fn Foo()
		{
			var ( fn() ) foo= Foo;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPoinerInitialization_Test2():
	c_program_text= """
		// Expression initializer and multiple functions.
		fn a( i32 x ){}
		fn a( f32 x ){}
		fn Foo()
		{
			var ( fn( i32 x ) ) int_func= a;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPoinerInitialization_Test3():
	c_program_text= """
		// Constructor initializer and multiple functions.
		fn a( i32 x ){}
		fn a( f32 x ){}
		fn Foo()
		{
			var ( fn( i32 x ) ) int_func( a );
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPoinerInitialization_Test4():
	c_program_text= """
		// Initialize, using other function pointer.
		fn a( i32 x ){}
		fn a( f32 x ){}
		fn Foo()
		{
			var ( fn( i32 x ) ) mut int_func_0( a );
			var ( fn( i32 x ) ) int_func_1= int_func_0;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPoinerInitialization_Test5():
	c_program_text= """
		// Initialize function pointer, using method.
		struct S
		{
			i32 x;
			fn GetX( this ) : i32 { return x; }
		}
		fn Foo()
		{
			var ( fn( S& s ) : i32 ) getter= S::GetX;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPoinerInitialization_Test5():
	c_program_text= """
		fn a( i32& mut x ) : i32{ return 666; }
		fn a( i32&imut x ) : i32{ return 999; }

		fn Foo() : i32
		{
			var ( fn( i32& mut x ) : i32 ) ptr= a;  // Must select function with same type as in pointer ( with mutable argument ).
			auto mut x= 0;
			return ptr(x);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 666 )


def FunctionPointerCall_Test0():
	c_program_text= """
		fn Bar() : i32 { return 666; }
		fn Foo() : i32
		{
			var ( fn() : i32 ) mut triple_six= Bar;
			return triple_six();
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 666 )


def FunctionPointerCall_Test1():
	c_program_text= """
		fn DoubleIt( i32 x ) : i32 { return x + x; }
		fn Foo( i32 arg ) : i32
		{
			var ( fn( i32 x ) : i32 ) x2= DoubleIt;
			return x2(arg);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Fooi", 8956 )
	assert( call_result == 8956 * 2 )


def FunctionPointerCall_Test2():
	c_program_text= """
		fn AddAndDevide( f32 x, f32 y, i32 z ) : f32
		{
			return ( x + y ) / f32(z);
		}
		fn Foo() : f32
		{
			var ( fn( f32 x, f32 y, i32 z ) : f32 ) mut fff= AddAndDevide;
			return fff( 7.25f, 82.75f, 9 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ( 7.25 + 82.75 ) / 9.0 )


def FunctionPointerCall_Test3():
	c_program_text= """
		fn Modify( i32& mut x ){ x= 555554; }
		fn Foo() : i32
		{
			auto mut x= 0;
			var fn( i32&mut x ) mutator= Modify;
			mutator(x);
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 555554 )


def AutoVariableInitialization_UsingFunctionPointer_Test0():
	c_program_text= """
		fn Get42() : i32 { return 42; }
		fn Foo() : i32
		{
			auto foo= ( fn() : i32 )( Get42 );
			return foo();
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 42 )


def FunctionPointerEqualityComparision_Test0():
	c_program_text= """
		fn a(){}
		fn b(){}
		fn Foo()
		{
			// Non-constexpr compare
			var (fn()) mut ptr_0= a;
			var (fn()) mut ptr_1= b;
			halt if( ptr_0 != ptr_0 );
			halt if( ptr_0 == ptr_1 );
			halt if( !( ptr_0 == ptr_0 ) );
			halt if( !( ptr_1 != ptr_0 ) );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def FunctionPointerEqualityComparision_Test1():
	c_program_text= """
		fn a(){}
		fn b(){}
		fn Foo()
		{
			// Constexpr compare.
			var (fn()) constexpr ptr_0= a;
			var (fn()) constexpr ptr_1= b;
			static_assert( ptr_0 == ptr_0 );
			static_assert( !( ptr_0 != ptr_0 ) );
			static_assert( ptr_0 != ptr_1 );
			static_assert( !( ptr_1 == ptr_0 ) );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def FunctionsPoitersAssignment_Test0():
	c_program_text= """
		fn a() : i32 { return 55; }
		fn b() : i32 { return 66; }
		fn Foo() : i32
		{
			var (fn() : i32) mut ptr_0= a;
			var (fn() : i32) mut ptr_1= b;
			ptr_0= ptr_1;
			return ptr_0();
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 66 )


def FunctionsPoitersAssignment_Test1():
	c_program_text= """
		fn a() : i32 { return 99985; }
		fn Foo() : i32
		{
			var (fn() : i32) mut ptr= zero_init;
			ptr= (fn() : i32)(a);  // Currently required explicit function to pointer conversion.
			return ptr();
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 99985 )


def FunctionPointersConversions_Test0():
	c_program_text= """
		fn a( i32 &imut x ) : i32 { return x; }
		fn Foo() : i32
		{
			var ( fn( i32 &mut x ) : i32 ) mut ptr= a;   // Must convert immutable reference argument to mutable reference argument.
			auto mut x= 998552;
			return ptr( x );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 998552 )


def FunctionPointersConversions_Test1():
	c_program_text= """
		fn a( i32 &mut x ) : i32 &mut { return x; }
		fn Foo() : i32
		{
			var ( fn( i32 &mut x ) : i32 &imut ) mut ptr= a;   // Must convert mutable reference return value to immutable reference return value.
			auto mut x= 55585;
			return ptr( x );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 55585 )


def FunctionPointersConversions_Test2():
	c_program_text= """
		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		fn RetFirst( i32& a, i32& b ) : i32& @(return_references)
		{
			return a;
		}

		var [ [ char8, 2 ], 2 ] return_references_both[ "0_", "1_" ];
		type RetBothType= fn( i32& a, i32& b ) : i32& @(return_references_both);

		fn Foo() : i32
		{
			var RetBothType mut ptr= RetFirst;   // Must convert function, returning less references, to function, returning more references.
			return ptr( 666, 999 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 666 )


def FunctionPointersConversions_Test3():
	c_program_text= """
		struct S
		{
			i32 &imut r;
		}

		fn DoNotPollution( S &mut s, i32& a ) {}

		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		type DoPollution= fn( S &mut s, i32& a ) @(pollution);

		fn Foo()
		{
			var DoPollution mut ptr= DoNotPollution;   // Must convert function, which does not pollution to function, which does pollution.

			var i32 x= 0, y= 0;
			var S mut s{ .r= x };
			ptr( s, y );
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPointersConversions_Test4():
	c_program_text= """
		type UnsafeFunction= fn() unsafe;

		fn SafeFunction(){}

		fn Foo()
		{
			var UnsafeFunction unsafe_function= SafeFunction;  // Must convert safe function to unsafe.
			unsafe{  unsafe_function();  }
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPointersConversions_Test5():
	c_program_text= """
		type SafeFunctionType= fn();
		type UnsafeFunctionType= fn() unsafe;

		fn SafeFunction(){}

		fn Foo()
		{
			var SafeFunctionType safe_function= SafeFunction;
			var UnsafeFunctionType unsafe_function= safe_function; // Must convert function pointer here.
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPointerAsFunctionArgument_Test0():
	c_program_text= """
		type FunType= fn() : i32;

		// Function pointer as value-argument.
		fn DoubleIt( FunType fun ) : i32
		{
			return fun() * 2;
		}

		fn Get1009() : i32 { return 1009; }

		fn Foo() : i32
		{
			// Call, using value without conversion.
			return DoubleIt( FunType( Get1009 ) );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 1009 * 2 )


def FunctionPointerAsFunctionArgument_Test1():
	c_program_text= """
		type FunType= fn() : i32;

		// Function pointer as reference-argument.
		fn DoubleIt( FunType& fun ) : i32
		{
			return fun() * 2;
		}

		fn Get1009() : i32 { return 1009; }

		fn Foo() : i32
		{
			// Call, using value without conversion.
			return DoubleIt( FunType( Get1009 ) );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 1009 * 2 )


def FunctionPointerAsReturnValue_Test0():
	c_program_text= """
		type IntReturner= fn() : i32;

		fn GetX() : i32{ return 999854; }

		fn GetFnPtr() : IntReturner
		{
			return IntReturner(GetX);
		}

		fn Foo() : i32
		{
			return GetFnPtr()();
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 999854 )


def FunctionPointerToTemplateFunction_Test0():
	c_program_text= """
		template</ type T />
		fn GetPi() : T { return T(3.1415926535); }

		fn Foo() : i32
		{
			var (fn() : f32) ptr_to_fn= GetPi</f32/>;   // Manully specialize all template parameters.
			return i32( ptr_to_fn() * 100.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 314 )


def FunctionPointerToTemplateFunction_Test1():
	c_program_text= """
		template<//>
		fn GetE() : f32 { return 2.718281828f; }

		fn Foo() : i32
		{
			var (fn() : f32) mut ptr_to_fn= GetE;   // Take pointer to function template with zero template parameters.
			return i32( ptr_to_fn() * 1000.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 2718 )


def FunctionPointerPointedToNonvirtualFunction_Test0():
	c_program_text= """
		class A polymorph
		{
			fn virtual Foo( this ) : i32 { return 5555; }
		}

		class B : A
		{
			fn virtual override Foo( this ) : i32 { return 6666; }
		}

		fn Foo() : i32
		{
			var B b;
			var (fn( A& a ) : i32) mut ptr= cast_ref</A/>(b).Foo;   // Must take here A::Foo, not B::Foo
			return ptr(b);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5555 )


def ReferencePollution_ForFunctionPointer_Test0():
	c_program_text= """
		struct S{ i32& r; }

		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		type DoPollutionType= fn( S& mut s, i32& x ) @(pollution);

		fn DoNotPollution( S&mut s, i32& x ){}

		fn Foo()
		{
			var DoPollutionType ptr= DoNotPollution;

			var i32 x= 0, mut y= 0;
			var S mut s{ .r= x };
			ptr(s, y);
			++y;  // Error, 'y' have reference inside 's'.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 16 )


def ReturnReferenceTags_ForFunctionPointers_Test0():
	c_program_text= """
		type RetBothType= fn( i32& a, i32& b ) : i32&;

		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		fn RetFirst( i32& a, i32& b ) : i32& @(return_references)
		{
			return a;
		}

		fn Foo()
		{
			var RetBothType ptr= RetFirst;

			var i32 mut a= 0, mut b= 0;
			auto& ref= ptr(a, b);
			++b; // Error, 'b' have reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 16 )


def FunctionPointerAsSpecializedTemplateParameter_Test0():
	c_program_text= """
		template</ type T />
		struct ExtractReturnValueType</ fn() : T />
		{
			type ReturnValueType= T;
		}

		fn Foo() : i32
		{
			var ExtractReturnValueType</ fn() : i32 />::ReturnValueType must_be_int= 6668542;
			return must_be_int;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 6668542 )


def FunctionPointerAsSpecializedTemplateParameter_Test1():
	c_program_text= """
		template</ type T />
		struct ExtractReturnValueType</ fn() : T />
		{
			type ReturnValueType= T;
		}

		type FnType= fn() : i32;

		fn Foo() : i32
		{
			var ExtractReturnValueType</ FnType />::ReturnValueType must_be_int= 365244;  // Works, if function type alias passed.
			return must_be_int;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 365244 )


def FunctionPointerAsSpecializedTemplateParameter_Test2():
	c_program_text= """
		template</ type T />
		struct ExtractFirstArgumentType</ fn(T x) />
		{
			type FirstArgumentType= T;
		}

		fn Foo() : f32
		{
			var ExtractFirstArgumentType</ fn( f32 x ) />::FirstArgumentType must_be_float= 5684.5f;
			return must_be_float;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5684.5 )


def MutabilityModifiersInFunctionTypeTest():
	c_program_text= """
		template</ type A /> struct is_same_type_impl
		{
			template</ type B /> struct same</ B /> { auto constexpr value= false; }
			template</ /> struct same</ A /> { auto constexpr value= true; }
		}

		// Mutability for value param doesn't matter.
		static_assert( is_same_type_impl</ fn(i32 mut x) />::same</ fn(i32 imut x) />::value );
		static_assert( is_same_type_impl</ fn(i32     x) />::same</ fn(i32 imut x) />::value );

		// Mutability for reference param matter.
		static_assert( !is_same_type_impl</ fn(i32 &mut x) />::same</ fn(i32 &imut x) />::value );
		static_assert(  is_same_type_impl</ fn(i32 &    x) />::same</ fn(i32 &imut x) />::value );

		// Mutability for return value matter.
		static_assert( !is_same_type_impl</ fn() : i32 &mut />::same</ fn() : i32 &imut />::value );
		static_assert(  is_same_type_impl</ fn() : i32 &    />::same</ fn() : i32 &imut />::value );
	"""
	tests_lib.build_program( c_program_text )
