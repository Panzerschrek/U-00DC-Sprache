#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"
#include "code_builder_types.hpp"

#include "code_builder_errors.hpp"

namespace U
{

bool operator==( const CodeBuilderError& l, const CodeBuilderError& r )
{
	return l.code == r.code && l.file_pos == r.file_pos && l.text == r.text;
}

bool operator!=( const CodeBuilderError& l, const CodeBuilderError& r )
{
	return !(l == r);
}

bool operator< ( const CodeBuilderError& l, const CodeBuilderError& r )
{
	return l.file_pos < r.file_pos;
}

const char* CodeBuilderErrorCodeToString( const CodeBuilderErrorCode code )
{
	switch(code)
	{
	#define PROCESS_ERROR(X) case CodeBuilderErrorCode::X: return #X;
	#include "errors_list.hpp"
	#undef PROCESS_ERROR
	};

	U_ASSERT(false);
	return "";
}

CodeBuilderError ReportBuildFailed()
{
	CodeBuilderError error;
	error.file_pos.line= 1u;
	error.file_pos.pos_in_line= 0u;
	error.code = CodeBuilderErrorCode::BuildFailed;

	error.text= "Build failed."_SpC;

	return error;
}

CodeBuilderError ReportNameNotFound( const FilePos& file_pos, const ProgramString& name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::NameNotFound;

	error.text= name + " was not declarated in this scope"_SpC;

	return error;
}

CodeBuilderError ReportNameNotFound( const FilePos& file_pos, const Synt::ComplexName& name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::NameNotFound;

	for( const Synt::ComplexName::Component& component : name.components )
	{
		error.text+= component.name;
		if( &component != &name.components.back() )
			error.text+= "::"_SpC;
	}
	error.text+= " was not declarated in this scope"_SpC;

	return error;
}

CodeBuilderError ReportUsingKeywordAsName( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::UsingKeywordAsName;

	error.text= "Using keyword as name."_SpC;

	return error;
}

CodeBuilderError ReportRedefinition( const FilePos& file_pos, const ProgramString& name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::Redefinition;

	error.text= name + " redefinition."_SpC;

	return error;
}

CodeBuilderError ReportUnknownNumericConstantType( const FilePos& file_pos, const ProgramString& unknown_type )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::UnknownNumericConstantType;

	error.text= "Unknown numeric constant type - "_SpC + unknown_type + "."_SpC;

	return error;
}

CodeBuilderError ReportUnknownStringLiteralSuffix( const FilePos& file_pos, const ProgramString& unknown_suffix )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::UnknownStringLiteralSuffix;

	error.text= "Unknown string literal suffix - "_SpC + unknown_suffix + "."_SpC;

	return error;
}

CodeBuilderError ReportInvalidSizeForCharLiteral( const FilePos& file_pos, const ProgramString& literal )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::InvalidSizeForCharLiteral;

	error.text= "Invalid char literal - \""_SpC + literal + "\" expected literal with exactly one symbol."_SpC;

	return error;
}

CodeBuilderError ReportOperationNotSupportedForThisType( const FilePos& file_pos, const ProgramString& type_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::OperationNotSupportedForThisType;

	// TODO - pirnt, what operation.
	error.text=
		"Operation is not supported for type \""_SpC +
		type_name +
		"\"."_SpC;

	return error;
}

CodeBuilderError ReportTypesMismatch(
	const FilePos& file_pos,
	const ProgramString& expected_type_name,
	const ProgramString& actual_type_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::TypesMismatch;

	error.text=
		"Unexpected type, expected \""_SpC + expected_type_name +
		"\", got \""_SpC + actual_type_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportNoMatchBinaryOperatorForGivenTypes(
	const FilePos& file_pos,
	const ProgramString& type_l_name, const ProgramString& type_r_name,
	const ProgramString& binary_operator )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::NoMatchBinaryOperatorForGivenTypes;

	error.text=
		"No match operator \""_SpC + binary_operator + "\" for types \""_SpC +
		type_l_name + "\" and \""_SpC + type_r_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportNotImplemented( const FilePos& file_pos, const char* const what )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::NotImplemented;

	error.text= "Sorry, "_SpC + ToProgramString( what ) + " not implemented."_SpC;

	return error;
}

CodeBuilderError ReportArraySizeIsNegative( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ArraySizeIsNegative;

	error.text= "Array size is neagative."_SpC;

	return error;
}

