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
	assert( errors_list[0].file_pos.line == 5 )


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
	assert( errors_list[0].file_pos.line == 9 )


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


def EnabledFunction_Test0():
	c_program_text= """
		fn enable_if(true) Foo() : i32 {}   // If condition is true, body compiled.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NoReturnInFunctionReturningNonVoid" )
	assert( errors_list[0].file_pos.line == 2 )


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

