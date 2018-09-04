#include <cmath>
#include <iostream>

#include <Python.h>

#include "../../code_builder_lib/code_builder.hpp"
#include "../../lex_synt_lib/assert.hpp"
#include "../../lex_synt_lib/lexical_analyzer.hpp"
#include "../../lex_synt_lib/syntax_analyzer.hpp"
#include "../../lex_synt_lib/source_graph_loader.hpp"
#include "../tests_common.hpp"

#include "../../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/ManagedStatic.h>
#include "../../code_builder_lib/pop_llvm_warnings.hpp"

namespace U
{

class SingeFileVfs final : public IVfs
{
public:

	SingeFileVfs( ProgramString file_path, const char* const text )
		: file_path_(file_path), file_text_(text)
	{}

	virtual boost::optional<LoadFileResult> LoadFileContent( const Path& file_path, const Path& full_parent_file_path ) override
	{
		U_UNUSED( full_parent_file_path );
		if( file_path == file_path_ )
			return LoadFileResult{ file_path_, DecodeUTF8( file_text_ ) };
		return boost::none;
	}

	virtual Path GetFullFilePath( const Path& file_path, const Path& full_parent_file_path ) override
	{
		U_UNUSED(full_parent_file_path);
		return file_path;
	}

private:
	const ProgramString file_path_;
	const char* const file_text_;
};

static std::unique_ptr<CodeBuilder> CreateCodeBuilder()
{
	return
		std::unique_ptr<CodeBuilder>(
			new CodeBuilder(
				Constants::tests_target_triple_str,
				llvm::DataLayout( Constants::tests_data_layout_str ) ) );
}

std::unique_ptr<llvm::Module> BuildProgram( const char* const text )
{
	const ProgramString file_path= "_"_SpC;
	const SourceGraphPtr source_graph=
		SourceGraphLoader( std::make_shared<SingeFileVfs>( file_path, text ) ).LoadSource( file_path );

	if( source_graph == nullptr ||
		!source_graph->lexical_errors.empty() ||
		!source_graph->syntax_errors.empty() ||
		!( source_graph->root_node_index < source_graph->nodes_storage.size() ) )
		return nullptr;

	ICodeBuilder::BuildResult build_result= CreateCodeBuilder()->BuildProgram( *source_graph );

	for( const CodeBuilderError& error : build_result.errors )
		std::cout << error.file_pos.line << ":" << error.file_pos.pos_in_line << " " << ToStdString( error.text ) << "\n";

	if( !build_result.errors.empty() )
		return nullptr;

	return std::move(build_result.module);
}

} // namespace U

class HaltException final : public std::exception
{
public:
	virtual const char* what() const noexcept override
	{
		return "Halt exception";
	}
};

static llvm::GenericValue HaltCalled( llvm::FunctionType*, llvm::ArrayRef<llvm::GenericValue> )
{
	// Return from interpreter, using native exception.
	throw HaltException();
}

static std::unique_ptr<llvm::ExecutionEngine> g_current_engine; // Can have only one.

static PyObject* BuildProgram( PyObject* const self, PyObject* const args )
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

	std::unique_ptr<llvm::Module> module= U::BuildProgram( program_text );

	if( module == nullptr )
	{
		llvm::llvm_shutdown();
		PyErr_SetString( PyExc_RuntimeError, "program build failed" );
		return nullptr;
	}

	if( print_llvm_asm != 0 )
		module->dump();

	g_current_engine.reset( llvm::EngineBuilder( std::move(module) ).create() );
	if( g_current_engine == nullptr )
	{
		llvm::llvm_shutdown();
		PyErr_SetString( PyExc_RuntimeError, "engine creation failed" );
		return nullptr;
	}

	llvm::sys::DynamicLibrary::AddSymbol( "lle_X___U_halt", reinterpret_cast<void*>( &HaltCalled ) );

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* FreeProgram( PyObject* const self, PyObject* const args )
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