CodeBuilderError ReportArraySizeIsNotInteger( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ArraySizeIsNotInteger;

	error.text= "Array size is not integer."_SpC;

	return error;
}

CodeBuilderError ReportBreakOutsideLoop( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::BreakOutsideLoop;

	error.text= "Break outside loop."_SpC;

	return error;
}

CodeBuilderError ReportContinueOutsideLoop( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ContinueOutsideLoop;

	error.text= "Continue outside loop."_SpC;

	return error;
}

CodeBuilderError ReportNameIsNotTypeName( const FilePos& file_pos, const ProgramString& name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::NameIsNotTypeName;

	error.text= "\""_SpC + name + "\" is not type name."_SpC;

	return error;
}

CodeBuilderError ReportUnreachableCode( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::UnreachableCode;

	error.text= "Unreachable code."_SpC;

	return error;
}

CodeBuilderError ReportNoReturnInFunctionReturningNonVoid( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::NoReturnInFunctionReturningNonVoid;

	error.text= "No return in function returning non-void."_SpC;

	return error;
}

CodeBuilderError ReportExpectedInitializer( const FilePos& file_pos, const ProgramString& variable_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExpectedInitializer;

	error.text= "Expected initializer or constructor for \""_SpC + variable_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportExpectedReferenceValue( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExpectedReferenceValue;

	error.text= "Expected reference value."_SpC;

	return error;
}

CodeBuilderError ReportBindingConstReferenceToNonconstReference( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::BindingConstReferenceToNonconstReference;

	error.text= "Binding constant reference to non-constant reference."_SpC;

	return error;
}

CodeBuilderError ReportExpectedVariable( const FilePos& file_pos, const ProgramString& got )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExpectedVariable;

	error.text= "Expected variable, got \"."_SpC + got + "\"."_SpC;

	return error;
}

CodeBuilderError ReportInvalidFunctionArgumentCount( const FilePos& file_pos, size_t given_arg_count, size_t required_arg_count )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::InvalidFunctionArgumentCount;

	error.text= "Invalid function argument count. Required "_SpC +
		ToProgramString(std::to_string(required_arg_count).c_str()) +
		", got "_SpC + ToProgramString(std::to_string(given_arg_count).c_str()) + "."_SpC;

	return error;
}

CodeBuilderError ReportCouldNotOverloadFunction( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::CouldNotOverloadFunction;

	error.text= "Could not overload function."_SpC;

	return error;
}

CodeBuilderError ReportTooManySuitableOverloadedFunctions( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::TooManySuitableOverloadedFunctions;

	error.text= "Could not select function for overloading - too many candidates."_SpC;

	return error;
}

CodeBuilderError ReportCouldNotSelectOverloadedFunction( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::CouldNotSelectOverloadedFunction;

	error.text= "Could not select function for overloading - no candidates."_SpC;

	return error;
}

