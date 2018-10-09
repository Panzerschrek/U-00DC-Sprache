from py_tests_common import *

def MacroDefenition_Test0():
	c_program_text= """
	?macro <? Pass ?e:expr ?>  ->  <? ?e ?>
	"""
	tests_lib.build_program( c_program_text )


def MacroDefenition_Test1():
	c_program_text= """
	?macro <? PassInBrackets( ?e:expr ) ?>  ->  <? while(true) { ?e; break; } ?>
	"""
	tests_lib.build_program( c_program_text )
