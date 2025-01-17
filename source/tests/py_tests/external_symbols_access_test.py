from py_tests_common import *


def ExternalFunctionAccessOperator_Test0():
	c_program_text= """
		fn Foo()
		{
			auto f= import fn</ fn() />( "some_func" );
		}
	"""
	tests_lib.build_program( c_program_text )


def ExternalFunctionAccessOperator_Test1():
	c_program_text= """
		fn Foo()
		{
			auto f= import fn</ fn( f32 x, i32 y ) : i32 & />( "__Starship_Flight_7_failed" );
		}
	"""
	tests_lib.build_program( c_program_text )


def ExternalFunctionAccessOperator_Test2():
	c_program_text= """
		fn Foo()
		{
			type MyFuncType= fn( $(byte8) ptr, size_type size ) : ssize_type;
			auto f= import fn</ MyFuncType />( "read_something" );
		}
	"""
	tests_lib.build_program( c_program_text )
