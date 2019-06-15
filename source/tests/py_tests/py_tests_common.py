import sprache_compiler_tests_py_lib as tests_lib


class FilePos:
	def __init__(self):
		self.file_index= 0
		self.line= 0
		self.pos_in_line= 0


class TemplateErrorsContext:
	def __init__(self):
		self.errors= []
		self.template_name= ""
		self.parameters_description= ""
		self.file_pos= FilePos()


class CodeBuilderError:
	def __init__(self):
		self.error_code= ""
		self.text= ""
		self.file_pos= FilePos()
		self.template_errors= TemplateErrorsContext()


def ConvertErrors( errors_list ):
	result= []

	for error in errors_list:
		out_error= CodeBuilderError()
		out_error.error_code= error["code"]
		out_error.text= error["text"]
		out_error.file_pos.file_index= error["file_pos"]["file_index"]
		out_error.file_pos.line= error["file_pos"]["line"]
		out_error.file_pos.pos_in_line= error["file_pos"]["pos_in_line"]

		if error.get("template_context") != None:
			out_error.template_errors.errors= ConvertErrors( error["template_context"]["errors"] )
			out_error.template_errors.template_name=  error["template_context"]["template_name"]
			out_error.template_errors.parameters_description= error["template_context"]["parameters_description"]
			out_error.template_errors.file_pos.file_index= error["template_context"]["file_pos"]["file_index"]
			out_error.template_errors.file_pos.line= error["template_context"]["file_pos"]["line"]
			out_error.template_errors.file_pos.pos_in_line= error["template_context"]["file_pos"]["pos_in_line"]

		result.append( out_error )

	return result
