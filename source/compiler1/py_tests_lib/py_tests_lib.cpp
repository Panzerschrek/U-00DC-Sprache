#include <iostream>
#include <unordered_set>

#include <Python.h>

#include "../../lex_synt_lib/assert.hpp"
#include "../../tests/tests_common.hpp"
#include "../tests_common/funcs_c.hpp"

#include "../../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../../code_builder_lib/pop_llvm_warnings.hpp"

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

	return std::unique_ptr<llvm::Module>( reinterpret_cast<llvm::Module*>(ptr) );
}

class HaltException final : public std::exception
{
public:
	virtual const char* what() const noexcept override
	{
		return "Halt exception";
	}
};

llvm::GenericValue HaltCalled( llvm::FunctionType*, llvm::ArrayRef<llvm::GenericValue> )
{
	// Return from interpreter, using native exception.
	throw HaltException();
}

std::unique_ptr<llvm::ExecutionEngine> g_current_engine; // Can have only one.

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

	if( print_llvm_asm != 0 )
	{
		llvm::raw_os_ostream stream(std::cout);
		module->print( stream, nullptr );
	}

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

	const auto error_handler=
	[](
		void* const data,
		const uint32_t line,
		const uint32_t column,
		const uint32_t error_code,
		const U1_StringView& error_text )
	{
		PyObject* const dict= PyDict_New();

		{
			PyObject* const file_pos_dict= PyDict_New();
			PyDict_SetItemString( file_pos_dict, "file_index", PyLong_FromLongLong(0) );
			PyDict_SetItemString( file_pos_dict, "line", PyLong_FromLongLong(line) );
			PyDict_SetItemString( file_pos_dict, "column", PyLong_FromLongLong(column) );

			PyDict_SetItemString( dict, "file_pos", file_pos_dict );
		}

		const char* error_code_str= nullptr;
		size_t error_code_len= 0u;
		U1_CodeBuilderCodeToString( error_code, error_code_str, error_code_len );
		PyDict_SetItemString( dict, "code", PyUnicode_DecodeUTF8( error_code_str, Py_ssize_t(error_code_len), nullptr ) );

		PyDict_SetItemString( dict, "text", PyUnicode_DecodeUTF8( error_text.data, Py_ssize_t(error_text.size), nullptr ) );

		PyList_Append( reinterpret_cast<PyObject*>(data), dict );
	};

	const bool ok=
		U1_BuildProgramWithErrors(
			text_view,
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout),
			error_handler,
			errors_list );

	llvm::llvm_shutdown();

	if( !ok )
	{
		PyErr_SetString( PyExc_RuntimeError, "source tree build failed" );
		return nullptr;
	}

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

	static const std::unordered_set<std::string> c_test_to_enable
	{
		"AutoVariableInitialization_UsingFunctionPointer_Test0",
		"BindingConstReferenceToNonconstReference_InFunctionPointerCall_Test0",
		"CharAsTemplateParameter_Test0",
		"CharIsEqualityComparable_Test0",
		"CharIsOrderComparable_Test0",
		"CouldNotConvertFunctionPointer_Test0",
		"CouldNotConvertFunctionPointer_Test1",
		"CouldNotConvertFunctionPointer_Test2",
		"CouldNotConvertFunctionPointer_Test3",
		"CouldNotConvertFunctionPointer_Test4",
		"CouldNotConvertFunctionPointer_Test5",
		"CouldNotConvertFunctionPointer_Test6",
		"CouldNotConvertFunctionPointer_Test7",
		"CouldNotConvertFunctionPointer_Test8",
		"CouldNotConvertFunctionPointer_Test9",
		"CouldNotConvertFunctionPointer_Test10",
		"CouldNotOverloadFunction_ForUnsafe_Test0",
		"CouldNotOverloadFunction_ForUnsafe_Test1",
		"ExpectedReferenceValue_InFunctionPointerCall_Test0",
		"ErrorsTest0",
		"ErrorsTest1",
		"FunctionPointersConversions_Test0",
		"FunctionPointersConversions_Test1",
		"FunctionPointersConversions_Test4",
		//"FunctionPointersConversions_Test5",
		"FunctionTypeDeclaration_Test0",
		"FunctionTypeDeclaration_Test1",
		"FunctionTypeDeclaration_Test2",
		"FunctionTypeDeclaration_Test3",
		"FunctionTypeDeclaration_Test4",
		"Int128_Test0",
		"OkTest",
		"SafeBlockResetsUnsafe_Test",
		"SimpliestTest",
		"SimplePassArgumentTest",
		"TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test0",
		"TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test1",
		"TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test2",
		"TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test3",
		"TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test4",
		"TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test5",
		"TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test6",
		"TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test7",
		"TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test8",
		"TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test9",
		"TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test10",
		"TypeNameInErrorMessage_ClassTemplate_Test0",
		"TypeNameInErrorMessage_ClassTemplate_Test1",
		"TypeNameInErrorMessage_ClassTemplate_Test2",
		"TypeNameInErrorMessage_ClassTemplate_Test3",
		"TypeNameInErrorMessage_ClassTemplate_Test4",
		"TypeNameInErrorMessage_ClassTemplate_Test5",
		"TypeNameInErrorMessage_ClassTemplate_Test6",
		"TypeNameInErrorMessage_ClassTemplate_Test7",
		//"TypeNameInErrorMessage_ClassTemplate_Test8",
		"TypeNameInErrorMessage_ClassTypeInGlobalNamespace",
		"TypeNameInErrorMessage_FundamentalTypes",
		"UnsafeBlockDeclaration_Test0",
		"UnsafeFunctionDeclaration_Test0",
		"UnsafeFunctionDeclaration_Test1",
		"UnsafeInsideUnsafe_Test",
		"ZeroInitializerForChar_Test0",
	};

	if( c_test_to_enable.count( func_name_str ) > 0 )
	{
		Py_INCREF(Py_True);
		return Py_True;
	}

	static const std::string c_tests_to_enable_pattern[]
	{
		"CharIsConstructibleFromChar_Test",
		"CharIsConstructibleFromInt_Test",
		"CharIsNotIntegerType_Test",
		"CharIsUnsigned_Test",
		"FunctionPointerAsFunctionArgument_Test",
		"FunctionPointerAsSpecializedTemplateParameter_Test",
		"FunctionPointerCall_Test",
		"FunctionPointerEqualityComparision_Test",
		"FunctionPoinerInitialization_Test",
		"FunctionPointerReferencesIsNotCompatible_Test",
		"FunctionsPoitersAssignment_Test",
		"InvalidFunctionArgumentCount_Test",
		"NonExistentTest",
		"NumericConstants_BinaryConstants",
		"NumericConstants_DecimalConstants",
		"NumericConstants_HexadecimalConstants",
		"NumericConstants_OctalConstants",
		"TypeNameInErrorMessage_ClassTypeInNamespace_Test",
		"TypesMismatch_InFunctionPointerCall_Test",
		"UnsafeFunctionCallInsideUnsafeBlock_Test",
		"UnsafeFunctionCallOutsideUnsafeBlock_Test",
	};

	if( std::find_if(
			std::begin(c_tests_to_enable_pattern),
			std::end(c_tests_to_enable_pattern),
			[&]( const std::string& pattern ) { return func_name_str.find( pattern ) != std::string::npos; } )
		!= std::end(c_tests_to_enable_pattern) )
	{
		Py_INCREF(Py_True);
		return Py_True;
	}

	Py_INCREF(Py_False);
	return Py_False;
}

PyMethodDef g_methods[]=
{
	{ "build_program"             , BuildProgram          , METH_VARARGS, "Build program."             },
	{ "free_program"              , FreeProgram           , METH_VARARGS, "Free program."              },
	{ "run_function"              , RunFunction           , METH_VARARGS, "Run function."              },
	{ "build_program_with_errors" , BuildProgramWithErrors, METH_VARARGS, "Build program with errors." },
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
	return PyModule_Create( &U::g_module );
}
