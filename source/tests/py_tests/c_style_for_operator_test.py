from py_tests_common import *


def CStyleForOperatorParsing_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut a= 0, mut b= 0;

			for( var i32 mut x= 0; x < 10; ++x ){} // variable declaration
			for( auto mut x= 0; x < 10; ++x ){} // Auto variable declaration
			for( ; false; a= 0 ) {} // Empty variables declaration part
			for( auto mut x= 0; ; x+= 2 ) { if(x > 100 ){ break; } } // empty condition
			for( auto mut x= 0; x < 100; ) { break; } //empty iteration part
			for( auto mut x= 0; x < 100; ++x, --x, x+=2 ) {} // multiple iteration parts
			for(;;){ break; } // all elements are empty
		}
	"""
	tests_lib.build_program( c_program_text )