CodeBuilderError ReportFunctionPrototypeDuplication( const FilePos& file_pos, const ProgramString& func_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::FunctionPrototypeDuplication;

	error.text= "Duplicated prototype of function \""_SpC + func_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportFunctionBodyDuplication( const FilePos& file_pos, const ProgramString& func_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::FunctionBodyDuplication;

	error.text= "Body fo function \""_SpC + func_name + "\" already exists."_SpC;

	return error;
}

CodeBuilderError ReportBodyForGeneratedFunction( const FilePos& file_pos, const ProgramString& func_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::BodyForGeneratedFunction;

	error.text= "Body for generated function \""_SpC + func_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportBodyForDeletedFunction( const FilePos& file_pos, const ProgramString& func_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::BodyForDeletedFunction;

	error.text= "Body for deleted function \""_SpC + func_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportFunctionDeclarationOutsideItsScope( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::FunctionDeclarationOutsideItsScope;

	error.text= "Function declaration outside its scope."_SpC;

	return error;
}

CodeBuilderError ReportClassDeclarationOutsideItsScope( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ClassDeclarationOutsideItsScope;

	error.text= "Class declaration outside its scope."_SpC;

	return error;
}

CodeBuilderError ReportClassBodyDuplication( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ClassBodyDuplication;

	error.text= "Class body duplication."_SpC;

	return error;
}

CodeBuilderError ReportUsingIncompleteType( const FilePos& file_pos, const ProgramString& type_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::UsingIncompleteType;

	error.text= "Using incomplete type \""_SpC + type_name + "\", expected complete type."_SpC;

	return error;
}

CodeBuilderError ReportGlobalsLoopDetected( const FilePos& file_pos, const ProgramString& loop_description )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::GlobalsLoopDetected;

	error.text= "Globals loop detected:\n"_SpC + loop_description;

	return error;
}

CodeBuilderError ReportAccessingNonpublicClassMember( const FilePos& file_pos, const ProgramString& class_name, const ProgramString& member_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::AccessingNonpublicClassMember;

	error.text= "Accessing member \""_SpC + member_name + "\" of class \""_SpC + class_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportFunctionsVisibilityMismatch( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::FunctionsVisibilityMismatch;

	error.text= "Visibility mismatch for function \""_SpC + function_name + "\". All functions with same name in class must have same visibility."_SpC;

	return error;
}

CodeBuilderError ReportTypeTemplatesVisibilityMismatch( const FilePos& file_pos, const ProgramString& type_template_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::TypeTemplatesVisibilityMismatch;

	error.text= "Visibility mismatch for type template \""_SpC + type_template_name + "\". All type templates with same name in class must have same visibility."_SpC;

	return error;
}

CodeBuilderError ReportVisibilityForStruct( const FilePos& file_pos, const ProgramString& struct_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::VisibilityForStruct;

	error.text= "Visibility label for struct \""_SpC + struct_name + "\". Visibility labels enabled only for classes, structs have all members public."_SpC;

	return error;
}

CodeBuilderError ReportExpectedConstantExpression( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExpectedConstantExpression;

	error.text= "Expected constant expression."_SpC;

	return error;
}

CodeBuilderError ReportVariableInitializerIsNotConstantExpression( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::VariableInitializerIsNotConstantExpression;

	error.text= "Variable declaration is not constant expression."_SpC;

	return error;
}

CodeBuilderError ReportInvalidTypeForConstantExpressionVariable( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::InvalidTypeForConstantExpressionVariable;

	error.text= "Invalid type for constant expression variable."_SpC;

	return error;
}

CodeBuilderError ReportConstantExpressionResultIsUndefined( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ConstantExpressionResultIsUndefined;

	error.text= "Constant expression result is undefined."_SpC;

	return error;
}

CodeBuilderError ReportConstexprFunctionEvaluationError( const FilePos& file_pos, const char* const what_error )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ConstexprFunctionEvaluationError;

	error.text= "Constexpr function evaluation error: "_SpC + ToProgramString( what_error ) + "."_SpC;

	return error;
}

CodeBuilderError ReportConstexprFunctionContainsUnallowedOperations( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ConstexprFunctionContainsUnallowedOperations;

	error.text= "Constexpr function contains unallowed operatios."_SpC;

	return error;
}

CodeBuilderError ReportInvalidTypeForConstexprFunction( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::InvalidTypeForConstexprFunction;

	error.text= "Invalid type for constexpr function."_SpC;

	return error;
}

CodeBuilderError ReportConstexprFunctionsMustHaveBody( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ConstexprFunctionsMustHaveBody;

	error.text= "Constexpr function must have body."_SpC;

	return error;
}

CodeBuilderError ReportConstexprFunctionCanNotBeVirtual( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ConstexprFunctionCanNotBeVirtual;

	error.text= "Constexpr function can not be virtual."_SpC;

	return error;
}

CodeBuilderError ReportStaticAssertExpressionMustHaveBoolType( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::StaticAssertExpressionMustHaveBoolType;

	error.text= "static_assert expression must have bool type."_SpC;

	return error;
}

CodeBuilderError ReportStaticAssertExpressionIsNotConstant( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::StaticAssertExpressionIsNotConstant;

	error.text= "Expression in static_assert is non constant."_SpC;

	return error;
}

CodeBuilderError ReportStaticAssertionFailed( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::StaticAssertionFailed;

	error.text= "Static assertion failed."_SpC;

	return error;
}

CodeBuilderError ReportArrayIndexOutOfBounds( const FilePos& file_pos, const SizeType index, const SizeType array_size )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ArrayIndexOutOfBounds;

	error.text=
		"Array inex out of bounds. Index is \""_SpC + ToProgramString(std::to_string(index).c_str()) +
		"\", but aray constains only \""_SpC + ToProgramString(std::to_string(array_size).c_str()) + "\" elements."_SpC;

	return error;
}

CodeBuilderError ReportArrayInitializerForNonArray( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ArrayInitializerForNonArray;

	error.text= "Array initializer for nonarray."_SpC;

	return error;
}

CodeBuilderError ReportArrayInitializersCountMismatch(
	const FilePos& file_pos,
	const SizeType expected_initializers,
	const SizeType real_initializers )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ArrayInitializersCountMismatch;

	error.text=
		"Array initializers count mismatch. Expected "_SpC +
		ToProgramString( std::to_string(expected_initializers).c_str() ) + ", got "_SpC +
		ToProgramString( std::to_string(real_initializers).c_str() ) + "."_SpC;

	return error;
}

