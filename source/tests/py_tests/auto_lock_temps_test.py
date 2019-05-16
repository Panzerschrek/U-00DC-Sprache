from py_tests_common import *


def LockTempsDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			auto lock_temps x= 0;
		}
	"""
	tests_lib.build_program( c_program_text )


def LockTempsDeclaration_Test1():
	c_program_text= """
		fn Foo()
		{
			auto lock_temps mut x= 0;
		}
	"""
	tests_lib.build_program( c_program_text )


def LockTempsDeclaration_Test2():
	c_program_text= """
		fn Foo()
		{
			auto lock_temps & x= Pass(666);
		}
		fn Pass( i32& x ) : i32& { return x; }
	"""
	tests_lib.build_program( c_program_text )


def LockTempsDeclaration_Test3():
	c_program_text= """
	fn Foo()
	{
		auto lock_temps &imut x= Pass(8888);
	}
	fn Pass( i32& x ) : i32& { return x; }
	"""
	tests_lib.build_program( c_program_text )
