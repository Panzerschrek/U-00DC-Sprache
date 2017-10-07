#include "assert.hpp"
#include "code_builder_types.hpp"
#include "keywords.hpp"

#include "code_builder_errors.hpp"

namespace U
{

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

CodeBuilderError ReportNameNotFound( const FilePos& file_pos, const ComplexName& name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::NameNotFound;

	for( const ComplexName::Component& component : name.components )
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

CodeBuilderError ReportFunctionSignatureMismatch( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::FunctionSignatureMismatch;

	error.text= "Function signature mismatch."_SpC; // TODO - print expected and actual signatures here.

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

CodeBuilderError ReportExpectedVariableInAssignment( const FilePos& file_pos, const ProgramString& got )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExpectedVariableInAssignment;

	error.text= "Expected variable in assignment, got \"."_SpC + got + "\"."_SpC;

	return error;
}

CodeBuilderError ReportExpectedVariableInBinaryOperator( const FilePos& file_pos, const ProgramString& got )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExpectedVariableInBinaryOperator;

	error.text= "Expected variable in binary operator, got \"."_SpC + got + "\"."_SpC;

	return error;
}

CodeBuilderError ReportExpectedVariableAsArgument( const FilePos& file_pos, const ProgramString& got )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExpectedVariableAsArgument;

	error.text= "Expected variable as argument, got \"."_SpC + got + "\"."_SpC;

	return error;
}

CodeBuilderError ReportExpectedVariableInAdditiveAssignment( const FilePos& file_pos, const ProgramString& got )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExpectedVariableInAdditiveAssignment;

	error.text= "Expected variable in additive assignment, got \"."_SpC + got + "\"."_SpC;

	return error;
}

CodeBuilderError ReportExpectedVariableInIncrementOrDecrement( const FilePos& file_pos, const ProgramString& got )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExpectedVariableInIncrementOrDecrement;

	error.text= "Expected variable in increment or decrement, got \"."_SpC + got + "\"."_SpC;

	return error;
}

CodeBuilderError ReprotExpectedVariableInArraySize( const FilePos& file_pos, const ProgramString& got )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExpectedVariableInArraySize;

	error.text= "Expected variable in array size expression, got \"."_SpC + got + "\"."_SpC;

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

CodeBuilderError ReportReturnValueDiffersFromPrototype( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ReturnValueDiffersFromPrototype;

	error.text= "Function return value differs from prototype."_SpC;

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

CodeBuilderError ReportArrayIndexOutOfBounds( const FilePos& file_pos, const size_t index, const size_t array_size )
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
	const size_t expected_initializers,
	const size_t real_initializers )
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

CodeBuilderError ReportExplicitThisInConstructorOrDestructor( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExplicitThisInConstructorOrDestructor;

	error.text= "Explicit \"this\" in constructor or destructor parameters."_SpC;

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

CodeBuilderError ReportMethodsCallInConstructorInitializerListIsForbidden( const FilePos& file_pos, const ProgramString& method_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::MethodsCallInConstructorInitializerListIsForbidden;

	error.text= "Call of method \""_SpC + method_name + "\" in constructor."_SpC;

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

CodeBuilderError ReportUnsupportedExpressionTypeForTemplateSignatureArgument( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::UnsupportedExpressionTypeForTemplateSignatureArgument;

	error.text= "Unsupported kind of expression in class template signature argument. Supported only named operands."_SpC;

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

} // namespace U
