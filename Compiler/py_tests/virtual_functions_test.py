from py_tests_common import *

def VirtualFunctionDeclaration_Test0():
	c_program_text= """
		class S polymorph
		{
			fn virtual Foo( this );
		}
	"""
	tests_lib.build_program( c_program_text )


def VirtualFunctionDeclaration_Test1():
	c_program_text= """
		class S abstract
		{
			fn virtual pure Foo( this );
		}
	"""
	tests_lib.build_program( c_program_text )


def VirtualFunctionDeclaration_Test2():
	c_program_text= """
		class T polymorph
		{
			fn virtual Foo( this );
		}
		class S : T
		{
			fn virtual override Foo( this );
		}
	"""
	tests_lib.build_program( c_program_text )


def VirtualFunctionDeclaration_Test3():
	c_program_text= """
		class T polymorph
		{
			fn virtual Foo( this );
		}
		class S : T
		{
			fn virtual final Foo( this );
		}
	"""
	tests_lib.build_program( c_program_text )


def VirtualFunctionCallTest0():
	c_program_text= """
		class T polymorph
		{
			fn virtual Bar( this ) : i32 { return 555; }
		}
		class S : T
		{
			fn virtual override Bar( this ) : i32 { return 666; }
		}

		fn Bar( T& t ) : i32 { return t.Bar(); }   // Must call S::Bar here

		fn Foo() : i32
		{
			var S s;
			return Bar(s);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 666 )


def VirtualForNonclassFunction_Test0():
	c_program_text= """
		fn virtual Foo();
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VirtualForNonclassFunction" )
	assert( errors_list[0].file_pos.line == 2 )


def VirtualForNonThisCallFunction_Test0():
	c_program_text= """
		class S polymorph
		{
			fn virtual Foo();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VirtualForNonThisCallFunction" )
	assert( errors_list[0].file_pos.line == 4 )


def FunctionCanNotBeVirtual_Test0():
	c_program_text= """
		class S polymorph
		{
			fn virtual constructor();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionCanNotBeVirtual" )
	assert( errors_list[0].file_pos.line == 4 )
