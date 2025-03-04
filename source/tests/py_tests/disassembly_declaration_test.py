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