CodeBuilderError ReportFundamentalTypesHaveConstructorsWithExactlyOneParameter( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::FundamentalTypesHaveConstructorsWithExactlyOneParameter;

	error.text= "Fundamental types have constructors with exactly one parameter."_SpC;

	return error;
}

CodeBuilderError ReportReferencesHaveConstructorsWithExactlyOneParameter( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ReferencesHaveConstructorsWithExactlyOneParameter;

	error.text= "References have constructors with exactly one parameter."_SpC;

	return error;
}

CodeBuilderError ReportUnsupportedInitializerForReference( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::UnsupportedInitializerForReference;

	error.text= "Unsupported initializer for reference."_SpC;

	return error;
}

CodeBuilderError ReportConstructorInitializerForUnsupportedType( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ConstructorInitializerForUnsupportedType;

	error.text= "Constructor initializer for unsupported type."_SpC;

	return error;
}

CodeBuilderError ReportZeroInitializerForClass( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ZeroInitializerForClass;

	error.text= "zero initializer for class."_SpC;

	return error;
}

CodeBuilderError ReportStructInitializerForNonStruct( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::StructInitializerForNonStruct;

	error.text= "Structure-initializer for nonstruct."_SpC;

	return error;
}

CodeBuilderError ReportInitializerForNonfieldStructMember(
	const FilePos& file_pos,
	const ProgramString& member_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::InitializerForNonfieldStructMember;

	error.text= "Initializer for \"."_SpC + member_name + "\" which is not a field."_SpC;

	return error;
}

CodeBuilderError ReportInitializerForBaseClassField( const FilePos& file_pos, const ProgramString& field_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::InitializerForBaseClassField;

	error.text= "Initializer for \"."_SpC + field_name + "\" which is not this class field."_SpC;

	return error;
}

CodeBuilderError ReportDuplicatedStructMemberInitializer( const FilePos& file_pos, const ProgramString& member_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::DuplicatedStructMemberInitializer;

	error.text= "Duplicated initializer for"_SpC + member_name + "."_SpC;

	return error;
}

CodeBuilderError ReportInitializerDisabledBecauseClassHaveExplicitNoncopyConstructors( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors;

	error.text= "This kind of initializer disabled for this class, because it have explicit noncopy constructor(s)."_SpC;

	return error;
}

CodeBuilderError ReportInvalidTypeForAutoVariable( const FilePos& file_pos, const ProgramString& type_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::InvalidTypeForAutoVariable;

	error.text= "Invalid type for auto variable: "_SpC + type_name + "."_SpC;

	return error;
}

CodeBuilderError ReportGlobalVariableMustBeConstexpr( const FilePos& file_pos, const ProgramString& variable_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::GlobalVariableMustBeConstexpr;

	error.text= "Global variable \""_SpC + variable_name + "\" must be constexpr."_SpC;

	return error;
}

CodeBuilderError ReportConstructorOrDestructorOutsideClass( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ConstructorOrDestructorOutsideClass;

	error.text= "Constructor or destructor outside class."_SpC;

	return error;
}

CodeBuilderError ReportConstructorAndDestructorMustReturnVoid( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ConstructorAndDestructorMustReturnVoid;

	error.text= "Constructors and destructors must return void."_SpC;

	return error;
}

CodeBuilderError ReportConversionConstructorMustHaveOneArgument( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ConversionConstructorMustHaveOneArgument;

	error.text= "Conversion constructor must have exactly 1 argument (except \"this\")."_SpC;

	return error;
}

CodeBuilderError ReportInitializationListInNonconstructor( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::InitializationListInNonconstructor;

	error.text= "Constructor outside class."_SpC;

	return error;
}

