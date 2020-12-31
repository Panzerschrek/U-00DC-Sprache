from py_tests_common import *

def PointerTypeDeclaration_Test0():
	c_program_text= """
		type IntPtr = $(i32);
	"""
	tests_lib.build_program( c_program_text )


def PointerTypeDeclaration_Test1():
	c_program_text= """
		type IntPtrPtr = $($(i32));
	"""
	tests_lib.build_program( c_program_text )


def PointerTypeDeclaration_Test2():
	c_program_text= """
		type IntArray4Ptr = $([i32, 4]);
	"""
	tests_lib.build_program( c_program_text )


def PointerTypeDeclaration_Test3():
	c_program_text= """
		type EmptyTupPtr = $(tup[]);
	"""
	tests_lib.build_program( c_program_text )


def RawPointerTypeUsage_Test0():
	c_program_text= """
		fn Foo( $(i32) x, $($(f32)) y ){}
	"""
	tests_lib.build_program( c_program_text )


def RawPointerInitializers_Test0():
	c_program_text= """
		fn Foo() : size_type
		{
			var $(i32) z= zero_init;
			unsafe{  return cast_ref_unsafe</size_type/>(z);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0 )
