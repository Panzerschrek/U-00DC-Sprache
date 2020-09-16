from py_tests_common import *


def MacroErrorsTest_MacroShouldStartWithName():
	c_program_text= """
		?macro <? & suka blat ?> -> <? ?>
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "unexpected lexem" ) != -1 )
	assert( errors_list[0].text.find( "&" ) != -1 )


def MacroErrorsTest_MacroBracketLeftIsForbidden_Test0():
	c_program_text= """
		?macro <? SomeMacro:expr <? ?> -> <? ?>  // <? inside macro match block
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "unexpected lexem" ) != -1 )
	assert( errors_list[0].text.find( "<?" ) != -1 )


def MacroErrorsTest_MacroBracketLeftIsForbidden_Test1():
	c_program_text= """
		?macro <? SomeMacro:expr ?> -> <? <? ?>  // <? inside macro result block
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "unexpected lexem" ) != -1 )
	assert( errors_list[0].text.find( "<?" ) != -1 )


def MacroErrorsTest_UnknowmMacroContext_Test():
	c_program_text= """
		?macro <? SomeMacro:unknown_context ?> -> <? ?>
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "unknown macro context" ) != -1 )
	assert( errors_list[0].text.find( "unknown_context" ) != -1 )


def MacroErrorsTest_UnknownMacroVariableKind_Test():
	c_program_text= """
		?macro <? SomeMacro:expr ?s:unknown_variable_kind ?> -> <? ?>
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "unknown macro variable type" ) != -1 )
	assert( errors_list[0].text.find( "unknown_variable_kind" ) != -1 )


def MacroErrorsTest_MacroVariableRedefinition_Test0():
	c_program_text= """
		?macro <? SomeMacro:expr ?abc:block + ?abc:block ?> -> <? ?>   // Redefinition with same kind of variable
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "redefinition" ) != -1 )
	assert( errors_list[0].text.find( "abc" ) != -1 )


def MacroErrorsTest_MacroVariableRedefinition_Test1():
	c_program_text= """
		?macro <? SomeMacro:expr ?def:expr + ?def:block ?> -> <? ?>   // Redefinition with different kind of variable
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "redefinition" ) != -1 )
	assert( errors_list[0].text.find( "def" ) != -1 )


def MacroErrorsTest_MacroVariableRedefinition_Test2():
	c_program_text= """
		?macro <? SomeMacro:expr ?def:expr ?o:opt<? + ?def:block ?> ?> -> <? ?>   // Ok - variable in block shadows variable from outer block
	"""
	tests_lib.build_program( c_program_text )


def MacroErrorsTest_Optional_SameLexemAtStartAndAfter_Test0():
	c_program_text= """
		?macro <? SomeMacro:expr ?o:opt<? ^ opt ?> ^ ?> -> <? ?>   // "^" is same
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "Start lexem of optional macro block must be different from first lexem after optional block." ) != -1 )


def MacroErrorsTest_Optional_SameLexemAtStartAndAfter_Test1():
	c_program_text= """
		?macro <? SomeMacro:expr ?o:opt<? WTF ?> WTF ?> -> <? ?>   // "WTF" is same
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "Start lexem of optional macro block must be different from first lexem after optional block." ) != -1 )


def MacroErrorsTest_Optional_ExpectedLexemAtStartOrAfter_Test0():
	c_program_text= """
		?macro <? SomeMacro:class ?o:opt<? ?b:block ?> ?e:expr ?> -> <? ?>
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "Expected lexem at start or after" ) != -1 )


def MacroErrorsTest_Repeated_SameLexemAtStartAndAfter_Test0():
	c_program_text= """
		?macro <? SomeMacro:expr ?o:rep<? * A b C ?> * ?> -> <? ?>   // "*" is same
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "Start lexem of repeated macro block without separator must be different from first lexem after repeated block." ) != -1 )


def MacroErrorsTest_Repeated_SameLexemAsSeparatorAndAfter_Test0():
	c_program_text= """
		?macro <? SomeMacro:expr ?o:rep<? ?e:expr ?><?,?> , ?> -> <? ?>   // "," is same
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "Separator lexem of repeated macro block must be different from first lexem after repeated block." ) != -1 )


def MacroErrorsTest_MacroVariableNotFound_Test0():
	c_program_text= """
		?macro <? SomeMacro:expr ?> -> <? ?i_am_not_exists ?>
		auto x= SomeMacro;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "not found" ) != -1 )
	assert( errors_list[0].text.find( "i_am_not_exists" ) != -1 )


def MacroErrorsTest_MacroVariableRedefinition_Test1():
	c_program_text= """
		?macro <? SomeMacro:expr ?o:opt<? AND ?> ?name:ident ?> -> <? ?o<? ?name ?> ?>   // Ok - use variable from outer block
		auto pi= 3.1415926535;
		auto two_pi= 2.0 * (  SomeMacro AND pi );
	"""
	tests_lib.build_program( c_program_text )


def MacroErrorsTest_MacroExpansionErrors_Test0():
	c_program_text= """
		?macro <? SomeMacro:namespace A B C ?> -> <?  ?>
		SomeMacro A C  // Error, expected "B", got "C"
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.lower().find( "unexpected lexem" ) != -1 )
	assert( errors_list[0].text.find( "C" ) != -1 )


def MacroErrorsTest_MacroExpansionErrors_Test1():
	c_program_text= """
		?macro <? SomeMacro:namespace A + C ?> -> <?  ?>
		SomeMacro A * C  // Error, expected "+", got "*"
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.lower().find( "unexpected lexem" ) != -1 )
	assert( errors_list[0].text.find( "*" ) != -1 )


def MacroErrorsTest_MacroExpansionErrors_Test2():
	c_program_text= """
		?macro <? SomeMacro:namespace ?name:ident ?> -> <?  ?>
		SomeMacro >>  // Error, expected identifier, got >>
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.lower().find( "unexpected lexem" ) != -1 )
	assert( errors_list[0].text.find( ">>" ) != -1 )


def MacroErrorsTest_MacroExpansionErrors_Test3():
	c_program_text= """
		?macro <? SomeMacro:block ?b:block ?> -> <? 0; ?>
		fn Foo()
		{
			SomeMacro lol  // Error, expected {, got identifier
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "unexpected lexem" ) != -1 )
	assert( errors_list[0].text.find( "lol" ) != -1 )


def MacroErrorsTest_MacroVariableExpansionMismatch_Test0():
	c_program_text= """
		?macro <? SomeMacro:expr ?e:expr ?> -> <? ?e<? WTF?> ?>   // <??> specified, but macro variable is not optional or repeated
		auto x= SomeMacro 2;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "Expected optional or repated" ) != -1 )


def MacroErrorsTest_SyntaxErrorInExpandedResult_Test0():
	c_program_text= """
		?macro <? SomeMacro:block  ?> -> <? {WTF} ?>
		fn Foo()
		{
			SomeMacro
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "unexpected lexem" ) != -1 )
	assert( errors_list[0].text.find( "}" ) != -1 )


def MacroErrorsTest_SyntaxErrorInExpandedResult_Test1():
	c_program_text= """
		?macro <? SomeMacro:namespace  ?> -> <? << ?>
		SomeMacro
		fn Foo();
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_syntax_errors(c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].text.find( "unexpected lexem" ) != -1 )
	assert( errors_list[0].text.find( "<<" ) != -1 )
