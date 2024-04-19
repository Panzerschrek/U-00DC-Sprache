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
	assert( not HasError( errors_list, "UnsafeFunctionCallOutsideUnsafeBlock", 6 ) )
	assert( HasError( errors_list, "UnsafeFunctionCallOutsideUnsafeBlock", 7 ) )


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
	assert( HasError( errors_list, "UnsafeFunctionCallOutsideUnsafeBlock", 5 ) )


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
	assert( HasError( errors_list, "UnsafeFunctionCallOutsideUnsafeBlock", 5 ) )


def UnsafeExpression_Test6():
	c_program_text= """
		fn Bar(i32 x) unsafe : i32;
		fn Foo()
		{
			unsafe( unsafe( Bar( unsafe( unsafe( unsafe(42) ) ) ) ) ); // Multiple "unsafe" expressions.
		}
	"""
	tests_lib.build_program( c_program_text )


def UnsafeExpression_Test7():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor()
			( x= unsafe(Bar()) ) // Use "unsafe" expression in constructor initializer list.
			{}
		}
		fn Bar() unsafe : i32;
	"""
	tests_lib.build_program( c_program_text )


def UnsafeExpressionInGlobalContext_Test0():
	c_program_text= """
	// Unsafe expression for global variable initializer.
	auto x= unsafe(0);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsafeExpressionInGlobalContext", 3 ) )


def UnsafeExpressionInGlobalContext_Test1():
	c_program_text= """
	// Unsafe expression for global variable initializer.
	var i32 mut x(unsafe(45));
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsafeExpressionInGlobalContext", 3 ) )


def UnsafeExpressionInGlobalContext_Test2():
	c_program_text= """
	// Unsafe expression for array type size.
	type T= [ i32, unsafe(4) ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsafeExpressionInGlobalContext", 3 ) )


def UnsafeExpressionInGlobalContext_Test3():
	c_program_text= """
	// Unsafe expression for type template argument.
	type T= S</unsafe(42)/>;
	template</i32 s/> struct S{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsafeExpressionInGlobalContext", 3 ) )


def UnsafeExpressionInGlobalContext_Test4():
	c_program_text= """
	// Unsafe expression for default template default param value.
	template</i32 s/> struct S</ s= unsafe(0) /> {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsafeExpressionInGlobalContext", 3 ) )


def UnsafeExpressionInGlobalContext_Test5():
	c_program_text= """
	// Unsafe expression for struct field default initializer.
	struct S
	{
		i32 x= unsafe(0);
		i32 y= safe(unsafe(1));
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) >= 2 )
	assert( HasError( errors_list, "UnsafeExpressionInGlobalContext", 5 ) )
	assert( HasError( errors_list, "UnsafeExpressionInGlobalContext", 6 ) )


def UnsafeExpressionInGlobalContext_Test6():
	c_program_text= """
	// Unsafe expression for "non_sync" tag.
	struct S non_sync( unsafe(false) ) {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnsafeExpressionInGlobalContext", 3 ) )


def UnsafeExpressionIsNotConstexpr_Test0():
	c_program_text= """
	fn foo()
	{
		auto constexpr x= unsafe(0);
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 4 ) )


def UnsafeExpressionIsNotConstexpr_Test1():
	c_program_text= """
	fn foo()
	{
		var f32 constexpr x= safe(unsafe(f32));
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 4 ) )


def UnsafeExpressionIsNotConstexpr_Test2():
	c_program_text= """
	fn foo()
	{
		var [i32, 3] constexpr x[ 5, unsafe(6), 7 ];
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 4 ) )


def ConstexprFunctionContainsUnallowedOperations_ForUnsafeExpression_Test0():
	c_program_text= """
	fn constexpr foo() : u32
	{
		auto x= unsafe(0u); // "Unsafe" expression is not allowed in "constexpr" functions.
		return x;
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConstexprFunctionContainsUnallowedOperations", 2 ) )


def ConstexprFunctionContainsUnallowedOperations_ForUnsafeExpression_Test1():
	c_program_text= """
	struct S
	{
		i32 x= 0;

		fn constexpr constructor()
			( x= unsafe(0) ) // "Unsafe" expression is not allowed in "constexpr" constructor.
		{}
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConstexprFunctionContainsUnallowedOperations", 6 ) )
