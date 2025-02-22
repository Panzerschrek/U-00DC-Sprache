from py_tests_common import *


def GlobalMutableVariableDeclaration_Test0():
	c_program_text= """
		var i32 mut x = 66;
	"""
	tests_lib.build_program( c_program_text )


def GlobalMutableVariableDeclaration_Test1():
	c_program_text= """
		auto mut ff= 0.25f;
	"""
	tests_lib.build_program( c_program_text )


def GlobalMutableVariableDeclaration_Test2():
	c_program_text= """
		var tup[i32, u64] mut tt= zero_init, mut ft[1i32, 2u64], imut it= zero_init;
	"""
	tests_lib.build_program( c_program_text )


def GlobalMutableVariableDeclaration_Test3():
	c_program_text= """
		struct S{ i32 x; f32 y; bool z; }
		var S mut ss{ .x= 65, .y= -45.0f, .z= true }, mut sss= zero_init;
	"""
	tests_lib.build_program( c_program_text )


def GlobalMutableVariableDeclaration_Test4():
	c_program_text= """
		namespace NN
		{
			var i32 mut n= -5;
			class CC
			{
				var f32 mut ff= 0.25f;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def GlobalMutableVariableUsage_Test0():
	c_program_text= """
		var i32 mut x= 12;
		fn Bar() : i32
		{
			unsafe{ return x; }
		}
		fn Foo() : i32
		{
			unsafe
			{
				x= 66;
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Barv" ) == 12 )
	assert( tests_lib.run_function( "_Z3Foov" ) == 66 )


def GlobalMutableVariableUsage_Test1():
	c_program_text= """
		auto mut x= -13.0f;
		fn Bar() : f32
		{
			unsafe{ return x; }
		}
		fn Foo() : f32
		{
			unsafe
			{
				x= 96.5f;
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Barv" ) == -13.0 )
	assert( tests_lib.run_function( "_Z3Foov" ) == 96.5 )


def GlobalMutableVariableUsage_Test2():
	c_program_text= """
		struct S{ u64 x; }
		var S mut s{ .x(22) };
		fn Inc() : u64
		{
			unsafe
			{
				auto res= s.x;
				++s.x;
				return res;
			}
		}
		fn Reset()
		{
			unsafe{  s.x= 0u64;  }
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Incv" ) == 22 )
	assert( tests_lib.run_function( "_Z3Incv" ) == 23 )
	assert( tests_lib.run_function( "_Z3Incv" ) == 24 )
	assert( tests_lib.run_function( "_Z3Incv" ) == 25 )
	tests_lib.run_function( "_Z5Resetv" )
	assert( tests_lib.run_function( "_Z3Incv" ) == 0 )
	assert( tests_lib.run_function( "_Z3Incv" ) == 1 )
	assert( tests_lib.run_function( "_Z3Incv" ) == 2 )


def GlobalMutableVariableAccesIsNotAllowedOutsideUnsafeBlock_Test0():
	c_program_text= """
		var i32 mut x= 0;
		fn Foo() : i32
		{
			return x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "GlobalMutableVariableAccessOutsideUnsafeBlock", 5 ) )


def GlobalMutableVariableAccesIsNotAllowedOutsideUnsafeBlock_Test1():
	c_program_text= """
		auto mut ff= -456341.053;
		fn Foo() : f64
		{
			return ff;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "GlobalMutableVariableAccessOutsideUnsafeBlock", 5 ) )


def GlobalMutableVariableAccesIsNotAllowedOutsideUnsafeBlock_Test2():
	c_program_text= """
		auto mut b= false;
		// Even global mutable variable access in another global mutable variable initialization is forbidden, because global context is not usafe.
		// Also this code should produce an error about non-constexpr global variable initializer.
		auto mut b_copy= b;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "GlobalMutableVariableAccessOutsideUnsafeBlock", 5 ) )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 5 ) )


def VariableInitializerIsNotConstantExpression_ForGlobalMutableVariable_Test0():
	c_program_text= """
		auto mut x= Foo(); // Initializer is result of non-constexpr function call.
		fn Foo() : u32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 2 ) )


def VariableInitializerIsNotConstantExpression_ForGlobalMutableVariable_Test1():
	c_program_text= """
		var i32 mut x= 0;
		var i32 mut y= x; // Global mutable variable 'x' became non-constexpr after initialization. So, it's not possible to use it for initializer of another global mutable variable.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 3 ) )


def VariableInitializerIsNotConstantExpression_ForGlobalMutableVariable_Test2():
	c_program_text= """
		struct SS
		{
			var i32 mut x= 0;
			var i32 imut y= x; // Global mutable variable 'x' became non-constexpr after initialization. So, it's not possible to use it for initializer of immutable global variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 5 ) )


def GlobalMutableVariableIsNotConstexpr_Test0():
	c_program_text= """
		var u32 mut ght=6754;
		fn Foo()
		{
			var u32 constexpr x= ght;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 5 ) )


def GlobalMutableVariableIsNotConstexpr_Test1():
	c_program_text= """
		auto mut s=16;
		type IVec= [ i32, s ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedConstantExpression", 3 ) )


def GlobalMutableVariableIsNotConstexpr_Test2():
	c_program_text= """
		auto mut bb= true;
		fn Foo()
		{
			static_if( bb ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedConstantExpression", 5 ) )


def GobalMutableVariableShouldHaveConstexprType_Test0():
	c_program_text= """
		struct S
		{
			fn destructor(){}
		}
		var S mut s= zero_init; // Type with explicit destructor declaration is not "constexpr"
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InvalidTypeForConstantExpressionVariable", 6 ) )


def GobalMutableVariableShouldHaveConstexprType_Test1():
	c_program_text= """
		class C{ }
		var C mut c; // Class is not "constexpr".
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InvalidTypeForConstantExpressionVariable", 3 ) )


def GobalMutableVariableShouldHaveConstexprType_Test2():
	c_program_text= """
		// Raw pointer type is not "constexpr" type.
		// So, now it's not possible to use raw pointers in global variables.
		var $(i32) mut ptr= zero_init;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InvalidTypeForConstantExpressionVariable", 4 ) )


def MutableGlobalReferencesAreNotAllowed_Test0():
	c_program_text= """
		var i32 mut x= 0;
		var i32 &mut x_ref= x;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "MutableGlobalReferencesAreNotAllowed", 3 ) )


def MutableGlobalReferencesAreNotAllowed_Test1():
	c_program_text= """
		auto mut x= 0;
		auto &mut x_ref= x;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "MutableGlobalReferencesAreNotAllowed", 3 ) )


