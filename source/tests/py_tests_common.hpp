#pragma once

// This file is intended to be included only in one translation unit.

#include <iostream>

#include <Python.h>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/ManagedStatic.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../lex_synt_lib_common/assert.hpp"
#include "../tests/execution_engine.hpp"


namespace U
{

// Functions needed to be implemented by a specific PyTests implementation.

std::unique_ptr<llvm::Module> BuildProgramImpl( const char* const text, const bool enable_unsed_name_errors );
PyObject* BuildProgramUnusedErrorsEnabled( PyObject* const self, PyObject* const args );
PyObject* BuildProgramWithErrors( PyObject* const self, PyObject* const args );
PyObject* BuildProgramWithSyntaxErrors( PyObject* const self, PyObject* const args );
bool FilterTestImpl( const char* test_name );

// End function protorypes.

// For simplicity use gobal instances of LLVM context and execution engine.
llvm::ManagedStatic<llvm::LLVMContext> g_llvm_context;

EnginePtr g_current_engine;

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
		PyErr_SetString( PyExc_RuntimeError, "can not have more than one program in one time" );
		return nullptr;
	}

	std::unique_ptr<llvm::Module> module= BuildProgramImpl( program_text, false );

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

PyObject* BuildProgramUnusedErrorsEnabled( PyObject* const self, PyObject* const args )
{
	U_UNUSED(self);

	const char* program_text= nullptr;

	if( !PyArg_ParseTuple( args, "s", &program_text ) )
		return nullptr;

	if( g_current_engine != nullptr )
	{
		PyErr_SetString( PyExc_RuntimeError, "can not have more than one program in one time" );
		return nullptr;
	}

	std::unique_ptr<llvm::Module> module= BuildProgramImpl( program_text, true );

	if( module == nullptr )
	{
		llvm::llvm_shutdown();
		PyErr_SetString( PyExc_RuntimeError, "program build failed" );
		return nullptr;
	}

	g_current_engine= CreateEngine( std::move(module) );

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

PyObject* FilterTest( PyObject* const self, PyObject* const args )
{
	U_UNUSED(self);

	PyObject* func_name_arg= nullptr;

	if( !PyArg_UnpackTuple( args, "", 1, 1, &func_name_arg ) )
		return nullptr; // Parse will raise

	const char* func_name= nullptr;
	if( !PyArg_Parse( func_name_arg, "s", &func_name ) )
		return nullptr; // Parse will raise

	if( FilterTestImpl( func_name ) )
	{
		Py_INCREF(Py_False);
		return Py_False;
	}

	Py_INCREF(Py_True);
	return Py_True;
}

static PyMethodDef g_methods[]=
{
	{ "build_program"             , BuildProgram          , METH_VARARGS, "Build program."             },
	{ "build_program_unused_errors_enabled", BuildProgramUnusedErrorsEnabled, METH_VARARGS, "Build program with enabled unused names errors." },
	{ "free_program"              , FreeProgram           , METH_VARARGS, "Free program."              },
	{ "run_function"              , RunFunction           , METH_VARARGS, "Run function."              },
	{ "build_program_with_errors" , BuildProgramWithErrors, METH_VARARGS, "Build program with errors." },
	{ "build_program_with_syntax_errors" , BuildProgramWithSyntaxErrors, METH_VARARGS, "Build program with syntax errors." },
	{ "filter_test"               , FilterTest            , METH_VARARGS, "Filter test returns False ih should skip test" },
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

} // namespace U

#ifdef __GNUC__
#define U_EXTERN_VISIBILITY __attribute__ ((visibility ("default")))
#endif // __GNUC__
#ifdef _MSC_VER
#define U_EXTERN_VISIBILITY __declspec(dllexport)
#endif // _MSC_VER

extern "C" U_EXTERN_VISIBILITY PyObject* PyInit_sprache_compiler_tests_py_lib()
{
	return PyModule_Create( &U::g_module );
}
