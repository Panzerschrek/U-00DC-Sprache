#include <iostream>
#include <unordered_set>

#include <Python.h>

#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/Signals.h>
#include <llvm/Support/ManagedStatic.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../code_builder_lib_common/code_builder_errors.hpp"
#include "../../lex_synt_lib_common/assert.hpp"
#include "../../tests/execution_engine.hpp"
#include "../../tests/tests_common.hpp"
#include "../launchers_common/funcs_c.hpp"

namespace U
{

namespace
{

llvm::ManagedStatic<llvm::LLVMContext> g_llvm_context;

std::unique_ptr<llvm::Module> BuildProgram( const char* const text )
{
	const U1_StringView text_view{ text, std::strlen(text) };

	llvm::LLVMContext& llvm_context= *g_llvm_context;

	llvm::DataLayout data_layout( GetTestsDataLayout() );

	auto ptr=
		U1_BuildProgram(
			text_view,
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout) );

	if( ptr == nullptr )
		return nullptr;

	return std::unique_ptr<llvm::Module>( llvm::unwrap(ptr) );
}

EnginePtr g_current_engine; // Can have only one.

PyObject* BuildProgram( PyObject* const self, PyObject* const args )
{
	U_UNUSED(self);

	PyObject* prorgam_text_arg= nullptr;
	PyObject* print_llvm_asm_arg= nullptr;

	if( !PyArg_UnpackTuple( args, "", 1, 2, &prorgam_text_arg, &print_llvm_asm_arg ) )
		return nullptr; // Parse will raise

	const char* program_text= nullptr;
	if( !PyArg_Parse( prorgam_text_arg, "s", &program_text ) )
		return nullptr; // Parse will raise

	int print_llvm_asm= 0;
	if( print_llvm_asm_arg != nullptr && !PyArg_Parse( print_llvm_asm_arg, "p", &print_llvm_asm ) )
		return nullptr; // Parse will raise

	if( g_current_engine != nullptr )
	{
		PyErr_SetString( PyExc_RuntimeError, "can not have more then one program in one time" );
		return nullptr;
	}

	std::unique_ptr<llvm::Module> module= BuildProgram( program_text );

	if( module == nullptr )
	{
		llvm::llvm_shutdown();
		PyErr_SetString( PyExc_RuntimeError, "program build failed" );
		return nullptr;
	}

	g_current_engine= CreateEngine( std::move(module), print_llvm_asm != 0 );

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* FreeProgram( PyObject* const self, PyObject* const args )
{
	U_UNUSED(self);
	U_UNUSED(args);

	if( g_current_engine != nullptr )
	{
		g_current_engine.reset();
		// We must kill ALL static internal llvm variables.
		llvm::llvm_shutdown();
	}

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* RunFunction( PyObject* const self, PyObject* const args )
{
	U_UNUSED(self);

	const unsigned c_max_args= 8;
	PyObject* function_name_param= nullptr;
	PyObject* function_args[c_max_args]= { nullptr };

	if( !PyArg_UnpackTuple(
			args, "", 1, 1 + c_max_args,
			&function_name_param,
			&function_args[0],
			&function_args[1],
			&function_args[2],
			&function_args[3],
			&function_args[4],
			&function_args[5],
			&function_args[6],
			&function_args[7] ) )
		return nullptr; // Parse will raise

	if( g_current_engine == nullptr )
	{
		PyErr_SetString( PyExc_RuntimeError, "current program is empty" );
		return nullptr;
	}

	const char* function_name= nullptr;
	if( !PyArg_Parse( function_name_param, "s", &function_name ) )
		return nullptr; // Parse will raise

	llvm::Function* const function= g_current_engine->FindFunctionNamed( function_name );
	if( function == nullptr )
	{
		PyErr_SetString( PyExc_RuntimeError, "can not get function" );
		return nullptr;
	}

	const llvm::FunctionType* const function_type= function->getFunctionType();

	llvm::GenericValue llvm_args[c_max_args];
	uint32_t arg_count= 0u;
	for( uint32_t i= 0u; i < c_max_args; ++i )
	{
		if( function_args[i] == nullptr )
			break;

		bool parse_result= false;
		const llvm::Type* const arg_type= function_type->params()[i];
		if( arg_type->isIntegerTy() )
		{
			const uint32_t bit_width= arg_type->getIntegerBitWidth();
			if( bit_width == 1 )
			{
				int bool_param= 0;
				parse_result= PyArg_Parse( function_args[i], "p", &bool_param );
				llvm_args[i].IntVal= llvm::APInt( 1, uint64_t(bool_param) );
			}
			if( bit_width == 8 )
			{
				int8_t int_param= 0;
				parse_result= PyArg_Parse( function_args[i], "b", &int_param );
				llvm_args[i].IntVal= llvm::APInt( 8, uint64_t(int_param) );
			}
			if( bit_width == 16 )
			{
				int16_t int_param= 0;
				parse_result= PyArg_Parse( function_args[i], "h", &int_param );
				llvm_args[i].IntVal= llvm::APInt( 16, uint64_t(int_param) );
			}
			if( bit_width == 32 )
			{
				int32_t int_param= 0;
				parse_result= PyArg_Parse( function_args[i], "i", &int_param );
				llvm_args[i].IntVal= llvm::APInt( 32, uint64_t(int_param) );
			}
			if( bit_width == 64 )
			{
				int64_t int_param= 0;
				parse_result= PyArg_Parse( function_args[i], "L", &int_param );
				llvm_args[i].IntVal= llvm::APInt( 64, uint64_t(int_param) );
			}
		}
		else if( arg_type->isFloatTy() )
			parse_result= PyArg_Parse( function_args[i], "f", &llvm_args[i].FloatVal );
		else if( arg_type->isDoubleTy() )
			parse_result= PyArg_Parse( function_args[i], "d", &llvm_args[i].DoubleVal );
		else
		{
			PyErr_SetString( PyExc_RuntimeError, "unsupported arg type" );
			return nullptr;
		}

		if( !parse_result )
			return nullptr; // Parse will raise

		++arg_count;
	} // for args

	if( function_type->getNumParams() != arg_count )
	{
		PyErr_SetString( PyExc_RuntimeError, "invalid param count" );
		return nullptr;
	}

	llvm::GenericValue result_value;
	try
	{
		result_value=
			g_current_engine->runFunction(
				function,
				llvm::ArrayRef<llvm::GenericValue>( llvm_args, arg_count ) );
	}
	catch( const HaltException& )
	{
		PyErr_SetString( PyExc_RuntimeError, "Program halted" );
		return nullptr;
	}
	catch( const ExecutionEngineException& ex )
	{
		for( const std::string& e : ex.errors )
			std::cerr << "\n" << e;
		std::cerr << std::endl;

		PyErr_SetString( PyExc_RuntimeError, "Execution engine error" );
		return nullptr;
	}

	const llvm::Type* const return_type= function_type->getReturnType();
	if( return_type->isVoidTy() )
	{
		Py_INCREF(Py_None);
		return Py_None;
	}
	if( return_type->isIntegerTy() )
		return Py_BuildValue( "L", result_value.IntVal.getLimitedValue() );
	if( return_type->isFloatTy() )
		return Py_BuildValue( "f", result_value.FloatVal );
	if( return_type->isDoubleTy() )
		return Py_BuildValue( "d", result_value.DoubleVal );

	Py_INCREF(Py_None);
	return Py_None;
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

	const char* program_text= nullptr;

	if( !PyArg_ParseTuple( args, "s", &program_text ) )
		return nullptr;

	const U1_StringView text_view{ program_text, std::strlen(program_text) };
	llvm::LLVMContext& llvm_context= *g_llvm_context;
	llvm::DataLayout data_layout( GetTestsDataLayout() );

	PyObject* const errors_list= PyList_New(0);

	const bool ok=
		U1_BuildProgramWithErrors(
			text_view,
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout),
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

PyObject* FilterTest( PyObject* const self, PyObject* const args )
{
	U_UNUSED(self);

	PyObject* func_name_arg= nullptr;

	if( !PyArg_UnpackTuple( args, "", 1, 1, &func_name_arg ) )
		return nullptr; // Parse will raise

	const char* func_name= nullptr;
	if( !PyArg_Parse( func_name_arg, "s", &func_name ) )
		return nullptr; // Parse will raise

	const std::string func_name_str= func_name;

	static const std::unordered_set<std::string> c_test_to_disable
	{
		"AutoForFunctionTemplate_Test0",
		"AutoForFunctionTemplate_Test1",
		"AutoForReturnType_Test0",
		"AutoForReturnType_Test1",
		"AutoForReturnType_Test2",
		"AutoForReturnType_Test3",
		"AutoForReturnType_Test4",
		"AutoForReturnType_Test5",
		"AutoForReturnType_Test6",
		"AutoFunctionInsideClassesNotAllowed_Test0",
		"AutoFunctionInsideClassesNotAllowed_Test1",
		"ExpectedBodyForAutoFunction_Test0",
		"ExpectedBodyForAutoFunction_Test1",
		"ExpectedReferenceValue_ForAutoReturnValue_Test0",
		"GlobalsLoop_ForFunctionWithAutoReturnType_Test0",
		"GlobalsLoop_ForFunctionWithAutoReturnType_Test1",
		"TemplateParametersDeductionFailed_Test11",
		"TypesMismtach_ForAutoReturnValue_Test0",
		"TypesMismtach_ForAutoReturnValue_Test1",
	};

	if( c_test_to_disable.count( func_name_str ) > 0 )
	{
		Py_INCREF(Py_False);
		return Py_False;
	}

	Py_INCREF(Py_True);
	return Py_True;
}

PyMethodDef g_methods[]=
{
	{ "build_program"             , BuildProgram          , METH_VARARGS, "Build program."             },
	{ "free_program"              , FreeProgram           , METH_VARARGS, "Free program."              },
	{ "run_function"              , RunFunction           , METH_VARARGS, "Run function."              },
	{ "build_program_with_errors" , BuildProgramWithErrors, METH_VARARGS, "Build program with errors." },
	{ "build_program_with_syntax_errors" , BuildProgramWithSyntaxErrors, METH_VARARGS, "Build program with syntax errors." },
	{ "filter_test"               , FilterTest            , METH_VARARGS, "Filter test returns False ih should skip test" },
	{ nullptr, nullptr, 0, nullptr } // Sentinel
};

struct PyModuleDef g_module=
{
	PyModuleDef_HEAD_INIT,
	"sprache_compiler_tests_py_lib",   // name of module
	nullptr,  // module documentation, may be NULL
	-1,       // size of per-interpreter state of the module,
			  // or -1 if the module keeps state in global variables.
	g_methods,
	0, 0, 0, 0,
};

} // namespace

} // namespace U

#ifdef __GNUC__
#define U_EXTERN_VISIBILITY __attribute__ ((visibility ("default")))
#endif // __GNUC__
#ifdef _MSC_VER
#define U_EXTERN_VISIBILITY __declspec(dllexport)
#endif // _MSC_VER

extern "C" U_EXTERN_VISIBILITY PyObject* PyInit_sprache_compiler_tests_py_lib()
{
	llvm::sys::PrintStackTraceOnErrorSignal("");
	return PyModule_Create( &U::g_module );
}
