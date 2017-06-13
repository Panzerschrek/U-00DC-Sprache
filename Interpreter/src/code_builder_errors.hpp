#pragma once
#include <string>

#include "lang_types.hpp"
#include "lexical_analyzer.hpp"
#include "program_string.hpp"

namespace Interpreter
{

enum class CodeBuilderErrorCode : unsigned int
{
	// Codes below 100 is reserved.
	BuildFailed, // Common error code for all reasons.

	NameNotFound= 101u,
	UsingKeywordAsName,
	Redefinition,
	OperationNotSupportedForThisType,
	TypesMismatch,
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
	CouldNotOverloadFunction,

	// Push new error codes at back.
};

struct CodeBuilderError
{
	FilePos file_pos;
	CodeBuilderErrorCode code;
	ProgramString text;
};

// Helper functions for errors generation.
CodeBuilderError ReportBuildFailed();
CodeBuilderError ReportNameNotFound( const FilePos& file_pos, const ProgramString& name );
CodeBuilderError ReportUsingKeywordAsName( const FilePos& file_pos );
CodeBuilderError ReportRedefinition( const FilePos& file_pos, const ProgramString& name );
CodeBuilderError ReportOperationNotSupportedForThisType( const FilePos& file_pos, const ProgramString& type_name );
CodeBuilderError ReportTypesMismatch( const FilePos& file_pos, const ProgramString& expected_type_name, const ProgramString& actual_type_name );
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
CodeBuilderError ReportCouldNotOverloadFunction( const FilePos& file_pos );

} // namespace Interpreter
