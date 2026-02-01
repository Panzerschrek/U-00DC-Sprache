from py_tests_common import *


def ArrayInitializersCountMismatch_ForArrayFiller_Test0():
	c_program_text= """
		fn Foo()
		{
			// Has too many initializers.
			var [ i32, 3 ] arr[ 1, 2, 3, 4, 5 ... ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ArrayInitializersCountMismatch", 5 ) )


def ArrayInitializersCountMismatch_ForArrayFiller_Test1():
	c_program_text= """
		fn Foo()
		{
			// Has one too many initializers, so that filler can't be applied.
			var [ i32, 4 ] arr[ 1, 2, 3, 4, 5 ... ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ArrayInitializersCountMismatch", 5 ) )


def ArrayInitializersCountMismatch_ForArrayFiller_Test2():
	c_program_text= """
		fn Foo()
		{
			// No error - filled the remaining array.
			var [ i32, 4 ] arr[ 1 ... ];
		}
	"""
	tests_lib.build_program( c_program_text )


def DestroyedVariableStillHasReferences_ForTemporaryInArrayFilleInitializer_Test0():
	c_program_text= """
		struct R{ i32& r; }
		fn Pass( i32& x ) : i32& { return x; }
		fn Foo()
		{
			// Reference to temporary numeric constant is saved in the initializer.
			var [ R, 3 ] arr[ { .r= Pass(7) } ... ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 7 ) )


def DestroyedVariableStillHasReferences_ForTemporaryInArrayFilleInitializer_Test1():
	c_program_text= """
		struct R{ C& c; }
		class C
		{
			fn constructor();
			fn destructor();
		}
		fn Pass( C& c ) : C& { return c; }
		fn Foo()
		{
			// Reference to temporary value of type "C" is saved in the initializer.
			var [ R, 3 ] arr[ { .c= Pass( C() ) } ... ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 12 ) )


def DestroyedVariableStillHasReferences_ForTemporaryInArrayFilleInitializer_Test2():
	c_program_text= """
		struct R{ i32& r; }
		fn Pass( i32& x ) : i32& { return x; }
		struct T{ [ R, 3 ] arr; }
		fn Bar( T& t );
		fn Foo()
		{
			// Reference to temporary numeric constant is saved in the initializer.
			Bar( T{ .arr[ { .r= Pass(7) } ... ] } );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 9 ) )
