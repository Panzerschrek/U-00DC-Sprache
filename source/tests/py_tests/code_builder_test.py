from py_tests_common import *


def SimpliestTest():
	tests_lib.build_program( "fn GetTrue() : bool { return true; }" )
	call_result= tests_lib.run_function( "_Z7GetTruev" )
	assert( call_result == True )


def SimplePassArgumentTest():
	tests_lib.build_program( "fn GetBool(bool b) : bool { return b; }" )
	assert( tests_lib.run_function( "_Z7GetBoolb", True  ) == True )
	assert( tests_lib.run_function( "_Z7GetBoolb", False ) == False )


def OkTest():
	tests_lib.build_program( "fn Foo( i32 x, i32 y ) : f32 { return f32(x * y) + 0.5f; }" )

	call_result= tests_lib.run_function( "_Z3Fooii", 3, 7 )
	assert( call_result == ( 3 * 7 ) + 0.50 )


def ErrorsTest0():
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( "fn Foo() : i32 {}" ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NoReturnInFunctionReturningNonVoid" )
	assert( errors_list[0].src_loc.line == 1 )


def ErrorsTest1():
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( "fn Foo() : UnknownType {}" ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].src_loc.line == 1 )


def ZeroInitializerForStructWithReferenceTest():
	c_program_text= """
	struct S{ i32& r; }
	fn Foo()
	{
		var S s= zero_init;
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsupportedInitializerForReference" )
	assert( errors_list[0].src_loc.line == 5 )


def VoidTypeReferenceMustBeReturned_Test():
	c_program_text= """
	fn Bar() : void&;
	fn Foo() : void&
	{
		if(false){ return Bar(); }
		Baz();
		// Does not return.
	}
	fn Baz();
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NoReturnInFunctionReturningNonVoid" )
	assert( errors_list[0].src_loc.line == 8 )


def VoidTypeReference_Test0():
	c_program_text= """
	fn Foo( void& v ) {}    // void type for reference-arg.
	"""
	tests_lib.build_program( c_program_text )


def VoidTypeReference_Test1():
	c_program_text= """
	fn Foo() : void&;    // void for returning reference.
	"""
	tests_lib.build_program( c_program_text )


def VoidTypeReference_Test2():
	c_program_text= """
	fn Foo() : void&;
	fn Bar()
	{
		var void &v= Foo();  // save returning void reference, using "var".
	}
	"""
	tests_lib.build_program( c_program_text )


def VoidTypeReference_Test3():
	c_program_text= """
	fn Foo() : void&;
	fn Bar()
	{
		auto &v= Foo();  // save returning void reference, using "auto".
	}
	"""
	tests_lib.build_program( c_program_text )


def VoidTypeReference_Test4():
	c_program_text= """
	struct S{ void& v; }   // void for reference field.
	"""
	tests_lib.build_program( c_program_text )


def VoidTypeReference_Test5():
	c_program_text= """
	fn Foo() : void&;
	fn Bar( void& v );
	fn Baz()
	{
		Bar(Foo()); // Pass void-reference result from one function to another.
	}
	"""
	tests_lib.build_program( c_program_text )


def DeepExpressionsCompilationTest0():
	c_program_text= """
		fn Foo() : i32
		{
			// 64 elements in chain of binary operators is equivalent of expression, like
			// add( 1, add( 1, add( 1, etc ) ) )
			return
				1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 +
				1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 +
				1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 +
				1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 +
				1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 +
				1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 +
				1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 +
				1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 ;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 64 )


def DeepExpressionsCompilationTest1():
	c_program_text= """
		fn add( i32 x, i32 y ) : i32 { return x + y; }
		fn add( f32 x, f32 y ) : f32 { return x + y; }
		fn add( bool a, bool b, bool c ) : bool { return a | b | c; }
		fn Foo() : i32
		{
			return
				add(
					add(
						add(
							add(
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  ),
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  )
								),
							add(
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  ),
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  )
								)
							),
						add(
							add(
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  ),
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  )
								),
							add(
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  ),
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  )
								)
							)
						),
					add(
						add(
							add(
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  ),
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  )
								),
							add(
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  ),
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  )
								)
							),
						add(
							add(
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  ),
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  )
								),
							add(
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  ),
								add(  add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) ), add( add(add(1,1),add(1,1)), add(add(1,1),add(1,1)) )  )
								)
							)
						)
					);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 256 )


