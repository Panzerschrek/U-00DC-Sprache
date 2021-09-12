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


def GlobalMutableVariableUsage_Test0():
	c_program_text= """
		var i32 mut x= 12;
		fn Bar() : i32
		{
			unsafe{ return x; }
		}
		fn Foo() : i32
		{
			unsafe
			{
				x= 66;
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Barv" ) == 12 )
	assert( tests_lib.run_function( "_Z3Foov" ) == 66 )


def GlobalMutableVariableUsage_Test1():
	c_program_text= """
		auto mut x= -13.0f;
		fn Bar() : f32
		{
			unsafe{ return x; }
		}
		fn Foo() : f32
		{
			unsafe
			{
				x= 96.5f;
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Barv" ) == -13.0 )
	assert( tests_lib.run_function( "_Z3Foov" ) == 96.5 )


def GlobalMutableVariableUsage_Test2():
	c_program_text= """
		struct S{ u64 x; }
		var S mut s{ .x(22) };
		fn Inc() : u64
		{
			unsafe
			{
				auto res= s.x;
				++s.x;
				return res;
			}
		}
		fn Reset()
		{
			unsafe{  s.x= 0u64;  }
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Incv" ) == 22 )
	assert( tests_lib.run_function( "_Z3Incv" ) == 23 )
	assert( tests_lib.run_function( "_Z3Incv" ) == 24 )
	assert( tests_lib.run_function( "_Z3Incv" ) == 25 )
	tests_lib.run_function( "_Z5Resetv" )
	assert( tests_lib.run_function( "_Z3Incv" ) == 0 )
	assert( tests_lib.run_function( "_Z3Incv" ) == 1 )
	assert( tests_lib.run_function( "_Z3Incv" ) == 2 )
