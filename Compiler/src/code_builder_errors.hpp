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
	// TODO - emit common error - ExpectedVariable.
	ExpectedVariable,

	CouldNotOverloadFunction,
	TooManySuitableOverloadedFunctions,
	CouldNotSelectOverloadedFunction,
	FunctionPrototypeDuplication,
	FunctionBodyDuplication,
	FunctionDeclarationOutsideItsScope,
	ClassDeclarationOutsideItsScope,
	ClassBodyDuplication,
	UsingIncompleteType,

	// Visibility
	AccessingNonpublicClassMember,
	FunctionsVisibilityMismatch,
	VisibilityForStruct,

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
	ZeroInitializerForClass,
	StructInitializerForNonStruct,
	InitializerForNonfieldStructMember,
	InitializerForBaseClassField,
	DuplicatedStructMemberInitializer,
	InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors,
	InvalidTypeForAutoVariable,
	GlobalVariableMustBeConstexpr,

	// Constructors errors
	ConstructorOrDestructorOutsideClass,
	ConstructorAndDestructorMustReturnVoid,
	InitializationListInNonconstructor,
	ClassHaveNoConstructors,
	ExplicitThisInDestructor,
	FieldIsNotInitializedYet,

	// Destructors errors
	ExplicitArgumentsInDestructor,

	// Methods errors.
	CallOfThiscallFunctionUsingNonthisArgument,
	ClassFiledAccessInStaticMethod,
	ThisInNonclassFunction,
	ThiscallMismatch,
	AccessOfNonThisClassField,
	ThisUnavailable,
	BaseUnavailable,

	// Template errors.
	InvalidValueAsTemplateArgument,
	InvalidTypeOfTemplateVariableArgument,
	TemplateParametersDeductionFailed,
	DeclarationShadowsTemplateArgument,
	ValueIsNotTemplate,
	TemplateInstantiationRequired,
	MandatoryTemplateSignatureArgumentAfterOptionalArgument,
	TemplateArgumentIsNotDeducedYet,
	TemplateArgumentNotUsedInSignature,
	IncompleteMemberOfClassTemplate,
	TemplateFunctionGenerationFailed,

	// Reference checking
	ReferenceProtectionError,
	DestroyedVariableStillHaveReferences,
	AccessingVariableThatHaveMutableReference,
	AccessingMovedVariable,
	ReturningUnallowedReference,
	InvalidReferenceTagCount,
	SelfReferencePollution,
	ArgReferencePollution,
	MutableReferencePollutionOfOuterLoopVariable,
	OuterVariableMoveInsideLoop,
	ConditionalMove,
	MovedVariableHaveReferences,
	UnallowedReferencePollution,
	ReferencePollutionForArgReference,
	ExplicitReferencePollutionForCopyConstructor,
	ExplicitReferencePollutionForCopyAssignmentOperator,

	// Operators overloading
	OperatorDeclarationOutsideClass,
	OperatorDoesNotHaveParentClassArguments,
	InvalidArgumentCountForOperator,
	InvalidReturnTypeForOperator,

	// Enums
	UnderlayingTypeForEnumIsTooSmall,

	// Inheritance errors
	CanNotDeriveFromThisType,
	DuplicatedParentClass,
	DuplicatedBaseClass,
	FieldsForInterfacesNotAllowed,
	BaseClassForInterface,
	ConstructorForInterface,

	// Virtual functions errors
	VirtualForNonclassFunction,
	VirtualForNonThisCallFunction,
	VirtualForNonpolymorphClass,
	FunctionCanNotBeVirtual,
	VirtualRequired,
	OverrideRequired,
	FunctionDoesNotOverride,
	OverrideFinalFunction,
	FinalForFirstVirtualFunction,
	BodyForPureVirtualFunction,
	ClassContainsPureVirtualFunctions,
	NonPureVirtualFunctionInInterface,
	PureDestructor,
	VirtualForPrivateFunction,
	VirtualForFunctionTemplate,
	VirtualForFunctionImplementation,

	// Unsafe
	UnsafeFunctionCallOutsideUnsafeBlock,
	ExplicitAccessToThisMethodIsUnsafe,
	UnsafeReferenceCastOutsideUnsafeBlock,
};

struct CodeBuilderError
{
	FilePos file_pos;
	CodeBuilderErrorCode code;
	ProgramString text;
};

