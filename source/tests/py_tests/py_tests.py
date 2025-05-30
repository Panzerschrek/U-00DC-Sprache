import importlib
import inspect
import sys
import threading
import traceback
import os
from py_tests_common import *


# Import tests modules and get list of all tests functions here.
# Returns list of tuples(name, function)
def GetTestsList( tests_modules_list ):
	result= []
	for module_name in tests_modules_list:
		importlib.import_module( module_name )

		for name, obj in inspect.getmembers(sys.modules[module_name]):
			if inspect.isfunction(obj) and obj != ConvertErrors and obj != HasError:
				result.append((name, obj))

	return result


def run_tests():
	tests_modules_list= [
		"alloca_test",
		"arrays_test",
		"async_functions_errors_test",
		"async_functions_test",
		"auto_constexpr_function_templates_test",
		"auto_for_return_type_test",
		"auto_for_return_type_errors_test",
		"await_operator_test",
		"break_from_block_test",
		"byte_types_test",
		"byval_this_test",
		"c_style_for_operator_test",
		"char_arrays_concatenation_test",
		"char_literals_test",
		"char_test",
		"code_builder_test",
		"composite_template_args_test",
		"constexpr_for_generated_methods_test",
		"constexpr_functions_errors_test",
		"constexpr_functions_test",
		"constexpr_structs_test",
		"decompose_declaration_test",
		"enable_if_test",
		"equality_operators_generation_test",
		"external_symbols_access_test",
		"fields_sort_test",
		"function_pointers_errors_test",
		"function_pointers_test",
		"function_reference_notation_test",
		"function_templates_errors_test",
		"function_templates_test",
		"generators_errors_test",
		"generators_test",
		"global_mutable_variables_test",
		"if_alternatives_test",
		"implicit_casts_test",
		"in_class_fields_initializers_test",
		"in_class_fields_initializers_errors_test",
		"inheritance_test",
		"inheritance_errors_test",
		"initializers_in_expression_context_errors_test",
		"initializers_in_expression_context_test",
		"lambdas_errors_test",
		"lambdas_test",
		"macro_errors_test",
		"macro_test",
		"member_access_operator_test",
		"methods_generation_test",
		"mixins_errors_test",
		"mixins_test",
		"multiple_inner_reference_tags_for_structs_test",
		"multiple_inner_reference_tags_test",
		"names_in_error_messages_test",
		"nodiscard_test",
		"nomangle_test",
		"non_sync_tag_errors_test",
		"non_sync_tag_test",
		"numeric_constants_test",
		"order_independent_name_resolving_test",
		"outer_loop_break_continue_test",
		"overloading_resolution_test",
		"public_private_protected_test",
		"raw_pointers_errors_test",
		"raw_pointers_test",
		"reference_cast_operators_test",
		"references_graph_test",
		"references_graph_child_nodes_test",
		"same_type_test",
		"second_order_references_inside_structs_test",
		"size_type_test",
		"static_if_test",
		"stack_variables_move_errors_test",
		"stack_variables_move_test",
		"string_literals_test",
		"strong_order_test",
		"switch_operator_errors_test",
		"switch_operator_test",
		"syntax_errors_test",
		"take_operator_test",
		"ternary_operator_errors_test",
		"ternary_operator_test",
		"tuples_test",
		"type_alias_template_in_template_signature_test",
		"type_templates_as_template_params_test",
		"type_templates_overloading_test",
		"type_conversions_test",
		"typeinfo_test",
		"typeof_test",
		"unconditional_loop_test",
		"uninitialized_initializer_test",
		"unsafe_expression_test",
		"unsafe_test",
		"useless_expression_root_errors_test",
		"unused_name_errors_test",
		"virtual_functions_test",
		"void_test",
		"with_operator_test"
		]

	tests_list= GetTestsList( tests_modules_list )

	print( "run " + str(len(tests_list)) + " py_tests" + "\n" )
	tests_passed= 0
	tests_failed= 0
	tests_filtered= 0

	for test_name, test_func in tests_list:
		if not tests_lib.filter_test( test_name ):
			tests_filtered+= 1
		else:
			try:
				test_func()
				tests_lib.free_program()
				tests_passed+= 1
			except Exception as ex:
				print( "test " + test_name + " failed" )
				traceback.print_exc( file= sys.stdout )
				print()
				tests_failed+= 1
				tests_lib.free_program()

	print( str(tests_passed) + " tests passed" )
	print( str(tests_filtered) + " tests filtered" )
	print( str(tests_failed) + " tests failed" )
	return tests_failed


# Since "Thread" class can't return a value, append return value into the passed dictionary.
def run_tests_thread_entry( res_dict ):
	res= run_tests()
	res_dict.append( res )

def main():
	# Create a separate thread for actual tests running.
	# It's necessary, since we need large stack size (for deep recursion)
	# and we can set stack size only for newly-created threads, but not for the main thread.

	threading.stack_size( 1024 * 1024 * 16 )

	res_dict= [] # Pass result via dictionary

	t= threading.Thread( target= run_tests_thread_entry, args= [ res_dict ] )
	t.start()
	t.join()

	return res_dict[0]


if __name__ == "__main__":
	sys.exit(main())
