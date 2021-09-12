from py_tests_common import *


def GlobalMutableVariableDeclaration_Test0():
	c_program_text= """
		var i32 mut x = 66;
	"""
	tests_lib.build_program( c_program_text )


def GlobalMutableVariableDeclaration_Test1():
	c_program_text= """
		auto mut ff= 0.25f;
	"""
	tests_lib.build_program( c_program_text )


def GlobalMutableVariableDeclaration_Test2():
	c_program_text= """
		var tup[i32, u64] mut tt= zero_init, mut ft[1i32, 2u64], imut it= zero_init;
	"""
	tests_lib.build_program( c_program_text )


def GlobalMutableVariableDeclaration_Test3():
	c_program_text= """
		struct S{ i32 x; f32 y; bool z; }
		var S mut ss{ .x= 65, .y= -45.0f, .z= true }, mut sss= zero_init;
	"""
	tests_lib.build_program( c_program_text )


def GlobalMutableVariableDeclaration_Test4():
	c_program_text= """
		namespace NN
		{
			var i32 mut n= -5;
			class CC
			{
				var f32 mut ff= 0.25f;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
