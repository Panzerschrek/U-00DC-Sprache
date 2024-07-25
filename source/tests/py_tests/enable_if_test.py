from py_tests_common import *


def EnableIfDeclaration_Test0():
	c_program_text= """
		fn enable_if( true ) Foo() {}
	"""
	tests_lib.build_program( c_program_text )


def EnableIfDeclaration_Test1():
	c_program_text= """
		class C polymorph
		{
			fn virtual enable_if( true ) Foo( this ) {}
		}
	"""
	tests_lib.build_program( c_program_text )


def EnableIfDeclaration_Test2():
	c_program_text= """
		class I interface
		{
			fn virtual pure enable_if( true ) Foo( this );
		}
	"""
	tests_lib.build_program( c_program_text )


def DisabledFunction_Test0():
	c_program_text= """
	fn enable_if( false ) Bar(){}
	fn Foo()
	{
		Bar();
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 5 )


def DisabledFunction_Test1():
	c_program_text= """
		fn Bar( i32 &imut x ) : i32 { return 656; }
		// enalbe_if condition here is always false, so, second function will not be generated and first function will be called.
		fn enable_if( typeinfo</u32/>.size_of != typeinfo</i32/>.size_of ) Bar( i32 &mut x ) : i32 { return 111; }
		fn Foo() : i32
		{
			auto mut x= 0;
			return Bar( x );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 656 )


def DisabledFunction_Test2():
	c_program_text= """
		class S
		{
			fn enable_if( false ) constructor( S &imut other ){}
		}
		fn Foo()
		{
			var S s;
			var S s_copy(s); // Error, class have no copy constructor
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 9 )


def DisabledFunction_Test3():
	c_program_text= """
		template</ type T />
		class S
		{
			// Copy constructor enabled only if template argument type is copy constructible.
			fn enable_if( typeinfo</T/>.is_copy_constructible ) constructor( S</T/> &imut other ){}
		}

		struct A{}
		class  B{}

		static_assert(  typeinfo</ S</A/> />.is_copy_constructible );
		static_assert( !typeinfo</ S</B/> />.is_copy_constructible );
	"""
	tests_lib.build_program( c_program_text )


def DisabledFunction_Test4():
	c_program_text= """
		template</ bool use_custom_copy_constructor />
		struct S
		{
			i32 x;

			type SelfType= S</use_custom_copy_constructor/>;

			// if copy constructor disabled, default copy constructor should be generated.
			fn enable_if(use_custom_copy_constructor) constructor( SelfType &imut other )
				( x(0) ) // HAHAHA - custom copy constructor works wrong!
			{}
		}

		fn Foo()
		{
			var S</false/> sf0{ .x= 99 };
			var S</false/> sf1(sf0); // Right copy

			var S</true/> st0{ .x= 6666 };
			var S</true/> st1(st0); // Wrong copy

			halt if( sf0.x != 99 );
			halt if( sf1.x != 99 );
			halt if( st0.x != 6666 );
			halt if( st1.x != 0 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DisabledTemplateFunction_Test0():
	c_program_text= """
		template</type T/>
		fn enable_if( false ) Foo(T x){}

		fn Bar()
		{
			Foo(0.5f); // Error, function is disabled
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 7 ) )


def DisabledTemplateFunction_Test1():
	c_program_text= """
		template</type T/>
		fn enable_if( false ) Foo(T x){}

		fn Bar()
		{
			Foo</i32/>(345); // Error, function is disabled
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 7 ) )


def DisabledTemplateFunction_Test2():
	c_program_text= """
		template</type T/>
		fn enable_if( false ) Foo(T x){}

		fn Bar()
		{
			var (fn(f64 x)) ptr( Foo</f64/> ); // Error, function is disabled
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 7 ) )


def DisabledTemplateFunction_Test3():
	c_program_text= """
		template</bool enable/>
		fn enable_if( enable ) Foo(){}

		fn Bar()
		{
			Foo</false/>(); // Error, function is disabled.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 7 ) )


def DisabledTemplateFunction_Test4():
	c_program_text= """
		template</bool enable/>
		fn enable_if( enable ) Foo(){}

		fn Bar()
		{
			Foo</true/>(); // Ok - function is enabled
		}
	"""
	tests_lib.build_program( c_program_text )


def EnabledFunction_Test0():
	c_program_text= """
		fn enable_if(true) Foo() : i32 {}   // If condition is true, body compiled.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NoReturnInFunctionReturningNonVoid" )
	assert( errors_list[0].src_loc.line == 2 )


def EnabledFunction_Test1():
	c_program_text= """
		class S
		{
			fn enable_if( true ) constructor( S &imut other ){}
		}
		fn Foo()
		{
			var S s;
			var S s_copy(s); // Ok, class is copyable.
		}
	"""
	tests_lib.build_program( c_program_text )


def DiabledFunctionContentNotCompiled_Test0():
	c_program_text= """
		fn enable_if( false ) Bar( UnknownType arg );  // Ok, arguments may be absolutely invalid for disabled function
	"""
	tests_lib.build_program( c_program_text )


def DiabledFunctionContentNotCompiled_Test1():
	c_program_text= """
		fn enable_if( false ) Bar() : UnknownType&;  // Ok, return type may be absolutely invalid for disabled function
	"""
	tests_lib.build_program( c_program_text )


def DiabledFunctionContentNotCompiled_Test2():
	c_program_text= """
		fn enable_if( false ) Bar() : i32
		{
			auto x= unknown_var;
			CallUnknownFunction( With, unknown * arguments ).and_use_unknown_member;
			// no return here
		}  // Ok,body may be absolutely invalid for disabled function
	"""
	tests_lib.build_program( c_program_text )


def TypesMismatch_ForEnableIf_Test0():
	c_program_text= """
	fn enable_if(42) Foo(); // Expected "bool", got "i32"
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 2 )


def ExpectedConstantExpression_ForEnableIf_Test0():
	c_program_text= """
	fn Bar() : bool;
	fn enable_if(Bar()) Foo(); // Result of "Bar" call is not constexpr
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedConstantExpression" )
	assert( errors_list[0].src_loc.line == 3 )


def DifferentFunctionImplementations_UsingEnableIf_Test0():
	c_program_text= """
		fn enable_if( false ) Foo() : i32{ return 65757; }
		fn enable_if( true  ) Foo() : i32{ return 546198; }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 546198 )