CodeBuilderError ReportClassHaveNoConstructors( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ClassHaveNoConstructors;

	error.text= "Class have no constructors."_SpC;

	return error;
}

CodeBuilderError ReportExplicitThisInDestructor( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExplicitThisInDestructor;

	error.text= "Explicit \"this\" in destructor parameters."_SpC;

	return error;
}

CodeBuilderError ReportFieldIsNotInitializedYet( const FilePos& file_pos, const ProgramString& field_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::FieldIsNotInitializedYet;

	error.text= "Field \""_SpC + field_name + "\" in not initialized yet."_SpC;

	return error;
}

CodeBuilderError ReportExplicitArgumentsInDestructor( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExplicitArgumentsInDestructor;

	error.text= "Explicit arguments in destructor."_SpC;

	return error;
}

CodeBuilderError ReportCallOfThiscallFunctionUsingNonthisArgument( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::CallOfThiscallFunctionUsingNonthisArgument;

	error.text= "Call of \"thiscall\" function using nonthis argument."_SpC;

	return error;
}

CodeBuilderError ReportClassFiledAccessInStaticMethod( const FilePos& file_pos, const ProgramString& field_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ClassFiledAccessInStaticMethod;

	error.text= "Accessing field \""_SpC + field_name + "\" in static method."_SpC;

	return error;
}

CodeBuilderError ReportThisInNonclassFunction( const FilePos& file_pos, const ProgramString& func_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ThisInNonclassFunction;

	error.text= "This in nonclass function \""_SpC + func_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportThiscallMismatch( const FilePos& file_pos, const ProgramString& func_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ThiscallMismatch;

	error.text= "Thiscall for function \""_SpC + func_name + "\" does not match to thiscall in prototype."_SpC;

	return error;
}

CodeBuilderError ReportAccessOfNonThisClassField( const FilePos& file_pos, const ProgramString& field_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::AccessOfNonThisClassField;

	error.text= "Access filed \""_SpC + field_name + "\" of non-this class."_SpC;

	return error;
}

CodeBuilderError ReportThisUnavailable( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ThisUnavailable;

	error.text= "\"this\" unavailable."_SpC;

	return error;
}

CodeBuilderError ReportBaseUnavailable( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::BaseUnavailable;

	error.text= "\"base\" unavailable."_SpC;

	return error;
}

CodeBuilderError ReportInvalidMethodForBodyGeneration( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::InvalidMethodForBodyGeneration;

	error.text= "Invalid method for body generation."_SpC;

	return error;
}

CodeBuilderError ReportMethodBodyGenerationFailed( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::MethodBodyGenerationFailed;

	error.text= "Method body generation failed."_SpC;

	return error;
}

CodeBuilderError ReportAccessingDeletedMethod( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::AccessingDeletedMethod;

	error.text= "Accessing deleted method."_SpC;

	return error;
}

CodeBuilderError ReportInvalidValueAsTemplateArgument( const FilePos& file_pos, const ProgramString& got )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::InvalidValueAsTemplateArgument;

	error.text= "Invalid value as template argument. Expected variable of type, got \""_SpC + got + "\"."_SpC;

	return error;
}

