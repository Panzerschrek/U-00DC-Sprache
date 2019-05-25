from py_tests_common import *


def OkTest():
	tests_lib.build_program( "fn Foo( i32 x, i32 y ) : f32 { return f32(x * y) + 0.5f; }" )

	call_result= tests_lib.run_function( "_Z3Fooii", 3, 7 )
	assert( call_result == ( 3 * 7 ) + 0.50 )


def ErrorsTest():
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( "fn Foo() : i32 {}" ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NoReturnInFunctionReturningNonVoid" )
	assert( errors_list[0].file_pos.line == 1 )


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
	assert( errors_list[0].file_pos.line == 5 )


def VoidTypeReferenceMustBeReturned_Test():
	c_program_text= """
	fn Bar() : void&;
	fn Foo() : void&
	{
		if(false){ return Bar(); }
		2 + 2;
		// Does not return.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NoReturnInFunctionReturningNonVoid" )
	assert( errors_list[0].file_pos.line == 8 )


def VoidTypeIsIncomplete_Test0():
	c_program_text= """
	fn Foo()
	{
		var void v; // void variable
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingIncompleteType" )
	assert( errors_list[0].file_pos.line == 4 )


def VoidTypeIsIncomplete_Test1():
	c_program_text= """
	fn Bar(){}
	fn Foo()
	{
		auto v= Bar(); // void auto variable
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingIncompleteType" )
	assert( errors_list[0].file_pos.line == 5 )


def VoidTypeIsIncomplete_Test2():
	c_program_text= """
	struct S{ void v; } // void struct field
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingIncompleteType" )
	assert( errors_list[0].file_pos.line == 2 )


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


def CastToVoidReference_Test0():
	c_program_text= """
		fn Bar( void& v );
		fn Foo()
		{
			var i32 x= 0;
			Bar(x); // Cast to void reference in call (imut to imut).
		}
	"""
	tests_lib.build_program( c_program_text )


def CastToVoidReference_Test1():
	c_program_text= """
		fn Bar( void& v );
		fn Foo()
		{
			var i32 mut x= 0;
			Bar(x); // Cast to void reference in call (mut to imut).
		}
	"""
	tests_lib.build_program( c_program_text )


def CastToVoidReference_Test2():
	c_program_text= """
		fn Bar( void &mut v );
		fn Foo()
		{
			var i32 mut x= 0;
			Bar(x); // Cast to void reference in call (mut to mut).
		}
	"""
	tests_lib.build_program( c_program_text )


def CastToVoidReference_Test3():
	c_program_text= """
		fn Bar( void& v );
		fn Foo()
		{
			Bar(42); // Cast value to void reference in call (imut to imut).
		}
	"""
	tests_lib.build_program( c_program_text )


def CastToVoidReference_Test4():
	c_program_text= """
		fn ToVoid( i32& x ) : void&
		{
			return x;  // Cast to void in return (imut to imut).
		}
	"""
	tests_lib.build_program( c_program_text )


def CastToVoidReference_Test5():
	c_program_text= """
		fn ToVoid( i32 &mut x ) : void &mut
		{
			return x;  // Cast to void in return (mut to mut).
		}
	"""
	tests_lib.build_program( c_program_text )


def CastToVoidReference_Test6():
	c_program_text= """
		struct S{ void& v; }
		fn Foo()
		{
			var f32 x= zero_init;
			var S S{ .v= x };  // Cast reference in field initialization.
		}
	"""
	tests_lib.build_program( c_program_text )


def CastToVoidReference_Test7():
	c_program_text= """
		fn Foo( f32& x ) : void&
		{
			var void &v= x;  // Cast reference to void in reference-variable initialization.
			return v;
		}
	"""
	tests_lib.build_program( c_program_text )


def CastToVoidReference_Test8():
	c_program_text= """
		fn Bar( void& v ){}
		fn Foo()
		{
			var [ i32, 4 ] arr= zero_init;
			Bar(arr);  // Cast reference to array in function all.
		}
	"""
	tests_lib.build_program( c_program_text )


def CastToVoidReference_Test9():
	c_program_text= """
		struct S{}
		fn Bar( void& v ){}
		fn Foo()
		{
			var S s= zero_init;
			Bar(s);  // Cast reference to struct in function all.
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