def DeepExpressionsCompilationTest2():
	c_program_text= """
		fn add( i32 x, i32 y ) : i32 { return x + y; }
		fn add( f32 x, f32 y ) : f32 { return x + y; }
		fn add( bool a, bool b, bool c ) : bool { return a | b | c; }
		fn Foo() : i32
		{
			return
				add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, add( 1, 1 ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 64 )


def FunctionContextForTypePreparing_Test0():
	c_program_text= """
	struct S { i32& x; }
	fn Foo()
	{
		auto constexpr x = 4;
		var S constexpr s{ .x= x };
		var [ i32, s.x ] arr= zero_init;
		static_assert( typeinfo</ [ i32, s.x ] />.element_count == size_type(4) );
	}
	"""
	tests_lib.build_program( c_program_text )


def Int128_Test0():
	c_program_text= """
	fn I128Add( i128 x, i128 y ) : i128
	{
		return x + y;
	}
	fn I128Mul( i128 x, i128 y ) : i128
	{
		return x * y;
	}
	fn I128Div( i128 x, i128 y ) : i128
	{
		return x / y;
	}
	fn I128Rem( i128 x, i128 y ) : i128
	{
		return x % y;
	}

	fn Foo() : i32
	{
		var i128 mut x(1), y(-8), z(8657), w(12), f= 1000000000i128;
		return i32( I128Rem( I128Add( I128Mul(x, y), I128Div(z, w) ), f ) );
	}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ( 1 * (-8) ) + int( 8657 / 12 ) )


def Int128_Test1():
	c_program_text= """
	fn constexpr I128Add( i128 x, i128 y ) : i128
	{
		return x + y;
	}
	fn constexpr I128Mul( i128 x, i128 y ) : i128
	{
		return x * y;
	}
	fn constexpr I128Div( i128 x, i128 y ) : i128
	{
		return x / y;
	}
	fn constexpr I128Rem( i128 x, i128 y ) : i128
	{
		return x % y;
	}

	fn Foo() : i32
	{
		var i128 x(1), y(-8), z(8657), w(12), f= 1000000000i128;
		return i32( I128Rem( I128Add( I128Mul(x, y), I128Div(z, w) ), f ) );
	}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ( 1 * (-8) ) + int( 8657 / 12 ) )


def Comments_Test0():
	c_program_text= """
	fn Foo() : i32
	{
		// Singleline comment.
		return 0;
	}
	"""
	tests_lib.build_program( c_program_text )


def Comments_Test1():
	c_program_text= """
	fn Foo() : i32
	{
		/* Multi
		line
		comment
		.
		*/
		return 0;
	}
	"""
	tests_lib.build_program( c_program_text )


def Comments_Test2():
	c_program_text= """
	fn Foo() : i32
	{
		/*
			/* Multi
			line
			comment
			. */
			Inside multiline
			comment
		*/
		return 0;
	}
	"""
	tests_lib.build_program( c_program_text )


def StaticAssertMessage_Test0():
	c_program_text= """
		static_assert( false, "Lorem ipsum" );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "StaticAssertionFailed" )
	assert( errors_list[0].src_loc.line == 2 )
	assert( errors_list[0].text.find( "Lorem ipsum" ) != -1 )


def RecursiveNumericTypeTemplate_Test0():
	# This test causes recursive instatitation of a large count of type templates.
	# Such cases should be handled properly without quadratic complexity.
	c_program_text= """
		template</size_type S/> struct Some { type Next= Some</S - 1s/>; }
		template<//> struct Some</0s/> {}

		type SomeLarge= Some</16384s/>;
	"""
	tests_lib.build_program( c_program_text )
