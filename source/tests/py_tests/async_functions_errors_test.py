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


def NoReturnInFunctionReturningNonVoid_ForAsyncFunction_Test0():
	c_program_text= """
		fn async Foo() : i32
		{
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NoReturnInFunctionReturningNonVoid", 4 ) )


def NoReturnInFunctionReturningNonVoid_ForAsyncFunction_Test1():
	c_program_text= """
		fn async Foo( bool cond ) : i32
		{
			if( cond ) { return 42; }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NoReturnInFunctionReturningNonVoid", 5 ) )


def NoReturnInFunctionReturningNonVoid_ForAsyncFunction_Test2():
	c_program_text= """
		fn async Foo( i32& x ) : i32&
		{
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NoReturnInFunctionReturningNonVoid", 4 ) )


def NoReturnInFunctionReturningNonVoid_ForAsyncFunction_Test3():
	c_program_text= """
		fn async Foo( void& x ) : void&
		{
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NoReturnInFunctionReturningNonVoid", 4 ) )


def NoReturnInFunctionReturningNonVoid_ForAsyncFunction_Test4():
	c_program_text= """
		fn async Foo( bool cond, f32& x ) : f32&
		{
			if( cond ) { return x; }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NoReturnInFunctionReturningNonVoid", 5 ) )


def TypesMismatch_ForASyncFunctionReturn_Test0():
	c_program_text= """
		fn async Foo()
		{
			return 12345; // Expected "void", got "i32"
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


def TypesMismatch_ForASyncFunctionReturn_Test1():
	c_program_text= """
		fn async Foo() : bool
		{
			return 12345; // Expected "bool", got "i32"
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


def TypesMismatch_ForASyncFunctionReturn_Test2():
	c_program_text= """
		fn async Foo() : i32
		{
			return; // Expected "i32", got "void"
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


def NonEmptyYieldInAsyncFunction_Test0():
	c_program_text= """
		fn async Foo() : i32
		{
			yield 123;
			return 42;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NonEmptyYieldInAsyncFunction", 4 ) )


def NonEmptyYieldInAsyncFunction_Test1():
	c_program_text= """
		fn async Foo(i32& x) : i32&
		{
			yield 123;
			return x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NonEmptyYieldInAsyncFunction", 4 ) )


def NonEmptyYieldInAsyncFunction_Test2():
	c_program_text= """
		fn async Foo() : i32
		{
			var void v;
			yield v; // For now even an usage of void-type expression in "yield" for async functions is an error.
			return 42;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NonEmptyYieldInAsyncFunction", 5 ) )


def AutoReturnCoroutine_ForAsyncFunction_Test0():
	c_program_text= """
		fn async Foo() : auto {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "AutoReturnCoroutine", 2 ) )


def CoroutineSpecialMethod_ForAsyncFunction_Test0():
	c_program_text= """
		struct S
		{
			fn async constructor();
			fn async constructor(i32 x);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "CoroutineSpecialMethod", 4 ) )
	assert( HaveError( errors_list, "CoroutineSpecialMethod", 5 ) )


def CoroutineSpecialMethod_ForAsyncFunction_Test1():
	c_program_text= """
		struct S
		{
			fn async destructor();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "CoroutineSpecialMethod", 4 ) )


def ReferencesPollution_ForAsyncFunction_Test2():
	c_program_text= """
		struct S{ i32 & x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn async Foo( S &mut s, i32 & x ) @(pollution) : i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NotImplemented", 4 ) )


def NonDefaultCallingConventionForCoroutine_ForAsyncFunction_Test0():
	c_program_text= """
		fn async Foo() call_conv("fast") : i32 { return 0; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NonDefaultCallingConventionForCoroutine", 2 ) )


def NonDefaultCallingConventionForCoroutine_ForAsyncFunction_Test1():
	c_program_text= """
		fn async Foo() call_conv("cold") : i32 { return 0; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NonDefaultCallingConventionForCoroutine", 2 ) )


def NonDefaultCallingConventionForCoroutine_ForAsyncFunction_Test2():
	c_program_text= """
		fn async Foo() call_conv("default") : i32 { return 0; } // Ok - using default calling convention.
	"""
	tests_lib.build_program( c_program_text )


def NonDefaultCallingConventionForCoroutine_ForAsyncFunction_Test3():
	c_program_text= """
		fn async Foo() call_conv("C") : i32 { return 0; } // Ok - "C" is default calling convention.
	"""
	tests_lib.build_program( c_program_text )


def VirtualAsyncFunction_Test0():
	c_program_text= """
		class A polymorph
		{
			fn virtual async Foo(this) : i32;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "VirtualCoroutine", 4 ) )


def CoroutineNonSyncRequired_ForAsyncFunction_Test0():
	c_program_text= """
		struct S non_sync {}
		fn async Foo(S s) {} // Async function value parameter is non-sync.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "CoroutineNonSyncRequired", 3 ) )


def CoroutineNonSyncRequired_ForAsyncFunction_Test1():
	c_program_text= """
		struct S non_sync {}
		fn async non_sync(false) Foo(S& s){} // Async function value parameter is non-sync.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "CoroutineNonSyncRequired", 3 ) )


def CoroutineNonSyncRequired_ForAsyncFunction_Test2():
	c_program_text= """
		struct S non_sync {}
		fn async Foo() : S {} // Async function return value is non-sync.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "CoroutineNonSyncRequired", 3 ) )


def CoroutineNonSyncRequired_ForAsyncFunction_Test3():
	c_program_text= """
		struct S non_sync {}
		fn async Foo() : S& {} // Async function return reference is non-sync.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "CoroutineNonSyncRequired", 3 ) )


def CoroutineNonSyncRequired_ForAsyncFunction_Test4():
	c_program_text= """
		struct S non_sync {}
		// Ok - non_sync tag exists and args/return value are non-sync.
		fn async non_sync(true) Foo(S& s){}
		fn async non_sync Bar(S s){}
		fn async non_sync Baz() : S { halt; }
		fn async non_sync Lol() : S& { halt; }
	"""
	tests_lib.build_program( c_program_text )