def DifferentFunctionImplementations_UsingEnableIf_Test1():
	c_program_text= """
		template</ u32 impl_index />
		struct S
		{
			fn enable_if( impl_index == 3u ) Get() : i32{ return 333; }
			fn enable_if( impl_index == 7u ) Get() : i32{ return 777; }
		}

		fn Three() : i32{ return S</3u/>::Get(); }
		fn Seven() : i32{ return S</7u/>::Get(); }
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z5Threev" ) == 333 )
	assert( tests_lib.run_function( "_Z5Sevenv" ) == 777 )


def EnableIf_ForPrototypeAndBody_Test0():
	c_program_text= """
		// Prototype have no "enable_if", but there is multiple bodies for this prototype with "enable_if".
		fn Foo() : i32;
		fn enable_if( false ) Foo() : i32 { return 11111; }
		fn enable_if( true  ) Foo() : i32 { return 22222; }
		fn enable_if( false ) Foo() : i32 { return 33333; }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 22222 )


def EnableIf_ForPrototypeAndBody_Test1():
	c_program_text= """
		// More than one body enabled.
		fn Foo() : i32;
		fn enable_if( true  ) Foo() : i32 { return 11111; }
		fn enable_if( false ) Foo() : i32 { return 22222; }
		fn enable_if( true  ) Foo() : i32 { return 33333; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionBodyDuplication" )
	assert( errors_list[0].src_loc.line == 4 or errors_list[0].src_loc.line == 6 )


def EnableIf_ForPrototypeAndBody_Test2():
	c_program_text= """
		// Have muliple prorotypes, but only one enabled. Body have no "enable_if".
		fn enable_if( false ) Foo() : i32;
		fn enable_if( true  ) Foo() : i32;
		fn enable_if( false ) Foo() : i32;
		fn Foo() : i32 { return 4321; }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 4321 )


def EnableIf_ForPrototypeAndBody_Test3():
	c_program_text= """
		// Have prototype and single disabled body.
		fn Foo() : i32;
		fn enable_if( false ) Foo() : i32 { lol + kek; } // Body contains errors, but it not compiled.
	"""
	tests_lib.build_program( c_program_text )


def EnableIf_ForPrototypeAndBody_Test4():
	c_program_text= """
		// "enable_if" both for body and prototype.
		fn enable_if( true ) Foo() : i32;
		fn enable_if( true )Foo() : i32 { return 12481632; }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 12481632 )
