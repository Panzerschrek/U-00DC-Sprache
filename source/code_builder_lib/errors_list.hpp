
// Errors list.
// Usage - define macro PROCESS_ERROR, include this file, undefune macro PROCESS_ERROR.
PROCESS_ERROR( BuildFailed, "Build failed" ) // Common error code for all reasons.

PROCESS_ERROR( NameNotFound, "\"%1%\" was not declarated in this scope." )
PROCESS_ERROR( UsingKeywordAsName, "Using keyword as name." )
PROCESS_ERROR( Redefinition, "\"%1%\" redefinition." )
PROCESS_ERROR( UnknownNumericConstantType, "Unknown numeric constant type - \"%1%\"." )
PROCESS_ERROR( UnknownStringLiteralSuffix, "Unknown string literal suffix - \"%1%\"." )
PROCESS_ERROR( InvalidSizeForCharLiteral, "Invalid char literal - \"%1%\", expected literal with exactly one symbol." )
PROCESS_ERROR( OperationNotSupportedForThisType, "Operation is not supported for type \"%1%\"." )
PROCESS_ERROR( TypesMismatch, "Unexpected type, expected \"%1%\", got \"%2%\"." )
PROCESS_ERROR( NoMatchBinaryOperatorForGivenTypes, "No match operator \"%1%\" for types \"%2%\" and \"%3%\"" )
PROCESS_ERROR( NotImplemented, "Sorry, %1% not implemented." )
PROCESS_ERROR( ArraySizeIsNegative, "Array size is neagative." )
PROCESS_ERROR( ArraySizeIsNotInteger, "Array size is not integer." )
PROCESS_ERROR( BreakOutsideLoop, "Break outside loop." )
PROCESS_ERROR( ContinueOutsideLoop, "Continue outside loop." )
PROCESS_ERROR( NameIsNotTypeName, "\"%1%\" is not type name." )
PROCESS_ERROR( UnreachableCode, "Unreachable code." )
PROCESS_ERROR( NoReturnInFunctionReturningNonVoid, "No return in function returning non-void." )
PROCESS_ERROR( ExpectedInitializer, "Expected initializer or constructor for \"%1%\"." )
PROCESS_ERROR( ExpectedReferenceValue, "Expected reference value." )
PROCESS_ERROR( BindingConstReferenceToNonconstReference, "Binding constant reference to non-constant reference." )
PROCESS_ERROR( ExpectedVariable, "Expected variable, got \"%1%\"." )

PROCESS_ERROR( InvalidFunctionArgumentCount, "Invalid function argument count. Required %1%, got %2%." )
PROCESS_ERROR( CouldNotOverloadFunction, "Could not overload function." )
PROCESS_ERROR( TooManySuitableOverloadedFunctions, "Could not select function for overloading - too many candidates." )
PROCESS_ERROR( CouldNotSelectOverloadedFunction, "Could not select function for overloading - no candidates." )
PROCESS_ERROR( FunctionPrototypeDuplication, "Duplicated prototype of function \"%1%\"." )
PROCESS_ERROR( FunctionBodyDuplication, "Body fo function \"%1%\" already exists." )
PROCESS_ERROR( BodyForGeneratedFunction, "Body for generated function \"%1%\"." )
PROCESS_ERROR( BodyForDeletedFunction, "Body for deleted function \"%1%\"." )
PROCESS_ERROR( FunctionDeclarationOutsideItsScope, "Function declaration outside it's scope." )
PROCESS_ERROR( ClassDeclarationOutsideItsScope, "Class declaration outside its scope." )
PROCESS_ERROR( ClassBodyDuplication, "Class body duplication." )
PROCESS_ERROR( UsingIncompleteType, "Using incomplete type \"%1%\", expected complete type." )
PROCESS_ERROR( GlobalsLoopDetected, "Globals loop detected:\n%1%" )

// Visibility
PROCESS_ERROR( AccessingNonpublicClassMember, "Accessing member \"%1%\" of class \"%2%\"." )
PROCESS_ERROR( FunctionsVisibilityMismatch, "Visibility mismatch for function \"%1%\". All functions with same name in class must have same visibility." )
PROCESS_ERROR( TypeTemplatesVisibilityMismatch, "Visibility mismatch for type template \"%1%\". All type templates with same name in class must have same visibility." )
PROCESS_ERROR( VisibilityForStruct, "Visibility label for struct \"%1%\". Visibility labels enabled only for classes, structs have all members public." )

