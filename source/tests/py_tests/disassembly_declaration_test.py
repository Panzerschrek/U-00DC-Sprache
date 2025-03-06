from py_tests_common import *


def DisassemblyDeclaration_Test0():
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


def DisassemblyDeclaration_Test1():
	c_program_text= """
		fn Bar() : tup[];
		fn Foo()
		{
			auto []= Bar();
		}
	"""
	tests_lib.build_program( c_program_text )


def DisassemblyDeclaration_Test2():
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


def DisassemblyDeclaration_Test3():
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


def DisassemblyDeclaration_Test4():
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


def DisassemblyDeclaration_Test5():
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


def DisassemblyDeclaration_Test6():
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


def DisassemblyDeclaration_Test7():
	c_program_text= """
		fn Bar() : [ [ i32, 2 ], 3 ]
		{
			var [ [ i32, 2 ], 3 ] res[ [ 1, 2 ], [ 3, 4 ], [ 5, 6 ] ];
			return res;
		}
		fn Foo()
		{
			// Can disassemble to non-terminal elements.
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


def DisassemblyDeclaration_Test8():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 3 ] mut arr[ 76, 543, 2109 ];
			// Move and disassembly a local variable.
			auto [ x, y, z ]= move(arr);
			halt if( x != 76 );
			halt if( y != 543 );
			halt if( z != 2109 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def DisassemblyDeclaration_Test9():
	c_program_text= """
		fn Bar( [ i32 , 3 ] mut arr )
		{
			// Move and disassembly an argument.
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


def DisassemblyDeclarationConstexpr_Test0():
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


def DisassemblyDeclarationConstexpr_Test1():
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


def DisassemblyDeclarationConstexpr_Test2():
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


def DisassemblyDeclarationConstexpr_Test3():
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


def DisassemblyDeclarationForReferenceField_Test0():
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


def DisassemblyDeclarationForReferenceField_Test1():
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


def DisassemblyDeclarationForReferenceField_Test2():
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


def DisassembledVariableIsImmutable_Test0():
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


def DisassembledVariableIsImmutable_Test1():
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


def DisassembledVariableIsImmutable_Test2():
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


def DisassembledVariableIsImmutable_Test3():
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


def DisassembledVariableIsImmutable_Test4():
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


def DisassembledVariableIsImmutable_Test5():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { imut x : a, imut y : b, imut z : c }= move(s);
			// Can't modify variables, which are declated with "imut:
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


def ImmediateValueExpectedInDisassemblyDeclaration_Test0():
	c_program_text= """
		fn Foo( [ i32, 2 ] arr )
		{
			auto [ x, y ]= arr; // Expected immediate value, got immutable reference to an argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDisassemblyDeclaration", 4 ) )


def ImmediateValueExpectedInDisassemblyDeclaration_Test1():
	c_program_text= """
		fn Foo( [ i32, 2 ] mut arr )
		{
			auto [ x, y ]= arr; // Expected immediate value, got mmutable reference to an argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDisassemblyDeclaration", 4 ) )


def ImmediateValueExpectedInDisassemblyDeclaration_Test2():
	c_program_text= """
		fn Foo()
		{
			var [i32, 2 ] arr= zer_init;
			auto [ x, y ]= arr; // Expected immediate value, got immutable reference to a local variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDisassemblyDeclaration", 5 ) )


def ImmediateValueExpectedInDisassemblyDeclaration_Test3():
	c_program_text= """
		fn Foo()
		{
			var [i32, 2 ] mut arr= zer_init;
			auto [ x, y ]= arr; // Expected immediate value, got mutable reference to a local variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDisassemblyDeclaration", 5 ) )



def ImmediateValueExpectedInDisassemblyDeclaration_Test4():
	c_program_text= """
		fn Foo( S s )
		{
			auto { a : x, b : y }= s; // Expected immediate value, got immutable reference to an argument.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDisassemblyDeclaration", 4 ) )


def ImmediateValueExpectedInDisassemblyDeclaration_Test5():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { a : x, b : y }= s; // Expected immediate value, got mutable reference to an argument.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDisassemblyDeclaration", 4 ) )


def ImmediateValueExpectedInDisassemblyDeclaration_Test6():
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
	assert( HasError( errors_list, "ImmediateValueExpectedInDisassemblyDeclaration", 5 ) )


def ImmediateValueExpectedInDisassemblyDeclaration_Test7():
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
	assert( HasError( errors_list, "ImmediateValueExpectedInDisassemblyDeclaration", 5 ) )


def ImmediateValueExpectedInDisassemblyDeclaration_Test8():
	c_program_text= """
		fn Foo( [ i32, 2 ]& arr )
		{
			auto [ x, y ]= arr; // Expected immediate value, got immutable reference to an argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDisassemblyDeclaration", 4 ) )


def ImmediateValueExpectedInDisassemblyDeclaration_Test9():
	c_program_text= """
		fn Foo( S &mut s )
		{
			auto { a : x, b : y }= s; // Expected immediate value, got mutable reference to an argument.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ImmediateValueExpectedInDisassemblyDeclaration", 4 ) )


def DisassemblingNonStructAsStruct_Test0():
	c_program_text= """
		fn Foo( i32 mut i )
		{
			auto { a : x } = move(i); // Can't disassemble scalar as struct.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def DisassemblingNonStructAsStruct_Test1():
	c_program_text= """
		fn Foo( $(u8) mut p )
		{
			auto { a : x } = move(p); // Can't disassemble scalar as struct.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def DisassemblingNonStructAsStruct_Test2():
	c_program_text= """
		fn Foo( E mut e )
		{
			auto { a : x } = move(e); // Can't disassemble scalar as struct.
		}
		enum E{ A, B, C }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def DisassemblingNonStructAsStruct_Test3():
	c_program_text= """
		fn Foo( [ i32, 2 ] mut arr )
		{
			auto { a : x, b : y } = move(arr); // Can't disassemble array as struct.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def DisassemblingNonStructAsStruct_Test4():
	c_program_text= """
		fn Foo( tup[ u32, f64 ] mut t )
		{
			auto { a : x, b : y } = move(t); // Can't disassemble tuple as struct.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def DisassemblingClassValue_Test0():
	c_program_text= """
		fn Foo( C mut c )
		{
			auto {} = move(c);
		}
		class C{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DisassemblingClassValue", 4 ) )


def DisassemblingClassValue_Test1():
	c_program_text= """
		fn Foo( C mut c )
		{
			auto { a : x, b : y } = move(c);
		}
		class C polymorph{ f32 x; u32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DisassemblingClassValue", 4 ) )


def DisassemblingClassValue_Test2():
	c_program_text= """
		fn Foo()
		{
			auto {} = lambda(){}; // Lambda is a class, it's not possible to disassemble it.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DisassemblingClassValue", 4 ) )


def DisassemblingClassValue_Test3():
	c_program_text= """
		fn Foo()
		{
			auto {} = Bar(); // Coroutine object is a class, it's not possible to disassemble it.
		}
		fn async Bar();
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DisassemblingClassValue", 4 ) )


def DisassemblingClassValue_Test4():
	c_program_text= """
		fn Foo()
		{
			auto {} = Bar(); // Coroutine object is a class, it's not possible to disassemble it.
		}
		fn generator Bar() : i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DisassemblingClassValue", 4 ) )


def DisassemblingStructWithExplicitDestructor_Test0():
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
	assert( HasError( errors_list, "DisassemblingStructWithExplicitDestructor", 4 ) )


def DisassemblingStructWithExplicitDestructor_Test1():
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
	assert( HasError( errors_list, "DisassemblingStructWithExplicitDestructor", 4 ) )


def DisassemblingStructWithExplicitDestructor_Test1():
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
	assert( HasError( errors_list, "DisassemblingStructWithExplicitDestructor", 4 ) )


def DisassemblingStructWithExplicitDestructor_Test2():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { a : x, b : t } = move(s); // Fine - "s" can be disassembled, since its destructor is generated, "t" with explicit destructor isn't disassembled.
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


def DisassemblingTypeinfoStruct_Test0():
	c_program_text= """
		fn Foo( typeof( typeinfo</i32/> ) mut t )
		{
			auto { } = move(t);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DisassemblingTypeinfoStruct", 4 ) )


def DisassemblingTypeinfoStruct_Test1():
	c_program_text= """
		fn Foo( typeof( typeinfo</ fn( i32 x ) />.params_list[0] ) mut t )
		{
			auto { } = move(t);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DisassemblingTypeinfoStruct", 4 ) )


def DuplicatedFieldInDisassemblyDeclaration_Test0():
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
	assert( HasError( errors_list, "DuplicatedFieldInDisassemblyDeclaration", 4 ) )


def DuplicatedFieldInDisassemblyDeclaration_Test1():
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
	assert( HasError( errors_list, "DuplicatedFieldInDisassemblyDeclaration", 4 ) )


def NameNotFound_ForStructDisassembly_Test0():
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


def NameNotFound_ForStructDisassembly_Test1():
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


def DisassemblingNonFieldStructMember_Test0():
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
	assert( HasError( errors_list, "DisassemblingNonFieldStructMember", 4 ) )


def DisassemblingNonFieldStructMember_Test1():
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
	assert( HasError( errors_list, "DisassemblingNonFieldStructMember", 4 ) )


def DisassemblingNonFieldStructMember_Test2():
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
	assert( HasError( errors_list, "DisassemblingNonFieldStructMember", 4 ) )


def BindingConstReferenceToNonconstReference_ForReferenceFieldDisassembly_Test0():
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


def BindingConstReferenceToNonconstReference_ForReferenceFieldDisassembly_Test1():
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


def BindingConstReferenceToNonconstReference_ForReferenceFieldDisassembly_Test2():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { imut x_ref : x } = move(s); // Ok - create immutable reference for mutable reference field.
		}
		struct S { i32 &mut x; }
	"""
	tests_lib.build_program( c_program_text )


def DisassemblingReferenceField_Test0():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { [ a, b ] : x } = move(s); // Error - can't disassemble "x" further, since it's a reference field.
		}
		struct S { [ i32, 2 ] & x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DisassemblingReferenceField", 4 ) )


def DisassemblingReferenceField_Test1():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto { { a : m, b : n } : t } = move(s); // Error - can't disassemble "t" further, since it's a reference field.
		}
		struct S { T & t; }
		struct T { f32 m; f64 n; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DisassemblingReferenceField", 4 ) )


def DisassemblySequenceElementCountMismatch_Test0():
	c_program_text= """
		fn Foo( [ i32, 3 ] mut arr )
		{
			auto [ a, b ]= move(arr); // Too few elements.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DisassemblySequenceElementCountMismatch", 4 ) )


def DisassemblySequenceElementCountMismatch_Test1():
	c_program_text= """
		fn Foo( [ i32, 3 ] mut arr )
		{
			auto [ a, b, c, d ]= move(arr); // Too many elements.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DisassemblySequenceElementCountMismatch", 4 ) )


def DisassemblySequenceElementCountMismatch_Test2():
	c_program_text= """
		fn Foo( tup[ i32, f32, bool ] mut t )
		{
			auto [ a, b ]= move(t); // Too few elements.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DisassemblySequenceElementCountMismatch", 4 ) )


def DisassemblySequenceElementCountMismatch_Test3():
	c_program_text= """
		fn Foo( tup[ i32, f32, bool ] mut t )
		{
			auto [ a, b, c, d ]= move(t); // Too many elements.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DisassemblySequenceElementCountMismatch", 4 ) )


def DisassemblySequenceElementCountMismatch_Test4():
	c_program_text= """
		fn Foo( tup[] mut t )
		{
			auto [ a ]= move(t); // Too many elements.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DisassemblySequenceElementCountMismatch", 4 ) )


def DisassemblySequenceElementCountMismatch_Test5():
	c_program_text= """
		fn Foo( tup[ i32 ] mut t )
		{
			auto []= move(t); // Too few elements.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DisassemblySequenceElementCountMismatch", 4 ) )


def SequenceDisassemblyForNonSequenceType_Test0():
	c_program_text= """
		fn Foo( i32 mut i )
		{
			auto [ x ]= move(i); // Can't disassemble scalar.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def SequenceDisassemblyForNonSequenceType_Test1():
	c_program_text= """
		fn Foo( E mut e )
		{
			auto [ x ]= move(e); // Can't disassemble scalar.
		}
		enum E{ A, B, C }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def SequenceDisassemblyForNonSequenceType_Test2():
	c_program_text= """
		fn Foo( $(f64) mut p )
		{
			auto [ x ]= move(p); // Can't disassemble scalar.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def SequenceDisassemblyForNonSequenceType_Test3():
	c_program_text= """
		fn Foo( (fn()) mut p )
		{
			auto [ x ]= move(p); // Can't disassemble scalar.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def SequenceDisassemblyForNonSequenceType_Test4():
	c_program_text= """
		fn Foo( S mut s )
		{
			auto [ a, b ]= move(s); // Can't disassemble structure as sequence.
		}
		struct S{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def SequenceDisassemblyForNonSequenceType_Test5():
	c_program_text= """
		fn Foo( C mut c )
		{
			auto [ a ]= move(c); // Can't disassemble class as sequence.
		}
		class C{ i32 x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 4 ) )


def Redefinition_ForDisassemblyDeclaration_Test0():
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


def Redefinition_ForDisassemblyDeclaration_Test1():
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


def Redefinition_ForDisassemblyDeclaration_Test2():
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


def Redefinition_ForDisassemblyDeclaration_Test3():
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


def Redefinition_ForDisassemblyDeclaration_Test4():
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


def Redefinition_ForDisassemblyDeclaration_Test5():
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


def UsingKeywordAsName_ForDisassemblyDeclaration_Test0():
	c_program_text= """
		fn Foo( tup[ f32 ] mut t )
		{
			auto [ yield ]= move(t); // Error "yield" is a keyword.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UsingKeywordAsName", 4 ) )


def UsingKeywordAsName_ForDisassemblyDeclaration_Test1():
	c_program_text= """
		fn Foo( [ f64, 2] mut arr )
		{
			auto [ x, constructor ]= move(arr); // Error "constructor" is a keyword.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UsingKeywordAsName", 4 ) )


def UsingKeywordAsName_ForDisassemblyDeclaration_Test2():
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


def UsingKeywordAsName_ForDisassemblyDeclaration_Test3():
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


def DisassemblyDeclarationReferenceLinking_Test0():
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


def DisassemblyDeclarationReferenceLinking_Test1():
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


def DisassemblyDeclarationReferenceLinking_Test2():
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


def DisassemblyDeclarationReferenceLinking_Test3():
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


def DisassemblyDeclarationReferenceLinking_Test4():
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


def DisassemblyDeclarationReferenceLinking_Test5():
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


def DisassemblyDeclarationReferenceLinking_Test6():
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
