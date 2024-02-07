import sprache_compiler_tests_py_lib as tests_lib


class SrcLoc:
	def __init__(self):
		self.file_index= 0
		self.line= 0
		self.column= 0


class TemplateErrorsContext:
	def __init__(self):
		self.errors= []
		self.template_name= ""
		self.parameters_description= ""
		self.src_loc= SrcLoc()


class CodeBuilderError:
	def __init__(self):
		self.error_code= ""
		self.text= ""
		self.src_loc= SrcLoc()
		self.template_errors= TemplateErrorsContext()


def ConvertErrors( errors_list ):
	result= []

	for error in errors_list:
		out_error= CodeBuilderError()
		out_error.error_code= error["code"]
		out_error.text= error["text"]
		out_error.src_loc.file_index= error["src_loc"]["file_index"]
		out_error.src_loc.line= error["src_loc"]["line"]
		out_error.src_loc.column= error["src_loc"]["column"]

		if error.get("template_context") != None:
			out_error.template_errors.errors= ConvertErrors( error["template_context"]["errors"] )
			out_error.template_errors.template_name=  error["template_context"]["template_name"]
			out_error.template_errors.parameters_description= error["template_context"]["parameters_description"]
			out_error.template_errors.src_loc.file_index= error["template_context"]["src_loc"]["file_index"]
			out_error.template_errors.src_loc.line= error["template_context"]["src_loc"]["line"]
			out_error.template_errors.src_loc.column= error["template_context"]["src_loc"]["column"]

		result.append( out_error )

	return result

def HasError( errors_list, code, line ):
	for err in errors_list:
		if( err.error_code == code and err.src_loc.line == line ):
			return True
	return False