// Constexpr errors.
PROCESS_ERROR( ExpectedConstantExpression, "Expected constant expression." )
PROCESS_ERROR( VariableInitializerIsNotConstantExpression, "Variable declaration is not constant expression." )
PROCESS_ERROR( InvalidTypeForConstantExpressionVariable, "Invalid type for constant expression variable." )
PROCESS_ERROR( ConstantExpressionResultIsUndefined, "Constant expression result is undefined." )
PROCESS_ERROR( ConstexprFunctionEvaluationError, "Constexpr function evaluation error: %1%." )
PROCESS_ERROR( ConstexprFunctionContainsUnallowedOperations, "Constexpr function contains unallowed operatios." )
PROCESS_ERROR( InvalidTypeForConstexprFunction, "Invalid type for constexpr function." )
PROCESS_ERROR( ConstexprFunctionsMustHaveBody, "Constexpr function must have body." )
PROCESS_ERROR( ConstexprFunctionCanNotBeVirtual, "Constexpr function can not be virtual." )

// Static assert errors.
PROCESS_ERROR( StaticAssertExpressionMustHaveBoolType, "static_assert expression must have bool type." )
PROCESS_ERROR( StaticAssertExpressionIsNotConstant, "Expression in static_assert is non constant." )
PROCESS_ERROR( StaticAssertionFailed, "Static assertion failed." )

// Compile-time checks.
PROCESS_ERROR( ArrayIndexOutOfBounds, "Array inex out of bounds. Index is %1%, but aray constains only %2% elements." )

// Initializers errors.
PROCESS_ERROR( ArrayInitializerForNonArray, "Array initializer for nonarray." )
PROCESS_ERROR( ArrayInitializersCountMismatch, "Array initializers count mismatch. Expected %1%, got %2%." )
PROCESS_ERROR( FundamentalTypesHaveConstructorsWithExactlyOneParameter, "Fundamental types have constructors with exactly one parameter." )
PROCESS_ERROR( ReferencesHaveConstructorsWithExactlyOneParameter, "References have constructors with exactly one parameter." )
PROCESS_ERROR( UnsupportedInitializerForReference, "Unsupported initializer for reference." )
PROCESS_ERROR( ConstructorInitializerForUnsupportedType, "Constructor initializer for unsupported type." )
PROCESS_ERROR( ZeroInitializerForClass, "Zero initializer for class." )
PROCESS_ERROR( StructInitializerForNonStruct, "Structure-initializer for nonstruct." )
PROCESS_ERROR( InitializerForNonfieldStructMember, "Initializer for \"%1%\", which is not a field." )
PROCESS_ERROR( InitializerForBaseClassField, "Initializer for \"%1%\", which is not this class field." )
PROCESS_ERROR( DuplicatedStructMemberInitializer, "Duplicated initializer for \"%1%\"." )
PROCESS_ERROR( InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors, "This kind of initializer disabled for this class, because it have explicit noncopy constructor(s)." )
PROCESS_ERROR( InvalidTypeForAutoVariable, "Invalid type for auto variable: \"%1%\"." )
PROCESS_ERROR( GlobalVariableMustBeConstexpr, "Global variable \"%1%\" must be constexpr." )

// Constructors errors
PROCESS_ERROR( ConstructorOrDestructorOutsideClass, "Constructor or destructor outside class." )
PROCESS_ERROR( ConstructorAndDestructorMustReturnVoid, "Constructors and destructors must return void." )
PROCESS_ERROR( ConversionConstructorMustHaveOneArgument, "Conversion constructor must have exactly 1 argument (except \"this\" )." )
PROCESS_ERROR( InitializationListInNonconstructor, "Constructor outside class." )
PROCESS_ERROR( ClassHaveNoConstructors, "Class have no constructors." )
PROCESS_ERROR( ExplicitThisInDestructor, "Explicit \"this\" in destructor parameters." )
PROCESS_ERROR( FieldIsNotInitializedYet, "Field \"%1%\" is not initialized yet." )

// Destructors errors
PROCESS_ERROR( ExplicitArgumentsInDestructor, "Explicit arguments in destructor." )

