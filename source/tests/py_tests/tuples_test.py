from py_tests_common import *


def TupleTypeParsing_Test0():
	c_program_text= """
		fn Foo( tup(i32, f32) & arg0 );
	"""
	tests_lib.build_program( c_program_text )


def TupleTypeParsing_Test1():
	c_program_text= """
		type EmptyTuple= tup();
		type LooooongTuple= tup( i32, f32, f32, bool, [ i32, 2 ], char8, fn(), [ char16, 8 ] );
	"""
	tests_lib.build_program( c_program_text )


def TupleElementAccess_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			var tup( f32, i32 ) mut t= zero_init;
			t[0u]= 45.3f;
			t[1u]= 9;
			return i32(t[0u]) - t[ u64(7 - 6) ];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 45 - 9 )


def TupleElementAccess_Test1():
	c_program_text= """
		fn Foo()
		{
			var tup( f32, i32 ) mut t= zero_init;
			var u32 mut x(0);
			t[x]; // index is not constant
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedConstantExpression" )
	assert( errors_list[0].file_pos.line == 6 )


def TupleElementAccess_Test2():
	c_program_text= """
		fn Foo()
		{
			var tup( f32, i32 ) mut t= zero_init;
			t[ 0.5f ]; // index is not integer
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 5 )


def TupleElementAccess_Test3():
	c_program_text= """
		fn Foo()
		{
			var tup( f32, i32 ) mut t= zero_init;
			t[7u]; // index out of bounds
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TupleIndexOutOfBounds" )
	assert( errors_list[0].file_pos.line == 5 )


def TupleElementAccess_Test4():
	c_program_text= """
		fn Foo()
		{
			var tup( f32, i32 ) mut t= zero_init;
			t[2u]; // index out of bounds
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TupleIndexOutOfBounds" )
	assert( errors_list[0].file_pos.line == 5 )


def TupleElementAccess_Test5():
	c_program_text= """
		fn Foo()
		{
			var tup() mut t= zero_init;
			t[0u]; // index out of bounds
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TupleIndexOutOfBounds" )
	assert( errors_list[0].file_pos.line == 5 )


def TupleFunctionArgument_Test0():
	c_program_text= """
		fn Diff( tup( f32, i32 ) t ) : i32 // Immutable value argument
		{
			return i32(t[0u]) - t[1u];
		}
		fn Foo() : i32
		{
			var tup( f32, i32 ) t[ 52.1f, 6 ];
			return Diff(t);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 52 - 6 )


def TupleFunctionArgument_Test1():
	c_program_text= """
		fn Diff( tup( f32, i32 ) mut t ) : i32 // Mutable value argument
		{
			t[0u]*= 3.0f;
			return i32(t[0u]) - t[1u];
		}
		fn Foo() : i32
		{
			var tup( f32, i32 ) t[ 52.1f, 17 ];
			return Diff(t);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 52 * 3 - 17 )


def TupleFunctionArgument_Test2():
	c_program_text= """
		fn ZeroIt( tup( f32, i32 ) &mut t ) // Mutable reference argument
		{
			t[0u]= 0.0f;
			t[1u]= 0;
		}
		fn Foo() : i32
		{
			var tup( f32, i32 ) mut t[ 6521.3f, -142 ];
			ZeroIt(t);
			return i32(t[0u]) + t[1u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0 )


def TupleReturnValue_Test0():
	c_program_text= """
		fn MakeTuple( i32 x, f64 y ) : tup( i32, f64 )
		{
			var tup( i32, f64 ) t[ x, y ];
			return t; // return copy
		}
		fn Foo() : i32
		{
			var tup( i32, f64 ) t= MakeTuple( 72, 52.0 );
			return t[0u] - i32(t[1u]);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 72 - 52 )


def TupleReturnValue_Test1():
	c_program_text= """
		fn MakeTuple( i32 x, f64 y ) : tup( i32, f64 )
		{
			var tup( i32, f64 ) mut t[ x, y ];
			return move(t); // return moved value
		}
		fn Foo() : i32
		{
			var tup( i32, f64 ) t= MakeTuple( 256, 13.0 );
			return t[0u] - i32(t[1u]);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 256 - 13 )


def TupleReturnValue_Test2():
	c_program_text= """
		fn Pass( tup( f32, i32 ) &imut t ) : tup( f32, i32 ) &imut // return reference to tuple
		{
			return t;
		}
		fn Foo() : i32
		{
			var tup( f32, i32 ) t[ 124.1f, 22 ];
			return i32(Pass(t)[0u]) - Pass(t)[1u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 124 - 22 )


def Typeinfo_ForTuples_Test0():
	c_program_text= """
		struct S{}
		static_assert( typeinfo</ tup() />.is_tuple );
		static_assert( typeinfo</ tup( i32 ) />.is_tuple );
		static_assert( typeinfo</ tup( f32, bool ) />.is_tuple );
		static_assert( typeinfo</ tup( S ) />.is_tuple );
		static_assert( typeinfo</ tup( S, bool, S ) />.is_tuple );
	"""
	tests_lib.build_program( c_program_text )


def Typeinfo_ForTuples_Test1():
	c_program_text= """
		struct S{}
		static_assert( typeinfo</ tup() />.element_count == size_type(0) );
		static_assert( typeinfo</ tup( i32 ) />.element_count == size_type(1) );
		static_assert( typeinfo</ tup( f32, bool ) />.element_count == size_type(2) );
		static_assert( typeinfo</ tup( S ) />.element_count == size_type(1) );
		static_assert( typeinfo</ tup( S, bool, S ) />.element_count == size_type(3) );
		static_assert( typeinfo</ tup( i32, i32, i32, bool, char8, f64, f32, i16, u8, u64 ) />.element_count == size_type(10) );
	"""
	tests_lib.build_program( c_program_text )


def Typeinfo_ForTuples_Test2():
	c_program_text= """
		template</ type T />
		fn constexpr TupleMemebersListNodeOffset( T& node, size_type index ) : size_type
		{
			static_if( T::is_end )
			{
				halt;
			}
			else
			{
				if( node.index == index )
				{
					return node.offset;
				}
				else
				{
					return ::TupleMemebersListNodeOffset( node.next, index );
				}
			}
		}

		static_assert( typeinfo</ tup(i32) />.elements_list.type.is_signed_integer );
		static_assert( typeinfo</ tup(i32) />.elements_list.offset == size_type(0) );

		type T= tup( u64, i64, i32, f32, char8, u8 );
		static_assert( TupleMemebersListNodeOffset( typeinfo</T/>.elements_list, size_type(0) ) == size_type(0) );
		static_assert( TupleMemebersListNodeOffset( typeinfo</T/>.elements_list, size_type(1) ) == size_type(8) );
		static_assert( TupleMemebersListNodeOffset( typeinfo</T/>.elements_list, size_type(2) ) == size_type(16) );
		static_assert( TupleMemebersListNodeOffset( typeinfo</T/>.elements_list, size_type(3) ) == size_type(20) );
		static_assert( TupleMemebersListNodeOffset( typeinfo</T/>.elements_list, size_type(4) ) == size_type(24) );
		static_assert( TupleMemebersListNodeOffset( typeinfo</T/>.elements_list, size_type(5) ) == size_type(25) );

	"""
	tests_lib.build_program( c_program_text )


def TupleCopyAssignment_Test0():
	c_program_text= """
		fn Foo()
		{
			var tup( f32, i32 ) t0[ 552.7f, 1237827 ];
			var tup( f32, i32 ) mut t1= zero_init;
			t1= t0;
			halt if( t1[0u] != 552.7f );
			halt if( t1[1u] !=1237827 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TupleCopyAssignment_Test1():
	c_program_text= """
		struct S{ i32 x; i32 y; }
		fn Foo()
		{
			var tup( S, bool ) t0[ { .x= 42, .y= 24 }, true ];
			var tup( S, bool ) mut t1= zero_init;
			t1= t0;
			halt if( t1[0u].x != 42 );
			halt if( t1[0u].y != 24 );
			halt if( t1[1u] != true );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TupleCopyAssignment_Test2():
	c_program_text= """
		fn Foo()
		{
			var tup( [ i32, 2 ], char8 ) t0[ [ 639, 582 ], "q"c8 ];
			var tup( [ i32, 2 ], char8 ) mut t1= zero_init;
			t1= t0; // Assignemnt works also for array elements of tuples.
			halt if( t1[0u][0u] != 639 );
			halt if( t1[0u][1u] != 582 );
			halt if( t1[1u] != "q"c8 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TupleCopyAssignment_Test3():
	c_program_text= """
		struct S
		{
			op=( mut this, S &imut other )= delete;
			i32 x; i32 y;
		}
		fn Foo()
		{
			var tup( S, bool ) t0[ { .x= 42, .y= 24 }, true ];
			var tup( S, bool ) mut t1= zero_init;
			t1= t0;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OperationNotSupportedForThisType" )
	assert( errors_list[0].file_pos.line == 11 )


def TupleCopyAssignment_Test3():
	c_program_text= """
		fn Foo()
		{
			var tup( f32, bool ) t0[ 0.25f, false ];
			var tup( f32, bool ) t1= zero_init;
			t1= t0; // Assign to const referenfe
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedReferenceValue" )
	assert( errors_list[0].file_pos.line == 6 )


def TupleFor_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			var tup( f32, i32, i64 ) t[ 952.1f, 56, 741i64 ];
			var i32 mut res= 0;
			for( e : t )
			{
				res+= i32(e);
			}
			return res;
		}
	"""
	tests_lib.build_program( c_program_text, True )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 952 + 56 + 741 )
