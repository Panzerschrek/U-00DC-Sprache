from py_tests_common import *


def UnitTestFunctionDefinition_Test0():
	c_program_text= """
		// Test function within root namespace.
		fn_test( SomeTest )
		{
			auto mut x= 10;
			auto mut y= 3;
			halt if( x - y != 7 );
		}
	"""
	tests_lib.build_program( c_program_text )


def UnitTestFunctionDefinition_Test1():
	c_program_text= """
		namespace SomeNamespace
		{
			// Test function within a namespace.
			fn_test( SomeTest )
			{
				auto mut x= 10;
				auto mut y= 3;
				halt if( x * y != 30 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def UnitTestFunctionDefinition_Test2():
	c_program_text= """
		struct SomeStruct
		{
			// Test function within a namespace.
			fn_test( SomeTest )
			{
				auto mut x= 10;
				auto mut y= 3;
				halt if( x / y != 3 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def UnitTestFunctionDefinition_Test3():
	c_program_text= """
		// It's fine if multiple unit-test functions use the exact same name.
		fn_test( IntsTest )
		{
			auto mut x= 225;
			halt if( x != 225 );
		}
		fn_test( IntsTest )
		{
			auto mut x= 37474;
			halt if( x != 37474 );
		}
		fn_test( IntsTest )
		{
			auto mut x= -53646;
			halt if( x != -53646 );
		}
	"""
	tests_lib.build_program( c_program_text )


def UnitTestFunctionDefinition_Test4():
	c_program_text= """
		// Unit-test names don't collide with other names.
		struct StructName{}
		fn FuncName(){}
		auto var_name= 123;
		fn_test( StructName ) {}
		fn_test( FuncName ) {}
		fn_test( var_name ) {}
	"""
	tests_lib.build_program( c_program_text )
