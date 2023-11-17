from py_tests_common import *


def SimpleAsyncFunction_Test0():
	c_program_text= """
		fn async SimpleFunc() : i32
		{
			return 42;
		}
		fn Foo()
		{
			auto mut f= SimpleFunc();
			if_coro_advance( x : f )
			{
				halt if( x != 42 );
			}
			else { halt; }

			if_coro_advance( x : f )
			{
				halt;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SimpleAsyncFunction_Test1():
	c_program_text= """
		fn async SimpleFunc() : i32
		{
			yield;
			yield;
			yield;
			return 555444;
		}
		fn Foo()
		{
			auto mut f= SimpleFunc();
			auto mut result= 0;
			auto mut num_iterations= 0s;
			loop
			{
				++num_iterations;
				if_coro_advance( x : f )
				{
					result= x;
					break;
				}
			}
			halt if( result != 555444 );
			halt if( num_iterations != 4s );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SimpleAsyncFunction_Test2():
	c_program_text= """
		fn async SimpleFunc() : i32
		{
			return 8877;
		}
		fn async SimpleFuncWrapper() : i32
		{
			return SimpleFunc().await;
		}
		fn Foo()
		{
			auto mut f= SimpleFuncWrapper();
			if_coro_advance( x : f )
			{
				halt if( x != 8877 );
			}
			else { halt; }
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ReturnForAsyncFunction_Test0():
	c_program_text= """
		fn async SimpleFunc( i32 x ) : i32
		{
			// Return fundamental type value
			return x * 3;
		}
		fn Foo()
		{
			auto mut f= SimpleFunc( 17 );
			if_coro_advance( x : f )
			{
				halt if( x != 17 * 3 );
			}
			else { halt; }

			if_coro_advance( x : f )
			{
				halt;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ReturnForAsyncFunction_Test1():
	c_program_text= """
		fn async SimpleFunc() : [ f32, 3 ]
		{
			// Return array
			var [ f32, 3 ] res[ 1.0f, -2.0f, 7.5f ];
			return res;
		}
		fn Foo()
		{
			auto mut f= SimpleFunc();
			if_coro_advance( x : f )
			{
				var [ f32, 3 ] x_expected[ 1.0f, -2.0f, 7.5f ];
				halt if( x != x_expected );
			}
			else { halt; }

			if_coro_advance( x : f )
			{
				halt;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ReturnForAsyncFunction_Test2():
	c_program_text= """
		fn async SimpleFunc() : tup[ u64, char8 ]
		{
			// Return tuple
			var tup[ u64, char8 ] res[ 7778u64, "&"c8 ];
			return res;
		}
		fn Foo()
		{
			auto mut f= SimpleFunc();
			if_coro_advance( x : f )
			{
				var tup[ u64, char8 ] x_expected[ 7778u64, "&"c8 ];
				halt if( x != x_expected );
			}
			else { halt; }

			if_coro_advance( x : f )
			{
				halt;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ReturnForAsyncFunction_Test3():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
			bool b;
		}
		fn async SimpleFunc() : S
		{
			// Return struct
			var S res{ .x= 765, .y= -0.33f, .b= true };
			return res;
		}
		fn Foo()
		{
			auto mut f= SimpleFunc();
			if_coro_advance( x : f )
			{
				var S x_expected{ .x= 765, .y= -0.33f, .b= true };
				halt if( x != x_expected );
			}
			else { halt; }

			if_coro_advance( x : f )
			{
				halt;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ReturnForAsyncFunction_Test4():
	c_program_text= """
		// Return a reference to one of the params.
		fn async AsyncMax( i32& x, i32& y ) : i32 &
		{
			if( x > y ) { return x; }
			return y;
		}
		fn Foo()
		{
			var [ [ i32, 2 ], 3 ] pairs
			[
				[ 7, 87 ],
				[ 765, -14 ],
				[ 66, 66 ],
			];
			for( auto mut i= 0s; i < 3s; ++i )
			{
				auto& pair= pairs[i];
				auto mut f= AsyncMax( pair[0], pair[1] );
				if_coro_advance( &x : f )
				{
					auto own_max= select( pair[0] > pair[1] ? pair[0] : pair[1] );
					halt if( x != own_max );
				}
				else { halt; }

				if_coro_advance( &x : f )
				{
					halt;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ReturnForAsyncFunction_Test5():
	c_program_text= """
		// Return a mutable reference to one of the params.
		fn async AsyncMax( i32 &mut x, i32 &mut y ) : i32 &mut
		{
			if( x > y ) { return x; }
			return y;
		}
		fn Foo()
		{
			var [ [ i32, 2 ], 3 ] mut pairs
			[
				[ 12345, -76 ],
				[ 77, 773233 ],
				[ -99886, -99886 ],
			];
			for( auto mut i= 0s; i < 3s; ++i )
			{
				var i32 mut first= pairs[i][0], mut second= pairs[i][1];
				var i32 own_max= select( first > second ? first : second );
				auto mut f= AsyncMax( first, second );
				if_coro_advance( &mut x : f )
				{
					halt if( x != own_max );
					x= 0;
				}
				else { halt; }

				halt if( !( first == 0 || second == 0 ) );

				if_coro_advance( &mut x : f )
				{
					halt;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ReturnForAsyncFunction_Test6():
	c_program_text= """
		fn async Foo()
		{
			// It's fine to have no "return" in async function returning void.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ReturnForAsyncFunction_Test7():
	c_program_text= """
		fn async Foo()
		{
			return; // Empty return for void-return async function.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
