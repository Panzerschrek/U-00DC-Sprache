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

CodeBuilderError ReportExpectedInitializer( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::ExpectedInitializer;

	error.text= "Expected initializer for this variable."_SpC;

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

CodeBuilderError ReportMissingStructMemberInitializer( const FilePos& file_pos, const ProgramString& member_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::MissingStructMemberInitializer;

	error.text= "Missing initializer for"_SpC + member_name + "."_SpC;

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

} // namespace U