def ThreadLocalVariableDeclaration_Test0():
	c_program_text= """
		thread_local i32 x = 66;
	"""
	tests_lib.build_program( c_program_text )


def ThreadLocalVariableDeclaration_Test1():
	c_program_text= """
		thread_local [ f32, 2 ] x= zero_init, y[ 0.5f, 0.25f ];
	"""
	tests_lib.build_program( c_program_text )


def ThreadLocalVariableDeclaration_Test2():
	c_program_text= """
		namespace NN
		{
			thread_local u64 sss(2);
		}
	"""
	tests_lib.build_program( c_program_text )


def ThreadLocalVariableDeclaration_Test3():
	c_program_text= """
		class SomeClass
		{
			thread_local ssize_type some(12345);
		}
	"""
	tests_lib.build_program( c_program_text )


def ThreadLocalVariableUnsage_Test0():
	c_program_text= """
		thread_local i32 x= 1;
		fn Bar() : i32
		{
			unsafe
			{
				auto res= x;
				x*= 2;
				return res;
			}
		}
		fn Foo()
		{
			halt if( Bar() != 1 );
			halt if( Bar() != 2 );
			halt if( Bar() != 4 );
			halt if( Bar() != 8 );
			halt if( Bar() != 16 );
			unsafe{ x= 33; }
			halt if( Bar() != 33 );
			halt if( Bar() != 66 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ThreadLocalVariableAccesIsNotAllowedOutsideUnsafeBlock_Test0():
	c_program_text= """
		thread_local i32 x= 0;
		fn Foo() : i32
		{
			return x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "GlobalMutableVariableAccessOutsideUnsafeBlock", 5 ) )


def ThreadLocalVariableAccesIsNotAllowedOutsideUnsafeBlock_Test1():
	c_program_text= """
		thread_local f64 ff= -456341.053;
		fn Foo() : f64
		{
			return ff;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "GlobalMutableVariableAccessOutsideUnsafeBlock", 5 ) )


def ThreadLocalVariableAccesIsNotAllowedOutsideUnsafeBlock_Test2():
	c_program_text= """
		thread_local bool b= false;
		// Even thread-local variable access in another thread-local variable initialization is forbidden, because global context is not usafe.
		// Also this code should produce an error about non-constexpr global variable initializer.
		thread_local bool b_copy= b;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "GlobalMutableVariableAccessOutsideUnsafeBlock", 5 ) )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 5 ) )


def VariableInitializerIsNotConstantExpression_ForThreadLocalVariable_Test0():
	c_program_text= """
		thread_local u32 x= Foo(); // Initializer is result of non-constexpr function call.
		fn Foo() : u32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 2 ) )


def VariableInitializerIsNotConstantExpression_ForThreadLocalVariable_Test1():
	c_program_text= """
		thread_local i32 x= 0;
		thread_local i32 y= x; // Thread-local variable 'x' became non-constexpr after initialization. So, it's not possible to use it for initializer of another thread-local variable.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 3 ) )


def VariableInitializerIsNotConstantExpression_ForThreadLocalVariable_Test2():
	c_program_text= """
		struct SS
		{
			thread_local mut x= 0;
			thread_local imut y= x; // Thread-local variable 'x' became non-constexpr after initialization. So, it's not possible to use it for initializer of immutable global variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 5 ) )


def ThreadLocalVariableIsNotConstexpr_Test0():
	c_program_text= """
		thread_local u32 ght=6754;
		fn Foo()
		{
			var u32 constexpr x= ght;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 5 ) )


def ThreadLocalVariableIsNotConstexpr_Test1():
	c_program_text= """
		thread_local i32 s=16;
		type IVec= [ i32, s ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedConstantExpression", 3 ) )


def ThreadLocalVariableIsNotConstexpr_Test2():
	c_program_text= """
		thread_local bool bb= true;
		fn Foo()
		{
			static_if( bb ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedConstantExpression", 5 ) )


def ThreadLocalVariableShouldHasConstexprType_Test0():
	c_program_text= """
		struct S
		{
			fn destructor(){}
		}
		thread_local S s= zero_init; // Type with explicit destructor declaration is not "constexpr"
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InvalidTypeForConstantExpressionVariable", 6 ) )


def ThreadLocalVariableShouldHaveConstexprType_Test1():
	c_program_text= """
		class C{ }
		thread_local C c; // Class is not "constexpr".
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InvalidTypeForConstantExpressionVariable", 3 ) )


def ThreadLocalVariableShouldHaveConstexprType_Test2():
	c_program_text= """
		// Raw pointer type is not "constexpr" type.
		// So, now it's not possible to use raw pointers in thread-local variables.
		thread_local $(i32) ptr= zero_init;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InvalidTypeForConstantExpressionVariable", 4 ) )
