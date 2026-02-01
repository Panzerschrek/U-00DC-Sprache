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


def ArrayFillerInitializerInAsyncFunction_Test0():
	c_program_text= """
		fn async Foo()
		{
			// Filler initializer can't be used in async functions,
			// since "await" is possible and it can return and thus leave the remaining array in partially-initialized state,
			// without any way to destroy only members which were actually initialized.
			var [ i32, 4 ] arr[ 1 ... ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NotImplemented", 7 ) )
	assert( errors_list[0].text.find( "array filler initializer in async functions" ) != -1 )


def ArrayFillerInitializerInAsyncFunction_Test1():
	c_program_text= """
		fn async Foo()
		{
			// Filler initializer can't be used in async functions,
			// since "await" is possible and it can return and thus leave the remaining array in partially-initialized state,
			// without any way to destroy only members which were actually initialized.
			var [ i32, 4 ] arr[ Bar().await ... ];
		}
		fn async Bar() : i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NotImplemented", 7 ) )
	assert( errors_list[0].text.find( "array filler initializer in async functions" ) != -1 )


def ArrayFillerInitializerInAsyncFunction_Test2():
	c_program_text= """
		fn Foo() : ( async : i32 )
		{
			// Fine - this isn't an async function but function returning async function object.
			// Using array filler initializer is allowed.
			var [ i32, 4 ] arr[ 3 ... ];
			return Bar();
		}
		fn async Bar() : i32;
	"""
	tests_lib.build_program( c_program_text )


def ArrayFillerInitializerInAsyncFunction_Test3():
	c_program_text= """
		fn generator Foo()
		{
			// Fine - this isn't an async function but a generator.
			// Using array filler initializer is allowed.
			var [ i32, 4 ] arr[ 3 ... ];
		}
	"""
	tests_lib.build_program( c_program_text )
