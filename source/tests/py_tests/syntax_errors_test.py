from py_tests_common import *


def SyntaxError_Test0():
	c_program_text= """
		fn Foo()wtf; // "wtf" - unexpected
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "unexpected lexem" ) != -1 )
	assert( errors_list[0].text.find( "wtf" ) != -1 )


def SyntaxError_Test1():
	c_program_text= """
		var i32 && x= 0; // extra "&"
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "unexpected lexem" ) != -1 )
	assert( errors_list[0].text.find( "&" ) != -1 )