const char* CodeBuilderErrorCodeToString( CodeBuilderErrorCode code );

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
CodeBuilderError ReportExpectedVariable( const FilePos& file_pos, const ProgramString& got );
CodeBuilderError ReportCouldNotOverloadFunction( const FilePos& file_pos );
CodeBuilderError ReportTooManySuitableOverloadedFunctions( const FilePos& file_pos );
CodeBuilderError ReportCouldNotSelectOverloadedFunction( const FilePos& file_pos );
CodeBuilderError ReportFunctionPrototypeDuplication( const FilePos& file_pos, const ProgramString& func_name );
CodeBuilderError ReportFunctionBodyDuplication( const FilePos& file_pos, const ProgramString& func_name );
CodeBuilderError ReportFunctionDeclarationOutsideItsScope( const FilePos& file_pos );
CodeBuilderError ReportClassDeclarationOutsideItsScope( const FilePos& file_pos );
CodeBuilderError ReportClassBodyDuplication( const FilePos& file_pos );
CodeBuilderError ReportUsingIncompleteType( const FilePos& file_pos, const ProgramString& type_name );
CodeBuilderError ReportAccessingNonpublicClassMember( const FilePos& file_pos, const ProgramString& class_name, const ProgramString& member_name );
CodeBuilderError ReportFunctionsVisibilityMismatch( const FilePos& file_pos, const ProgramString& function_name );
CodeBuilderError ReportVisibilityForStruct( const FilePos& file_pos, const ProgramString& struct_name );
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
CodeBuilderError ReportZeroInitializerForClass( const FilePos& file_pos );
CodeBuilderError ReportStructInitializerForNonStruct( const FilePos& file_pos );
CodeBuilderError ReportInitializerForNonfieldStructMember( const FilePos& file_pos, const ProgramString& member_name );
CodeBuilderError ReportInitializerForBaseClassField( const FilePos& file_pos, const ProgramString& field_name );
CodeBuilderError ReportDuplicatedStructMemberInitializer( const FilePos& file_pos, const ProgramString& member_name );
CodeBuilderError ReportInitializerDisabledBecauseClassHaveExplicitNoncopyConstructors( const FilePos& file_pos );
CodeBuilderError ReportInvalidTypeForAutoVariable( const FilePos& file_pos, const ProgramString& type_name );
CodeBuilderError ReportGlobalVariableMustBeConstexpr( const FilePos& file_pos, const ProgramString& variable_name );
CodeBuilderError ReportConstructorOrDestructorOutsideClass( const FilePos& file_pos );
CodeBuilderError ReportConstructorAndDestructorMustReturnVoid( const FilePos& file_pos );
CodeBuilderError ReportInitializationListInNonconstructor( const FilePos& file_pos );
CodeBuilderError ReportClassHaveNoConstructors( const FilePos& file_pos );
CodeBuilderError ReportExplicitThisInDestructor( const FilePos& file_pos );
CodeBuilderError ReportFieldIsNotInitializedYet( const FilePos& file_pos, const ProgramString& field_name );
CodeBuilderError ReportExplicitArgumentsInDestructor( const FilePos& file_pos );
CodeBuilderError ReportCallOfThiscallFunctionUsingNonthisArgument( const FilePos& file_pos );
CodeBuilderError ReportClassFiledAccessInStaticMethod( const FilePos& file_pos, const ProgramString& field_name );
CodeBuilderError ReportThisInNonclassFunction( const FilePos& file_pos, const ProgramString& func_name );
CodeBuilderError ReportThiscallMismatch( const FilePos& file_pos, const ProgramString& func_name );
CodeBuilderError ReportAccessOfNonThisClassField( const FilePos& file_pos, const ProgramString& field_name );
CodeBuilderError ReportThisUnavailable( const FilePos& file_pos );
CodeBuilderError ReportBaseUnavailable( const FilePos& file_pos );
CodeBuilderError ReportInvalidValueAsTemplateArgument( const FilePos& file_pos, const ProgramString& got );
CodeBuilderError ReportInvalidTypeOfTemplateVariableArgument( const FilePos& file_pos, const ProgramString& type_name );
CodeBuilderError ReportTemplateParametersDeductionFailed( const FilePos& file_pos );
CodeBuilderError ReportDeclarationShadowsTemplateArgument( const FilePos& file_pos, const ProgramString& name );
CodeBuilderError ReportValueIsNotTemplate( const FilePos& file_pos );
CodeBuilderError ReportTemplateInstantiationRequired( const FilePos& file_pos, const ProgramString& template_name );
CodeBuilderError ReportMandatoryTemplateSignatureArgumentAfterOptionalArgument( const FilePos& file_pos );
CodeBuilderError ReportTemplateArgumentIsNotDeducedYet( const FilePos& file_pos, const ProgramString& name );
CodeBuilderError ReportTemplateArgumentNotUsedInSignature( const FilePos& file_pos, const ProgramString& name );
CodeBuilderError ReportIncompleteMemberOfClassTemplate( const FilePos& file_pos, const ProgramString& name );
CodeBuilderError ReportTemplateFunctionGenerationFailed( const FilePos& file_pos, const ProgramString& function_template_name );
CodeBuilderError ReportReferenceProtectionError( const FilePos& file_pos, const ProgramString& var_name );
CodeBuilderError ReportDestroyedVariableStillHaveReferences( const FilePos& file_pos, const ProgramString& var_name );
CodeBuilderError ReportAccessingVariableThatHaveMutableReference( const FilePos& file_pos, const ProgramString& var_name );
CodeBuilderError ReportAccessingMovedVariable( const FilePos& file_pos, const ProgramString& var_name );
CodeBuilderError ReportReturningUnallowedReference( const FilePos& file_pos ); // TODO - add variable name
CodeBuilderError ReportInvalidReferenceTagCount( const FilePos& file_pos, size_t given, size_t required );
CodeBuilderError ReportSelfReferencePollution( const FilePos& file_pos );
CodeBuilderError ReportArgReferencePollution( const FilePos& file_pos );
CodeBuilderError ReportMutableReferencePollutionOfOuterLoopVariable( const FilePos& file_pos, const ProgramString& dst_name, const ProgramString& src_name );
CodeBuilderError ReportOuterVariableMoveInsideLoop( const FilePos& file_pos, const ProgramString& variable_name );
CodeBuilderError ReportConditionalMove( const FilePos& file_pos, const ProgramString& variable_name );
CodeBuilderError ReportMovedVariableHaveReferences( const FilePos& file_pos, const ProgramString& variable_name );
CodeBuilderError ReportUnallowedReferencePollution( const FilePos& file_pos ); // TODO - add some string information
CodeBuilderError ReportReferencePollutionForArgReference( const FilePos& file_pos ); // TODO - add some string information
CodeBuilderError ReportExplicitReferencePollutionForCopyConstructor( const FilePos& file_pos );
CodeBuilderError ReportExplicitReferencePollutionForCopyAssignmentOperator( const FilePos& file_pos );
CodeBuilderError ReportOperatorDeclarationOutsideClass( const FilePos& file_pos );
CodeBuilderError ReportOperatorDoesNotHaveParentClassArguments( const FilePos& file_pos );
CodeBuilderError ReportInvalidArgumentCountForOperator( const FilePos& file_pos );
CodeBuilderError ReportInvalidReturnTypeForOperator( const FilePos& file_pos, const ProgramString& expected_type_name );
CodeBuilderError ReportUnderlayingTypeForEnumIsTooSmall( const FilePos& file_pos, SizeType max_value, SizeType max_value_of_underlaying_type );
CodeBuilderError ReportCanNotDeriveFromThisType( const FilePos& file_pos, const ProgramString& type_name );
CodeBuilderError ReportDuplicatedParentClass( const FilePos& file_pos, const ProgramString& type_name );
CodeBuilderError ReportDuplicatedBaseClass( const FilePos& file_pos, const ProgramString& type_name );
CodeBuilderError ReportFieldsForInterfacesNotAllowed( const FilePos& file_pos );
CodeBuilderError ReportBaseClassForInterface( const FilePos& file_pos );
CodeBuilderError ReportConstructorForInterface( const FilePos& file_pos );
CodeBuilderError ReportVirtualForNonclassFunction( const FilePos& file_pos, const ProgramString& function_name );
CodeBuilderError ReportVirtualForNonThisCallFunction( const FilePos& file_pos, const ProgramString& function_name );
CodeBuilderError ReportFunctionCanNotBeVirtual( const FilePos& file_pos, const ProgramString& function_name );
CodeBuilderError ReportVirtualForNonpolymorphClass( const FilePos& file_pos, const ProgramString& function_name );
CodeBuilderError ReportVirtualRequired( const FilePos& file_pos, const ProgramString& function_name );
CodeBuilderError ReportOverrideRequired( const FilePos& file_pos, const ProgramString& function_name );
CodeBuilderError ReportFunctionDoesNotOverride( const FilePos& file_pos, const ProgramString& function_name );
CodeBuilderError ReportOverrideFinalFunction( const FilePos& file_pos, const ProgramString& function_name );
CodeBuilderError ReportFinalForFirstVirtualFunction( const FilePos& file_pos, const ProgramString& function_name );
CodeBuilderError ReportBodyForPureVirtualFunction( const FilePos& file_pos, const ProgramString& function_name );
CodeBuilderError ReportClassContainsPureVirtualFunctions( const FilePos& file_pos, const ProgramString& class_name );
CodeBuilderError ReportNonPureVirtualFunctionInInterface( const FilePos& file_pos, const ProgramString& class_name );
CodeBuilderError ReportPureDestructor( const FilePos& file_pos, const ProgramString& class_name );
CodeBuilderError ReportVirtualForPrivateFunction( const FilePos& file_pos, const ProgramString& function_name );
CodeBuilderError ReportVirtualForFunctionTemplate( const FilePos& file_pos, const ProgramString& function_name );
CodeBuilderError ReportVirtualForFunctionImplementation( const FilePos& file_pos, const ProgramString& function_name );
CodeBuilderError ReportUnsafeFunctionCallOutsideUnsafeBlock( const FilePos& file_pos );
CodeBuilderError ReportExplicitAccessToThisMethodIsUnsafe( const FilePos& file_pos, const ProgramString& method_name );
CodeBuilderError ReportUnsafeReferenceCastOutsideUnsafeBlock( const FilePos& file_pos );

} // namespace U
