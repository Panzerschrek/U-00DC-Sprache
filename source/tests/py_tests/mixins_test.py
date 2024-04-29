from py_tests_common import *


def NamespaceMixinDeclaration_Test0():
	c_program_text= """
		mixin( "fn Foo() : i32 { return 665533; }" );
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 665533 )


def NamespaceMixinDeclaration_Test1():
	c_program_text= """
		namespace Some
		{
			mixin( "fn Foo() : f32 { return 3.25f; }" );
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_ZN4Some3FooEv" ) == 3.25 )
