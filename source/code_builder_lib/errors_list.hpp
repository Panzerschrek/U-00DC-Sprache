
// Errors list.
// Usage - define macro PROCESS_ERROR, include this file, undefune macro PROCESS_ERROR.
PROCESS_ERROR( BuildFailed ) // Common error code for all reasons.

PROCESS_ERROR( NameNotFound )
PROCESS_ERROR( UsingKeywordAsName )
PROCESS_ERROR( Redefinition )
PROCESS_ERROR( UnknownNumericConstantType )
PROCESS_ERROR( UnknownStringLiteralSuffix )
PROCESS_ERROR( InvalidSizeForCharLiteral )
PROCESS_ERROR( OperationNotSupportedForThisType )
PROCESS_ERROR( TypesMismatch )
PROCESS_ERROR( NoMatchBinaryOperatorForGivenTypes )
PROCESS_ERROR( NotImplemented )
PROCESS_ERROR( ArraySizeIsNegative )
PROCESS_ERROR( ArraySizeIsNotInteger )
PROCESS_ERROR( BreakOutsideLoop )
PROCESS_ERROR( ContinueOutsideLoop )
PROCESS_ERROR( NameIsNotTypeName )
PROCESS_ERROR( UnreachableCode )
PROCESS_ERROR( NoReturnInFunctionReturningNonVoid )
PROCESS_ERROR( ExpectedInitializer )
PROCESS_ERROR( ExpectedReferenceValue )
PROCESS_ERROR( BindingConstReferenceToNonconstReference )
PROCESS_ERROR( ExpectedVariable )

PROCESS_ERROR( InvalidFunctionArgumentCount )
PROCESS_ERROR( CouldNotOverloadFunction )
PROCESS_ERROR( TooManySuitableOverloadedFunctions )
PROCESS_ERROR( CouldNotSelectOverloadedFunction )
PROCESS_ERROR( FunctionPrototypeDuplication )
PROCESS_ERROR( FunctionBodyDuplication )
PROCESS_ERROR( BodyForGeneratedFunction )
PROCESS_ERROR( BodyForDeletedFunction )
PROCESS_ERROR( FunctionDeclarationOutsideItsScope )
PROCESS_ERROR( ClassDeclarationOutsideItsScope )
PROCESS_ERROR( ClassBodyDuplication )
PROCESS_ERROR( UsingIncompleteType )
PROCESS_ERROR( GlobalsLoopDetected )

// Visibility
PROCESS_ERROR( AccessingNonpublicClassMember )
PROCESS_ERROR( FunctionsVisibilityMismatch )
PROCESS_ERROR( TypeTemplatesVisibilityMismatch )
PROCESS_ERROR( VisibilityForStruct )

// Constexpr errors.
PROCESS_ERROR( ExpectedConstantExpression )
PROCESS_ERROR( VariableInitializerIsNotConstantExpression )
PROCESS_ERROR( InvalidTypeForConstantExpressionVariable )
PROCESS_ERROR( ConstantExpressionResultIsUndefined )
PROCESS_ERROR( ConstexprFunctionEvaluationError )
PROCESS_ERROR( ConstexprFunctionContainsUnallowedOperations )
PROCESS_ERROR( InvalidTypeForConstexprFunction )
PROCESS_ERROR( ConstexprFunctionsMustHaveBody )
PROCESS_ERROR( ConstexprFunctionCanNotBeVirtual )

// Static assert errors.
PROCESS_ERROR( StaticAssertExpressionMustHaveBoolType )
PROCESS_ERROR( StaticAssertExpressionIsNotConstant )
PROCESS_ERROR( StaticAssertionFailed )

// Compile-time checks.
PROCESS_ERROR( ArrayIndexOutOfBounds )

// Initializers errors.
PROCESS_ERROR( ArrayInitializerForNonArray )
PROCESS_ERROR( ArrayInitializersCountMismatch )
PROCESS_ERROR( FundamentalTypesHaveConstructorsWithExactlyOneParameter )
PROCESS_ERROR( ReferencesHaveConstructorsWithExactlyOneParameter )
PROCESS_ERROR( UnsupportedInitializerForReference )
PROCESS_ERROR( ConstructorInitializerForUnsupportedType )
PROCESS_ERROR( ZeroInitializerForClass )
PROCESS_ERROR( StructInitializerForNonStruct )
PROCESS_ERROR( InitializerForNonfieldStructMember )
PROCESS_ERROR( InitializerForBaseClassField )
PROCESS_ERROR( DuplicatedStructMemberInitializer )
PROCESS_ERROR( InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors )
PROCESS_ERROR( InvalidTypeForAutoVariable )
PROCESS_ERROR( GlobalVariableMustBeConstexpr )

// Constructors errors
PROCESS_ERROR( ConstructorOrDestructorOutsideClass )
PROCESS_ERROR( ConstructorAndDestructorMustReturnVoid )
PROCESS_ERROR( ConversionConstructorMustHaveOneArgument )
PROCESS_ERROR( InitializationListInNonconstructor )
PROCESS_ERROR( ClassHaveNoConstructors )
PROCESS_ERROR( ExplicitThisInDestructor )
PROCESS_ERROR( FieldIsNotInitializedYet )

// Destructors errors
PROCESS_ERROR( ExplicitArgumentsInDestructor )

