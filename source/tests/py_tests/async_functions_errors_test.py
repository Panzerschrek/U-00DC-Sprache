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
	assert( HasError( errors_list, "ReturningUnallowedReference", 6 ) )


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
	assert( HasError( errors_list, "ReturningUnallowedReference", 6 ) )


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
	assert( HasError( errors_list, "ReturningUnallowedReference", 5 ) )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 5 ) )


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
	assert( HasError( errors_list, "ReturningUnallowedReference", 8 ) )


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
	assert( HasError( errors_list, "ReturningUnallowedReference", 8 ) )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 8 ) )


def NoReturnInFunctionReturningNonVoid_ForAsyncFunction_Test0():
	c_program_text= """
		fn async Foo() : i32
		{
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NoReturnInFunctionReturningNonVoid", 4 ) )


def NoReturnInFunctionReturningNonVoid_ForAsyncFunction_Test1():
	c_program_text= """
		fn async Foo( bool cond ) : i32
		{
			if( cond ) { return 42; }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NoReturnInFunctionReturningNonVoid", 5 ) )


def NoReturnInFunctionReturningNonVoid_ForAsyncFunction_Test2():
	c_program_text= """
		fn async Foo( i32& x ) : i32&
		{
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NoReturnInFunctionReturningNonVoid", 4 ) )


def NoReturnInFunctionReturningNonVoid_ForAsyncFunction_Test3():
	c_program_text= """
		fn async Foo( void& x ) : void&
		{
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NoReturnInFunctionReturningNonVoid", 4 ) )


def NoReturnInFunctionReturningNonVoid_ForAsyncFunction_Test4():
	c_program_text= """
		fn async Foo( bool cond, f32& x ) : f32&
		{
			if( cond ) { return x; }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NoReturnInFunctionReturningNonVoid", 5 ) )


def TypesMismatch_ForAsyncFunctionReturn_Test0():
	c_program_text= """
		fn async Foo()
		{
			return 12345; // Expected "void", got "i32"
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def TypesMismatch_ForAsyncFunctionReturn_Test1():
	c_program_text= """
		fn async Foo() : bool
		{
			return 12345; // Expected "bool", got "i32"
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def TypesMismatch_ForAsyncFunctionReturn_Test2():
	c_program_text= """
		fn async Foo() : i32
		{
			return; // Expected "i32", got "void"
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def TypesMismatch_ForAsyncFunctionReturn_Test3():
	c_program_text= """
		fn async Foo( i32& x ) : f32 &
		{
			return; // Expected "f32" reference, got "i32" reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def ExpectedReferenceValue_ForAsyncFunctionReturn_Test0():
	c_program_text= """
		fn async Foo() : f32 &
		{
			return 0.25f; // Expected reference, got value.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 4 ) )


def BindingConstReferenceToNonconstReference_ForAsyncFunctionReturn_Test0():
	c_program_text= """
		fn async Foo( i32& x ) : i32 &mut
		{
			return x; // Expected mutable reference, got immutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "BindingConstReferenceToNonconstReference", 4 ) )


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
	assert( HasError( errors_list, "NonEmptyYieldInAsyncFunction", 4 ) )


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
	assert( HasError( errors_list, "NonEmptyYieldInAsyncFunction", 4 ) )


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
	assert( HasError( errors_list, "NonEmptyYieldInAsyncFunction", 5 ) )


def AutoReturnCoroutine_ForAsyncFunction_Test0():
	c_program_text= """
		fn async Foo() : auto {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AutoReturnCoroutine", 2 ) )


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
	assert( HasError( errors_list, "CoroutineSpecialMethod", 4 ) )
	assert( HasError( errors_list, "CoroutineSpecialMethod", 5 ) )


def CoroutineSpecialMethod_ForAsyncFunction_Test1():
	c_program_text= """
		struct S
		{
			fn async destructor();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineSpecialMethod", 4 ) )


def ReferencesPollution_ForAsyncFunction_Test2():
	c_program_text= """
		struct S{ i32 & x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn async Foo( S &mut s, i32 & x ) @(pollution) : i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NotImplemented", 4 ) )


def NonDefaultCallingConventionForCoroutine_ForAsyncFunction_Test0():
	c_program_text= """
		fn async Foo() call_conv("fast") : i32 { return 0; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NonDefaultCallingConventionForCoroutine", 2 ) )


def NonDefaultCallingConventionForCoroutine_ForAsyncFunction_Test1():
	c_program_text= """
		fn async Foo() call_conv("cold") : i32 { return 0; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NonDefaultCallingConventionForCoroutine", 2 ) )


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
	assert( HasError( errors_list, "VirtualCoroutine", 4 ) )


def CoroutineNonSyncRequired_Test0():
	c_program_text= """
		struct S non_sync {}
		type AsyncFunc= async : S; // "S" is "non_sync", so, "non_sync" is required for generator type.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineNonSyncRequired", 3 ) )


def CoroutineMismatch_ForAsyncFunction_Test0():
	c_program_text= """
		fn async Foo() : i32;
		fn Foo() : ( async : i32 ) { halt; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineMismatch", 2 ) or HasError( errors_list, "CoroutineMismatch", 3 ) )


def CoroutineMismatch_ForAsyncFunction_Test1():
	c_program_text= """
		fn async Foo() : i32 { return 0; }
		fn Foo() : ( async : i32 );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineMismatch", 2 ) or HasError( errors_list, "CoroutineMismatch", 3 ) )


def CoroutineMismatch_ForAsyncFunction_Test2():
	c_program_text= """
		struct S
		{
			fn async Foo(this) : i32;
		}
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn S::Foo(this) : ( async'imut' : i32 ) @(return_inner_references) { halt; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineMismatch", 4 ) or HasError( errors_list, "CoroutineMismatch", 7 ) )


def CoroutineMismatch_ForAsyncFunction_Test3():
	c_program_text= """
		struct S
		{
			var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
			fn Foo(this) : ( async'imut' : i32 ) @(return_inner_references);
		}
		fn async S::Foo(this) : i32 { return 0; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineMismatch", 5 ) or HasError( errors_list, "CoroutineMismatch", 7 ) )


def AsyncReturn_ForNonCopyableValue_Test0():
	c_program_text= """
		struct S
		{
			fn constructor();
			fn constructor(mut this, S& other)= delete;
		}
		fn async Foo() : S
		{
			var S s;
			return safe(s); // Can't copy "s" here - it is non-copyable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 10 ) )


def AsyncReturn_ForAbstractValue_Test0():
	c_program_text= """
		class A abstract
		{
			fn constructor( mut this, A& other )= default;
		}
		fn async Foo( A& a ) : A
		{
			return a; // Trying to copy-construct abstract value.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConstructingAbstractClassOrInterface", 8 ) )


def AsyncFunctionIsNonCopyable_Test0():
	c_program_text= """
		fn async SomeFunc() : i32;
		fn Foo()
		{
			auto f= SomeFunc();
			auto f_copy= f;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 6 ) )


def AsyncFunctionIsNonCopyable_Test1():
	c_program_text= """
		fn async SomeFunc() : i32;
		fn Foo()
		{
			auto f= SomeFunc();
			var (async : i32) f_copy= f;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 6 ) )


def AsyncFunctionIsNonCopyable_Test2():
	c_program_text= """
		fn async SomeFunc() : i32;
		fn Foo()
		{
			auto f= SomeFunc();
			var (async : i32) f_copy(f);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ClassHasNoConstructors", 6 ) or HasError( errors_list, "CouldNotSelectOverloadedFunction", 6 ) )


def AsyncFunctionIsNonCopyable_Test3():
	c_program_text= """
		fn async SomeFunc() : i32;
		struct S{ (async : i32) f; }
		fn Foo()
		{
			auto f= SomeFunc();
			var S s { .f= f };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 7 ) )


def AsyncFunctionIsNonCopyable_Test4():
	c_program_text= """
		fn async SomeFunc() : i32;
		fn Foo()
		{
			auto f= SomeFunc();
			var [ (async : i32 ), 1 ] arr[ f ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 6 ) )


def AsyncFunctionIsNonCopyable_Test5():
	c_program_text= """
		fn async SomeFunc() : i32;
		fn Foo()
		{
			auto mut f0= SomeFunc();
			auto mut f1= SomeFunc();
			f0= f1; // Try to call copy assignment operator here.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 7 ) )


def AsyncFunctionIsNonCopyable_Test6():
	c_program_text= """
		fn async SomeFunc() : i32;
		fn Foo() : (async : i32)
		{
			auto f= SomeFunc();
			return safe(f); // Try to call copy constructor for return value.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 6 ) )


def AsyncFunctionIsNonCopyable_Test7():
	c_program_text= """
		fn async SomeFunc() : i32;
		fn Pass( (async : i32) f );
		fn Foo()
		{
			auto f= SomeFunc();
			Pass( f ); // Try to call copy constructor for argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 7 ) )


def AsyncFunctionIsNonCopyable_Test8():
	c_program_text= """
		type AsyncFunc= async : i32;
		static_assert( !typeinfo</AsyncFunc/>.is_copy_constructible );
		static_assert( !typeinfo</AsyncFunc/>.is_copy_assignable );
	"""
	tests_lib.build_program( c_program_text )


def DestroyedVariableStillHasReferences_ForAsyncFunction_Test0():
	c_program_text= """
		fn async SomeFunc( i32& x );
		fn Foo()
		{
			auto f= SomeFunc( 66 ); // async function object holds a reference to temporary value of type "i32".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 5 ) )


def DestroyedVariableStillHasReferences_ForAsyncFunction_Test1():
	c_program_text= """
		fn async SomeFunc( f32& x );
		fn Bar() : f32;
		fn Foo()
		{
			auto f= SomeFunc( Bar() ); // async function object holds a reference to temporary-result of "Bar" call.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 6 ) )


def DestroyedVariableStillHasReferences_ForAsyncFunction_Test2():
	c_program_text= """
		struct S{ i32& x; }
		fn async SomeFunc( S s );
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn MakeS( i32& x ) : S @(return_inner_references);
		fn Foo()
		{
			auto f= SomeFunc( MakeS( 789 ) ); // async function object holds a value of type "S" with a reference to a temporary inside.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 8 ) )


def DestroyedVariableStillHasReferences_ForAsyncFunction_Test3():
	c_program_text= """
		struct S{ i32& x; }
		fn async SomeFunc( S s );
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn MakeS( i32& x ) : S @(return_inner_references);
		fn Foo()
		{
			var i32 some_local= 0;
			auto f= SomeFunc( MakeS( some_local ) ); // async function object holds a value of type "S" with a reference to a local variable. This is ok.
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferenceIndirectionDepthExceeded_ForAsyncFunctions_Test0():
	c_program_text= """
		struct S{ i32 & x; }
		fn async Foo( S & s ) : i32 { return 0; } // Can't pass structs with references inside by a reference into a generator.
	"""
	tests_lib.build_program_with_errors( c_program_text )
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceIndirectionDepthExceeded", 3 ) )


def ReferenceIndirectionDepthExceeded_ForAsyncFunctions_Test1():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn async Foo( S & s ) : i32 { return 0; } // Can't pass structs with references inside by a reference into a generator.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceIndirectionDepthExceeded", 3 ) )


def ReferenceIndirectionDepthExceeded_ForForAsyncFunctions_Test2():
	c_program_text= """
		struct S{ i32 & x; }
		fn async Foo( S s ) : i32 { return 0; } // Ok - pass struct with reference inside by value.
	"""
	tests_lib.build_program( c_program_text )
