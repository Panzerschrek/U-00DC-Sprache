from py_tests_common import *

def ReturningUnallowedReference_ForAsyncReturn_Test0():
	c_program_text= """
		// Doesn't allow to return a reference to any param.
		var [ [ char8, 2 ], 0 ] return_references[];
		fn async Foo( i32& x ) : i32& @(return_references)
		{
			return x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReturningUnallowedReference", 6 ) )


def ReturningUnallowedReference_ForAsyncReturn_Test1():
	c_program_text= """
		// Doesn't allow to return reference param #1.
		var [ [ char8, 2 ], 2 ] return_references[ "0_", "2_"];
		fn async Foo( i32& x, i32& y, i32& z ) : i32& @(return_references)
		{
			return y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReturningUnallowedReference", 6 ) )


def ReturningUnallowedReference_ForAsyncReturn_Test2():
	c_program_text= """
		fn async Foo( i32& x ) : i32&
		{
			var i32 some_local= 0;
			return some_local; // It's not allowed to return a local rerference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReturningUnallowedReference", 5 ) )
	assert( HaveError( errors_list, "DestroyedVariableStillHaveReferences", 5 ) )


def ReturningUnallowedReference_ForAsyncReturn_Test3():
	c_program_text= """
		struct S{ i32& r; }
		// Doesn't allow to return reference param #1 inside inner reference.
		var tup[ [ [ char8, 2 ], 2 ] ] return_inner_references[ [ "0_", "2_"] ];
		fn async Foo( i32& x, i32& y, i32& z ) : S @(return_inner_references)
		{
			var S s{ .r= y };
			return s;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReturningUnallowedReference", 8 ) )


def ReturningUnallowedReference_ForAsyncReturn_Test4():
	c_program_text= """
		struct S{ i32& r; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn async Foo( i32& x ) : S @(return_inner_references)
		{
			var i32 some_local= 0;
			var S s{ .r= some_local };
			return s; // It's not allowed to return a local rerference inside a variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReturningUnallowedReference", 8 ) )
	assert( HaveError( errors_list, "DestroyedVariableStillHaveReferences", 8 ) )
