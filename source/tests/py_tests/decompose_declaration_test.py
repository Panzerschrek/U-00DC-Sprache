from py_tests_common import *


def DecomposeDeclaration_Test0():
	c_program_text= """
		fn Bar() : [ i32, 3 ]
		{
			var [ i32, 3 ] res[ 88, 999, 10101010 ];
			return res;
		}
		fn Foo()
		{
			auto [ x, y, z ]= Bar();
			halt if( x != 88 );
			halt if( y != 999 );
			halt if( z != 10101010 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DecomposeDeclaration_Test1():
	c_program_text= """
		fn Bar() : tup[];
		fn Foo()
		{
			auto []= Bar();
		}
	"""
	tests_lib.build_program( c_program_text )


def DecomposeDeclaration_Test2():
	c_program_text= """
		fn Bar() : tup[ f32, u64 ]
		{
			var tup[ f32, u64 ] res[ 0.5f, 656u64 ];
			return res;
		}
		fn Foo()
		{
			auto [ mut x, imut y ]= Bar();
			halt if( x != 0.5f );
			halt if( y != 656u64 );

			x*= 3.0f; // Can modify variable declared with "mut" modifier.
			halt if( x != 1.5f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DecomposeDeclaration_Test3():
	c_program_text= """
		fn Bar() : [ [ i32, 2 ], 3 ]
		{
			var [ [ i32, 2 ], 3 ] res[ [ 1, 2 ], [ 3, 4 ], [ 5, 6 ] ];
			return res;
		}
		fn Foo()
		{
			auto [ [ a, b ], [ c, d ], [ e, f ] ]= Bar();
			halt if( a != 1 );
			halt if( b != 2 );
			halt if( c != 3 );
			halt if( d != 4 );
			halt if( e != 5 );
			halt if( f != 6 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DecomposeDeclaration_Test4():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		fn Bar() : S
		{
			return S{ .x= 55551, .y= -76.25f };
		}
		fn Foo()
		{
			auto { a : x, b : y } = Bar();
			halt if( a != 55551 );
			halt if( b != -76.25f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DecomposeDeclaration_Test5():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		fn Bar() : S
		{
			return S{ .x= 7867, .y= 123.45f };
		}
		fn Foo()
		{
			auto { mut a : x, imut b : y } = Bar();
			halt if( a != 7867 );
			halt if( b != 123.45f );
			// Can modify variable declared as "mut".
			a= 665544;
			halt if( a != 665544 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DecomposeDeclaration_Test6():
	c_program_text= """
		struct S{ i32 x; [ f32, 4 ] y; }
		fn Bar() : tup[ bool, S ]
		{
			var tup[ bool, S ] res[ true, { .x= 432, .y[ 1.0f, 2.0f, 3.0f, 4.0f ] } ];
			return res;
		}
		fn Foo()
		{
			auto [ b, { a : x, [ S, P, Q, R ] : y } ] = Bar();
			halt if( b != true );
			halt if( S != 1.0f );
			halt if( P != 2.0f );
			halt if( Q != 3.0f );
			halt if( R != 4.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DecomposeDeclaration_Test7():
	c_program_text= """
		fn Bar() : [ [ i32, 2 ], 3 ]
		{
			var [ [ i32, 2 ], 3 ] res[ [ 1, 2 ], [ 3, 4 ], [ 5, 6 ] ];
			return res;
		}
		fn Foo()
		{
			// Can decompose to non-terminal elements.
			auto [ ab, cd, ef ]= Bar();
			halt if( ab[0] != 1 );
			halt if( ab[1] != 2 );
			halt if( cd[0] != 3 );
			halt if( cd[1] != 4 );
			halt if( ef[0] != 5 );
			halt if( ef[1] != 6 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DecomposeDeclaration_Test8():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 3 ] mut arr[ 76, 543, 2109 ];
			// Move and decompose a local variable.
			auto [ x, y, z ]= move(arr);
			halt if( x != 76 );
			halt if( y != 543 );
			halt if( z != 2109 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DecomposeDeclaration_Test9():
	c_program_text= """
		fn Bar( [ i32 , 3 ] mut arr )
		{
			// Move and decompose an argument.
			auto [ x, y, z ]= move(arr);
			halt if( x != 2109 );
			halt if( y != 543 );
			halt if( z != 76 );
		}
		fn Foo()
		{
			var [ i32, 3 ] arr[ 2109, 543, 76 ];
			Bar( arr );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructDecomposeShortForm_Test0():
	c_program_text= """
		struct S{ i32 x; i32 y; }
		fn Foo()
		{
			// Short form with only names.
			auto { x, y }= S{ .x= 78, .y= 92 };
			halt if( x != 78 );
			halt if( y != 92 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructDecomposeShortForm_Test1():
	c_program_text= """
		struct S{ i32 x; i32 y; }
		fn Foo()
		{
			// Short form with mutability modifier.
			auto { mut x, mut y }= S{ .x= 567, .y= -65 };
			halt if( x != 567 );
			halt if( y != -65 );
			x*= 3;
			halt if( x != 567 * 3 );
			y/= 4;
			halt if( y != -65 / 4 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructDecomposeShortForm_Test2():
	c_program_text= """
		struct S{ i32 x; i32 y; }
		fn Foo()
		{
			// Can mix short and full form.
			auto { x_renamed : x, y }= S{ .x= 78, .y= 92 };
			halt if( x_renamed != 78 );
			halt if( y != 92 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructDecomposeShortForm_Test3():
	c_program_text= """
		struct S{ i32 x; i32 y; }
		fn Foo()
		{
			// Can mix short and full form.
			auto { x, mut y_renamed : y }= S{ .x= 987, .y= 3232 };
			halt if( x != 987 );
			halt if( y_renamed != 3232 );
			y_renamed+= 100;
			halt if( y_renamed != 3232 + 100 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DecomposeDeclarationConstexpr_Test0():
	c_program_text= """
		fn constexpr Bar( i32 scale ) : [ i32, 3 ]
		{
			var [ i32, 3 ] arr[ 2 * scale, 3 * scale, 4 * scale ];
			return arr;
		}
		fn Foo()
		{
			auto [ x, y, z ]= Bar(7);
			static_assert( x == 14 );
			static_assert( y == 21 );
			static_assert( z == 28 );
		}
	"""
	tests_lib.build_program( c_program_text )


def DecomposeDeclarationConstexpr_Test1():
	c_program_text= """
		fn constexpr Bar( i32 scale ) : tup[ i32, f32, bool ]
		{
			var bool even= ( scale & 1 ) == 0;
			var tup[ i32, f32, bool ] t[ scale * 3, f32(scale) * 4.0f, even ];
			return t;
		}
		fn Foo()
		{
			auto [ x, y, z ]= Bar( 9 );
			static_assert( x == 27 );
			static_assert( y == 36.0f );
			static_assert( !z );
			auto [ a, b, c ]= Bar( 4 );
			static_assert( a == 12 );
			static_assert( b == 16.0f );
			static_assert( c );
		}
	"""
	tests_lib.build_program( c_program_text )


def DecomposeDeclarationConstexpr_Test2():
	c_program_text= """
		struct S{ i32 x; u32 y; }
		fn constexpr Bar( i32 scale ) : S
		{
			return S{ .x= -3 * scale, .y= u32(scale) * 7u };
		}
		fn Foo()
		{
			auto { a : x, b : y }= Bar(11);
			static_assert( a == -33 );
			static_assert( b == 77u );
		}
	"""
	tests_lib.build_program( c_program_text )


def DecomposeDeclarationConstexpr_Test3():
	c_program_text= """
		struct S{ i32& x; }
		fn Foo()
		{
			var i32 x= 87678;
			auto { x_ref : x } = S{ .x= x };
			static_assert( x_ref == x ); // Constexpr references should be preserved.
		}
	"""
	tests_lib.build_program( c_program_text )


def DecomposeDeclarationForReferenceField_Test0():
	c_program_text= """
		struct S{ u64 v; i32& x; }
		fn Foo()
		{
			var i32 n= 66;
			auto { a : v, ref : x }= S{ .v(77), .x= n };
			halt if( a != 77u64 );
			halt if( ref != n );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DecomposeDeclarationForReferenceField_Test1():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Foo()
		{
			var i32 mut n= 66;
			{
				auto { mut ref : x }= S{ .x= n };
				ref*= 2;
			}
			halt if( n != 66 * 2 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DecomposeDeclarationForReferenceField_Test2():
	c_program_text= """
		struct S{ i32 &mut @('a') x; i32 &mut @('b') y; }
		fn Foo()
		{
			var i32 mut n= 13, mut m= 78;
			{
				auto { mut a : x, mut b : y }= S{ .x= n, .y= m };
				a*= 3;
				b/= 2;
			}
			halt if( n != 13 * 3 );
			halt if( m != 78 / 2 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DecomposeInCStyleForOperator_Test0():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 2 ] mut arr[ 3, 13 ];
			for( auto [ mut i, mut j ]= move(arr); i < 10; ++i, j*= 2 )
			{
				halt if( i != 3 );
				halt if( j != 13 );
				break;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DecomposeInCStyleForOperator_Test1():
	c_program_text= """
		struct S{ i32 x; i32 y; }
		fn Foo()
		{
			for( auto { mut i : x, mut j : y } = S{ .x= 3, .y= 13 }; i < 10; ++i, j*= 2 )
			{
				halt if( i != 3 );
				halt if( j != 13 );
				break;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DecomposedVariableIsImmutable_Test0():
	c_program_text= """
		fn Foo( [ i32, 2 ] mut arr )
		{
			auto [ x, y ]= move(arr);
			// Can't modify variables, which are immutable by default.
			++x;
			y*= 2;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 6 ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 7 ) )


def DecomposedVariableIsImmutable_Test1():
	c_program_text= """
		fn Foo( [ i32, 3 ] mut arr )
		{
			auto [ imut x, imut y, imut z ]= move(arr);
			// Can't modify variables, which are declated with "imut:
			++x;
			y*= 2;
			z= 54;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 6 ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 7 ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 8 ) )


def DecomposedVariableIsImmutable_Test2():
	c_program_text= """
		fn Foo( tup[ i32, f32 ] mut t )
		{
			auto [ x, y ]= move(t);
			// Can't modify variables, which are immutable by default.
			++x;
			y*= 2.0f;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 6 ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 7 ) )


def DecomposedVariableIsImmutable_Test3():
	c_program_text= """
		fn Foo( tup[ i32, f32, u32 ] mut t )
		{
			auto [ imut x, imut y, imut z ]= move(t);
			// Can't modify variables, which are declated with "imut:
			++x;
			y*= 2.0f;
			z= 54u;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 6 ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 7 ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 8 ) )


def DecomposedVariableIsImmutable_Test4():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { x : a, y : b }= move(s);
			// Can't modify variables, which are immutable by default.
			++x;
			y*= 2.0f;
		}
		struct S{ i32 a; f32 b; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 6 ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 7 ) )


def DecomposedVariableIsImmutable_Test5():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { imut x : a, imut y : b, imut z : c }= move(s);
			// Can't modify variables, which are declated with "imut".
			++x;
			y*= 2.0f;
			z= 54u;
		}
		struct S{ i32 a; f32 b; u32 c; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 6 ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 7 ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 8 ) )


def DecomposedVariableIsImmutable_Test6():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { x, y }= move(s);
			// Can't modify variables, which are immutable by default.
			++x;
			y*= 2;
		}
		struct S{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 6 ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 7 ) )


def DecomposedVariableIsImmutable_Test7():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { imut x, imut y }= move(s);
			// Can't modify variables, which are declated with "imut".
			++x;
			y*= 2;
		}
		struct S{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 6 ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 7 ) )


def ImmediateValueExpectedInDecomposeDeclaration_Test0():
	c_program_text= """
		fn Foo( [ i32, 2 ] arr )
		{
			auto [ x, y ]= arr; // Expected immediate value, got immutable reference to an argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDecomposeDeclaration", 4 ) )


def ImmediateValueExpectedInDecomposeDeclaration_Test1():
	c_program_text= """
		fn Foo( [ i32, 2 ] mut arr )
		{
			auto [ x, y ]= arr; // Expected immediate value, got mmutable reference to an argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDecomposeDeclaration", 4 ) )


def ImmediateValueExpectedInDecomposeDeclaration_Test2():
	c_program_text= """
		fn Foo()
		{
			var [i32, 2 ] arr= zer_init;
			auto [ x, y ]= arr; // Expected immediate value, got immutable reference to a local variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDecomposeDeclaration", 5 ) )


def ImmediateValueExpectedInDecomposeDeclaration_Test3():
	c_program_text= """
		fn Foo()
		{
			var [i32, 2 ] mut arr= zer_init;
			auto [ x, y ]= arr; // Expected immediate value, got mutable reference to a local variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDecomposeDeclaration", 5 ) )



def ImmediateValueExpectedInDecomposeDeclaration_Test4():
	c_program_text= """
		fn Foo( S s )
		{
			auto { a : x, b : y }= s; // Expected immediate value, got immutable reference to an argument.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDecomposeDeclaration", 4 ) )


def ImmediateValueExpectedInDecomposeDeclaration_Test5():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { a : x, b : y }= s; // Expected immediate value, got mutable reference to an argument.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDecomposeDeclaration", 4 ) )


def ImmediateValueExpectedInDecomposeDeclaration_Test6():
	c_program_text= """
		fn Foo()
		{
			var S s= zero_init;
			auto { a : x, b : y }= s; // Expected immediate value, got immutable reference to a local variable.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDecomposeDeclaration", 5 ) )


def ImmediateValueExpectedInDecomposeDeclaration_Test7():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			auto { a : x, b : y }= s; // Expected immediate value, got mutable reference to a local variable.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDecomposeDeclaration", 5 ) )


def ImmediateValueExpectedInDecomposeDeclaration_Test8():
	c_program_text= """
		fn Foo( [ i32, 2 ]& arr )
		{
			auto [ x, y ]= arr; // Expected immediate value, got immutable reference to an argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDecomposeDeclaration", 4 ) )


def ImmediateValueExpectedInDecomposeDeclaration_Test9():
	c_program_text= """
		fn Foo( S &mut s )
		{
			auto { a : x, b : y }= s; // Expected immediate value, got mutable reference to an argument.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDecomposeDeclaration", 4 ) )


def DecomposingNonStructAsStruct_Test0():
	c_program_text= """
		fn Foo( i32 mut i )
		{
			auto { a : x } = move(i); // Can't decompose scalar as struct.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def DecomposingNonStructAsStruct_Test1():
	c_program_text= """
		fn Foo( $(u8) mut p )
		{
			auto { a : x } = move(p); // Can't decompose scalar as struct.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def DecomposingNonStructAsStruct_Test2():
	c_program_text= """
		fn Foo( E mut e )
		{
			auto { a : x } = move(e); // Can't decompose scalar as struct.
		}
		enum E{ A, B, C }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def DecomposingNonStructAsStruct_Test3():
	c_program_text= """
		fn Foo( [ i32, 2 ] mut arr )
		{
			auto { a : x, b : y } = move(arr); // Can't decompose array as struct.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def DecomposingNonStructAsStruct_Test4():
	c_program_text= """
		fn Foo( tup[ u32, f64 ] mut t )
		{
			auto { a : x, b : y } = move(t); // Can't decompose tuple as struct.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def DecomposingClassValue_Test0():
	c_program_text= """
		fn Foo( C mut c )
		{
			auto {} = move(c);
		}
		class C{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposingClassValue", 4 ) )


def DecomposingClassValue_Test1():
	c_program_text= """
		fn Foo( C mut c )
		{
			auto { a : x, b : y } = move(c);
		}
		class C polymorph{ f32 x; u32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposingClassValue", 4 ) )


def DecomposingClassValue_Test2():
	c_program_text= """
		fn Foo()
		{
			auto {} = lambda(){}; // Lambda is a class, it's not possible to decompose it.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposingClassValue", 4 ) )


def DecomposingClassValue_Test3():
	c_program_text= """
		fn Foo()
		{
			auto {} = Bar(); // Coroutine object is a class, it's not possible to decompose it.
		}
		fn async Bar();
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposingClassValue", 4 ) )


def DecomposingClassValue_Test4():
	c_program_text= """
		fn Foo()
		{
			auto {} = Bar(); // Coroutine object is a class, it's not possible to decompose it.
		}
		fn generator Bar() : i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposingClassValue", 4 ) )


def DecomposingStructWithExplicitDestructor_Test0():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { a : x, b : y } = move(s);
		}
		struct S
		{
			f32 x;
			u32 y;
			fn destructor() {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposingStructWithExplicitDestructor", 4 ) )


def DecomposingStructWithExplicitDestructor_Test1():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto {} = move(s);
		}
		struct S
		{
			fn destructor();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposingStructWithExplicitDestructor", 4 ) )


def DecomposingStructWithExplicitDestructor_Test1():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { a : x, { b : y } : t } = move(s);
		}
		struct S
		{
			i32 x;
			T t;
		}
		struct T
		{
			f32 y;
			fn destructor();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposingStructWithExplicitDestructor", 4 ) )


def DecomposingStructWithExplicitDestructor_Test2():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { a : x, b : t } = move(s); // Fine - "s" can be decomposed, since its destructor is generated, "t" with explicit destructor isn't decomposed.
		}
		struct S
		{
			i32 x;
			T t;
		}
		struct T
		{
			f32 y;
			fn destructor();
		}
	"""
	tests_lib.build_program( c_program_text )


def DecomposingTypeinfoStruct_Test0():
	c_program_text= """
		fn Foo( typeof( typeinfo</i32/> ) mut t )
		{
			auto { } = move(t);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposingTypeinfoStruct", 4 ) )


def DecomposingTypeinfoStruct_Test1():
	c_program_text= """
		fn Foo( typeof( typeinfo</ fn( i32 x ) />.params_list[0] ) mut t )
		{
			auto { } = move(t);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposingTypeinfoStruct", 4 ) )


def DuplicatedFieldInDecomposeDeclaration_Test0():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { a : x, b : y, c : x } = move(s);
		}
		struct S
		{
			i32 x; f32 y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DuplicatedFieldInDecomposeDeclaration", 4 ) )


def DuplicatedFieldInDecomposeDeclaration_Test1():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { a : x, b : y, c : y } = move(s);
		}
		struct S
		{
			i32 x; f32 y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DuplicatedFieldInDecomposeDeclaration", 4 ) )


def NameNotFound_ForStructDecompose_Test0():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { a : X } = move(s); // There is no "X" inside "S".
		}
		struct S
		{
			i32 x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NameNotFound", 4 ) )


def NameNotFound_ForStructDecompose_Test1():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { a : smoe_filed } = move(s); // There is no "smoe_filed" inside "S".
		}
		struct S
		{
			i32 some_field;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NameNotFound", 4 ) )


def NameNotFound_ForStructDecompose_Test2():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { not_x } = move(s); // There is no "not_x" inside "S".
		}
		struct S { i32 x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NameNotFound", 4 ) )


def DecomposingNonFieldStructMember_Test0():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { a : x } = move(s); // "x" isn't a field, but global variable.
		}
		struct S
		{
			var i32 mut x= 0;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposingNonFieldStructMember", 4 ) )


def DecomposingNonFieldStructMember_Test1():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { a : SomeFunc } = move(s); // "SomeFunc" isn't a field, but a function.
		}
		struct S
		{
			fn SomeFunc( this );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposingNonFieldStructMember", 4 ) )


def DecomposingNonFieldStructMember_Test2():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { a : X } = move(s); // "X" isn't a field, but a type alias.
		}
		struct S
		{
			type X= f32;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposingNonFieldStructMember", 4 ) )


def BindingConstReferenceToNonconstReference_ForReferenceFieldDecompose_Test0():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { mut x_ref : x } = move(s); // Can't declare "x_ref" with "mut", since original field is immutable.
		}
		struct S { i32& x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "BindingConstReferenceToNonconstReference", 4 ) )


def BindingConstReferenceToNonconstReference_ForReferenceFieldDecompose_Test1():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { mut x_ref : x } = move(s); // Can't declare "x_ref" with "mut", since original field is immutable.
		}
		struct S { i32 &imut x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "BindingConstReferenceToNonconstReference", 4 ) )


def BindingConstReferenceToNonconstReference_ForReferenceFieldDecompose_Test2():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { imut x_ref : x } = move(s); // Ok - create immutable reference for mutable reference field.
		}
		struct S { i32 &mut x; }
	"""
	tests_lib.build_program( c_program_text )


def DecomposingReferenceField_Test0():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { [ a, b ] : x } = move(s); // Error - can't decompose "x" further, since it's a reference field.
		}
		struct S { [ i32, 2 ] & x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposingReferenceField", 4 ) )


def DecomposingReferenceField_Test1():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { { a : m, b : n } : t } = move(s); // Error - can't decompose "t" further, since it's a reference field.
		}
		struct S { T & t; }
		struct T { f32 m; f64 n; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposingReferenceField", 4 ) )


def DecomposeSequenceElementCountMismatch_Test0():
	c_program_text= """
		fn Foo( [ i32, 3 ] mut arr )
		{
			auto [ a, b ]= move(arr); // Too few elements.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposeSequenceElementCountMismatch", 4 ) )


def DecomposeSequenceElementCountMismatch_Test1():
	c_program_text= """
		fn Foo( [ i32, 3 ] mut arr )
		{
			auto [ a, b, c, d ]= move(arr); // Too many elements.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposeSequenceElementCountMismatch", 4 ) )


def DecomposeSequenceElementCountMismatch_Test2():
	c_program_text= """
		fn Foo( tup[ i32, f32, bool ] mut t )
		{
			auto [ a, b ]= move(t); // Too few elements.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposeSequenceElementCountMismatch", 4 ) )


def DecomposeSequenceElementCountMismatch_Test3():
	c_program_text= """
		fn Foo( tup[ i32, f32, bool ] mut t )
		{
			auto [ a, b, c, d ]= move(t); // Too many elements.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposeSequenceElementCountMismatch", 4 ) )


def DecomposeSequenceElementCountMismatch_Test4():
	c_program_text= """
		fn Foo( tup[] mut t )
		{
			auto [ a ]= move(t); // Too many elements.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposeSequenceElementCountMismatch", 4 ) )


def DecomposeSequenceElementCountMismatch_Test5():
	c_program_text= """
		fn Foo( tup[ i32 ] mut t )
		{
			auto []= move(t); // Too few elements.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DecomposeSequenceElementCountMismatch", 4 ) )


def SequenceDecomposeForNonSequenceType_Test0():
	c_program_text= """
		fn Foo( i32 mut i )
		{
			auto [ x ]= move(i); // Can't decompose scalar.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def SequenceDecomposeForNonSequenceType_Test1():
	c_program_text= """
		fn Foo( E mut e )
		{
			auto [ x ]= move(e); // Can't decompose scalar.
		}
		enum E{ A, B, C }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def SequenceDecomposeForNonSequenceType_Test2():
	c_program_text= """
		fn Foo( $(f64) mut p )
		{
			auto [ x ]= move(p); // Can't decompose scalar.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def SequenceDecomposeForNonSequenceType_Test3():
	c_program_text= """
		fn Foo( (fn()) mut p )
		{
			auto [ x ]= move(p); // Can't decompose scalar.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def SequenceDecomposeForNonSequenceType_Test4():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto [ a, b ]= move(s); // Can't decompose structure as sequence.
		}
		struct S{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def SequenceDecomposeForNonSequenceType_Test5():
	c_program_text= """
		fn Foo( C mut c )
		{
			auto [ a ]= move(c); // Can't decompose class as sequence.
		}
		class C{ i32 x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def Redefinition_ForDecomposeDeclaration_Test0():
	c_program_text= """
		fn Foo( [ i32, 1 ] mut arr )
		{
			var f64 a= 0.0;
			auto [ a ]= move(arr); // Error, "a" already defined in this scope.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "Redefinition", 5 ) )


def Redefinition_ForDecomposeDeclaration_Test1():
	c_program_text= """
		fn Foo( tup[ f32, bool ] mut t )
		{
			var u8 b= zero_init;
			auto [ a, b ]= move(t); // Error, "b" already defined in this scope.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "Redefinition", 5 ) )


def Redefinition_ForDecomposeDeclaration_Test2():
	c_program_text= """
		fn Foo( S mut s )
		{
			var f64 a= 0.0;
			auto { a : x }= move(s); // Error, "a" already defined in this scope.
		}
		struct S{ i32 x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "Redefinition", 5 ) )


def Redefinition_ForDecomposeDeclaration_Test3():
	c_program_text= """
		fn Foo( S mut s )
		{
			var f64 a= 0.0;
			auto { a : x }= move(s); // Error, "a" already defined in this scope.
		}
		struct S{ i32& x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "Redefinition", 5 ) )


def Redefinition_ForDecomposeDeclaration_Test4():
	c_program_text= """
		fn Foo( [ i32, 1 ] mut arr )
		{
			var f64 a= 0.0;
			{
				auto [ a ]= move(arr); // Fine, "a" is defined in outer scope.
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def Redefinition_ForDecomposeDeclaration_Test5():
	c_program_text= """
		fn Foo( S mut s )
		{
			var f64 a= 0.0;
			{
				auto { a : x }= move(s); // Fine, "a" is defined in outer scope.
			}
		}
		struct S{ i32 x; }
	"""
	tests_lib.build_program( c_program_text )


def UsingKeywordAsName_ForDecomposeDeclaration_Test0():
	c_program_text= """
		fn Foo( tup[ f32 ] mut t )
		{
			auto [ yield ]= move(t); // Error "yield" is a keyword.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UsingKeywordAsName", 4 ) )


def UsingKeywordAsName_ForDecomposeDeclaration_Test1():
	c_program_text= """
		fn Foo( [ f64, 2] mut arr )
		{
			auto [ x, constructor ]= move(arr); // Error "constructor" is a keyword.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UsingKeywordAsName", 4 ) )


def UsingKeywordAsName_ForDecomposeDeclaration_Test2():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { label : x } = move(s); // Error "label" is a keyword.
		}
		struct S{ i32 x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UsingKeywordAsName", 4 ) )


def UsingKeywordAsName_ForDecomposeDeclaration_Test3():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { protected : x } = move(s); // Error "protected" is a keyword.
		}
		struct S{ i32& x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UsingKeywordAsName", 4 ) )


def DecomposeDeclarationReferenceLinking_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var [ S, 1 ] mut arr[ { .x= x } ];
			auto [ s ]= move(arr); // Inner reference of "arr" should be linked with "s".
			++x; // Error here, "s" still holds a reference to "x".
		}
		struct S{ i32& x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def DecomposeDeclarationReferenceLinking_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var tup[ S ] mut t[ { .x= x } ];
			auto [ s ]= move(t); // Inner reference of "t" should be linked with "s".
			++x; // Error here, "s" still holds a reference to "x".
		}
		struct S{ i32& x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def DecomposeDeclarationReferenceLinking_Test2():
	c_program_text= """
		fn Foo()
		{
			var i32 mut a= 0, mut b= 0;
			var [ S, 2 ] mut arr[ { .x= a }, { .x= b } ];
			auto [ mut sa, mut sb ]= move(arr); // Inner reference of "sa" and "sb" should be linked with "arr".
			move( sa );
			++a; // Error here, even if "sa" is destroyed, it's assumed that "sb" may point to "a".
		}
		struct S{ i32& x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def DecomposeDeclarationReferenceLinking_Test3():
	c_program_text= """
		fn Foo()
		{
			var i32 mut a= 0, mut b= 0;
			var tup[ S, S ] mut t[ { .x= a }, { .x= b } ];
			auto [ mut sa, mut sb ]= move(t); // Inner reference of "sa" points to "a", inner reference of "sb" points to "b".
			move( sa );
			++a; // Fine, there is no alive reference to "a".
		}
		struct S{ i32& x; }
	"""
	tests_lib.build_program( c_program_text )


def DecomposeDeclarationReferenceLinking_Test4():
	c_program_text= """
		fn Foo()
		{
			var i32 mut a= 0;
			auto { a_ref : x } = S{ .x= a };
			++a; // Error, "a_ref" points to "a", so we can't mutate it.
		}
		struct S{ i32& x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def DecomposeDeclarationReferenceLinking_Test5():
	c_program_text= """
		fn Foo()
		{
			var i32 mut a= 0;
			auto { { a_ref : x } : s } = T{ .s{ .x= a } };
			++a; // Error, "a_ref" points to "a", so we can't mutate it.
		}
		struct S{ i32& x; }
		struct T{ S s; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def DecomposeDeclarationReferenceLinking_Test6():
	c_program_text= """
		var [ [ char8, 2 ], 0 ] return_references[];
		fn Foo( i32& a ) : i32& @(return_references)
		{
			var S s{ .x= a };
			auto { s_ref : s } = T{ .s= s }; // "s_ref" contains reference to "a"
			return s_ref.x; // This results into "a" returning, which isn't allowed.
		}
		struct S{ i32& x; }
		struct T{ S& s; } // This struct contains second order reference inside.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 7 ) )


def VariableInitializerIsNotConstantExpression_ForDecomposeOperator_Test0():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 2 ] mut arr= zero_init;
			auto [ constexpr x, constexpr y ]= move(arr);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 5 ) )


def VariableInitializerIsNotConstantExpression_ForDecomposeOperator_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			var S mut s{ .x= x };
			auto{ constexpr x_ref : x }= move(s); // Error, "x_ref" can't be constexpr, since "s" is declared as mutable.
		}
		struct S{ i32& x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 6 ) )


def ConstexprValueResetForMutableVariableInDecompose_Test0():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut t= zero_init;
			auto [ mut x, mut y ] = move(t);
			static_assert( x == 0 );
			static_assert( y == 0.0f );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "StaticAssertExpressionIsNotConstant", 6 ) )
	assert( HasError( errors_list, "StaticAssertExpressionIsNotConstant", 7 ) )
