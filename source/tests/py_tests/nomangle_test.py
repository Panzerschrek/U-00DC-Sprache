from py_tests_common import *


def NomangleTest0():
	c_program_text= """
		fn nomangle Foo() : i32 { return 666; }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "Foo" )
	assert( call_result == 666 )


def NomangleTest1():
	c_program_text= """
		fn nomangle GlobalFunction( i32 x, f32 y ) : i32 { return 123698745; }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "GlobalFunction", 0, 0.0 )
	assert( call_result == 123698745 )


def NomangleFunctionMustBeGlobal_Test0():
	c_program_text= """
		namespace N
		{
			fn nomangle Foo(){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NoMangleForNonglobalFunction" )
	assert( errors_list[0].src_loc.line == 4 )


def NomangleFunctionMustBeGlobal_Test1():
	c_program_text= """
		struct S
		{
			fn nomangle Foo(this){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NoMangleForNonglobalFunction" )
	assert( errors_list[0].src_loc.line == 4 )


def NomangleFunctionMustBeGlobal_Test2():
	c_program_text= """
		struct S
		{
			op nomangle -(this) : i32 { return 0; }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NoMangleForNonglobalFunction" )
	assert( errors_list[0].src_loc.line == 4 )


def NomangleFunctionMustBeGlobal_Test3():
	c_program_text= """
		template</ type T /> fn nomangle Foo(){} // Nomangle also forbidden for function templates
		fn Bar() { Foo</i32/>(); }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TemplateContext" )
	assert( errors_list[0].template_errors.errors[0].error_code == "NoMangleForNonglobalFunction" )
	assert( errors_list[0].template_errors.errors[0].src_loc.line == 2 )


def CouldNotOverloadFunctionIfNomangle_Test0():
	c_program_text= """
		fn nomangle Foo();
		fn Foo( i32 x, f32 y );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotOverloadFunction" )
	assert( errors_list[0].src_loc.line == 2 or errors_list[0].src_loc.line == 3 )


def CouldNotOverloadFunctionIfNomangle_Test1():
	c_program_text= """
		fn Foo();
		fn nomangle Foo( i32 x, char8 c );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotOverloadFunction" )
	assert( errors_list[0].src_loc.line == 2 or errors_list[0].src_loc.line == 3 )


def NoMangleMismatch_Test0():
	c_program_text= """
		fn Foo();
		fn nomangle Foo() {} // "nomangle" for body, but prototype is not "nomangle".
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NoMangleMismatch" )
	assert( errors_list[0].src_loc.line == 2 or errors_list[0].src_loc.line == 3 )


def NoMangleMismatch_Test1():
	c_program_text= """
		fn nomangle Bar() : i32;

		fn Bar() : i32 // Body have no "nomangle" specifier
		{
			return 15951;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NoMangleMismatch" )
	assert( errors_list[0].src_loc.line == 2 or errors_list[0].src_loc.line == 4 )
