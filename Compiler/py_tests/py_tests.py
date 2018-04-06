import inspect
import sys
import traceback
from py_tests_common import *

import code_builder_test
import inheritance_test
import inheritance_errors_test


# Get list of all tests here.
def GetTestsList( tests_modules_list ):
	result=[]
	for module_name in tests_modules_list:
		module_tests= [ obj for name, obj in inspect.getmembers(sys.modules[module_name]) if inspect.isfunction(obj) and obj != ConvertErrors ]
		result = result + module_tests

	return result


def main():
	tests_list= GetTestsList( [ "code_builder_test", "inheritance_test", "inheritance_errors_test" ] )

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
