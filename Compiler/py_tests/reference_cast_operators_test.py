from py_tests_common import *


def CastRef_OperatorDeclaration_Test():
	c_program_text= """
		fn Foo()
		{
			auto x= 0;
			cast_ref</ void />( x );
		}
	"""
	tests_lib.build_program( c_program_text )


def CastRefUnsafe_OperatorDeclaration_Test():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			unsafe{  cast_ref_unsafe</ f32 />( x );  }
		}
	"""
	tests_lib.build_program( c_program_text )


def CastImut_OperatorDeclaration_Test():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			cast_imut( x );
		}
	"""
	tests_lib.build_program( c_program_text )


def CastMut_OperatorDeclaration_Test():
	c_program_text= """
		fn Foo()
		{
			var i32 imut x= 0;
			unsafe{  cast_mut( x );  }
		}
	"""
	tests_lib.build_program( c_program_text )
