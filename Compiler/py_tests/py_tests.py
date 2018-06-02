import importlib
import inspect
import sys
import traceback
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
		"code_builder_test",
		"constexpr_structs_test",
		"function_pointers_test",
		"function_templates_errors_test",
		"function_templates_test",
		"inheritance_test",
		"inheritance_errors_test",
		"overloading_resolution_test",
		"public_private_protected_test",
		"reference_cast_operators_test",
		"reference_check_for_templates_test",
		"stack_variables_move_errors_test",
		"stack_variables_move_test",
		"static_if_test",
		"unsafe_test",
		"virtual_functions_test" ]

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


if __name__ == "__main__":
	sys.exit(main())
