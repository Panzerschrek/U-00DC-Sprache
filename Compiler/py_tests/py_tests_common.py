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
