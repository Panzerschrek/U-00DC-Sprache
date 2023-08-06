#include <iostream>
#include <sstream>

#include <Python.h>

#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/Signals.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../code_builder_lib/code_builder.hpp"
#include "../../code_builder_lib_common/source_file_contents_hash.hpp"
#include "../../lex_synt_lib_common/assert.hpp"
#include "../lex_synt_lib/lexical_analyzer.hpp"
#include "../lex_synt_lib/syntax_analyzer.hpp"
#include "../lex_synt_lib/source_graph_loader.hpp"
#include "../../tests/execution_engine.hpp"
#include "../../tests/tests_common.hpp"

namespace U
{

namespace
{

class SingeFileVfs final : public IVfs
{
public:

	SingeFileVfs( std::string file_path, const char* const text )
		: file_path_(file_path), file_text_(text)
	{}

	virtual std::optional<FileContent> LoadFileContent( const Path& full_file_path ) override
	{
		if( full_file_path == file_path_ )
			return file_text_;
		return std::nullopt;
	}

	virtual Path GetFullFilePath( const Path& file_path, const Path& full_parent_file_path ) override
	{
		U_UNUSED(full_parent_file_path);
		return file_path;
	}

private:
	const std::string file_path_;
	const char* const file_text_;
};

void PrintLexSyntErrors( const SourceGraph& source_graph )
{
	std::vector<std::string> source_files;
	source_files.reserve( source_graph.nodes_storage.size() );
	for( const auto& node : source_graph.nodes_storage )
		source_files.push_back( node.file_path );

	PrintLexSyntErrors( source_files, source_graph.errors );
}

llvm::ManagedStatic<llvm::LLVMContext> g_llvm_context;

std::unique_ptr<CodeBuilder> CreateCodeBuilder(const bool enable_unused_name_errors)
{
	CodeBuilderOptions options;
	options.build_debug_info= true;
	options.generate_tbaa_metadata= true;
	options.report_about_unused_names= false; // It is easier to silence such errors, rather than fixing a lot of tests.
	options.report_about_unused_names= enable_unused_name_errors;

	return
		std::make_unique<CodeBuilder>(
			*g_llvm_context,
			llvm::DataLayout( GetTestsDataLayout() ),
			GetTestsTargetTriple(),
			options );
}

std::unique_ptr<llvm::Module> BuildProgram( const char* const text )
{
	const std::string file_path= "_";
	SingeFileVfs vfs( file_path, text );
	const SourceGraph source_graph= LoadSourceGraph( vfs, CalculateSourceFileContentsHash, file_path );

	PrintLexSyntErrors( source_graph );

	if( !source_graph.errors.empty() )
		return nullptr;

	CodeBuilder::BuildResult build_result= CreateCodeBuilder(false)->BuildProgram( source_graph );

	for( const CodeBuilderError& error : build_result.errors )
		std::cerr << error.src_loc.GetLine() << ":" << error.src_loc.GetColumn() << " " << error.text << "\n";

	if( !build_result.errors.empty() )
		return nullptr;

	return std::move(build_result.module);
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


PyObject* BuildSrcLoc( const SrcLoc& src_loc )
{
	PyObject* const dict= PyDict_New();
	PyDict_SetItemString( dict, "file_index", PyLong_FromLongLong( src_loc.GetFileIndex() ) );
	PyDict_SetItemString( dict, "line", PyLong_FromLongLong( src_loc.GetLine() ) );
	PyDict_SetItemString( dict, "column", PyLong_FromLongLong( src_loc.GetColumn() ) );
	return dict;
}

PyObject* BuildString( const std::string& str )
{
	return PyUnicode_DecodeUTF8( str.data(), Py_ssize_t(str.size()), nullptr );
}

PyObject* BuildErrorsList( const CodeBuilderErrorsContainer& errors )
{
	PyObject* const list= PyList_New(0);

	for( const CodeBuilderError& error : errors )
	{
		PyObject* const dict= PyDict_New();

		PyDict_SetItemString( dict, "src_loc", BuildSrcLoc( error.src_loc ) );

		const std::string_view error_code_str= CodeBuilderErrorCodeToString( error.code );
		PyDict_SetItemString( dict, "code", PyUnicode_DecodeUTF8( error_code_str.data(), Py_ssize_t(error_code_str.size()), nullptr ) );

		PyDict_SetItemString( dict, "text", BuildString( error.text ) );

		if( error.template_context != nullptr )
		{
			PyObject* const template_context_dict= PyDict_New();

			PyDict_SetItemString( template_context_dict, "errors", BuildErrorsList( error.template_context->errors ) );
			PyDict_SetItemString( template_context_dict, "src_loc", BuildSrcLoc( error.template_context->context_declaration_src_loc ) );
			PyDict_SetItemString( template_context_dict, "template_name", BuildString( error.template_context->context_name ) );
			PyDict_SetItemString( template_context_dict, "parameters_description", BuildString( error.template_context->parameters_description ) );

			PyDict_SetItemString( dict, "template_context", template_context_dict );
		}

		PyList_Append( list, dict );
	}

	return list;
}

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

	const std::string file_path= "_";
	SingeFileVfs vfs( file_path, program_text );
	const SourceGraph source_graph= LoadSourceGraph( vfs, CalculateSourceFileContentsHash, file_path );

	PrintLexSyntErrors( source_graph );

	if( !source_graph.errors.empty() )
	{
		PyErr_SetString( PyExc_RuntimeError, "source tree build failed" );
		return nullptr;
	}

	PyObject* const list= BuildErrorsList( CreateCodeBuilder(enable_unused_variable_errors != 0)->BuildProgram( source_graph ).errors );
	llvm::llvm_shutdown();

	return list;
}

PyObject* BuildProgramWithSyntaxErrors( PyObject* const self, PyObject* const args )
{
	U_UNUSED(self);

	const char* program_text= nullptr;

	if( !PyArg_ParseTuple( args, "s", &program_text ) )
		return nullptr;

	const std::string file_path= "_";

	SingeFileVfs vfs( file_path, program_text );
	const SourceGraph source_graph= LoadSourceGraph( vfs, CalculateSourceFileContentsHash, file_path );

	std::vector<CodeBuilderError> errors_converted;
	errors_converted.reserve( source_graph.errors.size() );
	for( const LexSyntError& error_message : source_graph.errors )
	{
		CodeBuilderError error_converted;
		error_converted.code= CodeBuilderErrorCode::BuildFailed;
		error_converted.src_loc= error_message.src_loc;
		error_converted.text= error_message.text;
		errors_converted.push_back( std::move(error_converted) );
	}

	return BuildErrorsList( errors_converted );
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

	static const std::unordered_set<std::string> c_tests_to_ignore
	{
		"TemplateParametersDeductionFailed_Test11",
		"AutoForFunctionTemplate_Test1", // Auto return type doesn't work properly with auto-constexpr for templates.
	};

	if( c_tests_to_ignore.count(func_name) > 0 )
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