// Methods errors.
PROCESS_ERROR( CallOfThiscallFunctionUsingNonthisArgument, "Call of \"thiscall\" function using nonthis argument." )
PROCESS_ERROR( ClassFiledAccessInStaticMethod, "Accessing field \"%1%\" in static method." )
PROCESS_ERROR( ThisInNonclassFunction, "This in nonclass function \"%1%\"." )
PROCESS_ERROR( ThiscallMismatch, "Thiscall for function \"%1%\" does not match to thiscall in prototype." )
PROCESS_ERROR( AccessOfNonThisClassField, "Access filed \"%1%\" of non-this class." )
PROCESS_ERROR( ThisUnavailable, "\"this\" unavailable." )
PROCESS_ERROR( BaseUnavailable, "\"base\" unavailable." )
PROCESS_ERROR( InvalidMethodForBodyGeneration, "Invalid method for body generation." )
PROCESS_ERROR( MethodBodyGenerationFailed, "Method body generation failed." )
PROCESS_ERROR( AccessingDeletedMethod, "Accessing deleted method." )

// Template errors.
PROCESS_ERROR( InvalidValueAsTemplateArgument, "Invalid value as template argument. Expected variable of type, got \"%1%\"." )
PROCESS_ERROR( InvalidTypeOfTemplateVariableArgument, "Invalid type for template variable-argument: \"%1%\"." )
PROCESS_ERROR( TemplateParametersDeductionFailed, "Template parameters deduction failed." )
PROCESS_ERROR( DeclarationShadowsTemplateArgument, "Declaration of \"%1%\" shadows template argument." )
PROCESS_ERROR( ValueIsNotTemplate, "Value is not template." )
PROCESS_ERROR( TemplateInstantiationRequired, "\"%1%\" template instantiation required." )
PROCESS_ERROR( MandatoryTemplateSignatureArgumentAfterOptionalArgument, "Mandatory template signature argument after optional argument." )
PROCESS_ERROR( TemplateArgumentIsNotDeducedYet, "\"%1%\" is not deduced yet." )
PROCESS_ERROR( TemplateArgumentNotUsedInSignature, "Template argument \"%1%\" not used in signature." )
PROCESS_ERROR( IncompleteMemberOfClassTemplate, "\"%1%\" is incomplete." )
PROCESS_ERROR( TemplateFunctionGenerationFailed, "Instantiation of function template \"%1%\" failed." )
PROCESS_ERROR( CouldNotSelectMoreSpicializedTypeTemplate, "Could not select more spicialized type template." )

// Reference checking
PROCESS_ERROR( ReferenceProtectionError, "Reference protection check for variable \"%1%\" failed." )
PROCESS_ERROR( DestroyedVariableStillHaveReferences, "Destroyed variable \"%1%\" still have reference(s)." )
PROCESS_ERROR( AccessingMovedVariable, "Accessing moved variable \"%1%\"." )
PROCESS_ERROR( ReturningUnallowedReference, "Returning unallowed reference." )
PROCESS_ERROR( InvalidReferenceTagCount, "Invalid reference tag count, expected %1%, got %2%." )
PROCESS_ERROR( SelfReferencePollution, "Reference self-pollution." )
PROCESS_ERROR( ArgReferencePollution, "Pollution of arg reference." )
PROCESS_ERROR( MutableReferencePollutionOfOuterLoopVariable, "Mutable reference pollution for outer variables inside loop. \"%1%\" polluted by \"%2%\"." )
PROCESS_ERROR( OuterVariableMoveInsideLoop, "Outer loop variable \"%1%\" move inside loop." )
PROCESS_ERROR( ConditionalMove, "Variable \"%1%\" moved not in all if-else bracnes." )
PROCESS_ERROR( MovedVariableHaveReferences, "Moved variable \"%1%\" have references." )
PROCESS_ERROR( UnallowedReferencePollution, "Unallowed reference pollution." )
PROCESS_ERROR( ReferencePollutionForArgReference, "Pollution of inner reference of argument." )
PROCESS_ERROR( ExplicitReferencePollutionForCopyConstructor, "Explicit reference pollution for copy constructor. Reference pollution for copy constructors generated automatically." )
PROCESS_ERROR( ExplicitReferencePollutionForCopyAssignmentOperator, "Explicit reference pollution for copy assignment operator. Reference pollution for copy assignment operators generated automatically." )
PROCESS_ERROR( ReferenceFiledOfTypeWithReferencesInside, "Reference filed \"%1%\" have type, with other references inside." )

