#include "../../tests/py_tests_common.hpp"

#include <unordered_set>

#include "../../code_builder_lib_common/code_builder_errors.hpp"
#include "../../tests/tests_common.hpp"
#include "../imports/funcs_c.hpp"

namespace U
{

std::unique_ptr<llvm::Module> BuildProgramImpl( const char* const text, const bool enable_unsed_name_errors )
{
	const U1_StringView text_view{ text, std::strlen(text) };

	llvm::LLVMContext& llvm_context= *g_llvm_context;

	llvm::DataLayout data_layout( GetTestsDataLayout() );

	auto ptr=
		U1_BuildProgram(
			text_view,
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout),
			enable_unsed_name_errors );

	if( ptr == nullptr )
		return nullptr;

	return std::unique_ptr<llvm::Module>( llvm::unwrap(ptr) );
}

PyObject* BuildSrcLoc( const uint32_t file_index, const uint32_t line, const uint32_t column )
{
	PyObject* const dict= PyDict_New();
	PyDict_SetItemString( dict, "file_index", PyLong_FromLongLong(file_index) );
	PyDict_SetItemString( dict, "line", PyLong_FromLongLong(line) );
	PyDict_SetItemString( dict, "column", PyLong_FromLongLong(column) );
	return dict;
}

PyObject* BuildString( const U1_StringView& str )
{
	return PyUnicode_DecodeUTF8( str.data, Py_ssize_t(str.size), nullptr );
}

U1_UserHandle ErrorHandler(
	const U1_UserHandle data, // Should be python list
	const uint32_t file_index,
	const uint32_t line,
	const uint32_t column,
	const uint32_t error_code,
	const U1_StringView& error_text )
{
	PyObject* const dict= PyDict_New();

	PyDict_SetItemString( dict, "src_loc", BuildSrcLoc( file_index, line, column ) );

	const std::string_view error_code_str= CodeBuilderErrorCodeToString( CodeBuilderErrorCode(error_code) );
	PyDict_SetItemString( dict, "code", PyUnicode_DecodeUTF8( error_code_str.data(), Py_ssize_t(error_code_str.size()), nullptr ) );

	PyDict_SetItemString( dict, "text", BuildString( error_text ) );

	PyList_Append( reinterpret_cast<PyObject*>(data), dict );

	return reinterpret_cast<U1_UserHandle>(dict);
}

U1_UserHandle TemplateErrorsContextHandler(
	const U1_UserHandle data, // should be python dictionary
	const uint32_t file_index,
	const uint32_t line,
	const uint32_t column,
	const U1_StringView& context_name,
	const U1_StringView& args_description )
{
	PyObject* const dict= PyDict_New();

	PyDict_SetItemString( dict, "src_loc", BuildSrcLoc( file_index, line, column ) );
	PyDict_SetItemString( dict, "template_name", BuildString( context_name ) );
	PyDict_SetItemString( dict, "parameters_description", BuildString( args_description ) );

	const auto errors_list= PyList_New(0);
	PyDict_SetItemString( dict, "errors", errors_list );

	PyDict_SetItemString( reinterpret_cast<PyObject*>(data), "template_context", dict );

	return reinterpret_cast<U1_UserHandle>(errors_list);
}

const U1_ErrorsHandlingCallbacks g_error_handling_callbacks
{
	ErrorHandler,
	TemplateErrorsContextHandler,
};

PyObject* BuildProgramWithErrors( PyObject* const self, PyObject* const args )
{
	U_UNUSED(self);

	PyObject* prorgam_text_arg= nullptr;
	PyObject* enable_unused_variable_errors_arg= nullptr;

	if( !PyArg_UnpackTuple( args, "", 1, 2, &prorgam_text_arg, &enable_unused_variable_errors_arg ) )
		return nullptr; // Parse will raise

	const char* program_text= nullptr;
	if( !PyArg_Parse( prorgam_text_arg, "s", &program_text ) )
		return nullptr; // Parse will raise

	int enable_unused_variable_errors= 0;
	if( enable_unused_variable_errors_arg != nullptr && !PyArg_Parse( enable_unused_variable_errors_arg, "p", &enable_unused_variable_errors ) )
		return nullptr; // Parse will raise

	const U1_StringView text_view{ program_text, std::strlen(program_text) };
	llvm::LLVMContext& llvm_context= *g_llvm_context;
	llvm::DataLayout data_layout( GetTestsDataLayout() );

	PyObject* const errors_list= PyList_New(0);

	const bool ok=
		U1_BuildProgramWithErrors(
			text_view,
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout),
			enable_unused_variable_errors != 0,
			g_error_handling_callbacks,
			reinterpret_cast<U1_UserHandle>(errors_list) );

	llvm::llvm_shutdown();

	if( !ok )
	{
		PyErr_SetString( PyExc_RuntimeError, "source tree build failed" );
		return nullptr;
	}

	return errors_list;
}

PyObject* BuildProgramWithSyntaxErrors( PyObject* const self, PyObject* const args )
{
	U_UNUSED(self);

	const char* program_text= nullptr;

	if( !PyArg_ParseTuple( args, "s", &program_text ) )
		return nullptr;

	const U1_StringView text_view{ program_text, std::strlen(program_text) };
	PyObject* const errors_list= PyList_New(0);
	U1_BuildProgramWithSyntaxErrors( text_view, g_error_handling_callbacks, reinterpret_cast<U1_UserHandle>(errors_list) );

	return errors_list;
}

bool FilterTestImpl( const char* const test_name )
{
	const std::string test_name_str= test_name;

	static const std::unordered_set<std::string> c_test_to_disable
	{
	};

	return c_test_to_disable.count( test_name_str ) > 0;
}

} // namespace
