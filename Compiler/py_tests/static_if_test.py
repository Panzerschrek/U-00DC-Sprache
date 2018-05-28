from py_tests_common import *


def StaticIfDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			static_if( true ) {}
		}
	"""
	tests_lib.build_program( c_program_text )


def StaticIfDeclaration_Test1():
	c_program_text= """
		fn Foo()
		{
			static_if( true ) {}
			else {}
		}
	"""
	tests_lib.build_program( c_program_text )


def StaticIfDeclaration_Test2():
	c_program_text= """
		fn Foo()
		{
			static_if( true ) {}
			else if( true ) {}
		}
	"""
	tests_lib.build_program( c_program_text )


def StaticIfDeclaration_Test3():
	c_program_text= """
		fn Foo()
		{
			static_if( true ) {}
			else if( true ) {}
			else {}
		}
	"""
	tests_lib.build_program( c_program_text )


def FalseBranchesSkipped_Test0():
	c_program_text= """
		fn Foo()
		{
			static_if( true ) {}
			else
			{
				call_to_undefined_function( 5, unknown_variable );
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def FalseBranchesSkipped_Test1():
	c_program_text= """
		fn Foo()
		{
			static_if( false )
			{
				var UnknownType invalid_variable= zero_init;
			}
			else if( true ) {}
			else
			{
				call_to_undefined_function( 5, unknown_variable );
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def FalseBranchesSkipped_Test2():
	c_program_text= """
		fn Foo()
		{
			static_if( false )
			{
				call_to_undefined_function( 5, unknown_variable );
			}
			else {}
		}
	"""
	tests_lib.build_program( c_program_text )


def StaticIfIsUnconditional_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			// Functin always returns value.
			static_if( true ) { return 42; }
		}
	"""
	tests_lib.build_program( c_program_text )


def StaticIfIsUnconditional_Test1():
	c_program_text= """
		fn Foo() : i32
		{
			// Functin never returns value.
			static_if( false ) { return 42; }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NoReturnInFunctionReturningNonVoid" )
	assert( errors_list[0].file_pos.line == 6 )


def StaticIfIsUnconditional_Test2():
	c_program_text= """
		fn Foo()
		{
			auto mut i= 0;
			while( i < 1 )
			{
				static_if( true ) { break; }
				++i;  // Unreachable
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnreachableCode" )
	assert( errors_list[0].file_pos.line == 8 )

