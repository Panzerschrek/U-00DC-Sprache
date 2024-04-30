from py_tests_common import *


def MixinLexicalError_Test0():
	c_program_text= """
		mixin( " auto s= \\"\\\\urrrr\\"; " );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinLexicalError", 2 ) )


def MixinLexicalError_Test1():
	c_program_text= """

		// Lines in mixin expansions are counted started from mixin expansion line.

		mixin( "\\n\\nauto s= \\"\\\\urrrr\\"; " );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinLexicalError", 7 ) )


def MixinSyntaxError_Test0():
	c_program_text= """
		mixin( "auto s= " ); // statement isn't finished
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 2 ) )


def MixinSyntaxError_Test1():
	c_program_text= """
		mixin( "fn Foo()\\n{\\n lol\\n }" ); // syntactically-incorrect function body
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 5 ) )
