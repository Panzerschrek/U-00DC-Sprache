from py_tests_common import *


def TupleTypeParsing_Test0():
	c_program_text= """
		fn Foo( tup[i32, f32] & arg0 );
	"""
	tests_lib.build_program( c_program_text )


def TupleTypeParsing_Test1():
	c_program_text= """
		type EmptyTuple= tup[];
		type LooooongTuple= tup[ i32, f32, f32, bool, [ i32, 2 ], char8, fn(), [ char16, 8 ] ];
	"""
	tests_lib.build_program( c_program_text )


def TupleElementAccess_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			var tup[ f32, i32 ] mut t= zero_init;
			t[0]= 45.3f;
			t[1]= 9;
			return i32(t[0]) - t[ u64(7 - 6) ];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 45 - 9 )


def TupleElementAccess_Test1():
	c_program_text= """
		fn Foo()
		{
			var tup[ f32, i32 ] mut t= zero_init;
			var u32 mut x(0);
			t[x]; // index is not constant
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedConstantExpression" )
	assert( errors_list[0].src_loc.line == 6 )


def TupleElementAccess_Test2():
	c_program_text= """
		fn Foo()
		{
			var tup[ f32, i32 ] mut t= zero_init;
			t[ 0.5f ]; // index is not integer
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 5 )


