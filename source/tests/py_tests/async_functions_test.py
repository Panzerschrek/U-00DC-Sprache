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


def AsyncFunctionTypeName_Test0():
	c_program_text= """
		fn async SimpleFunc() : u32 { return 0u; }
		fn Foo()
		{
			var async : u32 f= SimpleFunc();
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AsyncFunctionTypeName_Test1():
	c_program_text= """
		fn async SimpleFunc() : u32 { return 0u; }
		fn Foo()
		{
			var async : u32 mut f= SimpleFunc();
			var (async : u32) &mut f_ref= f;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AsyncFunctionTypeName_Test2():
	c_program_text= """
		fn async SimpleFunc() : u32 & { halt; }
		fn Foo()
		{
			var async : u32& f= SimpleFunc(); // Type name for async function that returns reference.
			var async : u32 & &imut f_ref= f;
			var (async : u32 &imut) & f_ref_ref= f_ref;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AsyncFunctionTypeName_Test3():
	c_program_text= """
		type FloatFunc= async : f32;
		type IntRefFunc= async : i32 &mut;
		type ArrayMutRefFunc= ((( async : [ u64, 4 ] &mut )));
	"""
	tests_lib.build_program( c_program_text )


def AsyncFunctionTypeName_Test4():
	c_program_text= """
		struct S
		{
			async : u64 f; // Use async function type name as name for struct field.
		}
		fn async SimpleFunc() : u64
		{
			return 12345u64;
		}
		fn Foo()
		{
			var S mut s{ .f= SimpleFunc() };
			if_coro_advance( x : s.f )
			{
				halt if( x != 12345u64);
			}
			else { halt; }

			if_coro_advance( x : s.f )
			{
				halt;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AsyncFunctionTypeName_Test5():
	c_program_text= """
		type Func= async'mut, imut, mut' : i32;
		static_assert( typeinfo</Func/>.references_tags_count == 3s );
	"""
	tests_lib.build_program( c_program_text )


def AsyncFunctionTypeName_Test6():
	c_program_text= """
		type MutFunc= async'imut' : i32 &;
		static_assert( typeinfo</MutFunc/>.references_tags_count == 1s );
	"""
	tests_lib.build_program( c_program_text )


def AsyncFunctionTypeName_Test7():
	c_program_text= """
		struct S{ i32 &imut x; }
		type ImutFunc= async'mut' : S;
		static_assert( typeinfo</ImutFunc/>.references_tags_count == 1s );
	"""
	tests_lib.build_program( c_program_text )


def AsyncFunctionTypeName_Test8():
	c_program_text= """
		type Func= async non_sync : i32;
		static_assert( non_sync</ Func /> );
	"""
	tests_lib.build_program( c_program_text )


def AsyncFunctionTypeName_Test9():
	c_program_text= """
		type Func= async'imut' non_sync : i32;
		static_assert( non_sync</ Func /> );
	"""
	tests_lib.build_program( c_program_text )


def AsyncFunctionTypeName_Test10():
	c_program_text= """
		type Func= async non_sync(false) : i32;
		static_assert( !non_sync</ Func /> );
	"""
	tests_lib.build_program( c_program_text )


def AsyncFunctionTypeName_Test11():
	c_program_text= """
		var [ [ char8, 2 ], 1 ] return_references[ "0a" ];
		type Func= async'imut' : i32 & @(return_references);
	"""
	tests_lib.build_program( c_program_text )


def AsyncFunctionTypeName_Test12():
	c_program_text= """
		struct S{ i32& mut x; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0a" ] ];
		type Func= async'mut' : S @(return_inner_references);
	"""
	tests_lib.build_program( c_program_text )


def AsyncFunctionTypeName_Test13():
	c_program_text= """
		type Gen= generator : i32;
		type AsyncFunc = async : i32;
		// Generator types and async function types are distinct.
		static_assert( !same_type</ Gen, AsyncFunc /> );
	"""
	tests_lib.build_program( c_program_text )


def AsyncFunctionTypeName_AsTemplateSignatureArgument_Test0():
	c_program_text= """
		template</ type T />
		struct S</ async : T />
		{
			type AsyncRet= T;
		}
		static_assert( typeinfo</ S</ async : bool />::AsyncRet />.is_bool );
	"""
	tests_lib.build_program( c_program_text )


def AsyncFunctionTypeName_AsTemplateSignatureArgument_Test1():
	c_program_text= """
		template</ type T />
		struct S</ async : tup[T] />
		{
			type AsyncRet= T;
		}
		static_assert( typeinfo</ S</ async : tup[f32] />::AsyncRet />.is_float );
	"""
	tests_lib.build_program( c_program_text )


def AsyncFunctionTypeName_AsTemplateSignatureArgument_Test2():
	c_program_text= """
		template</ type T />
		struct S</ async : T& />
		{
			type AsyncRet= T;
		}
		type Some= S</ async : i32 />; // Deduction failed - expected generator, returning reference, given generator, returning value.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "TemplateParametersDeductionFailed", 7 ) )


def AsyncFunctionTypeName_AsTemplateSignatureArgument_Test3():
	c_program_text= """
		template</ type T />
		struct S</ async non_sync : T />
		{
			type AsyncRet= T;
		}
		type Some= S</ async : i32 />; // Deduction failed - expected "non_sync" generator.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "TemplateParametersDeductionFailed", 7 ) )


def AsyncFunctionTypeName_AsTemplateSignatureArgument_Test4():
	c_program_text= """
		template</ type T />
		struct S</ generator : T />
		{
			var bool is_generator= true;
			var bool is_async_func= false;
		}

		template</ type T />
		struct S</ async : T />
		{
			var bool is_generator= false;
			var bool is_async_func= true;
		}

		type SForGen= S</ generator : u32 />;
		type SForAsyncFunc= S</ async : u32 />;
		static_assert( SForGen::is_generator );
		static_assert( !SForGen::is_async_func );
		static_assert( !SForAsyncFunc::is_generator );
		static_assert( SForAsyncFunc::is_async_func );
	"""
	tests_lib.build_program( c_program_text )


def Typeinfo_ForAsyncFunctions_Test0():
	c_program_text= """
		type IntAsyncFunc= async : i32;
		auto& int_async_func_typeinfo= typeinfo</IntAsyncFunc/>;

		static_assert( int_async_func_typeinfo.size_of == typeinfo</$(byte8)/>.size_of ); // Coroutines have size of pointer.
		static_assert( int_async_func_typeinfo.is_class ); // Async functions are classes, because they have destructor method.
		static_assert( int_async_func_typeinfo.is_coroutine );
		static_assert( !int_async_func_typeinfo.is_generator );
		static_assert( int_async_func_typeinfo.is_async_func );
		static_assert( int_async_func_typeinfo.coroutine_return_type.is_integer );
		static_assert( int_async_func_typeinfo.coroutine_return_type.size_of == 4s );
		static_assert( !int_async_func_typeinfo.coroutine_return_value_is_mutable );
		static_assert( !int_async_func_typeinfo.coroutine_return_value_is_reference );
		static_assert( int_async_func_typeinfo.references_tags_count == 0s ); // This coroutine type has no references inside.
	"""
	tests_lib.build_program( c_program_text )


def Typeinfo_ForAsyncFunctions_Test1():
	c_program_text= """
		type F64RefAsyncFunc= async'imut' : f64 &;
		auto& f64_async_func_gen_typeinfo= typeinfo</F64RefAsyncFunc/>;

		static_assert( f64_async_func_gen_typeinfo.size_of == typeinfo</$(byte8)/>.size_of ); // Coroutines have size of pointer.
		static_assert( f64_async_func_gen_typeinfo.is_class ); // Async functions are classes, because they have destructor method.
		static_assert( f64_async_func_gen_typeinfo.is_coroutine );
		static_assert( !f64_async_func_gen_typeinfo.is_generator );
		static_assert( f64_async_func_gen_typeinfo.is_async_func );
		static_assert( f64_async_func_gen_typeinfo.coroutine_return_type.is_float );
		static_assert( f64_async_func_gen_typeinfo.coroutine_return_type.size_of == 8s );
		static_assert( !f64_async_func_gen_typeinfo.coroutine_return_value_is_mutable );
		static_assert( f64_async_func_gen_typeinfo.coroutine_return_value_is_reference );
		static_assert( f64_async_func_gen_typeinfo.references_tags_count == 1s ); // This coroutine type has references inside.
	"""
	tests_lib.build_program( c_program_text )


def Typeinfo_ForAsyncFunctions_Test2():
	c_program_text= """
		type MutCharRefAsyncFunc= async'mut' : char8 &mut;
		auto& mut_ref_char_async_func_typeinfo= typeinfo</MutCharRefAsyncFunc/>;

		static_assert( mut_ref_char_async_func_typeinfo.size_of == typeinfo</$(byte8)/>.size_of ); // Coroutines have size of pointer.
		static_assert( mut_ref_char_async_func_typeinfo.is_class ); // Generators are classes, because they have destructor method.
		static_assert( mut_ref_char_async_func_typeinfo.is_coroutine );
		static_assert( !mut_ref_char_async_func_typeinfo.is_generator );
		static_assert( mut_ref_char_async_func_typeinfo.is_async_func );
		static_assert( mut_ref_char_async_func_typeinfo.coroutine_return_type.is_char );
		static_assert( mut_ref_char_async_func_typeinfo.coroutine_return_type.size_of == 1s );
		static_assert( mut_ref_char_async_func_typeinfo.coroutine_return_value_is_mutable );
		static_assert( mut_ref_char_async_func_typeinfo.coroutine_return_value_is_reference );
		static_assert( mut_ref_char_async_func_typeinfo.references_tags_count == 1s ); // This coroutine type has references inside.
	"""
	tests_lib.build_program( c_program_text )
