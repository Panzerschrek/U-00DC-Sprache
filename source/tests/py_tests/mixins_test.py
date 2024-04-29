from py_tests_common import *


def NamespaceMixinDeclaration_Test0():
	c_program_text= """
		mixin( "fn Foo() {}" );
	"""
	tests_lib.build_program( c_program_text )


def NamespaceMixinDeclaration_Test1():
	c_program_text= """
		namespace Some
		{
			mixin( "fn Foo() {}" );
		}
	"""
	tests_lib.build_program( c_program_text )