CodeBuilderError ReportInvalidTypeOfTemplateVariableArgument( const FilePos& file_pos, const ProgramString& type_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::InvalidTypeOfTemplateVariableArgument;

	error.text= "Invalid type for template variable-argument: \""_SpC + type_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportTemplateParametersDeductionFailed( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::TemplateParametersDeductionFailed;

	error.text= "Template parameters deduction failed."_SpC;

	return error;
}

CodeBuilderError ReportDeclarationShadowsTemplateArgument( const FilePos& file_pos, const ProgramString& name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::DeclarationShadowsTemplateArgument;

	error.text= "Declaration of \""_SpC + name + "\" shadows template argument."_SpC;

	return error;
}

CodeBuilderError ReportValueIsNotTemplate( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ValueIsNotTemplate;

	error.text= "Value is not template."_SpC;

	return error;
}

CodeBuilderError ReportTemplateInstantiationRequired( const FilePos& file_pos, const ProgramString& template_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::TemplateInstantiationRequired;

	error.text= "\""_SpC + template_name + "\" template instantiation required."_SpC;

	return error;
}

CodeBuilderError ReportMandatoryTemplateSignatureArgumentAfterOptionalArgument( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::MandatoryTemplateSignatureArgumentAfterOptionalArgument;

	error.text= "Mandatory template signature argument after optional argument."_SpC;

	return error;
}

CodeBuilderError ReportTemplateArgumentIsNotDeducedYet( const FilePos& file_pos, const ProgramString& name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::TemplateArgumentIsNotDeducedYet;

	error.text= name + " is not deduced yet."_SpC;

	return error;
}

CodeBuilderError ReportTemplateArgumentNotUsedInSignature( const FilePos& file_pos, const ProgramString& name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::TemplateArgumentNotUsedInSignature;

	error.text= "Template argument \""_SpC + name + "\" not used in signature."_SpC;

	return error;
}

CodeBuilderError ReportIncompleteMemberOfClassTemplate( const FilePos& file_pos, const ProgramString& name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::IncompleteMemberOfClassTemplate;

	error.text= "\""_SpC + name + "\" is incomplete."_SpC;

	return error;
}

CodeBuilderError ReportTemplateFunctionGenerationFailed( const FilePos& file_pos, const ProgramString& function_template_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::TemplateFunctionGenerationFailed;

	error.text= "Instantiation of function template \""_SpC + function_template_name + "\" failed."_SpC;

	return error;
}

CodeBuilderError ReportCouldNotSelectMoreSpicializedTypeTemplate( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::CouldNotSelectMoreSpicializedTypeTemplate;

	error.text= "Could not select more spicialized type template."_SpC;

	return error;
}

CodeBuilderError ReportReferenceProtectionError( const FilePos& file_pos, const ProgramString& var_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ReferenceProtectionError;

	error.text= "Reference protection check for variable \""_SpC + var_name + "\" failed."_SpC;

	return error;
}

CodeBuilderError ReportDestroyedVariableStillHaveReferences( const FilePos& file_pos, const ProgramString& var_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::DestroyedVariableStillHaveReferences;

	error.text= "Destroyed variable \""_SpC + var_name + "\" still have reference(s)."_SpC;

	return error;
}

CodeBuilderError ReportAccessingMovedVariable( const FilePos& file_pos, const ProgramString& var_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::AccessingMovedVariable;

	error.text= "Accessing moved variable \""_SpC + var_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportReturningUnallowedReference( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ReturningUnallowedReference;

	error.text= "Returning unallowed reference."_SpC;

	return error;
}

CodeBuilderError ReportInvalidReferenceTagCount( const FilePos& file_pos, const size_t given, const size_t required )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::InvalidReferenceTagCount;

	error.text= "Invalid reference tag count, expected "_SpC +
		ToProgramString(std::to_string(required).c_str()) + ", given "_SpC +
		ToProgramString(std::to_string(given).c_str()) + "."_SpC;

	return error;
}

CodeBuilderError ReportSelfReferencePollution( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::SelfReferencePollution;

	error.text= "Reference self-pollution."_SpC;

	return error;
}

CodeBuilderError ReportArgReferencePollution( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ArgReferencePollution;

	error.text= "Pollution of arg reference."_SpC;

	return error;
}

CodeBuilderError ReportMutableReferencePollutionOfOuterLoopVariable( const FilePos& file_pos, const ProgramString& dst_name, const ProgramString& src_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::MutableReferencePollutionOfOuterLoopVariable;

	error.text= "Mutable reference pollution for outer variables inside loop. \""_SpC + dst_name + "\" polluted by \""_SpC + src_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportOuterVariableMoveInsideLoop( const FilePos& file_pos, const ProgramString& variable_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::OuterVariableMoveInsideLoop;

	error.text= "Outer loop variable \""_SpC + variable_name + "\" move inside loop."_SpC;

	return error;
}

CodeBuilderError ReportConditionalMove( const FilePos& file_pos, const ProgramString& variable_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ConditionalMove;

	error.text= "Variable \""_SpC + variable_name + "\" moved not in all if-else bracnes."_SpC;

	return error;
}

CodeBuilderError ReportMovedVariableHaveReferences( const FilePos& file_pos, const ProgramString& variable_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::MovedVariableHaveReferences;

	error.text= "Moved variable \""_SpC + variable_name + "\" have references."_SpC;

	return error;
}

CodeBuilderError ReportUnallowedReferencePollution( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::UnallowedReferencePollution;

	error.text= "Unallowed reference pollution."_SpC;

	return error;
}

CodeBuilderError ReportReferencePollutionForArgReference( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ReferencePollutionForArgReference;

	error.text= "Pollution of inner reference of argument."_SpC;

	return error;
}

CodeBuilderError ReportExplicitReferencePollutionForCopyConstructor( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExplicitReferencePollutionForCopyConstructor;

	error.text= "Explicit reference pollution for copy constructor. Reference pollution for copy constructors generated automatically."_SpC;

	return error;
}

CodeBuilderError ReportExplicitReferencePollutionForCopyAssignmentOperator( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExplicitReferencePollutionForCopyAssignmentOperator;

	error.text= "Explicit reference pollution for copy assignment operator. Reference pollution for copy assignment operators generated automatically."_SpC;

	return error;
}

CodeBuilderError ReportReferenceFiledOfTypeWithReferencesInside( const FilePos& file_pos, const ProgramString& field_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ReferenceFiledOfTypeWithReferencesInside;

	error.text= "Reference filed \""_SpC + field_name + "\" have type, with other references inside."_SpC;

	return error;
}

CodeBuilderError ReportOperatorDeclarationOutsideClass( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::OperatorDeclarationOutsideClass;

	error.text= "Operator declaration outside class. Operators can be declared only inside classes."_SpC;

	return error;
}

CodeBuilderError ReportOperatorDoesNotHaveParentClassArguments( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::OperatorDoesNotHaveParentClassArguments;

	error.text= "Operator does not have parent class arguments. At least one argument of operator must have parent class type."_SpC;

	return error;
}

CodeBuilderError ReportInvalidArgumentCountForOperator( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::InvalidArgumentCountForOperator;

	error.text= "Invalid argument count for operator."_SpC;

	return error;
}

CodeBuilderError ReportInvalidReturnTypeForOperator( const FilePos& file_pos, const ProgramString& expected_type_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::InvalidReturnTypeForOperator;

	error.text= "Invalid return type for operator, expected \""_SpC + expected_type_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportUnderlayingTypeForEnumIsTooSmall( const FilePos& file_pos, const SizeType max_value, const SizeType max_value_of_underlaying_type )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::UnderlayingTypeForEnumIsTooSmall;

	error.text= "Underlaying type for enum is too small - enum max value is "_SpC
		+ ToProgramString( std::to_string(max_value).c_str() ) + " but type max value is "_SpC +
		ToProgramString( std::to_string(max_value_of_underlaying_type).c_str() ) + "."_SpC;

	return error;
}

CodeBuilderError ReportCanNotDeriveFromThisType( const FilePos& file_pos, const ProgramString& type_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::CanNotDeriveFromThisType;

	error.text= "Can not derive from \""_SpC + type_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportDuplicatedParentClass( const FilePos& file_pos, const ProgramString& type_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::DuplicatedParentClass;

	error.text= "Parent class \""_SpC + type_name + "\" is duplicated."_SpC;

	return error;
}

CodeBuilderError ReportDuplicatedBaseClass( const FilePos& file_pos, const ProgramString& type_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::DuplicatedBaseClass;

	error.text= "Can not inherit from \""_SpC + type_name + "\" because class already have base."_SpC;

	return error;
}

CodeBuilderError ReportFieldsForInterfacesNotAllowed( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::FieldsForInterfacesNotAllowed;

	error.text= "Fields for interfaces not allowed."_SpC;

	return error;
}

CodeBuilderError ReportBaseClassForInterface( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::BaseClassForInterface;

	error.text= "Base class for interface."_SpC;

	return error;
}

CodeBuilderError ReportConstructorForInterface( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ConstructorForInterface;

	error.text= "Constructor for interface."_SpC;

	return error;
}

CodeBuilderError ReportExpectedBodyForAutoFunction( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExpectedBodyForAutoFunction;

	error.text= "Expected body for function \""_SpC + function_name + "\", because return type declared as \"auto\"."_SpC;

	return error;
}

CodeBuilderError ReportAutoFunctionInsideClassesNotAllowed( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::AutoFunctionInsideClassesNotAllowed;

	error.text= "\"auto\" for member function \""_SpC + function_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportVirtualForNonclassFunction( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::VirtualForNonclassFunction;

	error.text= "Virtual for non-class function \"."_SpC + function_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportVirtualForNonThisCallFunction( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::VirtualForNonThisCallFunction;

	error.text= "Virtual for non-thiscall function \"."_SpC + function_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportFunctionCanNotBeVirtual( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::FunctionCanNotBeVirtual;

	error.text= "Function \"."_SpC + function_name + "\" can not be virtual."_SpC;

	return error;
}

CodeBuilderError ReportVirtualForNonpolymorphClass( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::VirtualForNonpolymorphClass;

	error.text= "Function \"."_SpC + function_name + "\" can not be virtual, because it`s class is not polymorph."_SpC;

	return error;
}

CodeBuilderError ReportVirtualRequired( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::VirtualRequired;

	error.text= "\"virtual\" required for function \"."_SpC + function_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportOverrideRequired( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::OverrideRequired;

	error.text= "\"override\" required for function \"."_SpC + function_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportFunctionDoesNotOverride( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::FunctionDoesNotOverride;

	error.text= "Function \"."_SpC + function_name + "\" marked as \"override\", but does not override."_SpC;

	return error;
}

CodeBuilderError ReportOverrideFinalFunction( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::OverrideFinalFunction;

	error.text= "\"override\" for final function \"."_SpC + function_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportFinalForFirstVirtualFunction( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::FinalForFirstVirtualFunction;

	error.text= "\"final\" for first virtual function \"."_SpC + function_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportBodyForPureVirtualFunction( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::BodyForPureVirtualFunction;

	error.text= "Body for pure virtual function \"."_SpC + function_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportClassContainsPureVirtualFunctions( const FilePos& file_pos, const ProgramString& class_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ClassContainsPureVirtualFunctions;

	error.text= "Class \"."_SpC + class_name + "\" is not interface or abstract and contains pure virtual functions."_SpC;

	return error;
}

CodeBuilderError ReportNonPureVirtualFunctionInInterface( const FilePos& file_pos, const ProgramString& class_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::NonPureVirtualFunctionInInterface;

	error.text= "Interface \"."_SpC + class_name + "\" contains non-pure virtual functions."_SpC;

	return error;
}

CodeBuilderError ReportPureDestructor( const FilePos& file_pos, const ProgramString& class_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::PureDestructor;

	error.text= "Pure destructor for class \"."_SpC + class_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportVirtualForPrivateFunction( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::VirtualForPrivateFunction;

	error.text= "Virtual for private function \"."_SpC + function_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportVirtualForFunctionTemplate( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::VirtualForFunctionTemplate;

	error.text= "\"virtual\" for template function \"."_SpC + function_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportVirtualForFunctionImplementation( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::VirtualForFunctionImplementation;

	error.text= "\"virtual\" for function implementation \"."_SpC + function_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportVirtualMismatch( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::VirtualMismatch;

	error.text= "\"virtual\" specifiers mismatch for function \""_SpC + function_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportNoMangleForNonglobalFunction( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::NoMangleForNonglobalFunction;

	error.text= "\"nomangle\" for non-global function \""_SpC + function_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportNoMangleMismatch( const FilePos& file_pos, const ProgramString& function_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::NoMangleMismatch;

	error.text= "\"nomangle\" specifiers mismatch for function \""_SpC + function_name + "\"."_SpC;

	return error;
}

CodeBuilderError ReportUnsafeFunctionCallOutsideUnsafeBlock( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::UnsafeFunctionCallOutsideUnsafeBlock;

	error.text= "Calling unsafe function outside unsafe block."_SpC;

	return error;
}

CodeBuilderError ReportExplicitAccessToThisMethodIsUnsafe( const FilePos& file_pos, const ProgramString& method_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExplicitAccessToThisMethodIsUnsafe;

	error.text= "Explicit access to method \""_SpC + method_name + "\" is unsafe."_SpC;

	return error;
}

CodeBuilderError ReportUnsafeReferenceCastOutsideUnsafeBlock( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::UnsafeReferenceCastOutsideUnsafeBlock;

	error.text= "Unsafe reference cast outside unsafe block."_SpC;

	return error;
}

CodeBuilderError ReportMutableReferenceCastOutsideUnsafeBlock( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::MutableReferenceCastOutsideUnsafeBlock;

	error.text= "Mutable reference cast outside unsafe block."_SpC;

	return error;
}

CodeBuilderError ReportUninitializedInitializerOutsideUnsafeBlock( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::UninitializedInitializerOutsideUnsafeBlock;

	error.text= "unsafe initializer outside unsafe block."_SpC;

	return error;
}

} // namespace U
