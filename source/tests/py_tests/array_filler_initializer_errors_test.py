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


def ArrayInitializersCountMismatch_ForArrayFiller_Test3():
	c_program_text= """
		struct S
		{
			fn constructor()
				// Has too many initializers.
				( arr[ 1, 2, 3, 4, 5 ... ] )
			{}

			[ i32, 3 ] arr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ArrayInitializersCountMismatch", 6 ) )


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


def ReferencePollutionOfOuterLoopVariable_ForArrayFillerInitializer_Test0():
	c_program_text= """
		struct R{ i32& x; }
		fn Foo()
		{
			var i32 x= 0;
			// Error here - we create immutable references to variable "x" inside "arr" inside filler loop.
			var[ R, 3 ] arr[ { .x= x } ... ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferencePollutionOfOuterLoopVariable", 7 ) )


def ReferencePollutionOfOuterLoopVariable_ForArrayFillerInitializer_Test1():
	c_program_text= """
		struct R{ i32 &mut x; }
		fn Foo()
		{
			var i32 mut x= 0;
			// Error here - we create mutable references to variable "x" inside "arr" inside filler loop.
			var[ R, 3 ] arr[ { .x= x } ... ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferencePollutionOfOuterLoopVariable", 7 ) )


def ReferencePollutionOfOuterLoopVariable_ForArrayFillerInitializer_Test2():
	c_program_text= """
		struct S{ i32& x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn Pollution( S &mut s, i32& x ) @(pollution) : i32;
		fn Foo()
		{
			var i32 x= 0, y= 0;
			var S mut s{ .x= x };
			// Perform linking of inner reference node of "s" with "y" inside filler loop.
			var [ i32, 10 ] arr[ Pollution( s, y ) ... ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferencePollutionOfOuterLoopVariable", 10 ) )


def ReferencePollutionOfOuterLoopVariable_ForArrayFillerInitializer_Test3():
	c_program_text= """
		struct S{ i32 &mut x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn Pollution( S &mut s, i32 &mut x ) @(pollution) : i32;
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };
			// Perform mutable linking of inner reference node of "s" with "y" inside filler loop.
			var [ i32, 10 ] arr[ Pollution( s, y ) ... ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferencePollutionOfOuterLoopVariable", 10 ) )


def ReferencePollutionOfOuterLoopVariable_ForArrayFillerInitializer_Test4():
	c_program_text= """
		struct R{ i32& x; }
		fn Foo()
		{
			var i32 x= 0;
			// Error here - we create immutable references to variable "x" inside "arr" inside filler loop, even if this loop has only single iteration.
			var[ R, 1 ] arr[ { .x= x } ... ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferencePollutionOfOuterLoopVariable", 7 ) )


def ReferencePollutionOfOuterLoopVariable_ForArrayFillerInitializer_Test5():
	c_program_text= """
		struct R{ i32& x; }
		struct T
		{
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			fn constructor( i32& x ) @( pollution )
				// Error here - we create immutable references to variable "x" inside "arr" inside filler loop,.
				( arr[ { .x= x } ... ] )
			{ }

			[ R, 4 ] arr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferencePollutionOfOuterLoopVariable", 8 ) )


def ReferencePollutionOfOuterLoopVariable_ForArrayFillerInitializer_Test6():
	c_program_text= """
		struct R{ i32 &mut x; }
		struct T
		{
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			fn constructor( i32 &mut x ) @( pollution )
				// Error here - we create immutable references to variable "x" inside "arr" inside filler loop,.
				( arr[ { .x= x } ... ] )
			{ }

			[ R, 4 ] arr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferencePollutionOfOuterLoopVariable", 8 ) )


def OuterVariableMoveInsideLoop_ForArrayFillerInitializer_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			// Error here - "x" moved multiple times, since "..." creates a loop.
			var[ i32, 3 ] arr[ move(x) ... ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OuterVariableMoveInsideLoop", 6 ) )


def OuterVariableMoveInsideLoop_ForArrayFillerInitializer_Test1():
	c_program_text= """
		fn Foo( u32 mut x )
		{
			// Error here - "x" moved multiple times, since "..." creates a loop, even if this loop has only single iteration.
			var[ u32, 1 ] arr[ move(x) ... ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OuterVariableMoveInsideLoop", 5 ) )


def OuterVariableMoveInsideLoop_ForArrayFillerInitializer_Test2():
	c_program_text= """
		struct S
		{
			fn constructor( f64 mut x )
				// Error here - "x" moved multiple times, since "..." creates a loop, even if this loop has only single iteration.
				( arr[ move(x) ... ] )
			{}

			[ f64, 64 ] arr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OuterVariableMoveInsideLoop", 6 ) )
