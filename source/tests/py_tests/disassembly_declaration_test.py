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
		fn Bar() : S;
		fn Foo()
		{
			auto { a : x, b : y } = Bar();
		}
	"""
	tests_lib.build_program( c_program_text )


def DisassemblyDeclaration_Test5():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		fn Bar() : S;
		fn Foo()
		{
			auto { mut a : x, imut b : y } = Bar();
		}
	"""
	tests_lib.build_program( c_program_text )


def DisassemblyDeclaration_Test6():
	c_program_text= """
		struct S{ i32 x; [ f32, 4 ] y; }
		fn Bar() : tup[ bool, S ];
		fn Foo()
		{
			auto [ b, { a : x, [ S, P, Q, R ] : y } ] = Bar();
		}
	"""
	tests_lib.build_program( c_program_text )


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
