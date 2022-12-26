from py_tests_common import *


def UnsafeExpressionDeclaration_Test0():
	c_program_text= """
		fn Bar(i32 x) unsafe : i32;
		fn Baz(i32 x);
		fn Foo()
		{
			var i32 x= unsafe( 66 );
			var f32 y= unsafe( f32(x) * 2.0f ) / 4.0f;
			auto z= safe( unsafe( y ) );
			Baz( unsafe(Bar(42)) );
		}
	"""
	tests_lib.build_program( c_program_text )