static PyObject* RunFunction( PyObject* const self, PyObject* const args )
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
	unsigned int arg_count= 0u;
	for( unsigned int i= 0u; i < c_max_args; ++i )
	{
		if( function_args[i] == nullptr )
			break;

		bool parse_result= false;
		const llvm::Type* const arg_type= function_type->params()[i];
		if( arg_type->isIntegerTy() )
		{
			const unsigned int bit_width= arg_type->getIntegerBitWidth();
			if( bit_width == 1 )
			{
				int bool_param= 0;
				parse_result= PyArg_Parse( function_args[i], "p", &bool_param );
				llvm_args[i].IntVal= llvm::APInt( 1, bool_param );
			}
			if( bit_width == 8 )
			{
				int8_t int_param= 0;
				parse_result= PyArg_Parse( function_args[i], "b", &int_param );
				llvm_args[i].IntVal= llvm::APInt( 8, int_param );
			}
			if( bit_width == 16 )
			{
				int16_t int_param= 0;
				parse_result= PyArg_Parse( function_args[i], "h", &int_param );
				llvm_args[i].IntVal= llvm::APInt( 16, int_param );
			}
			if( bit_width == 32 )
			{
				int32_t int_param= 0;
				parse_result= PyArg_Parse( function_args[i], "i", &int_param );
				llvm_args[i].IntVal= llvm::APInt( 32, int_param );
			}
			if( bit_width == 64 )
			{
				int64_t int_param= 0;
				parse_result= PyArg_Parse( function_args[i], "L", &int_param );
				llvm_args[i].IntVal= llvm::APInt( 64, int_param );
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

static PyObject* BuildProgramWithErrors( PyObject* const self, PyObject* const args )
{
	U_UNUSED(self);

	const char* program_text= nullptr;

	if( !PyArg_ParseTuple( args, "s", &program_text ) )
		return nullptr;

	const U::ProgramString file_path= U::ToProgramString("_");
	const U::SourceGraphPtr source_graph=
		U::SourceGraphLoader( std::make_shared<U::SingeFileVfs>( file_path, program_text ) ).LoadSource( file_path );

	if( source_graph == nullptr ||
		!source_graph->lexical_errors.empty() ||
		!source_graph->syntax_errors.empty() )
	{
		PyErr_SetString( PyExc_RuntimeError, "source tree build failed" );
		return nullptr;
	}

	PyObject* const list= PyList_New(0);
	for( const U::CodeBuilderError& error : U::CreateCodeBuilder()->BuildProgram( *source_graph ).errors )
	{
		PyObject* const file_pos_dict= PyDict_New();
		PyDict_SetItemString( file_pos_dict, "file_index", PyLong_FromLongLong( error.file_pos.file_index ) );
		PyDict_SetItemString( file_pos_dict, "line", PyLong_FromLongLong( error.file_pos.line ) );
		PyDict_SetItemString( file_pos_dict, "pos_in_line", PyLong_FromLongLong( error.file_pos.pos_in_line ) );

		PyObject* const dict= PyDict_New();
		PyDict_SetItemString( dict, "file_pos", file_pos_dict );

		const char* const error_code_str= U::CodeBuilderErrorCodeToString( error.code );
		PyDict_SetItemString( dict, "code", PyUnicode_DecodeASCII( error_code_str, std::strlen(error_code_str), nullptr ) );

		std::string message= U::ToStdString( error.text );
		PyDict_SetItemString( dict, "text", PyUnicode_DecodeASCII( message.data(), message.size(), nullptr ) );

		PyList_Append( list, dict );
	}

	llvm::llvm_shutdown();

	return list;
}

static PyMethodDef g_methods[]=
{
	{ "build_program"           ,   BuildProgram,           METH_VARARGS, "Build program." },
	{ "free_program"             ,  FreeProgram,            METH_VARARGS, "Free program."  },
	{ "run_function"             ,  RunFunction,            METH_VARARGS, "Run function."  },
	{ "build_program_with_errors",  BuildProgramWithErrors, METH_VARARGS, "Build program with errors." },
	{ nullptr, nullptr, 0, nullptr } // Sentinel
};

static struct PyModuleDef g_module=
{
	PyModuleDef_HEAD_INIT,
	"sprache_compiler_tests_py_lib",   // name of module
	nullptr,  // module documentation, may be NULL
	-1,       // size of per-interpreter state of the module,
			  // or -1 if the module keeps state in global variables.
	g_methods,
	0, 0, 0, 0,
};

PyMODINIT_FUNC PyInit_sprache_compiler_tests_py_lib(void)
{
	return PyModule_Create( &g_module );
}
