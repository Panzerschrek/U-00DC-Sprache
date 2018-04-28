from py_tests_common import *


def VisibilityLabelDeclaration_Test0():
	c_program_text= """
		class A
		{
		public:
			fn Foo(this);
		private:
			i32 x;
		}
	"""
	tests_lib.build_program( c_program_text )


def VisibilityLabelDeclaration_Test1():
	c_program_text= """
		class A polymorph
		{
		protected:
			i32 x;
		}
	"""
	tests_lib.build_program( c_program_text )