def TupleElementAccess_Test3():
	c_program_text= """
		fn Foo()
		{
			var tup[ f32, i32 ] mut t= zero_init;
			t[7u]; // index out of bounds
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TupleIndexOutOfBounds" )
	assert( errors_list[0].src_loc.line == 5 )


def TupleElementAccess_Test4():
	c_program_text= """
		fn Foo()
		{
			var tup[ f32, i32 ] mut t= zero_init;
			t[2u]; // index out of bounds
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TupleIndexOutOfBounds" )
	assert( errors_list[0].src_loc.line == 5 )


def TupleElementAccess_Test5():
	c_program_text= """
		fn Foo()
		{
			var tup[] mut t= zero_init;
			t[0u]; // index out of bounds
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TupleIndexOutOfBounds" )
	assert( errors_list[0].src_loc.line == 5 )


def TupleElementAccess_Test6():
	c_program_text= """
		fn Foo()
		{
			var tup[ f32, i32 ] mut t= zero_init;
			t[ -1 ]; // index out of bounds
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TupleIndexOutOfBounds" )
	assert( errors_list[0].src_loc.line == 5 )


def TupleFunctionArgument_Test0():
	c_program_text= """
		fn Diff( tup[ f32, i32 ] t ) : i32 // Immutable value argument
		{
			return i32(t[0u]) - t[1u];
		}
		fn Foo() : i32
		{
			var tup[ f32, i32 ] t[ 52.1f, 6 ];
			return Diff(t);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 52 - 6 )


def TupleFunctionArgument_Test1():
	c_program_text= """
		fn Diff( tup[ f32, i32 ] mut t ) : i32 // Mutable value argument
		{
			t[0u]*= 3.0f;
			return i32(t[0u]) - t[1u];
		}
		fn Foo() : i32
		{
			var tup[ f32, i32 ] t[ 52.1f, 17 ];
			return Diff(t);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 52 * 3 - 17 )


def TupleFunctionArgument_Test2():
	c_program_text= """
		fn ZeroIt( tup[ f32, i32 ] &mut t ) // Mutable reference argument
		{
			t[0u]= 0.0f;
			t[1u]= 0;
		}
		fn Foo() : i32
		{
			var tup[ f32, i32 ] mut t[ 6521.3f, -142 ];
			ZeroIt(t);
			return i32(t[0u]) + t[1u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0 )


def TupleReturnValue_Test0():
	c_program_text= """
		fn MakeTuple( i32 x, f64 y ) : tup[ i32, f64 ]
		{
			var tup[ i32, f64 ] t[ x, y ];
			return t; // return copy
		}
		fn Foo() : i32
		{
			var tup[ i32, f64 ] t= MakeTuple( 72, 52.0 );
			return t[0u] - i32(t[1u]);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 72 - 52 )


def TupleReturnValue_Test1():
	c_program_text= """
		fn MakeTuple( i32 x, f64 y ) : tup[ i32, f64 ]
		{
			var tup[ i32, f64 ] mut t[ x, y ];
			return move(t); // return moved value
		}
		fn Foo() : i32
		{
			var tup[ i32, f64 ] t= MakeTuple( 256, 13.0 );
			return t[0u] - i32(t[1u]);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 256 - 13 )


def TupleReturnValue_Test2():
	c_program_text= """
		fn Pass( tup[ f32, i32 ] &imut t ) : tup[ f32, i32 ] &imut // return reference to tuple
		{
			return t;
		}
		fn Foo() : i32
		{
			var tup[ f32, i32 ] t[ 124.1f, 22 ];
			return i32(Pass(t)[0u]) - Pass(t)[1u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 124 - 22 )


def Typeinfo_ForTuples_Test0():
	c_program_text= """
		struct S{}
		static_assert( typeinfo</ tup[] />.is_tuple );
		static_assert( typeinfo</ tup[ i32 ] />.is_tuple );
		static_assert( typeinfo</ tup[ f32, bool ] />.is_tuple );
		static_assert( typeinfo</ tup[ S ] />.is_tuple );
		static_assert( typeinfo</ tup[ S, bool, S ] />.is_tuple );
	"""
	tests_lib.build_program( c_program_text )


def Typeinfo_ForTuples_Test1():
	c_program_text= """
		struct S{}
		static_assert( typeinfo</ tup[] />.element_count == size_type(0) );
		static_assert( typeinfo</ tup[ i32 ] />.element_count == size_type(1) );
		static_assert( typeinfo</ tup[ f32, bool ] />.element_count == size_type(2) );
		static_assert( typeinfo</ tup[ S ] />.element_count == size_type(1) );
		static_assert( typeinfo</ tup[ S, bool, S ] />.element_count == size_type(3) );
		static_assert( typeinfo</ tup[ i32, i32, i32, bool, char8, f64, f32, i16, u8, u64 ] />.element_count == size_type(10) );
	"""
	tests_lib.build_program( c_program_text )


def Typeinfo_ForTuples_Test2():
	c_program_text= """
		template</ type T />
		fn constexpr TupleMemebersListNodeOffset( T& list, size_type index ) : size_type
		{
			for( & list_element : list )
			{
				if( list_element.index == index )
				{
					return list_element.offset;
				}
			}
			halt;
		}

		static_assert( typeinfo</ tup[i32] />.elements_list[0].type.is_signed_integer );
		static_assert( typeinfo</ tup[i32] />.elements_list[0].offset == size_type(0) );

		type T= tup[ u64, i64, i32, f32, char8, u8 ];
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
			var tup[ f32, i32 ] t0[ 552.7f, 1237827 ];
			var tup[ f32, i32 ] mut t1= zero_init;
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
			var tup[ S, bool ] t0[ { .x= 42, .y= 24 }, true ];
			var tup[ S, bool ] mut t1= zero_init;
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
			var tup[ [ i32, 2 ], char8 ] t0[ [ 639, 582 ], "q"c8 ];
			var tup[ [ i32, 2 ], char8 ] mut t1= zero_init;
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
			var tup[ S, bool ] t0[ { .x= 42, .y= 24 }, true ];
			var tup[ S, bool ] mut t1= zero_init;
			t1= t0;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OperationNotSupportedForThisType" )
	assert( errors_list[0].src_loc.line == 11 )


def TupleCopyAssignment_Test4():
	c_program_text= """
		fn Foo()
		{
			var tup[ f32, bool ] t0[ 0.25f, false ];
			var tup[ f32, bool ] t1= zero_init;
			t1= t0; // Assign to const referenfe
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedReferenceValue" )
	assert( errors_list[0].src_loc.line == 6 )


def TupleMoveAsignment_Test0():
	c_program_text= """
		class C // Class is noncopyable.
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
		}
		fn Foo()
		{
			var tup[ f32, C, bool ] mut t0[ 0.25f, C(654), true ], mut t1[ 0.0f, C(0), false ];
			t1= move(t0); // Should move assign here.
			halt if( t1[0] != 0.25f );
			halt if( t1[1].x != 654 );
			halt if( t1[2] != true );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TupleFor_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			var tup[ f32, i32, i64 ] t[ 952.1f, 56, 741i64 ];
			var i32 mut res= 0;
			for( e : t ) // Value for element
			{
				res+= i32(e);
			}
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 952 + 56 + 741 )


def TupleFor_Test1():
	c_program_text= """
		fn Foo()
		{
			var tup[ f32, i32, i64 ] mut t[ 0.25f, 14, 29i64 ];
			for( mut e : t ) // Can modify value for element
			{
				e*= typeof(e)(10);
			}
			// Tuple itslef unchanged
			halt if( t[0u] != 0.25f);
			halt if( t[1u] != 14 );
			halt if( t[2u] != 29i64 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TupleFor_Test2():
	c_program_text= """
		fn Foo()
		{
			var tup[ f32, i32, i64 ] mut t[ 12.1f, 745, 124i64 ];
			for( &mut e : t ) // Mutable reference for element
			{
				e*= typeof(e)(2);
			}
			halt if( t[0u] != 12.1f * 2.0f );
			halt if( t[1u] != 745 * 2 );
			halt if( t[2u] != 124i64 * 2i64 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TupleFor_Test3():
	c_program_text= """
		fn Foo()
		{
			var tup[ f32, i32, i64 ] t= zero_init;
			for( &mut e : t ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "BindingConstReferenceToNonconstReference" )
	assert( errors_list[0].src_loc.line == 5 )


def TupleFor_Test4():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S &imut other )= delete;
		}
		fn Foo()
		{
			var tup[ S ] t= zero_init;
			for( e : t ) {} // Can not copy value, because on of types is not copy-constructible.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CopyConstructValueOfNoncopyableType" )
	assert( errors_list[0].src_loc.line == 9 )


def TupleFor_Test5():
	c_program_text= """
		fn Foo() : f32
		{
			var tup[ f32, i64, bool ] t[ 0.25f, 29i64, true ];
			for( e : t )
			{
				return e;
				// Later iterations of the loop are ignored, because first iteration is terminal.
			}
			// 'return' does not required here.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result = tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0.25 )


def TupleFor_Test6():
	c_program_text= """
		struct S{}
		template</type T/> struct IsF64{ auto value= false; }
		template<//> struct IsF64</f64/>{ auto value= true; }
		fn Foo() : f64
		{
			var tup[ i32, f64, S ] t[ 33, 1.25, {} ];
			var f64 mut res= 0.0;
			for( e : t )
			{
				res+= f64(e);
				static_if( IsF64</ typeof(e) />::value )
				{
					break;
				}
				// Loop iterations after "f64" are ignored, because iteration with "f64" is terminal.
			}
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result = tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 34.25 )


def TupleFor_Test7():
	c_program_text= """
		fn Foo()
		{
			var tup[ bool, f32 ] t= zero_init;
			for( e : t )
			{
				return;
			}
			auto x= 0; // This code is unreachable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnreachableCode" )
	assert( errors_list[0].src_loc.line == 9 )


def TupleFor_Test8():
	c_program_text= """
		fn Foo()
		{
			var tup[ f32, i32, i64 ] t= zero_init;
			for( &e : t )
			{
				// Tuple-for with reference preserves constexpr.
				static_assert( f64(e) == 0.0 );
			}
			for( e : t )
			{
				// Tuple-for with value preserves constexpr.
				static_assert( f64(e) == 0.0 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleFor_Test9():
	c_program_text= """
		fn Foo()
		{
			var tup[ f32, i32, i64 ] t= zero_init;
			for( mut e : t ) // Tuple element is initialized with constant but not is actually constant, because of "mut"
			{
				static_assert( f64(e) == 0.0 );
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "StaticAssertExpressionIsNotConstant", 7 ) )


def AutoVariableDeclaration_ForTuples_Test0():
	c_program_text= """
		fn Foo()
		{
			var tup[ f32, i32 ] t0[ 75.5f, 666 ];
			auto t1= t0; // Copy tuple with fundamental elements
			halt if( t1[0u] != 75.5f );
			halt if( t1[1u] != 666 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoVariableDeclaration_ForTuples_Test1():
	c_program_text= """
		struct S{ i32 x; i32 y; }
		fn Foo()
		{
			var tup[ S, bool ] t0[ { .x= 42, .y= 24 }, true ];
			auto t1= t0; // Copy tuple with struct elements
			halt if( t1[0u].x != 42 );
			halt if( t1[0u].y != 24 );
			halt if( t1[1u] != true );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoVariableDeclaration_ForTuples_Test3():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S &imut other )= delete;
			i32 x; i32 y;
		}
		fn Foo()
		{
			var tup[ S, bool ] t0[ { .x= 42, .y= 24 }, true ];
			auto t1= t0;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CopyConstructValueOfNoncopyableType" )
	assert( errors_list[0].src_loc.line == 10 )


def TupleFieldCopy_Test0():
	c_program_text= """
		struct S
		{
			tup[ i32, f32 ] t;
		}
		fn Foo()
		{
			var S s{ .t[ 541, 56.0f ] };
			var S s_copy(s); // Generated copy constructor must copy tuple elements.
			halt if( s_copy.t[0u] != 541 );
			halt if( s_copy.t[1u] != 56.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TupleFieldCopy_Test1():
	c_program_text= """
		struct S
		{
			tup[ f32, i32 ] t;
		}
		fn Foo()
		{
			var S s0{ .t[ -92.5f, 11111 ] };
			var S mut s1= zero_init;
			s1= s0; // Generated copy assignnment operator must copy tuple elements.
			halt if( s1.t[0u] != -92.5f );
			halt if( s1.t[1u] != 11111 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TupleFieldCopy_Test2():
	c_program_text= """
		struct T
		{
			fn constructor( mut this, T &imut other )= delete;
		}
		struct S
		{
			tup[ T ] t;
		}
		fn Foo()
		{
			var S s;
			var S s_copy(s); // Copy constructor not generated, because memmber of struct tuple field is noncopyable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 13 )


def TupleFieldCopy_Test3():
	c_program_text= """
		struct T
		{
			op=( mut this, T &imut other )= delete;
		}
		struct S
		{
			tup[ T ] t;
		}
		fn Foo()
		{
			var S s0;
			var S mut s1;
			s1= s0; // Copy assignment operator not generated, because memmber of struct tuple field is noncopyable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OperationNotSupportedForThisType" )
	assert( errors_list[0].src_loc.line == 14 )


def TuplesConstexpr_Test0():
	c_program_text= """
		var tup[ i32, f32 ] constexpr t[ 851, 52.3f ]; // Array initializer can produce constexpr tuple value.
		static_assert( t[0u] == 851 );
		static_assert( t[1u] == 52.3f );
	"""
	tests_lib.build_program( c_program_text )


def TuplesConstexpr_Test1():
	c_program_text= """
		var tup[ i32, f32 ] constexpr t[ -52, 3.5f ];
		var tup[ i32, f32 ] constexpr t_copy0(t); // Constructor initializer can produce constexpr tuple value.
		var tup[ i32, f32 ] constexpr t_copy1= t; // Expression initializer can produce constexpr tuple value.
		auto constexpr t_copy2= t; // Auto variable initialization can produce constexpr tuple value.
		static_assert( t_copy0[0u] == -52 );
		static_assert( t_copy0[1u] == 3.5f );
		static_assert( t_copy0[0u] == t[0u] );
		static_assert( t_copy0[1u] == t[1u] );
		static_assert( t_copy1[0u] == t[0u] );
		static_assert( t_copy1[1u] == t[1u] );
		static_assert( t_copy2[0u] == t[0u] );
		static_assert( t_copy2[1u] == t[1u] );
	"""
	tests_lib.build_program( c_program_text )


def TuplesConstexpr_Test2():
	c_program_text= """
		struct S
		{
			i32 x= 0; i32 y= 0;
		}
		var tup[ S ] constexpr t0[ {} ], constexpr t1[ { .x= 555, .y= 222 } ];
		static_assert( t0[0u].x == 0 );
		static_assert( t0[0u].y == 0 );
		static_assert( t1[0u].x == 555 );
		static_assert( t1[0u].y == 222 );
	"""
	tests_lib.build_program( c_program_text )


def TuplesConstexpr_Test3():
	c_program_text= """
		fn constexpr MakeTuple( i32 x, f64 y ) : tup[ i32, f64 ]
		{
			var tup[ i32, f64 ] t[ x, y ];
			return t;
		}
		auto constexpr t= MakeTuple( -65643, 3.25 );
		static_assert( t[0] == -65643 );
		static_assert( t[1] == 3.25 );
	"""
	tests_lib.build_program( c_program_text )


def TupleTemplateArgument_Test0():
	c_program_text= """
		template</ type T />
		struct S</ tup[ f32, T ] />
		{
			type RealT= T;
		}

		fn Foo()
		{
			var S</ tup[ f32, bool ] />::RealT b= false;
			var S</ tup[ f32, f64 ] />::RealT d= 0.125;
			var S</ tup[ f32, tup[ i32 ] ] />::RealT t[ 66 ];
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleTemplateArgument_Test1():
	c_program_text= """
		template</ type T /> struct S</ tup[ T ] /> { auto constexpr x= 555; }
		template</ type T/> struct S</ T /> { auto constexpr x= 999; }
		template<//> struct S</ tup[ i64 ] /> { auto constexpr x= 11111; }

		static_assert( S</ tup[ f32 ] />::x == 555 ); // Select more specialized template.
		static_assert( S</ bool />::x == 999 ); // More specialized template applied only for tuples.
		static_assert( S</ tup[ i64 ] />::x == 11111 ); // Full specialization for tuple.
	"""
	tests_lib.build_program( c_program_text )


def TupleTemplateArgument_Test2():
	c_program_text= """
		template</ type T /> fn constexpr GetX( T& t ) : i32 { return 951; }
		template</ type T /> fn constexpr GetX( tup[ T, T ] & t ) : i32 { return 753; }

		var tup[ i32, i32 ] constexpr t0= zero_init;
		static_assert( GetX(t0) == 753 ); // Select more specialized tempate function.

		var tup[ i32, f32 ] constexpr t1= zero_init;
		static_assert( GetX(t1) == 951 ); // Only less specialized template matches.
	"""
	tests_lib.build_program( c_program_text )