// Operators overloading
PROCESS_ERROR( OperatorDeclarationOutsideClass, "Operator declaration outside class. Operators can be declared only inside classes." )
PROCESS_ERROR( OperatorDoesNotHaveParentClassArguments, "Operator does not have parent class arguments. At least one argument of operator must have parent class type." )
PROCESS_ERROR( InvalidArgumentCountForOperator, "Invalid argument count for operator." )
PROCESS_ERROR( InvalidReturnTypeForOperator, "Invalid return type for operator, expected \"%1%\" ." )

// Enums
PROCESS_ERROR( UnderlayingTypeForEnumIsTooSmall, "Underlaying type for enum is too small - enum max value is %1%, but type max value is %2%." )

// Inheritance errors
PROCESS_ERROR( CanNotDeriveFromThisType, "Can not derive from \"%1%\"." )
PROCESS_ERROR( DuplicatedParentClass, "Parent class \"%1%\" is duplicated." )
PROCESS_ERROR( DuplicatedBaseClass, "Can not inherit from \"%1%\" because class already have base." )
PROCESS_ERROR( FieldsForInterfacesNotAllowed, "Fields for interfaces not allowed." )
PROCESS_ERROR( BaseClassForInterface, "Base class for interface." )
PROCESS_ERROR( ConstructorForInterface, "Constructor for interface." )

// Auto functions errors
PROCESS_ERROR( ExpectedBodyForAutoFunction, "Expected body for function \"%1%\", because return type declared as \"auto\"." )
PROCESS_ERROR( AutoFunctionInsideClassesNotAllowed, "\"auto\" for member function \"%1%\"." )

// Virtual functions errors
PROCESS_ERROR( VirtualForNonclassFunction, "Virtual for non-class function \"%1%\"." )
PROCESS_ERROR( VirtualForNonThisCallFunction, "Virtual for non-thiscall function \"%1%\"." )
PROCESS_ERROR( VirtualForNonpolymorphClass, "Function \"%1%\" can not be virtual, because it's class is not polymorph." )
PROCESS_ERROR( FunctionCanNotBeVirtual, "Function \"%1%\" can not be virtual." )
PROCESS_ERROR( VirtualRequired, "\"virtual\" required for function \"%1%\"." )
PROCESS_ERROR( OverrideRequired, "\"override\" required for function \"%1%\"." )
PROCESS_ERROR( FunctionDoesNotOverride, "Function \"%1%\" marked as \"override\", but does not override." )
PROCESS_ERROR( OverrideFinalFunction, "\"override\" for final function \"%1%\"." )
PROCESS_ERROR( FinalForFirstVirtualFunction, "\"final\" for first virtual function \"%1%\"." )
PROCESS_ERROR( BodyForPureVirtualFunction, "Body for pure virtual function \"%1%\"." )
PROCESS_ERROR( ClassContainsPureVirtualFunctions, "Class \"%1%\" is not interface or abstract and contains pure virtual functions." )
PROCESS_ERROR( NonPureVirtualFunctionInInterface, "Interface \"%1%\" contains non-pure virtual functions." )
PROCESS_ERROR( PureDestructor, "Pure destructor for class \"%1%\"." )
PROCESS_ERROR( VirtualForPrivateFunction, "Virtual for private function \"%1%\"." )
PROCESS_ERROR( VirtualForFunctionTemplate, "\"virtual\" for template function \"%1%\"." )
PROCESS_ERROR( VirtualForFunctionImplementation, "\"virtual\" for function implementation \"%1%\"." )
PROCESS_ERROR( VirtualMismatch, "\"virtual\" specifiers mismatch for function \"%1%\"." )

// NoMangle
PROCESS_ERROR( NoMangleForNonglobalFunction, "\"nomangle\" for non-global function \"%1%\"." )
PROCESS_ERROR( NoMangleMismatch, "\"nomangle\" specifiers mismatch for function \"%1%\"." )

// Unsafe
PROCESS_ERROR( UnsafeFunctionCallOutsideUnsafeBlock, "Calling unsafe function outside unsafe block." )
PROCESS_ERROR( ExplicitAccessToThisMethodIsUnsafe, "Explicit access to method \"%1%\" is unsafe." )
PROCESS_ERROR( UnsafeReferenceCastOutsideUnsafeBlock, "Unsafe reference cast outside unsafe block." )
PROCESS_ERROR( MutableReferenceCastOutsideUnsafeBlock, "Mutable reference cast outside unsafe block." )
PROCESS_ERROR( UninitializedInitializerOutsideUnsafeBlock, "Unsafe initializer outside unsafe block." )
