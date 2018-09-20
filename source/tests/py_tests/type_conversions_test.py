from py_tests_common import *


def ConversionConstructorIsConstructor_Test0():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor( i32 in_x )
			( x= in_x )
			{}
		}

		fn Foo() : i32
		{
			var IntWrapper i( 85411 ); // Conversion constructor may be called as regular constructor.
			return i.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 85411 )
