#pragma once
#include <string>

#include "lang_types.hpp"
#include "lexical_analyzer.hpp"
#include "program_string.hpp"

namespace U
{

enum class CodeBuilderErrorCode : unsigned int
{
	// Codes below 100 is reserved.
	BuildFailed, // Common error code for all reasons.

	NameNotFound= 101u,
	UsingKeywordAsName,
	Redefinition,
	UnknownNumericConstantType,
	OperationNotSupportedForThisType,
	TypesMismatch,
	NoMatchBinaryOperatorForGivenTypes,
	FunctionSignatureMismatch,
	NotImplemented,
	ArraySizeIsNegative,
	ArraySizeIsNotInteger,
	BreakOutsideLoop,
	ContinueOutsideLoop,
	NameIsNotTypeName,
	UnreachableCode,
	NoReturnInFunctionReturningNonVoid,
	ExpectedInitializer,
	ExpectedReferenceValue,
	BindingConstReferenceToNonconstReference,
	ExpectedVariableInAssignment,
	ExpectedVariableInBinaryOperator,
	ExpectedVariableAsArgument,
	CouldNotOverloadFunction,
	TooManySuitableOverloadedFunctions,
	CouldNotSelectOverloadedFunction,
	FunctionPrototypeDuplication,
	FunctionBodyDuplication,
	ReturnValueDiffersFromPrototype,

	// Initializers errors.
	ArrayInitializerForNonArray,
	ArrayInitializersCountMismatch,
	FundamentalTypesHaveConstructorsWithExactlyOneParameter,
	ReferencesHaveConstructorsWithExactlyOneParameter,
	UnsupportedInitializerForReference,
	ConstructorInitializerForUnsupportedType,
	StructInitializerForNonStruct,
	DuplicatedStructMemberInitializer,
	MissingStructMemberInitializer,
	InvalidTypeForAutoVariable,

	// Push new error codes at back.
};

struct CodeBuilderError
{
	FilePos file_pos;
	CodeBuilderErrorCode code;
	ProgramString text;
};

// Helper functions for errors generation.

// TODO - add more parameters for errors.
CodeBuilderError ReportBuildFailed();
CodeBuilderError ReportNameNotFound( const FilePos& file_pos, const ProgramString& name );
CodeBuilderError ReportUsingKeywordAsName( const FilePos& file_pos );
CodeBuilderError ReportRedefinition( const FilePos& file_pos, const ProgramString& name );
CodeBuilderError ReportUnknownNumericConstantType( const FilePos& file_pos, const ProgramString& unknown_type );
CodeBuilderError ReportOperationNotSupportedForThisType( const FilePos& file_pos, const ProgramString& type_name );
CodeBuilderError ReportTypesMismatch( const FilePos& file_pos, const ProgramString& expected_type_name, const ProgramString& actual_type_name );
CodeBuilderError ReportNoMatchBinaryOperatorForGivenTypes( const FilePos& file_pos, const ProgramString& type_l_name, const ProgramString& type_r_name, const ProgramString& binary_operator );
CodeBuilderError ReportFunctionSignatureMismatch( const FilePos& file_pos );
CodeBuilderError ReportNotImplemented( const FilePos& file_pos, const char* what );
CodeBuilderError ReportArraySizeIsNegative( const FilePos& file_pos );
CodeBuilderError ReportArraySizeIsNotInteger( const FilePos& file_pos );
CodeBuilderError ReportBreakOutsideLoop( const FilePos& file_pos );
CodeBuilderError ReportContinueOutsideLoop( const FilePos& file_pos );
CodeBuilderError ReportNameIsNotTypeName( const FilePos& file_pos, const ProgramString& name );
CodeBuilderError ReportUnreachableCode( const FilePos& file_pos );
CodeBuilderError ReportNoReturnInFunctionReturningNonVoid( const FilePos& file_pos );
CodeBuilderError ReportExpectedInitializer( const FilePos& file_pos );
CodeBuilderError ReportExpectedReferenceValue( const FilePos& file_pos );
CodeBuilderError ReportBindingConstReferenceToNonconstReference( const FilePos& file_pos );
CodeBuilderError ReportExpectedVariableInAssignment( const FilePos& file_pos, const ProgramString& got );
CodeBuilderError ReportExpectedVariableInBinaryOperator( const FilePos& file_pos, const ProgramString& got );
CodeBuilderError ReportExpectedVariableAsArgument( const FilePos& file_pos, const ProgramString& got );
CodeBuilderError ReportCouldNotOverloadFunction( const FilePos& file_pos );
CodeBuilderError ReportTooManySuitableOverloadedFunctions( const FilePos& file_pos );
CodeBuilderError ReportCouldNotSelectOverloadedFunction( const FilePos& file_pos );
CodeBuilderError ReportFunctionPrototypeDuplication( const FilePos& file_pos, const ProgramString& func_name );
CodeBuilderError ReportFunctionBodyDuplication( const FilePos& file_pos, const ProgramString& func_name );
CodeBuilderError ReportReturnValueDiffersFromPrototype( const FilePos& file_pos );
CodeBuilderError ReportArrayInitializerForNonArray( const FilePos& file_pos );
CodeBuilderError ReportArrayInitializersCountMismatch( const FilePos& file_pos, size_t expected_initializers, size_t real_initializers );
CodeBuilderError ReportFundamentalTypesHaveConstructorsWithExactlyOneParameter( const FilePos& file_pos );
CodeBuilderError ReportReferencesHaveConstructorsWithExactlyOneParameter( const FilePos& file_pos );
CodeBuilderError ReportUnsupportedInitializerForReference( const FilePos& file_pos );
CodeBuilderError ReportConstructorInitializerForUnsupportedType( const FilePos& file_pos );
CodeBuilderError ReportStructInitializerForNonStruct( const FilePos& file_pos );
CodeBuilderError ReportDuplicatedStructMemberInitializer( const FilePos& file_pos, const ProgramString& member_name );
CodeBuilderError ReportMissingStructMemberInitializer( const FilePos& file_pos, const ProgramString& member_name );
CodeBuilderError ReportInvalidTypeForAutoVariable( const FilePos& file_pos, const ProgramString& type_name );

} // namespace U
