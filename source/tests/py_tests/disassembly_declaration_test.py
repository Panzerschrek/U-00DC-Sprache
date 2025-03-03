from py_tests_common import *


def DisassemblyDeclaration_Test0():
	c_program_text= """
		fn Bar() : [ i32, 3 ];
		fn Foo()
		{
			auto [ x, y, z ]= Bar();
		}
	"""
	tests_lib.build_program( c_program_text )


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
		fn Bar() : tup[ f32, u64 ];
		fn Foo()
		{
			auto [ mut x, imut y ]= Bar();
		}
	"""
	tests_lib.build_program( c_program_text )


def DisassemblyDeclaration_Test3():
	c_program_text= """
		fn Bar() : [ [ i32, 2 ], 3 ];
		fn Foo()
		{
			auto [ [ a, b ], [ c, d ], [ e, f ] ]= Bar();
		}
	"""
	tests_lib.build_program( c_program_text )


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
