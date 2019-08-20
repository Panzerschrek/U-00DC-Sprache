import importlib
import inspect
import sys
import traceback
import os
from py_tests_common import *


# Import tests modules and get list of all tests functions here.
def GetTestsList( tests_modules_list ):
	result=[]
	for module_name in tests_modules_list:
		importlib.import_module( module_name )
		module_tests= [ obj for name, obj in inspect.getmembers(sys.modules[module_name]) if inspect.isfunction(obj) and obj != ConvertErrors ]
		result = result + module_tests

	return result


def main():
	tests_modules_list= [
		"auto_constexpr_function_templates_test",
		"auto_for_return_type_test",
		"auto_for_return_type_errors_test",
		"auto_lock_temps_test",
		"char_literals_test",
		"char_test",
		"code_builder_test",
		"constexpr_functions_errors_test",
		"constexpr_functions_test",
		"constexpr_structs_test",
		"enable_if_test",
		"fields_sort_test",
		"function_pointers_errors_test",
		"function_pointers_test",
		"function_templates_errors_test",
		"function_templates_test",
		"in_class_fields_initializers_test",
		"in_class_fields_initializers_errors_test",
		"inheritance_test",
		"inheritance_errors_test",
		"macro_test",
		"methods_generation_test",
		"names_in_error_messages_test",
		"nomangle_test",
		"numeric_constants_test",
		"order_independent_name_resolving_test",
		"overloading_resolution_test",
		"public_private_protected_test",
		"reference_cast_operators_test",
		"reference_check_for_templates_test",
		"stack_variables_move_errors_test",
		"stack_variables_move_test",
		"string_literals_test",
		"tuples_test",
		"ternary_operator_errors_test",
		"ternary_operator_test",
		"type_templates_overloading_test",
		"type_conversions_test",
		"typeinfo_test",
		"typeof_test",
		"static_if_test",
		"uninitialized_initializer_test",
		"unsafe_test",
		"virtual_functions_test",
		"references_graph_test"
		]

	tests_list= GetTestsList( tests_modules_list )

	print( "run " + str(len(tests_list)) + " py_tests" + "\n" )
	tests_failed= 0

	for test in tests_list:
		try:
			test()
			tests_lib.free_program()
		except Exception as ex:
			print( "test " + str(test) + " failed" )
			traceback.print_exc( file= sys.stdout )
			print()
			tests_failed= tests_failed + 1
			tests_lib.free_program()

	print( str( len(tests_list) - tests_failed ) + " tests passed" )
	print( str(tests_failed) + " tests failed" )
	return tests_failed


if __name__ == "__main__":
	sys.exit(main())
