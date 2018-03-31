import inspect
import sys
import traceback
import sprache_compiler_tests_py_lib as tests_lib

class FilePos:
	file_index= 0
	line= 0
	pos_in_line= 0


class CodeBuilderError:
	error_code= ""
	file_pos= FilePos()


def ConvertErrors( errors_list ):
	result= []
	for error in errors_list:
		out_error= CodeBuilderError()
		out_error.error_code= error["code"]
		out_error.file_pos.file_index= error["file_pos"]["file_index"]
		out_error.file_pos.line= error["file_pos"]["line"]
		out_error.file_pos.pos_in_line= error["file_pos"]["pos_in_line"]
		result.append( out_error )
	return result


def OkTest():
	tests_lib.build_program( "fn Foo( i32 x, i32 y ) : f32 { return f32(x * y) + 0.5f; }" )

	call_result= tests_lib.run_function( "_Z3Fooii", 3, 7 )
	assert( call_result == ( 3 * 7 ) + 0.50 )


def ErrorsTest():
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( "fn Foo() : i32 {}" ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NoReturnInFunctionReturningNonVoid" )
	assert( errors_list[0].file_pos.line == 1 )


def ZeroInitializerForStructWithReferenceTest():
	c_program_text= """
	struct S{ i32& r; }
	fn Foo()
	{
		var S s= zero_init;
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsupportedInitializerForReference" )
	assert( errors_list[0].file_pos.line == 5 )


def VoidTypeIsIncomplete_Test0():
	c_program_text= """
	fn Foo()
	{
		var void v; // void variable
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingIncompleteType" )
	assert( errors_list[0].file_pos.line == 4 )


def VoidTypeIsIncomplete_Test1():
	c_program_text= """
	fn Bar(){}
	fn Foo()
	{
		auto v= Bar(); // void auto variable
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingIncompleteType" )
	assert( errors_list[0].file_pos.line == 5 )


def VoidTypeIsIncomplete_Test2():
	c_program_text= """
	struct S{ void v; } // void struct field
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingIncompleteType" )
	assert( errors_list[0].file_pos.line == 2 )


# Get list of all tests here.
tests_list = [ obj for name, obj in inspect.getmembers(sys.modules[__name__]) if inspect.isfunction(obj) and obj != ConvertErrors ]

def main():

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
