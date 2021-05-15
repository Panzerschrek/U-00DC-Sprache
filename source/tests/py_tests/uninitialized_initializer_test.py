from py_tests_common import *


def UninitializedInitializer_Test0():
	c_program_text= """
		struct S
		{
			fn constructor() { halt; }
		}
		fn Foo()
		{
			unsafe
			{
				var S s= uninitialized;  // Must NOT call constructor here
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def UninitializedInitializer_Test1():
	c_program_text= """
		struct S
		{
			fn constructor() { halt; }
		}
		fn Foo()
		{
			unsafe
			{
				var [ S, 16 ]  mut s= uninitialized;  // Must NOT call constructor here
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def UninitializedInitializer_Test2():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
		}
		fn Foo()
		{
			unsafe
			{
				var S s{ .x= 0, .y= uninitialized };   // Uninitialized initializer for struct named initializer.
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def UninitializedInitializer_Test3():
	c_program_text= """
		fn Foo()
		{
			unsafe
			{
				var [ i32, 3 ] arr[ 0, uninitialized, 2 ];  // Uninitialized initializer for array member.
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def UninitializedInitializerIsNotConstexpr_Test0():
	c_program_text= """
		fn Foo()
		{
			unsafe
			{
				var i32 constexpr x= uninitialized;
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VariableInitializerIsNotConstantExpression" )
	assert( errors_list[0].src_loc.line == 6 )


def UninitializedInitializerIsNotConstexpr_Test1():
	c_program_text= """
		struct S{ i32 x; }
		fn Foo()
		{
			unsafe
			{
				var S constexpr s { .x= uninitialized };
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VariableInitializerIsNotConstantExpression" )
	assert( errors_list[0].src_loc.line == 7 )


def UninitializedInitializerOutsideUnsafeBlock_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 x= uninitialized;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UninitializedInitializerOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 4 )


def UninitializedInitializerOutsideUnsafeBlock_Test1():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor() ( x= uninitialized ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UninitializedInitializerOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 5 )
