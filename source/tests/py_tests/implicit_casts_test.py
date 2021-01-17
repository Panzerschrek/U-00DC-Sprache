from py_tests_common import *

def ImplicitCast_Test0():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn Bar( A& a );
		fn Foo()
		{
			var B b;
			Bar(b); // Cast to in call (imut to imut).
		}
	"""
	tests_lib.build_program( c_program_text )


def ImplicitCast_Test1():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn Bar( A &mut a );
		fn Foo()
		{
			var B mut b;
			Bar(b); // Cast  reference in call (mut to imut).
		}
	"""
	tests_lib.build_program( c_program_text )


def ImplicitCast_Test2():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn Bar( A &mut a );
		fn Foo()
		{
			var B mut b;
			Bar(b); // Cast reference in call (mut to mut).
		}
	"""
	tests_lib.build_program( c_program_text )


def ImplicitCast_Test3():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn Bar( A& a );
		fn Foo()
		{
			Bar(B()); // Cast value to reference in call (imut to imut).
		}
	"""
	tests_lib.build_program( c_program_text )


def ImplicitCast_Test4():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn ToA( B& b ) : A&
		{
			return b;  // Cast in return (imut to imut).
		}
	"""
	tests_lib.build_program( c_program_text )


def ImplicitCast_Test5():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn ToA( B &mut b ) : A &mut
		{
			return b;  // Cast in return (imut to imut).
		}
	"""
	tests_lib.build_program( c_program_text )


def ImplicitCast_Test6():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		struct S{ A& a; }
		fn Foo()
		{
			var B b;
			var S S{ .a= b };  // Cast reference in field initialization.
		}
	"""
	tests_lib.build_program( c_program_text )


def ImplicitCast_Test7():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn Foo( B& b ) : A&
		{
			var A &a= b;  // Cast reference in reference-variable initialization.
			return a;
		}
	"""
	tests_lib.build_program( c_program_text )
