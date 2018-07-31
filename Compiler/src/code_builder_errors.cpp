#include "assert.hpp"
#include "code_builder_types.hpp"
#include "keywords.hpp"

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
	case CodeBuilderErrorCode::BuildFailed:
		return "BuildFailed";
	case CodeBuilderErrorCode::NameNotFound:
		return "NameNotFound";
	case CodeBuilderErrorCode::UsingKeywordAsName:
		return "UsingKeywordAsName";
	case CodeBuilderErrorCode::Redefinition:
		return "Redefinition";
	case CodeBuilderErrorCode::DeclarationOutsideEnclosingNamespace:
		return "DeclarationOutsideEnclosingNamespace";
	case CodeBuilderErrorCode::UnknownNumericConstantType:
		return "UnknownNumericConstantType";
	case CodeBuilderErrorCode::UnknownStringLiteralSuffix:
		return "UnknownStringLiteralSuffix";
	case CodeBuilderErrorCode::OperationNotSupportedForThisType:
		return "OperationNotSupportedForThisType";
	case CodeBuilderErrorCode::TypesMismatch:
		return "TypesMismatch";
	case CodeBuilderErrorCode::NoMatchBinaryOperatorForGivenTypes:
		return "NoMatchBinaryOperatorForGivenTypes";
	case CodeBuilderErrorCode::NotImplemented:
		return "NotImplemented";
	case CodeBuilderErrorCode::ArraySizeIsNegative:
		return "ArraySizeIsNegative";
	case CodeBuilderErrorCode::ArraySizeIsNotInteger:
		return "ArraySizeIsNotInteger";
	case CodeBuilderErrorCode::BreakOutsideLoop:
		return "BreakOutsideLoop";
	case CodeBuilderErrorCode::ContinueOutsideLoop:
		return "ContinueOutsideLoop";
	case CodeBuilderErrorCode::NameIsNotTypeName:
		return "NameIsNotTypeName";
	case CodeBuilderErrorCode::UnreachableCode:
		return "UnreachableCode";
	case CodeBuilderErrorCode::NoReturnInFunctionReturningNonVoid:
		return "NoReturnInFunctionReturningNonVoid";
	case CodeBuilderErrorCode::ExpectedInitializer:
		return "ExpectedInitializer";
	case CodeBuilderErrorCode::ExpectedReferenceValue:
		return "ExpectedReferenceValue";
	case CodeBuilderErrorCode::BindingConstReferenceToNonconstReference:
		return "BindingConstReferenceToNonconstReference";
	case CodeBuilderErrorCode::ExpectedVariable:
		return "ExpectedVariable";
	case CodeBuilderErrorCode::InvalidFunctionArgumentCount:
		return "InvalidFunctionArgumentCount";
	case CodeBuilderErrorCode::CouldNotOverloadFunction:
		return "CouldNotOverloadFunction";
	case CodeBuilderErrorCode::TooManySuitableOverloadedFunctions:
		return "TooManySuitableOverloadedFunctions";
	case CodeBuilderErrorCode::CouldNotSelectOverloadedFunction:
		return "CouldNotSelectOverloadedFunction";
	case CodeBuilderErrorCode::FunctionPrototypeDuplication:
		return "FunctionPrototypeDuplication";
	case CodeBuilderErrorCode::FunctionBodyDuplication:
		return "FunctionBodyDuplication";
	case CodeBuilderErrorCode::FunctionDeclarationOutsideItsScope:
		return "FunctionDeclarationOutsideItsScope";
	case CodeBuilderErrorCode::ClassDeclarationOutsideItsScope:
		return "ClassDeclarationOutsideItsScope";
	case CodeBuilderErrorCode::ClassBodyDuplication:
		return "ClassBodyDuplication";
	case CodeBuilderErrorCode::UsingIncompleteType:
		return "UsingIncompleteType";
	case CodeBuilderErrorCode::GlobalsLoopDetected:
		return "GlobalsLoopDetected";
	case CodeBuilderErrorCode::AccessingNonpublicClassMember:
		return "AccessingNonpublicClassMember";
	case CodeBuilderErrorCode::FunctionsVisibilityMismatch:
		return "FunctionsVisibilityMismatch";
	case CodeBuilderErrorCode::VisibilityForStruct:
		return "VisibilityForStruct";
	case CodeBuilderErrorCode::ExpectedConstantExpression:
		return "ExpectedConstantExpression";
	case CodeBuilderErrorCode::VariableInitializerIsNotConstantExpression:
		return "VariableInitializerIsNotConstantExpression";
	case CodeBuilderErrorCode::InvalidTypeForConstantExpressionVariable:
		return "InvalidTypeForConstantExpressionVariable";
	case CodeBuilderErrorCode::ConstantExpressionResultIsUndefined:
		return "ConstantExpressionResultIsUndefined";
	case CodeBuilderErrorCode::ConstexprFunctionEvaluationError:
		return "ConstexprFunctionEvaluationError";
	case CodeBuilderErrorCode::ConstexprFunctionContainsUnallowedOperations:
		return "ConstexprFunctionContainsUnallowedOperations";
	case CodeBuilderErrorCode::InvalidTypeForConstexprFunction:
		return "InvalidTypeForConstexprFunction";
	case CodeBuilderErrorCode::ConstexprFunctionsMustHaveBody:
		return "ConstexprFunctionsMustHaveBody";
	case CodeBuilderErrorCode::ConstexprFunctionCanNotBeVirtual:
		return "ConstexprFunctionCanNotBeVirtual";
	case CodeBuilderErrorCode::StaticAssertExpressionMustHaveBoolType:
		return "StaticAssertExpressionMustHaveBoolType";
	case CodeBuilderErrorCode::StaticAssertExpressionIsNotConstant:
		return "StaticAssertExpressionIsNotConstant";
	case CodeBuilderErrorCode::StaticAssertionFailed:
		return "StaticAssertionFailed";
	case CodeBuilderErrorCode::ArrayIndexOutOfBounds:
		return "ArrayIndexOutOfBounds";
	case CodeBuilderErrorCode::ArrayInitializerForNonArray:
		return "ArrayInitializerForNonArray";
	case CodeBuilderErrorCode::ArrayInitializersCountMismatch:
		return "ArrayInitializersCountMismatch";
	case CodeBuilderErrorCode::FundamentalTypesHaveConstructorsWithExactlyOneParameter:
		return "FundamentalTypesHaveConstructorsWithExactlyOneParameter";
	case CodeBuilderErrorCode::ReferencesHaveConstructorsWithExactlyOneParameter:
		return "ReferencesHaveConstructorsWithExactlyOneParameter";
	case CodeBuilderErrorCode::UnsupportedInitializerForReference:
		return "UnsupportedInitializerForReference";
	case CodeBuilderErrorCode::ConstructorInitializerForUnsupportedType:
		return "ConstructorInitializerForUnsupportedType";
	case CodeBuilderErrorCode::ZeroInitializerForClass:
		return "ZeroInitializerForClass";
	case CodeBuilderErrorCode::StructInitializerForNonStruct:
		return "StructInitializerForNonStruct";
	case CodeBuilderErrorCode::InitializerForBaseClassField:
		return "InitializerForBaseClassField";
	case CodeBuilderErrorCode::InitializerForNonfieldStructMember:
		return "InitializerForNonfieldStructMember";
	case CodeBuilderErrorCode::DuplicatedStructMemberInitializer:
		return "DuplicatedStructMemberInitializer";
	case CodeBuilderErrorCode::InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors:
		return "InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors";
	case CodeBuilderErrorCode::InvalidTypeForAutoVariable:
		return "InvalidTypeForAutoVariable";
	case CodeBuilderErrorCode::GlobalVariableMustBeConstexpr:
		return "GlobalVariableMustBeConstexpr";
	case CodeBuilderErrorCode::ConstructorOrDestructorOutsideClass:
		return "ConstructorOrDestructorOutsideClass";
	case CodeBuilderErrorCode::ConstructorAndDestructorMustReturnVoid:
		return "ConstructorAndDestructorMustReturnVoid";
	case CodeBuilderErrorCode::InitializationListInNonconstructor:
		return "InitializationListInNonconstructor";
	case CodeBuilderErrorCode::ClassHaveNoConstructors:
		return "ClassHaveNoConstructors";
	case CodeBuilderErrorCode::ExplicitThisInDestructor:
		return "ExplicitThisInDestructor";
	case CodeBuilderErrorCode::FieldIsNotInitializedYet:
		return "FieldIsNotInitializedYet";
	case CodeBuilderErrorCode::ExplicitArgumentsInDestructor:
		return "ExplicitArgumentsInDestructor";
	case CodeBuilderErrorCode::CallOfThiscallFunctionUsingNonthisArgument:
		return "CallOfThiscallFunctionUsingNonthisArgument";
	case CodeBuilderErrorCode::ClassFiledAccessInStaticMethod:
		return "ClassFiledAccessInStaticMethod";
	case CodeBuilderErrorCode::ThisInNonclassFunction:
		return "ThisInNonclassFunction";
	case CodeBuilderErrorCode::ThiscallMismatch:
		return "ThiscallMismatch";
	case CodeBuilderErrorCode::AccessOfNonThisClassField:
		return "AccessOfNonThisClassField";
	case CodeBuilderErrorCode::ThisUnavailable:
		return "ThisUnavailable";
	case CodeBuilderErrorCode::BaseUnavailable:
		return "BaseUnavailable";
	case CodeBuilderErrorCode::InvalidMethodForBodyGeneration:
		return "InvalidMethodForBodyGeneration";
	case CodeBuilderErrorCode::MethodBodyGenerationFailed:
		return "MethodBodyGenerationFailed";
	case CodeBuilderErrorCode::AccessingDeletedMethod:
		return "AccessingDeletedMethod";
	case CodeBuilderErrorCode::InvalidValueAsTemplateArgument:
		return "InvalidValueAsTemplateArgument";
	case CodeBuilderErrorCode::InvalidTypeOfTemplateVariableArgument:
		return "InvalidTypeOfTemplateVariableArgument";
	case CodeBuilderErrorCode::TemplateParametersDeductionFailed:
		return "TemplateParametersDeductionFailed";
	case CodeBuilderErrorCode::DeclarationShadowsTemplateArgument:
		return "DeclarationShadowsTemplateArgument";
	case CodeBuilderErrorCode::ValueIsNotTemplate:
		return "ValueIsNotTemplate";
	case CodeBuilderErrorCode::TemplateInstantiationRequired:
		return "TemplateInstantiationRequired";
	case CodeBuilderErrorCode::MandatoryTemplateSignatureArgumentAfterOptionalArgument:
		return "MandatoryTemplateSignatureArgumentAfterOptionalArgument";
	case CodeBuilderErrorCode::TemplateArgumentIsNotDeducedYet:
		return "TemplateArgumentIsNotDeducedYet";
	case CodeBuilderErrorCode::TemplateArgumentNotUsedInSignature:
		return "TemplateArgumentNotUsedInSignature";
	case CodeBuilderErrorCode::IncompleteMemberOfClassTemplate:
		return "IncompleteMemberOfClassTemplate";
	case CodeBuilderErrorCode::TemplateFunctionGenerationFailed:
		return "TemplateFunctionGenerationFailed";
	case CodeBuilderErrorCode::CouldNotSelectMoreSpicializedTypeTemplate:
		return "CouldNotSelectMoreSpicializedTypeTemplate";
	case CodeBuilderErrorCode::ReferenceProtectionError:
		return "ReferenceProtectionError";
	case CodeBuilderErrorCode::DestroyedVariableStillHaveReferences:
		return "DestroyedVariableStillHaveReferences";
	case CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference:
		return "AccessingVariableThatHaveMutableReference";
	case CodeBuilderErrorCode::AccessingMovedVariable:
		return "AccessingMovedVariable";
	case CodeBuilderErrorCode::ReturningUnallowedReference:
		return "ReturningUnallowedReference";
	case CodeBuilderErrorCode::InvalidReferenceTagCount:
		return "InvalidReferenceTagCount";
	case CodeBuilderErrorCode::SelfReferencePollution:
		return "SelfReferencePollution";
	case CodeBuilderErrorCode::ArgReferencePollution:
		return "ArgReferencePollution";
	case CodeBuilderErrorCode::MutableReferencePollutionOfOuterLoopVariable:
		return "MutableReferencePollutionOfOuterLoopVariable";
	case CodeBuilderErrorCode::OuterVariableMoveInsideLoop:
		return "OuterVariableMoveInsideLoop";
	case CodeBuilderErrorCode::ConditionalMove:
		return "ConditionalMove";
	case CodeBuilderErrorCode::MovedVariableHaveReferences:
		return "MovedVariableHaveReferences";
	case CodeBuilderErrorCode::UnallowedReferencePollution:
		return "UnallowedReferencePollution";
	case CodeBuilderErrorCode::ReferencePollutionForArgReference:
		return "ReferencePollutionForArgReference";
	case CodeBuilderErrorCode::ExplicitReferencePollutionForCopyConstructor:
		return "ExplicitReferencePollutionForCopyConstructor";
	case CodeBuilderErrorCode::ExplicitReferencePollutionForCopyAssignmentOperator:
		return "ExplicitReferencePollutionForCopyAssignmentOperator";
	case CodeBuilderErrorCode::OperatorDeclarationOutsideClass:
		return "OperatorDeclarationOutsideClass";
	case CodeBuilderErrorCode::OperatorDoesNotHaveParentClassArguments:
		return "OperatorDoesNotHaveParentClassArguments";
	case CodeBuilderErrorCode::InvalidArgumentCountForOperator:
		return "InvalidArgumentCountForOperator";
	case CodeBuilderErrorCode::InvalidReturnTypeForOperator:
		return "InvalidReturnTypeForOperator";
	case CodeBuilderErrorCode::UnderlayingTypeForEnumIsTooSmall:
		return "UnderlayingTypeForEnumIsTooSmall";
	case CodeBuilderErrorCode::CanNotDeriveFromThisType:
		return "CanNotDeriveFromThisType";
	case CodeBuilderErrorCode::DuplicatedParentClass:
		return "DuplicatedParentClass";
	case CodeBuilderErrorCode::DuplicatedBaseClass:
		return "DuplicatedBaseClass";
	case CodeBuilderErrorCode::FieldsForInterfacesNotAllowed:
		return "FieldsForInterfacesNotAllowed";
	case CodeBuilderErrorCode::BaseClassForInterface:
		return "BaseClassForInterface";
	case CodeBuilderErrorCode::ConstructorForInterface:
		return "ConstructorForInterface";
	case CodeBuilderErrorCode::VirtualForNonclassFunction:
		return "VirtualForNonclassFunction";
	case CodeBuilderErrorCode::VirtualForNonThisCallFunction:
		return "VirtualForNonThisCallFunction";
	case CodeBuilderErrorCode::FunctionCanNotBeVirtual:
		return "FunctionCanNotBeVirtual";
	case CodeBuilderErrorCode::VirtualForNonpolymorphClass:
		return "VirtualForNonpolymorphClass";
	case CodeBuilderErrorCode::VirtualRequired:
		return "VirtualRequired";
	case CodeBuilderErrorCode::OverrideRequired:
		return "OverrideRequired";
	case CodeBuilderErrorCode::FunctionDoesNotOverride:
		return "FunctionDoesNotOverride";
	case CodeBuilderErrorCode::OverrideFinalFunction:
		return "OverrideFinalFunction";
	case CodeBuilderErrorCode::FinalForFirstVirtualFunction:
		return "FinalForFirstVirtualFunction";
	case CodeBuilderErrorCode::BodyForPureVirtualFunction:
		return "BodyForPureVirtualFunction";
	case CodeBuilderErrorCode::ClassContainsPureVirtualFunctions:
		return "ClassContainsPureVirtualFunctions";
	case CodeBuilderErrorCode::NonPureVirtualFunctionInInterface:
		return "NonPureVirtualFunctionInInterface";
	case CodeBuilderErrorCode::PureDestructor:
		return "PureDestructor";
	case CodeBuilderErrorCode::VirtualForPrivateFunction:
		return "VirtualForPrivateFunction";
	case CodeBuilderErrorCode::VirtualForFunctionTemplate:
		return "VirtualForFunctionTemplate";
	case CodeBuilderErrorCode::VirtualForFunctionImplementation:
		return "VirtualForFunctionImplementation";
	case CodeBuilderErrorCode::UnsafeFunctionCallOutsideUnsafeBlock:
		return "UnsafeFunctionCallOutsideUnsafeBlock";
	case CodeBuilderErrorCode::ExplicitAccessToThisMethodIsUnsafe:
		return "ExplicitAccessToThisMethodIsUnsafe";
	case CodeBuilderErrorCode::UnsafeReferenceCastOutsideUnsafeBlock:
		return "UnsafeReferenceCastOutsideUnsafeBlock";
	case CodeBuilderErrorCode::MutableReferenceCastOutsideUnsafeBlock:
		return "MutableReferenceCastOutsideUnsafeBlock";
	case CodeBuilderErrorCode::UninitializedInitializerOutsideUnsafeBlock:
		return "UninitializedInitializerOutsideUnsafeBlock";
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

CodeBuilderError ReportDeclarationOutsideEnclosingNamespace( const FilePos& file_pos )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::DeclarationOutsideEnclosingNamespace;

	error.text= "Declaration outside enclosing namespace."_SpC;

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

CodeBuilderError ReportAccessingVariableThatHaveMutableReference( const FilePos& file_pos, const ProgramString& var_name )
{
	CodeBuilderError error;
	error.file_pos= file_pos;
	error.code= CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference;

	error.text= "Accessing variable \""_SpC + var_name + "\", that have mutable reference."_SpC;

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
