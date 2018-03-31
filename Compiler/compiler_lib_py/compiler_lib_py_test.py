import sys
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
	assert( call_result == ( 3 * 7 ) + 0.5 )

	tests_lib.free_program()


def ErrorsTest():
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( "fn Foo() : i32 {}" ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NoReturnInFunctionReturningNonVoid" )
	assert( errors_list[0].file_pos.line == 1 )


def main():
	OkTest()
	ErrorsTest()


if __name__ == "__main__":
	sys.exit(main())
