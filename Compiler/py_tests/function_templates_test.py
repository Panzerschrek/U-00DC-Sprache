from py_tests_common import *


def FunctionTemplateDeclaration_Test0():
	c_program_text= """
		template</ type T />
		fn ToVoid( T& t ) : void&
		{
			return t;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTemplateDeclaration_Test1():
	c_program_text= """
		template</ type T, u32 size />
		fn ZeroFill( [ T, size ] &mut arr )
		{
			var u32 i= 0u;
			while( i < size )
			{
				arr[i]= T(0);
				++i;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTemplateDeclaration_Test2():
	c_program_text= """
		class C
		{
			template</ type T, type U />
			fn DoNothing( T& t, U& u ) {}
		}
	"""
	tests_lib.build_program( c_program_text )
