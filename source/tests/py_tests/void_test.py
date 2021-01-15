from py_tests_common import *


def VoidTypeIsComplete_Test0():
	c_program_text= """
		struct S{ void v; }
	"""
	tests_lib.build_program( c_program_text )


def VoidTypeIsComplete_Test1():
	c_program_text= """
		fn Foo( void v ){}
	"""
	tests_lib.build_program( c_program_text )


def VoidTypeIsComplete_Test2():
	c_program_text= """
		fn Foo( void& v ){}
	"""
	tests_lib.build_program( c_program_text )


def VoidTypeInitialization_Test0():
	c_program_text= """
		fn Foo()
		{
			var void v= zero_init;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidTypeInitialization_Test1():
	c_program_text= """
		fn Foo()
		{
			var void v0= zero_init;
			var void v1= v0;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidTypeInitialization_Test2():
	c_program_text= """
		fn Foo()
		{
			var void v0= zero_init;
			var void v1(v0);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidTypeInitialization_Test3():
	c_program_text= """
		fn Foo()
		{
			var void v0= zero_init;
			auto v1= v0;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
