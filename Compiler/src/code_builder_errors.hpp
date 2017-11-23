#pragma once
#include <string>

#include "lang_types.hpp"
#include "lexical_analyzer.hpp"
#include "program_string.hpp"
#include "syntax_elements.hpp"

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

	// ExpectedVariable* errors
	ExpectedVariableInAssignment,
	ExpectedVariableInBinaryOperator,
	ExpectedVariableAsArgument,
	ExpectedVariableInAdditiveAssignment,
	ExpectedVariableInIncrementOrDecrement,
	ExpectedVariableInArraySize,

	CouldNotOverloadFunction,
	TooManySuitableOverloadedFunctions,
	CouldNotSelectOverloadedFunction,
	FunctionPrototypeDuplication,
	FunctionBodyDuplication,
	ReturnValueDiffersFromPrototype,
	FunctionDeclarationOutsideItsScope,
	ClassDeclarationOutsideItsScope,
	ClassBodyDuplication,
	UsingIncompleteType,

	// Constexpr errors.
	ExpectedConstantExpression,
	VariableInitializerIsNotConstantExpression,
	InvalidTypeForConstantExpressionVariable,
	ConstantExpressionResultIsUndefined,

	// Static assert errors.
	StaticAssertExpressionMustHaveBoolType,
	StaticAssertExpressionIsNotConstant,
	StaticAssertionFailed,

	// Compile-time checks.
	ArrayIndexOutOfBounds,

	// Initializers errors.
	ArrayInitializerForNonArray,
	ArrayInitializersCountMismatch,
	FundamentalTypesHaveConstructorsWithExactlyOneParameter,
	ReferencesHaveConstructorsWithExactlyOneParameter,
	UnsupportedInitializerForReference,
	ConstructorInitializerForUnsupportedType,
	StructInitializerForNonStruct,
	InitializerForNonfieldStructMember,
	DuplicatedStructMemberInitializer,
	InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors,
	InvalidTypeForAutoVariable,
	GlobalVariableMustBeConstexpr,

	// Constructors errors
	ConstructorOrDestructorOutsideClass,
	ConstructorAndDestructorMustReturnVoid,
	InitializationListInNonconstructor,
	ClassHaveNoConstructors,
	ExplicitThisInConstructorOrDestructor,
	FieldIsNotInitializedYet,
	MethodsCallInConstructorInitializerListIsForbidden,

	// Destructors errors
	ExplicitArgumentsInDestructor,

	// Methods errors.
	CallOfThiscallFunctionUsingNonthisArgument,
	ClassFiledAccessInStaticMethod,
	ThisInNonclassFunction,
	AccessOfNonThisClassField,
	ThisUnavailable,

	// Template errors.
	InvalidValueAsTemplateArgument,
	InvalidTypeOfTemplateVariableArgument,
	TemplateParametersDeductionFailed,
	DeclarationShadowsTemplateArgument,
	ValueIsNotTemplate,
	TemplateInstantiationRequired,
	MandatoryTemplateSignatureArgumentAfterOptionalArgument,
	TemplateArgumentIsNotDeducedYet,
	UnsupportedExpressionTypeForTemplateSignatureArgument,
	TemplateArgumentNotUsedInSignature,
	IncompleteMemberOfClassTemplate,

	// Reference checking
	ReferenceProtectionError,
	DestroyedVariableStillHaveReferences,
	AccessingVariableThatHaveMutableReference,
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
CodeBuilderError ReportNameNotFound( const FilePos& file_pos, const Synt::ComplexName& name );
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
CodeBuilderError ReportExpectedInitializer( const FilePos& file_pos, const ProgramString& variable_name );
CodeBuilderError ReportExpectedReferenceValue( const FilePos& file_pos );
CodeBuilderError ReportBindingConstReferenceToNonconstReference( const FilePos& file_pos );
CodeBuilderError ReportExpectedVariableInAssignment( const FilePos& file_pos, const ProgramString& got );
CodeBuilderError ReportExpectedVariableInBinaryOperator( const FilePos& file_pos, const ProgramString& got );
CodeBuilderError ReportExpectedVariableAsArgument( const FilePos& file_pos, const ProgramString& got );
CodeBuilderError ReportExpectedVariableInAdditiveAssignment( const FilePos& file_pos, const ProgramString& got );
CodeBuilderError ReportExpectedVariableInIncrementOrDecrement( const FilePos& file_pos, const ProgramString& got );
CodeBuilderError ReprotExpectedVariableInArraySize( const FilePos& file_pos, const ProgramString& got );
CodeBuilderError ReportCouldNotOverloadFunction( const FilePos& file_pos );
CodeBuilderError ReportTooManySuitableOverloadedFunctions( const FilePos& file_pos );
CodeBuilderError ReportCouldNotSelectOverloadedFunction( const FilePos& file_pos );
CodeBuilderError ReportFunctionPrototypeDuplication( const FilePos& file_pos, const ProgramString& func_name );
CodeBuilderError ReportFunctionBodyDuplication( const FilePos& file_pos, const ProgramString& func_name );
CodeBuilderError ReportReturnValueDiffersFromPrototype( const FilePos& file_pos );
CodeBuilderError ReportFunctionDeclarationOutsideItsScope( const FilePos& file_pos );
CodeBuilderError ReportClassDeclarationOutsideItsScope( const FilePos& file_pos );
CodeBuilderError ReportClassBodyDuplication( const FilePos& file_pos );
CodeBuilderError ReportUsingIncompleteType( const FilePos& file_pos, const ProgramString& type_name );
CodeBuilderError ReportExpectedConstantExpression( const FilePos& file_pos );
CodeBuilderError ReportVariableInitializerIsNotConstantExpression( const FilePos& file_pos );
CodeBuilderError ReportInvalidTypeForConstantExpressionVariable( const FilePos& file_pos );
CodeBuilderError ReportConstantExpressionResultIsUndefined( const FilePos& file_pos );
CodeBuilderError ReportStaticAssertExpressionMustHaveBoolType( const FilePos& file_pos );
CodeBuilderError ReportStaticAssertExpressionIsNotConstant( const FilePos& file_pos );
CodeBuilderError ReportStaticAssertionFailed( const FilePos& file_pos );
CodeBuilderError ReportArrayIndexOutOfBounds( const FilePos& file_pos, SizeType index, SizeType array_size );
CodeBuilderError ReportArrayInitializerForNonArray( const FilePos& file_pos );
CodeBuilderError ReportArrayInitializersCountMismatch( const FilePos& file_pos, SizeType expected_initializers, SizeType real_initializers );
CodeBuilderError ReportFundamentalTypesHaveConstructorsWithExactlyOneParameter( const FilePos& file_pos );
CodeBuilderError ReportReferencesHaveConstructorsWithExactlyOneParameter( const FilePos& file_pos );
CodeBuilderError ReportUnsupportedInitializerForReference( const FilePos& file_pos );
CodeBuilderError ReportConstructorInitializerForUnsupportedType( const FilePos& file_pos );
CodeBuilderError ReportStructInitializerForNonStruct( const FilePos& file_pos );
CodeBuilderError ReportInitializerForNonfieldStructMember( const FilePos& file_pos, const ProgramString& member_name );
CodeBuilderError ReportDuplicatedStructMemberInitializer( const FilePos& file_pos, const ProgramString& member_name );
CodeBuilderError ReportInitializerDisabledBecauseClassHaveExplicitNoncopyConstructors( const FilePos& file_pos );
CodeBuilderError ReportInvalidTypeForAutoVariable( const FilePos& file_pos, const ProgramString& type_name );
CodeBuilderError ReportGlobalVariableMustBeConstexpr( const FilePos& file_pos, const ProgramString& variable_name );
CodeBuilderError ReportConstructorOrDestructorOutsideClass( const FilePos& file_pos );
CodeBuilderError ReportConstructorAndDestructorMustReturnVoid( const FilePos& file_pos );
CodeBuilderError ReportInitializationListInNonconstructor( const FilePos& file_pos );
CodeBuilderError ReportClassHaveNoConstructors( const FilePos& file_pos );
CodeBuilderError ReportExplicitThisInConstructorOrDestructor( const FilePos& file_pos );
CodeBuilderError ReportFieldIsNotInitializedYet( const FilePos& file_pos, const ProgramString& field_name );
CodeBuilderError ReportMethodsCallInConstructorInitializerListIsForbidden( const FilePos& file_pos, const ProgramString& method_name );
CodeBuilderError ReportExplicitArgumentsInDestructor( const FilePos& file_pos );
CodeBuilderError ReportCallOfThiscallFunctionUsingNonthisArgument( const FilePos& file_pos );
CodeBuilderError ReportClassFiledAccessInStaticMethod( const FilePos& file_pos, const ProgramString& field_name );
CodeBuilderError ReportThisInNonclassFunction( const FilePos& file_pos, const ProgramString& func_name );
CodeBuilderError ReportAccessOfNonThisClassField( const FilePos& file_pos, const ProgramString& field_name );
CodeBuilderError ReportThisUnavailable( const FilePos& file_pos );
CodeBuilderError ReportInvalidValueAsTemplateArgument( const FilePos& file_pos, const ProgramString& got );
CodeBuilderError ReportInvalidTypeOfTemplateVariableArgument( const FilePos& file_pos, const ProgramString& type_name );
CodeBuilderError ReportTemplateParametersDeductionFailed( const FilePos& file_pos );
CodeBuilderError ReportDeclarationShadowsTemplateArgument( const FilePos& file_pos, const ProgramString& name );
CodeBuilderError ReportValueIsNotTemplate( const FilePos& file_pos );
CodeBuilderError ReportTemplateInstantiationRequired( const FilePos& file_pos, const ProgramString& template_name );
CodeBuilderError ReportMandatoryTemplateSignatureArgumentAfterOptionalArgument( const FilePos& file_pos );
CodeBuilderError ReportTemplateArgumentIsNotDeducedYet( const FilePos& file_pos, const ProgramString& name );
CodeBuilderError ReportUnsupportedExpressionTypeForTemplateSignatureArgument( const FilePos& file_pos );
CodeBuilderError ReportTemplateArgumentNotUsedInSignature( const FilePos& file_pos, const ProgramString& name );
CodeBuilderError ReportIncompleteMemberOfClassTemplate( const FilePos& file_pos, const ProgramString& name );
CodeBuilderError ReportReferenceProtectionError( const FilePos& file_pos ); // TODO - add variable name
CodeBuilderError ReportDestroyedVariableStillHaveReferences( const FilePos& file_pos ); // TODO - add variable name
CodeBuilderError ReportAccessingVariableThatHaveMutableReference( const FilePos& file_pos ); // TODO - add variable name

} // namespace U
