from py_tests_common import *


def UnsafeExpressionDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 x= unsafe( 66 );
			var f32 y= unsafe( f32(x) * 2.0f ) / 4.0f;
			auto z= safe( unsafe( y ) );
		}
	"""
	tests_lib.build_program( c_program_text )


def UnsafeExpression_Test0():
	c_program_text= """
		fn Bar(i32 x) unsafe : i32;
		fn Baz(i32 x);
		fn Foo()
		{
			Baz( unsafe(Bar(42)) ); // Cann call "unsafe" function inside "unsafe" expression.
		}
	"""
	tests_lib.build_program( c_program_text )


def UnsafeExpression_Test1():
	c_program_text= """
		fn Bar(i32 x) unsafe : i32;
		fn Baz(i32 x);
		fn Foo()
		{
			unsafe
			{
				Baz( unsafe(Bar(42)) );
				Bar(24); // "unsafe" block flag should not be resetted after "unsafe" expression.
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def UnsafeExpression_Test2():
	c_program_text= """
	fn Bar(i32 x) unsafe : i32;
	fn Baz(i32 x);
	fn Foo()
	{
		Baz( unsafe(Bar(42)) );
		Bar(24); // "unsafe" block flag should not be resetted after "unsafe" expression.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HaveError( errors_list, "UnsafeFunctionCallOutsideUnsafeBlock", 6 ) )
	assert( HaveError( errors_list, "UnsafeFunctionCallOutsideUnsafeBlock", 7 ) )


def UnsafeExpression_Test3():
	c_program_text= """

		fn Foo()
		{
			var i32 x= 42;
			var f32& x_as_float= unsafe( cast_ref_unsafe</f32/>(x) ); // Calling unsafe case inside unsafe expression.
		}
	"""
	tests_lib.build_program( c_program_text )


def UnsafeExpression_Test4():
	c_program_text= """
	fn Bar(i32 x) unsafe : i32;
	fn Foo()
	{
		unsafe( safe( Bar(0) ) ); // "safe" expression resets "unsafe" context.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnsafeFunctionCallOutsideUnsafeBlock", 5 ) )


def UnsafeExpression_Test5():
	c_program_text= """
	fn Bar(i32 x) unsafe : i32;
	fn Foo()
	{
		unsafe( Bar(0) * safe( Bar(1) ) ); // "safe" expression resets "unsafe" context.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnsafeFunctionCallOutsideUnsafeBlock", 5 ) )


def UnsafeExpression_Test6():
	c_program_text= """
		fn Bar(i32 x) unsafe : i32;
		fn Foo()
		{
			unsafe( unsafe( Bar( unsafe( unsafe( unsafe(42) ) ) ) ) ); // Multiple "unsafe" expressions.
		}
	"""
	tests_lib.build_program( c_program_text )
