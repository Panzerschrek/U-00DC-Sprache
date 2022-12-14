from py_tests_common import *

def SharedTagDeclaration_Test0():
	c_program_text= """
		// "shared" as simple tag (equivalent to shared(true)).
		struct A shared {}
		class B shared {}
		class C shared ordered {}
		class D interface shared {}
		class E : D shared {}
	"""
	tests_lib.build_program( c_program_text )


def SharedTagDeclaration_Test1():
	c_program_text= """
		// "shared" as tag with condition expression.
		class A shared(true) {}
		class B shared(false) {}
		class C shared( 2 + 2 == 4 ) {}

		auto constexpr c= 66;
		class D shared( c / 2 != 13 ) {}
		class E shared( 12 == c ) {}
	"""
	tests_lib.build_program( c_program_text )