// Methods errors.
PROCESS_ERROR( CallOfThiscallFunctionUsingNonthisArgument )
PROCESS_ERROR( ClassFiledAccessInStaticMethod )
PROCESS_ERROR( ThisInNonclassFunction )
PROCESS_ERROR( ThiscallMismatch )
PROCESS_ERROR( AccessOfNonThisClassField )
PROCESS_ERROR( ThisUnavailable )
PROCESS_ERROR( BaseUnavailable )
PROCESS_ERROR( InvalidMethodForBodyGeneration )
PROCESS_ERROR( MethodBodyGenerationFailed )
PROCESS_ERROR( AccessingDeletedMethod )

// Template errors.
PROCESS_ERROR( InvalidValueAsTemplateArgument )
PROCESS_ERROR( InvalidTypeOfTemplateVariableArgument )
PROCESS_ERROR( TemplateParametersDeductionFailed )
PROCESS_ERROR( DeclarationShadowsTemplateArgument )
PROCESS_ERROR( ValueIsNotTemplate )
PROCESS_ERROR( TemplateInstantiationRequired )
PROCESS_ERROR( MandatoryTemplateSignatureArgumentAfterOptionalArgument )
PROCESS_ERROR( TemplateArgumentIsNotDeducedYet )
PROCESS_ERROR( TemplateArgumentNotUsedInSignature )
PROCESS_ERROR( IncompleteMemberOfClassTemplate )
PROCESS_ERROR( TemplateFunctionGenerationFailed )
PROCESS_ERROR( CouldNotSelectMoreSpicializedTypeTemplate )

// Reference checking
PROCESS_ERROR( ReferenceProtectionError )
PROCESS_ERROR( DestroyedVariableStillHaveReferences )
PROCESS_ERROR( AccessingMovedVariable )
PROCESS_ERROR( ReturningUnallowedReference )
PROCESS_ERROR( InvalidReferenceTagCount )
PROCESS_ERROR( SelfReferencePollution )
PROCESS_ERROR( ArgReferencePollution )
PROCESS_ERROR( MutableReferencePollutionOfOuterLoopVariable )
PROCESS_ERROR( OuterVariableMoveInsideLoop )
PROCESS_ERROR( ConditionalMove )
PROCESS_ERROR( MovedVariableHaveReferences )
PROCESS_ERROR( UnallowedReferencePollution )
PROCESS_ERROR( ReferencePollutionForArgReference )
PROCESS_ERROR( ExplicitReferencePollutionForCopyConstructor )
PROCESS_ERROR( ExplicitReferencePollutionForCopyAssignmentOperator )
PROCESS_ERROR( ReferenceFiledOfTypeWithReferencesInside )

// Operators overloading
PROCESS_ERROR( OperatorDeclarationOutsideClass )
PROCESS_ERROR( OperatorDoesNotHaveParentClassArguments )
PROCESS_ERROR( InvalidArgumentCountForOperator )
PROCESS_ERROR( InvalidReturnTypeForOperator )

// Enums
PROCESS_ERROR( UnderlayingTypeForEnumIsTooSmall )

// Inheritance errors
PROCESS_ERROR( CanNotDeriveFromThisType )
PROCESS_ERROR( DuplicatedParentClass )
PROCESS_ERROR( DuplicatedBaseClass )
PROCESS_ERROR( FieldsForInterfacesNotAllowed )
PROCESS_ERROR( BaseClassForInterface )
PROCESS_ERROR( ConstructorForInterface )

// Auto functions errors
PROCESS_ERROR( ExpectedBodyForAutoFunction )
PROCESS_ERROR( AutoFunctionInsideClassesNotAllowed )

// Virtual functions errors
PROCESS_ERROR( VirtualForNonclassFunction )
PROCESS_ERROR( VirtualForNonThisCallFunction )
PROCESS_ERROR( VirtualForNonpolymorphClass )
PROCESS_ERROR( FunctionCanNotBeVirtual )
PROCESS_ERROR( VirtualRequired )
PROCESS_ERROR( OverrideRequired )
PROCESS_ERROR( FunctionDoesNotOverride )
PROCESS_ERROR( OverrideFinalFunction )
PROCESS_ERROR( FinalForFirstVirtualFunction )
PROCESS_ERROR( BodyForPureVirtualFunction )
PROCESS_ERROR( ClassContainsPureVirtualFunctions )
PROCESS_ERROR( NonPureVirtualFunctionInInterface )
PROCESS_ERROR( PureDestructor )
PROCESS_ERROR( VirtualForPrivateFunction )
PROCESS_ERROR( VirtualForFunctionTemplate )
PROCESS_ERROR( VirtualForFunctionImplementation )
PROCESS_ERROR( VirtualMismatch )

// NoMangle
PROCESS_ERROR( NoMangleForNonglobalFunction )
PROCESS_ERROR( NoMangleMismatch )

// Unsafe
PROCESS_ERROR( UnsafeFunctionCallOutsideUnsafeBlock )
PROCESS_ERROR( ExplicitAccessToThisMethodIsUnsafe )
PROCESS_ERROR( UnsafeReferenceCastOutsideUnsafeBlock )
PROCESS_ERROR( MutableReferenceCastOutsideUnsafeBlock )
PROCESS_ERROR( UninitializedInitializerOutsideUnsafeBlock )
