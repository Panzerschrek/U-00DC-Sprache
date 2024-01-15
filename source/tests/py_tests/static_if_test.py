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
			else static_if( true ) {}
		}
	"""
	tests_lib.build_program( c_program_text )


def StaticIfDeclaration_Test3():
	c_program_text= """
		fn Foo()
		{
			static_if( true ) {}
			else static_if( true ) {}
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
			else static_if( true ) {}
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
	assert( errors_list[0].src_loc.line == 6 )


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
	assert( errors_list[0].src_loc.line == 8 )


def StaticIfForTemplateDependentExpression_Test0():
	c_program_text= """
		template</ type T />
		fn Foo()
		{
			var T constexpr t;
			static_if( t ) {}  // Ok, "T" here can be "bool"
		}
	"""
	tests_lib.build_program( c_program_text )


def ExpectedVariable_InStaticIf_Test0():
	c_program_text= """
		fn Foo()
		{
			static_if( Foo ) {}
		}
		fn Foo( i32 x );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 1 )
	assert( errors_list[1].error_code == "ExpectedVariable" )
	assert( errors_list[1].src_loc.line == 4 )


def ExpectedBool_InStaticIf_Test0():
	c_program_text= """
		fn Foo()
		{
			static_if( 666 ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 4 )


def ExpectedConstantExpression_InStaticIf_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut false_val= false;
			static_if( false_val ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedConstantExpression" )
	assert( errors_list[0].src_loc.line == 5 )


def StaticIfHaveSeparateVisibilityScope_Test():
	c_program_text= """
		fn Foo()
		{
			static_if(true)
			{
				auto mut x= 0;
			}
			var i32 y= x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "NameNotFound", 8 ) )
