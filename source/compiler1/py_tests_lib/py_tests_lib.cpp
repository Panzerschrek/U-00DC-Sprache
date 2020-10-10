#include <iostream>
#include <unordered_set>

#include <Python.h>

#include "../../lex_synt_lib/assert.hpp"
#include "../../tests/tests_common.hpp"
#include "../tests_common/funcs_c.hpp"

#include "../../compilers_common/push_disable_llvm_warnings.hpp"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/Signals.h>
#include "../../compilers_common/pop_llvm_warnings.hpp"

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

static UserHandle ErrorHandler(
	const UserHandle data,
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

	const auto list_object= reinterpret_cast<PyObject*>(data);
	PyList_Append( list_object, dict );

	return reinterpret_cast<UserHandle>(list_object);
}

UserHandle TemplateErrorsContextHandler(
	const UserHandle data, // should be "CodeBuilderError*"
	const uint32_t line,
	const uint32_t column,
	const U1_StringView& context_name,
	const U1_StringView& args_description )
{
	(void)data;
	(void)line;
	(void)column;
	(void)context_name;
	(void)args_description;
	return 0;
}

static const ErrorsHandlingCallbacks g_error_handling_callbacks
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
			reinterpret_cast<UserHandle>(errors_list) );

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
	U1_BuildProgramWithSyntaxErrors( text_view, g_error_handling_callbacks, reinterpret_cast<UserHandle>(errors_list) );

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
		"ArrayTypesInfo_Test0",
		"AutoVariableInitialization_UsingFunctionPointer_Test0",
		"BaseClassForInterface_Test0",
		"BindingConstReferenceToNonconstReference_InFunctionPointerCall_Test0",
		"CastImut_OperatorDeclaration_Test",
		"CastImut_Test0_CastMutableReferenceToImmutableReference",
		"CastImut_Test1_CastImmutableReferenceToImmutableReference",
		"CastImut_Test2_CastValueToImmutableReference",
		"CastImut_Test3_ShouldPreserveReferences",
		"CastMut_OperatorDeclaration_Test",
		"CastMut_Test0_CastImmutableReferenceToMutableReference",
		"CastMut_Test1_CastMutableReferenceToMutableReference",
		"CastMut_Test2_CastValueToMutableReference",
		"CastMut_Test3_ShouldPreserveReferences",
		"CastMut_Test4_OperationIsUnsafe",
		"CastMut_Test5_OperationIsUnsafe",
		"CastMut_Test6_ConstexprLostInConversion",
		"CastRef_OperatorDeclaration_Test",
		"CastRef_Test0_ShouldCastToVoid",
		"CastRef_Test1_ShouldCastChildToParent",
		"CastRef_Test2_CastToSameType",
		"CastRef_Test3_ShouldSaveMutability",
		"CastRef_Test4_ShouldSaveMutability",
		"CastRef_Test5_ShouldCastValue",
		//"CastRef_Test7_CompleteteTypeRequiredForSource",
		"CastRef_Test8_CompleteteTypeRequiredForDestination",
		"CastRef_Test9_ShouldPreserveReferencedVariables",
		"CastRef_Test10_ShouldPreserveReferencedVariables",
		"CastRefUnsafe_OperatorDeclaration_Test",
		"CastRefUnsafe_Test0_SholudCastIncompatibleReferences",
		"CastRefUnsafe_Test1_SholudCastVoidToAnything",
		"CastRefUnsafe_Test2_SholudCastVoidToAnything",
		"CastRefUnsafe_Test3_ShouldSaveMutability",
		"CastRefUnsafe_Test4_ShouldSaveMutability",
		"CastRefUnsafe_Test5_UnsafeCastWorksAlsoForSafeCasting",
		"CastRefUnsafe_Test6_OperatorAllowedOnlyInsideUnsafeBlock",
		"CastRefUnsafe_Test7_OperatorAllowedOnlyInsideUnsafeBlock",
		"CastRefUnsafe_Test8_CompletenessStillRequiredForUnsafeCast",
		//"CastRefUnsafe_Test9_CompletenessStillRequiredForUnsafeCast",
		"CharAsTemplateParameter_Test0",
		"CharAsTemplateParameter_Test1",
		"CharIsEqualityComparable_Test0",
		"CharIsOrderComparable_Test0",
		"CharLiteral_Test0",
		"CharLiteral_Test1",
		"CharLiteral_Test2",
		"CharLiteral_Test3",
		"CharLiteralIsConstantValue_Test0",
		"CharSize_Test0",
		"ClassHaveNoCopyAssignementOperatorByDefault_Test0",
		"ClassTypesInfo_Test0",
		"ClassTypesInfo_Test1",
		"ClassTypesInfo_Test2",
		"ClassTypesInfo_Test3",
		"ClassTypesInfo_Test4",
		"ConstexprHalt_Test0",
		"ConstructingAbstractClassOrInterface_Test0",
		"ConstructingAbstractClassOrInterface_Test1",
		"ConstructingAbstractClassOrInterface_Test2",
		"ConstructingAbstractClassOrInterface_Test3",
		"ConstructingAbstractClassOrInterface_Test4",
		"ConstructingAbstractClassOrInterface_Test5",
		"ConstructingAbstractClassOrInterface_Test6",
		"ConstructingAbstractClassOrInterface_Test7",
		"ConstructingAbstractClassOrInterface_Test8",
		"ConstructingAbstractClassOrInterface_Test9",
		"ConstructingAbstractClassOrInterface_Test10",
		"ConstructingAbstractClassOrInterface_Test11",
		"ConstructorForInterface_Test0",
		"CopyAssignentOperatorGeneration_Test0",
		"CopyConstructorGeneration_Test0",
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
		"CouldNotConvertFunctionPointer_Test11",
		"CouldNotConvertFunctionPointer_Test12",
		"CouldNotOverloadFunction_Test0",
		"CouldNotOverloadFunction_Test1",
		"CouldNotOverloadFunction_Test2",
		"CouldNotOverloadFunction_Test3",
		"CouldNotOverloadFunction_Test4",
		"CouldNotSelectFunctionForPointer_Test0",
		"CouldNotSelectOverloadedFunction_Test0",
		"CreateMutableReferenceToVariableWithMutableReference",
		"DefaultClassVisibilityIsPublic_Test0",
		"DefaultConstructorGeneration_Test0",
		"DestructorsCall_ForTernaryOperatorBranches_Test0",
		"DestructorsCall_ForTernaryOperatorResult_Test0",
		"DisableCopyAssignmentOperator_Test0",
		"DisableCopyConstructor_Test0",
		"DisabledFunction_Test0",
		"DisabledFunction_Test1",
		"DisabledFunction_Test2",
		"DisabledFunction_Test3",
		"DisabledFunction_Test4",
		"DuplicatedBaseClass_Test0",
		"EnabledFunction_Test0",
		"EnabledFunction_Test1",
		"EnumAsConstexprStructMember_Test0",
		"EnumTypesInfo_Test0",
		"ErrorsTest0",
		"ErrorsTest1",
		"ExpectedConstantExpression_ForEnableIf_Test0",
		"ExpectedReferenceValue_InFunctionPointerCall_Test0",
		"FieldIsNotInitializedYet_ForBase_Test0",
		"FieldIsNotInitializedYet_ForBase_Test1",
		"FieldIsNotInitializedYet_ForBase_Test2",
		"FieldsForInterfacesNotAllowed_Test0",
		"FinalForFirstVirtualFunction_Test0",
		"FunctionBodyVisibilityIsUnsignificant_Test1",
		"FunctionCanNotBeVirtual_Test0",
		"FunctionContextForTypePreparing_Test0",
		"FunctionDeclarationOutsideItsScope_ForFunctionTemplates_Test0",
		"FunctionPointerInConstexprStruct_Test0",
		"FunctionPointersConversions_Test0",
		"FunctionPointersConversions_Test1",
		"FunctionPointersConversions_Test2",
		"FunctionPointersConversions_Test3",
		"FunctionPointersConversions_Test4",
		"FunctionPointersConversions_Test5",
		"FunctionPointerPointedToNonvirtualFunction_Test0",
		"FunctionPointerTypesInfo_Test0",
		"FunctionsVisibilityMismatch_Test0",
		"FunctionsVisibilityMismatch_Test1",
		"FunctionsVisibilityMismatch_Test2",
		"FunctionsVisibilityMismatch_Test3",
		"FunctionsVisibilityMismatch_Test4",
		"FunctionsVisibilityMismatch_Test5",
		"FunctionsVisibilityMismatch_Test6",
		"FunctionTypeDeclaration_Test0",
		"FunctionTypeDeclaration_Test1",
		"FunctionTypeDeclaration_Test2",
		"FunctionTypeDeclaration_Test3",
		"FunctionTypeDeclaration_Test4",
		"FunctionTypeDeclaration_Test5",
		"FundamentalTypesInfo_Test0",
		"GeneratedCopyConstructor_Test0",
		"GlobalsLoopDetected_Test0",
		"GlobalsLoopDetected_Test1",
		"GlobalsLoopDetected_Test2",
		"GlobalsLoopDetected_Test3",
		"GlobalsLoopDetected_Test4",
		"GlobalsLoopDetected_Test5",
		"GlobalsLoopDetected_Test6",
		"GlobalsLoopDetected_Test7",
		"ImplicitInitializerUsedInsteadOf_InClassFieldInitializer_Test0",
		"ImplicitInitializerUsedInsteadOf_InClassFieldInitializer_Test1",
		"ImplicitInitializerUsedInsteadOf_InClassFieldInitializer_Test2",
		"ImplicitInitializerUsedInsteadOf_InClassFieldInitializer_Test3",
		"InClassFieldInitializerCheck_Test0",
		"InClassFieldInitializerCheck_Test1",
		"InClassFieldInitializerCheck_Test2",
		"InClassFieldInitializerCheck_Test3",
		"InClassFieldInitializerCheck_Test4",
		"InClassFieldInitializer_UnsafeInitializerDisabled_Test0",
		"IncompleteMemberOfClassTemplate_ForFunctionTemplates_Test0",
		"InnereReferenceTransferedInTakeOperator_Test0",
		"Int128_Test0",
		"InvalidTypeOfTemplateVariableArgument_ForFunctionTemplateParameter",
		"LessSpecializedTemplateTypesNotGenerated_Test0",
		"LocalVariableTest",
		"MacroExpansion_Test0",
		"MacroExpansion_Test1",
		"MacroExpansion_Test2",
		"MacroExpansion_Test3",
		"MacroExpansion_Test4",
		"MacroExpansion_Test5",
		"MacroOptional_Test0",
		"MacroOptional_Test1",
		"MacroOptional_Test2",
		"MacroOptional_Test3",
		"MacroParamExpression_Test0",
		"MacroParamIdentifier_Test0",
		"MacroParamTypeName_Test0",
		"MacroRepeated_Test0",
		"MacroRepeated_Test1",
		"MacroRepeated_Test2",
		"MacroRepeated_Test3",
		"MacroRepeated_Test4",
		"MoveBeforeIf_Test0",
		"MoveBeforeLoop_Test0",
		"MoveClassWithParent_Test",
		"NameIsNotTypeName_ForFunctionTemplateParameter",
		"NameNotFound_ForFunctionTemplateParameter",
		"NonPureVirtualFunctionInInterface_Test0",
		"NomangleFunctionMustBeGlobal_Test0",
		"NomangleFunctionMustBeGlobal_Test1",
		"NomangleFunctionMustBeGlobal_Test2",
		"NomangleTest0",
		"NomangleTest1",
		"OkTest",
		"PassImmutableReferenceTwoTimes_Test0",
		"PreResolve_Test0",
		"PreResolve_Test1",
		//"PreResolve_Test2",
		"PreResolve_Test4",
		"PureDestructor_Test0",
		"RecursiveTemplateFunctionCall_Test0",
		"Redefinition_ForFunctionTemplateParameter",
		"ReferenceFieldAccess_Test0",
		"ReferencePollution_ForFunctionPointer_Test0",
		"ReferencesTagsField_Test0",
		"ReturnReferenceFromStruct_Test0",
		"ReturnReferenceTags_ForFunctionPointers_Test0",
		"SafeBlockResetsUnsafe_Test",
		"SimpliestTest",
		"SimplePassArgumentTest",
		"SimpleProgramTest",
		"SizeAndAlignmentFields_Test0",
		"StringLiteral_Test0",
		"StringLiteralElementIsCharByDefault_Test0",
		"StringLiteralEmpty_Test0",
		"StringLiteral_EscapeSequences_Test0",
		"StringLiteral_EscapeSequences_Test1",
		"StringLiteral_EscapeSequences_Test2",
		"StringLiteral_EscapeSequences_Test3",
		"StringLiteralIsArray_Test0",
		"StringLiteralIsConstantExpression_Test0",
		"StringLiteralIsConstantExpression_Test1",
		"StringLiteralIsConstantReference_Test0",
		"StringLiteralIsNotNullTerminated_Test0",
		"StringLiteral_UTF8_Test0",
		"StringLiteral_UTF16_Test0",
		"StringLiteral_UTF32_Test0",
		"TemplateParametersDeductionFailed_Test0",
		"TemplateParametersDeductionFailed_Test1",
		"TemplateParametersDeductionFailed_Test2",
		"TemplateParametersDeductionFailed_Test3",
		"TemplateParametersDeductionFailed_Test4",
		"TemplateParametersDeductionFailed_Test5",
		"TemplateParametersDeductionFailed_Test6",
		"TemplateParametersDeductionFailed_Test7",
		"TemplateParametersDeductionFailed_Test8",
		"TemplateParametersDeductionFailed_Test9",
		"TemplateParametersDeductionFailed_Test10",
		"TernaryOperator_SavesInnerReferences_Test0",
		"TernaryOperator_VariablesStateMerge_Test0",
		"TernaryOperatorParsing_Test0",
		"TernaryOperatorParsing_Test1",
		"TupleFor_Test0",
		"TupleFor_Test1",
		"TupleFor_Test2",
		"TupleFor_Test3",
		"TupleFor_Test4",
		"TupleMoveAsignment_Test0",
		"TuplesConstexpr_Test0",
		"TuplesConstexpr_Test1",
		"TuplesConstexpr_Test2",
		"TuplesConstexpr_Test3",
		"TupleReturnValue_Test0",
		"TupleReturnValue_Test1",
		"TupleReturnValue_Test2",
		"TypeAdditionalCommonFields_Test0",
		"TypeConversion_InReturn_Test0",
		"TypeConversion_InReturn_Test1",
		"TypeConversion_InReturn_Test2",
		"TypeConversion_InReturn_Test3",
		"TypeConversion_InReturn_Test4",
		"TypeinfoCalssIsSameForSameTypes_Test0",
		"TypeinfoForIncompleteType_Test0",
		"TypeinfoForIncompleteType_Test1",
		"TypeinfoForIncompleteTypeIsIncomplete_Test0",
		"TypeinfoForIncompleteTypeIsIncomplete_Test1",
		"TypeinfoForTemplateDependentType_Test0",
		"TypeinfoForTypeinfo_Test0",
		"TypeinfoForTypeinfo_Test1",
		"TypeinfoList_ClassFieldsList_Test0",
		"TypeinfoList_ClassFieldsList_Test1",
		"TypeinfoList_ClassTypesList_Test0",
		"TypeinfoList_FunctionTypeParams_Test0",
		"Typeinfo_ForTuples_Test0",
		"Typeinfo_ForTuples_Test1",
		"Typeinfo_ForTuples_Test2",
		"Typeinfo_SrcType_Test0",
		"TypeInfoOperator_Test0",
		"TypeKindFields_Test0",
		"TypeNameInErrorMessage_ClassTemplate_Test0",
		"TypeNameInErrorMessage_ClassTemplate_Test1",
		"TypeNameInErrorMessage_ClassTemplate_Test2",
		"TypeNameInErrorMessage_ClassTemplate_Test3",
		"TypeNameInErrorMessage_ClassTemplate_Test4",
		"TypeNameInErrorMessage_ClassTemplate_Test5",
		"TypeNameInErrorMessage_ClassTemplate_Test6",
		"TypeNameInErrorMessage_ClassTemplate_Test7",
		"TypeNameInErrorMessage_ClassTemplate_Test8",
		"TypeNameInErrorMessage_ClassTypeInGlobalNamespace",
		"TypeNameInErrorMessage_FundamentalTypes",
		"TypeofHasNoEffects_Test0",
		"Typeof_ChecksExpression_Test0",
		"Typeof_Test0",
		"Typeof_Test1",
		"Typeof_Test2",
		"Typeof_Test3",
		"Typeof_Test4",
		"Typeof_Test5",
		"Typeof_Test6",
		"TypesMismatch_ForEnableIf_Test0",
		"TypeTemplatesVisibilityMismatch_Test0",
		"UnsafeBlockDeclaration_Test0",
		"UnsafeInsideUnsafe_Test",
		"UsingIncompleteType_ForInheritance_Test0",
		"VirtualCallInDestructor_Test0",
		"VirtualCall_UsingDifferentVirtualTables_Test0",
		"VirtualForFunctionImplementation_Test2",
		"VirtualForNonclassFunction_Test0",
		"VirtualForNonpolymorphClass_Test0",
		"VirtualForNonThisCallFunction_Test0",
		"VirtualMismatch_Test0",
		"VirtualRequired_Test0",
		"VirtualTablePointerSave_InMove_Test0",
		"VisibilityForEnumMember_Test0",
		"VisibilityForStruct_Test0",
		"VisibilityForTypeTempateMember_Test0",
		"VoidTypeReferenceMustBeReturned_Test",
		"ZeroInitForStructIsConstexpr_Test0",
		"ZeroInitForStructIsConstexpr_Test1",
		"ZeroInitializerForChar_Test0",
		"ZeroInitializerForStructWithReferenceTest",
	};

	if( c_test_to_enable.count( func_name_str ) > 0 )
	{
		Py_INCREF(Py_True);
		return Py_True;
	}

	static const std::string c_tests_to_enable_pattern[]
	{
		"AbstractClassConstructor_Test",
		"AbstractClassDestructor_Test",
		"AccessingMovedVariable_InTupleFor_Test",
		"AccessingMovedVariable_Test",
		"AccessingPrivateMemberInsideClass_Test",
		"AccessingPrivateMemberOutsideClass_ViaMemberAccessOperator_Test",
		"AccessingPrivateMemberOutsideClass_Test",
		"AccessingProtectedMember_Test",
		"AccessingVariableThatHaveMutableReference_Test",
		"AutoConstexprFunctionTemplate_Test",
		"AutoVariableDeclaration_ForTuples",
		"BaseUnavailable_Test",
		"BodyForDeletedFunction_Test",
		"BodyForGeneratedFunction_Test",
		"BodyForPureVirtualFunction_Test",
		"CanNotDeriveFromThisType_Test",
		"CastToVoidReference_Test",
		"CharIsConstructibleFromChar_Test",
		"CharIsConstructibleFromInt_Test",
		"CharIsNotIntegerType_Test",
		"CharIsUnsigned_Test",
		"ChildClassNameOverridesParentClassNameAndVisibility_Test",
		"ChildToParentReferenceCast_Test",
		"ClassContainsPureVirtualFunctions_Test",
		"ClassHaveNoCopyConstructorByDefault_Test",
		"CommonValueType_Test",
		"ConditionalMove",
		"ConstexprCall_ResultTypeIs_",
		"ConstexprFunctionAccessGlobalVariable",
		"ConstexprFunctionArithmeticOperatorsTest",
		"ConstexprFunctionCallOtherFunction",
		"ConstexprFunctionCanNotBeVirtual_Test",
		"ConstexprFunctionContainsUnallowedOperations",
		"ConstexprFunctionControlFlow",
		"ConstexprFunctionEvaluationError",
		"ConstexprFunctionInternalArray",
		"ConstexprFunctionInternalStruct",
		"ConstexprFunctionsMustHaveBody",
		"ConstexprFunctionWithMutableArguments",
		"ConstexprFunction_CompositeArgument",
		"ConstexprFunction_RecursiveCall",
		"ConstexprFunction_ReturnStruct",
		"ConstexprFunction_ReturningReference",
		"ConstexprReferenceInsideStruct_Test",
		"ConstexprStructDeclaration",
		"ConstexprStructGeneratedMethodsAreConstexpr",
		"ConstexprStructMemberIsConstexpr_Test",
		"ConversionConstructorIsConstructor_Test",
		"ConversionConstructorMustHaveOneArgument_Test",
		"CopyChildToParent_Test",
		"CouldNotOverloadFunction_ForUnsafe_Test",
		"CouldNotOverloadFunctionIfNomangle_Test",
		"CreateMutableReferenceToAnotherMutableReference_Test",
		"CreateMutableReferenceToVariableWithImmutableReference_Test",
		"CStyleForOperator",
		"DeepExpressionsCompilationTest",
		"DeepTypeConversionDisabled_Test",
		"DestroyedVariableStillHaveReference",
		"Desturctors_ForInheritance_Test",
		"DifferentFunctionImplementations_UsingEnableIf",
		"DirectFunctionTemplateParametersSet_Test",
		"DisableDefaultConstructor_Test",
		"DiabledFunctionContentNotCompiled_Test",
		"DisabledTemplateFunction_Test",
		"DuplicatedParentClass_Test",
		"EnableIfDeclaration_Test",
		"EnableIf_ForPrototypeAndBody_Test",
		"ExpandMacroInArgumentOfOtherMacro_Test",
		"ExpandMacroWhileExpandingMacro_Test",
		"ExpectedReferenceValue_ForMove_Test",
		"ExpectedVariable_ForMove_Test",
		"ExplicitAccessToSpecialMethodsIsUnsafe_Test",
		"FalseBranchesSkipped_Test",
		"FieldInitializerDeclaration_Test",
		"FieldsSort_Test",
		"FunctionPointerAsFunctionArgument_Test",
		"FunctionPointerAsReturnValue_Test0",
		"FunctionPointerAsSpecializedTemplateParameter_Test",
		"FunctionPointerCall_Test",
		"FunctionPointerEqualityComparision_Test",
		"FunctionPoinerInitialization_Test",
		"FunctionPointerReferencesIsNotCompatible_Test",
		"FunctionsPoitersAssignment_Test",
		"FunctionPointerToTemplateFunction_Test",
		"FunctionReferencePass_Test",
		"FunctionTemplateBase_Test",
		"FunctionTemplateDeclaration_Test",
		"InClassFieldInitializer_EvaluatesInClassScope_Test",
		"InClassFieldInitializer_ForReferenceField",
		"InClassFieldInitializer_InConstructorInitializer",
		"InClassFieldInitializer_InDefaultConstructor",
		"InClassFieldInitializer_InStructNamedInitializer_Test",
		"InClassFieldInitializer_MayBeConstexpr_Test",
		"InClassFieldInitializer_OtherFieldCanNotBeUsed_Test",
		"InheritanceTest",
		"InvalidFunctionArgumentCount_Test",
		"InvalidMethodForBodyGeneration_Test",
		"InvalidSizeForCharLiteral_Test",
		"InvalidTypeForConstantExpressionVariable",
		"InvalidTypeForConstexprFunction_Test",
		"KeepFieldsOrder_Test",
		"MacroDefenition_Test",
		"MacroErrorsTest",
		"MethodBodyGenerationFailed_Test",
		"Move_InLazyLogicalOperator_Test",
		"MoveClassWithParent_Test",
		"MovedVariableHaveReferences_Test",
		"MoveInsideIf_Test",
		"MoveInsideLoop_Test",
		"MoveOperatorDeclaration_Test",
		"MoveOperatorTest",
		"NonExistentTest",
		"NoMangleMismatch_Test",
		"NumericConstants",
		"OrderIndependent",
		"OverloadingResolutionTest_ForFunctionTemplates_Test",
		"OverloadingResolutionTest_MutabilityAndReferenceConversions",
		"OverloadingResolutionTest_OnlyMutabilityCheck",
		"OverloadingResolutionTest_ParentClassCall_Test",
		"OverloadingResolutionTest_ReferenceConversions_Test",
		"OverloadingResolutionTest_StaticClassFunctions",
		"OverloadingResolutionTest_TypeConversions_Test",
		"Override",
		"PassMutableReferenceTwoTimes_Tes",
		"PointerCastForVirtualCall_Test",
		"PollutionTest",
		"PrivateMembersNotInherited_Test",
		"ReferenceInsideStruct_Test",
		"ReferencesLoop_Test",
		"ReferenceTagForTypeWithoutReferencesInside",
		"ReferenceTagsForTemplateDependentType_Test",
		"Specialization_Test",
		"StartBlockLexemAsOptionalIndicator_Test",
		"StartBlockLexemAsRepeatedIndicator_Test",
		"StaticIf",
		"StringLiteral_CharNumber_Test",
		"StringLiteralSuffix_Test",
		"SyntaxError_Test",
		"TakeForConstReference_Test",
		"TakeForPart_Test",
		"TakeForValueVariable_Test",
		"TakenVariableHaveReferences_Test",
		"TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test",
		"TemplateDependentFunctionTemplateArguments_Test",
		"TemplateFunctionGenerationFailed_Test",
		"TemplateFunction_versus_TypesConversion_Test",
		"TemplateMethod_Test",
		"TemplateOperator_Test",
		"TernaryOperatorIsLazy_Test",
		"TernaryOperator_Constexpr_Test",
		"TernaryOperator_ForReferenceValue_Test",
		"TernaryOperator_ReferenceProtectionError",
		"TernaryOperator_Test",
		"TernaryOperator_TypesMismatch",
		"TupleCopyAssignment_Test",
		"TupleElementAccess",
		"TupleFieldCopy",
		"TupleFunctionArgument_Test",
		"TupleTemplateArgument",
		"TupleTypeParsing",
		"TypeConversion_InExpressionInitializer_Test",
		"TypeConversion_InFunctionCall_Test",
		"TypeinfoFieldsDependsOnTypeKind",
		"TypeinfoList_ClassFunctionsList_Test",
		"TypeinfoList_ClassParentsList_Test",
		"TypeinfoList_EnumList_Test",
		"TypeNameInErrorMessage_ClassTypeInNamespace_Test",
		"TypeofOperatorDeclaration",
		"TypesMismatch_InFunctionPointerCall_Test",
		"TypeTemplatesOvelroading_MustSelectSpecializedTemplate_Test",
		"TypeTemplatesOvelroading_SpecializationErrors_Test",
		"TypeTemplatesOvelroading_Specialization_Test",
		"UninitializedInitializer",
		"UniqueMacroLexem_Test",
		"UnknownStringLiteralSuffix_Test",
		"UnsafeFunctionCallInsideUnsafeBlock_Test",
		"UnsafeFunctionCallOutsideUnsafeBlock_Test",
		"UnsafeFunctionDeclaration_Test",
		"ValueIsNotTemplate_ForFunctionTemplates_Test",
		"VariableInitializerIsNotConstantExpression_ForStructs_Test",
		"VariablesStateMerge_ForTernaryOperator_Test",
		"VariativeReferenceTagsCount_InTemplateClass_Test",
		"VirtualCallInConstructor_Test",
		"VirtualDestructor_Test",
		"VirtualFunctionCallTest",
		"VirtualFunctionDeclaration_Test",
		"VirtualOperatorCall_Test",
		"VisibilityLabelDeclaration_Test",
		"VoidTypeIsIncomplete_Test",
		"VoidTypeReference_Test",
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
