from py_tests_common import *


def AwaitOperator_Test0():
	c_program_text= """
		fn async SimpleFunc() : i32
		{
			yield;
			yield;
			yield;
			return 42;
		}
		fn async Bar()
		{
			// Obtain a fundamental type value from "await".
			var i32 x= SimpleFunc().await;
			halt if( x != 42 );
		}
		fn Foo()
		{
			auto mut f= Bar();
			loop
			{
				if_coro_advance( r : f )
				{
					break;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AwaitOperator_Test1():
	c_program_text= """
		class C
		{
			i32 x= 0;
			f32 y= 0.0f;
		}
		static_assert( !typeinfo</C/>.is_copy_constructible );
		fn async SimpleFunc() : C
		{
			var C mut c;
			c.x= 776;
			c.y= -0.125f;
			return move(c);
		}
		fn async Bar()
		{
			// Obtain a non-copyable value from "await".
			auto c= SimpleFunc().await;
			halt if( c.x != 776 );
			halt if( c.y != -0.125f );
		}
		fn Foo()
		{
			auto mut f= Bar();
			loop
			{
				if_coro_advance( r : f )
				{
					break;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AwaitOperator_Test2():
	c_program_text= """
		fn async SimpleFunc( i32& x ) : i32&
		{
			return x;
		}
		fn async Bar()
		{
			// Obtain a reference from "await".
			var i32 x= 765;
			auto x_ptr= unsafe( $<( cast_mut(x) ) );
			var i32& x_ref= SimpleFunc(x).await;
			halt if( x_ref != 765 );
			unsafe
			{
				halt if($<( cast_mut(x_ref) ) != x_ptr );
			}
		}
		fn Foo()
		{
			auto mut f= Bar();
			loop
			{
				if_coro_advance( r : f )
				{
					break;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AwaitOperator_Test3():
	c_program_text= """
		fn async SimpleFunc( i32 &mut x ) : i32 &mut
		{
			yield;
			return x;
		}
		fn async Bar()
		{
			// Obtain a mutable reference from "await".
			var i32 mut x= -1;
			SimpleFunc(x).await = 789;
			halt if( x != 789 );
		}
		fn Foo()
		{
			auto mut f= Bar();
			loop
			{
				if_coro_advance( r : f )
				{
					break;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AwaitOperator_Test4():
	c_program_text= """
		fn async SimpleFunc() : f64
		{
			return 3.141592535;
		}
		fn DoubleIt( f64 x ) : f64
		{
			return x * 2.0;
		}
		fn async Bar()
		{
			// Use "await" result as function argument.
			var f64 x= DoubleIt( SimpleFunc().await );
			halt if( x != 3.141592535 * 2.0 );
		}
		fn Foo()
		{
			auto mut f= Bar();
			loop
			{
				if_coro_advance( r : f )
				{
					break;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AwaitOperator_Test5():
	c_program_text= """
		fn async SimpleFunc() : u32
		{
			return 123456u;
		}
		fn async Bar()
		{
			// Use "await" result as binary operator argument (left part).
			var u32 x= SimpleFunc().await / 5u;
			halt if( x != 123456u / 5u );
		}
		fn Foo()
		{
			auto mut f= Bar();
			loop
			{
				if_coro_advance( r : f )
				{
					break;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AwaitOperator_Test6():
	c_program_text= """
		fn async SimpleFunc() : i32
		{
			return 66;
		}
		fn async Bar()
		{
			// Use "await" result as binary operator argument (right part).
			auto x= 666 - SimpleFunc().await;
			halt if( x != 600 );
		}
		fn Foo()
		{
			auto mut f= Bar();
			loop
			{
				if_coro_advance( r : f )
				{
					break;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AwaitOperator_Test7():
	c_program_text= """
		fn async MakeArr() : [ i32, 3 ]
		{
			var [ i32, 3 ] arr[ 1, 22, 333 ];
			return arr;
		}
		fn async Bar()
		{
			// Use "await" result in [] operator (left part).
			var i32 x= MakeArr().await [2];
			halt if( x != 333 );
		}
		fn Foo()
		{
			auto mut f= Bar();
			loop
			{
				if_coro_advance( r : f )
				{
					break;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AwaitOperator_Test8():
	c_program_text= """
		fn async SimpleFunc() : u32
		{
			return 1u;
		}
		fn async Bar()
		{
			var [ f32, 4 ] arr[ 0.5f, 0.25f, 0.125f, 0.0625f ];
			// Use "await" result in [] operator (right part).
			var f32 x= arr[ SimpleFunc().await ];
			halt if( x != 0.25f );
		}
		fn Foo()
		{
			auto mut f= Bar();
			loop
			{
				if_coro_advance( r : f )
				{
					break;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ImmediateValueExpectedInAwaitOperator_Test0():
	c_program_text= """
		fn async SomeFunc() : i32;
		fn async Bar()
		{
			auto f= SomeFunc();
			f.await; // Expected immediate value, got immutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ImmediateValueExpectedInAwaitOperator", 6 ) )


def ImmediateValueExpectedInAwaitOperator_Test1():
	c_program_text= """
		fn async SomeFunc() : i32;
		fn async Bar()
		{
			auto mut f= SomeFunc();
			f.await; // Expected immediate value, got mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ImmediateValueExpectedInAwaitOperator", 6 ) )


def AwaitForNonAsyncFunctionValue_Test0():
	c_program_text= """
		fn SomeFunc() : i32; // Regular (non-async) function.
		fn async Bar()
		{
			SomeFunc().await;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "AwaitForNonAsyncFunctionValue", 5 ) )


def AwaitForNonAsyncFunctionValue_Test1():
	c_program_text= """
		fn async Bar()
		{
			var i32 mut x= 121212;
			move(x).await; // await for fundamental type value.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "AwaitForNonAsyncFunctionValue", 5 ) )


def AwaitForNonAsyncFunctionValue_Test2():
	c_program_text= """
		struct S{}
		fn MakeS() : S;
		fn async Bar()
		{
			MakeS().await; // await for struct type value.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "AwaitForNonAsyncFunctionValue", 6 ) )


def AwaitForNonAsyncFunctionValue_Test3():
	c_program_text= """
		fn generator SomeGen();
		fn async Bar()
		{
			SomeGen().await; // await for generator (not an async function).
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "AwaitForNonAsyncFunctionValue", 5 ) )


def AwaitOutsideAsyncFunction_Test0():
	c_program_text= """
		fn async SomeFunc();
		fn Foo()
		{
			SomeFunc().await; // await in a regular function.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "AwaitOutsideAsyncFunction", 5 ) )


def AwaitOutsideAsyncFunction_Test1():
	c_program_text= """
		fn async SomeFunc();
		fn generator Foo()
		{
			SomeFunc().await;  // await in a generator function.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "AwaitOutsideAsyncFunction", 5 ) )


def AwaitOperatorResultReferences_Test0():
	c_program_text= """
		fn async Pass( i32& x ) : i32&;
		fn async Foo()
		{
			var i32 mut x= 0;
			auto& ref= Pass(x).await;
			++x; // Error - there is a reference to "x" - "ref".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 7 ) )


def AwaitOperatorResultReferences_Test1():
	c_program_text= """
		fn async Pass( i32& x ) : i32&;
		var [ [ char8, 2 ], 0 ] foo_return_references[];
		fn async Foo( i32& x ) : i32 & @(foo_return_references)
		{
			return Pass(x).await; // Result of "await" is linked with param "x", but it's not allowed to return a reference to this param.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReturningUnallowedReference", 6 ) )


def AwaitOperatorResultReferences_Test2():
	c_program_text= """
		struct S{ i32& r; }
		var tup[ [ [ char8, 2 ], 1 ] ] wrap_return_inner_references[ [ "0_" ] ];
		fn async Wrap( i32& x ) : S @(wrap_return_inner_references);
		fn async Foo()
		{
			var i32 mut x= 0;
			var S s= Wrap(x).await;
			++x; // Error - there is a reference to "x" inside "s".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 9 ) )


def AwaitOperatorResultReferences_Test3():
	c_program_text= """
		struct S{ i32& r; }
		var tup[ [ [ char8, 2 ], 1 ] ] wrap_return_inner_references[ [ "0_" ] ];
		fn async Wrap( i32& x ) : S @(wrap_return_inner_references);
		var tup[ [ [ char8, 2 ], 0 ] ] foo_return_inner_references[ [] ];
		fn async Foo( i32& x ) : S @(foo_return_inner_references)
		{
			return Wrap(x).await; // Result of "await" contains a reference to param "x" inside, but it's not allowed to return an inner reference to this param.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReturningUnallowedReference", 8 ) )
