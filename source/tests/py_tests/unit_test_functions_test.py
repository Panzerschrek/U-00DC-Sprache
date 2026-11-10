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
	tests_lib.run_function( "_test_fn_SomeTest_b14a7b8059d9c055954c92674ce60032_3_11_" )


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
	tests_lib.run_function( "_test_fn_SomeTest_b14a7b8059d9c055954c92674ce60032_5_12_" )


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
	tests_lib.run_function( "_test_fn_SomeTest_b14a7b8059d9c055954c92674ce60032_5_12_" )


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
	tests_lib.run_function( "_test_fn_IntsTest_b14a7b8059d9c055954c92674ce60032_3_11_" )
	tests_lib.run_function( "_test_fn_IntsTest_b14a7b8059d9c055954c92674ce60032_8_11_" )
	tests_lib.run_function( "_test_fn_IntsTest_b14a7b8059d9c055954c92674ce60032_13_11_" )


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
	tests_lib.run_function( "_test_fn_StructName_b14a7b8059d9c055954c92674ce60032_6_11_" )
	tests_lib.run_function( "_test_fn_FuncName_b14a7b8059d9c055954c92674ce60032_7_11_" )
	tests_lib.run_function( "_test_fn_var_name_b14a7b8059d9c055954c92674ce60032_8_11_" )


def UnitTestFunction_NamesAccess_Test0():
	c_program_text= """
		// A unit-test function has direct access to names within namespace where it's defined (just like any regular function).
		namespace A
		{
			fn FooA() : i32 { return 111; }
			namespace B
			{
				fn FooB() : i32 { return 222; }

				fn_test( ABTest )
				{
					halt if( FooA() != 111 );
					halt if( FooB() != 222 );
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_test_fn_ABTest_b14a7b8059d9c055954c92674ce60032_10_13_" )


def UnitTestFunction_NamesAccess_Test1():
	c_program_text= """
		// A unit-test function has private access to class members if it's defined within a class.
		class SomeClass
		{
			fn_test( SomeClassInternalsTest )
			{
				var SomeClass mut instance;
				halt if( instance.i_field_ != 123 );
				instance.i_field_= 456;
				halt if( instance.i_field_ != 456 );
			}

		private:
			i32 i_field_= 123;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_test_fn_SomeClassInternalsTest_b14a7b8059d9c055954c92674ce60032_5_12_" )


def UnitTestFunction_ExplicitReturn_Test0():
	c_program_text= """
		fn_test( SomeTest )
		{
			auto mut x= 10;
			auto mut y= 3;
			halt if( x - y != 7 );
			if( true )
			{
				return; // Can use "return" within a unit-test, since it's just a function.
			}
			halt if( x == 100 ); // This check is unreachable.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_test_fn_SomeTest_b14a7b8059d9c055954c92674ce60032_2_11_" )


def UnitTestFunction_ExplicitReturn_Test1():
	c_program_text= """
		fn_test( SomeTest )
		{
			return 123; // Error here - unit-test functions return "void".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )
