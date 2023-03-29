
// Errors list.
// Usage - define macro PROCESS_ERROR, include this file, undefune macro PROCESS_ERROR.
PROCESS_ERROR( BuildFailed, "Build failed" ) // Common error code for all reasons.


PROCESS_ERROR( TemplateContext, "Requered from here" )
PROCESS_ERROR( MacroExpansionContext, "Requered from here" )

PROCESS_ERROR( NameNotFound, "\"{0}\" was not declarated in this scope." )
PROCESS_ERROR( UsingKeywordAsName, "Using keyword as name." )
PROCESS_ERROR( Redefinition, "\"{0}\" redefinition." )
PROCESS_ERROR( UnknownNumericConstantType, "Unknown numeric constant type - \"{0}\"." )
PROCESS_ERROR( UnknownStringLiteralSuffix, "Unknown string literal suffix - \"{0}\"." )
PROCESS_ERROR( InvalidSizeForCharLiteral, "Invalid char literal - \"{0}\", expected literal with exactly one symbol." )
PROCESS_ERROR( OperationNotSupportedForThisType, "Operation is not supported for type \"{0}\"." )
PROCESS_ERROR( TypesMismatch, "Unexpected type, expected \"{0}\", got \"{1}\"." )
PROCESS_ERROR( NoMatchBinaryOperatorForGivenTypes, "No match operator \"{2}\" for types \"{1}\" and \"{0}\"" )
PROCESS_ERROR( NotImplemented, "Sorry, {0} not implemented." )
PROCESS_ERROR( ArraySizeIsNegative, "Array size is neagative." )
PROCESS_ERROR( ArraySizeIsNotInteger, "Array size is not integer." )
PROCESS_ERROR( BreakOutsideLoop, "Break outside loop." )
PROCESS_ERROR( ContinueOutsideLoop, "Continue outside loop." )
PROCESS_ERROR( NameIsNotTypeName, "\"{0}\" is not type name." )
PROCESS_ERROR( UnreachableCode, "Unreachable code." )
PROCESS_ERROR( NoReturnInFunctionReturningNonVoid, "No return in function returning non-void." )
PROCESS_ERROR( ExpectedInitializer, "Expected initializer or constructor for \"{0}\"." )
PROCESS_ERROR( ExpectedReferenceValue, "Expected reference value." )
PROCESS_ERROR( BindingConstReferenceToNonconstReference, "Binding constant reference to non-constant reference." )
PROCESS_ERROR( ExpectedVariable, "Expected variable, got \"{0}\"." )
PROCESS_ERROR( MutableGlobalReferencesAreNotAllowed, "Mutable global references are not allowed." )

PROCESS_ERROR( InvalidFunctionArgumentCount, "Invalid function argument count. Required {0}, got {1}." )
PROCESS_ERROR( CouldNotOverloadFunction, "Could not overload function." )
PROCESS_ERROR( TooManySuitableOverloadedFunctions, "Could not select function for overloading - too many candidates. Args are: {0}." )
PROCESS_ERROR( CouldNotSelectOverloadedFunction, "Could not select function for overloading - no candidates. Args are: {0}." )
PROCESS_ERROR( FunctionPrototypeDuplication, "Duplicated prototype of function \"{0}\"." )
PROCESS_ERROR( FunctionBodyDuplication, "Body fo function \"{0}\" already exists." )
PROCESS_ERROR( BodyForGeneratedFunction, "Body for generated function \"{0}\"." )
PROCESS_ERROR( BodyForDeletedFunction, "Body for deleted function \"{0}\"." )
PROCESS_ERROR( FunctionDeclarationOutsideItsScope, "Function declaration outside it's scope." )
PROCESS_ERROR( ClassDeclarationOutsideItsScope, "Class declaration outside its scope." )
PROCESS_ERROR( UsingIncompleteType, "Using incomplete type \"{0}\", expected complete type." )
PROCESS_ERROR( GlobalsLoopDetected, "Globals loop detected:\n{0}" )

// Visibility
PROCESS_ERROR( AccessingNonpublicClassMember, "Accessing member \"{0}\" of class \"{1}\"." )
PROCESS_ERROR( FunctionsVisibilityMismatch, "Visibility mismatch for function \"{0}\". All functions with same name in class must have same visibility." )
PROCESS_ERROR( TypeTemplatesVisibilityMismatch, "Visibility mismatch for type template \"{0}\". All type templates with same name in class must have same visibility." )
PROCESS_ERROR( VisibilityForStruct, "Visibility label for struct \"{0}\". Visibility labels enabled only for classes, structs have all members public." )
PROCESS_ERROR( ThisMethodMustBePublic, "Method \"{0}\" must be public." )

// Constexpr errors.
PROCESS_ERROR( ExpectedConstantExpression, "Expected constant expression." )
PROCESS_ERROR( VariableInitializerIsNotConstantExpression, "Variable declaration is not constant expression." )
PROCESS_ERROR( InvalidTypeForConstantExpressionVariable, "Invalid type for constant expression variable." )
PROCESS_ERROR( ConstantExpressionResultIsUndefined, "Constant expression result is undefined." )
PROCESS_ERROR( ConstexprFunctionEvaluationError, "Constexpr function evaluation error: {0}." )
PROCESS_ERROR( ConstexprFunctionContainsUnallowedOperations, "Constexpr function contains unallowed operatios." )
PROCESS_ERROR( InvalidTypeForConstexprFunction, "Invalid type for constexpr function." )
PROCESS_ERROR( ConstexprFunctionsMustHaveBody, "Constexpr function must have body." )
PROCESS_ERROR( ConstexprFunctionCanNotBeVirtual, "Constexpr function can not be virtual." )

// Static assert errors.
PROCESS_ERROR( StaticAssertExpressionMustHaveBoolType, "static_assert expression must have bool type." )
PROCESS_ERROR( StaticAssertExpressionIsNotConstant, "Expression in static_assert is non constant." )
PROCESS_ERROR( StaticAssertionFailed, "Static assertion failed." )

// Compile-time checks.
PROCESS_ERROR( ArrayIndexOutOfBounds, "Array index out of bounds. Index is {0}, but array contains only {1} elements." )
PROCESS_ERROR( TupleIndexOutOfBounds, "Tuple index out of bounds. Index is {0}, but tuple contains only {1} elements." )

// Initializers errors.
PROCESS_ERROR( ArrayInitializerForNonArray, "Array initializer for not array or tuple." )
PROCESS_ERROR( ArrayInitializersCountMismatch, "Array initializers count mismatch. Expected {0}, got {1}." )
PROCESS_ERROR( TupleInitializersCountMismatch, "Tuple initializers count mismatch. Expected {0}, got {1}." )
PROCESS_ERROR( FundamentalTypesHaveConstructorsWithExactlyOneParameter, "Fundamental types have constructors with exactly one parameter." )
PROCESS_ERROR( ReferencesHaveConstructorsWithExactlyOneParameter, "References have constructors with exactly one parameter." )
PROCESS_ERROR( UnsupportedInitializerForReference, "Unsupported initializer for reference." )
PROCESS_ERROR( ConstructorInitializerForUnsupportedType, "Constructor initializer for unsupported type." )
PROCESS_ERROR( ZeroInitializerForClass, "Zero initializer for class." )
PROCESS_ERROR( StructInitializerForNonStruct, "Structure-initializer for nonstruct." )
PROCESS_ERROR( InitializerForNonfieldStructMember, "Initializer for \"{0}\", which is not a field." )
PROCESS_ERROR( InitializerForBaseClassField, "Initializer for \"{0}\", which is not this class field." )
PROCESS_ERROR( DuplicatedStructMemberInitializer, "Duplicated initializer for \"{0}\"." )
PROCESS_ERROR( InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors, "This kind of initializer disabled for this class, because it have explicit noncopy constructor(s)." )
PROCESS_ERROR( InvalidTypeForAutoVariable, "Invalid type for auto variable: \"{0}\"." )

// Constructors errors
PROCESS_ERROR( ConstructorOrDestructorOutsideClass, "Constructor or destructor outside class." )
PROCESS_ERROR( ConstructorAndDestructorMustReturnVoid, "Constructors and destructors must return void." )
PROCESS_ERROR( ConversionConstructorMustHaveOneArgument, "Conversion constructor must have exactly 1 argument (except \"this\" )." )
PROCESS_ERROR( InitializationListInNonconstructor, "Constructor outside class." )
PROCESS_ERROR( ClassHaveNoConstructors, "Class have no constructors." )
PROCESS_ERROR( FieldIsNotInitializedYet, "Field \"{0}\" is not initialized yet." )

// Destructors errors
PROCESS_ERROR( ExplicitArgumentsInDestructor, "Explicit arguments in destructor." )

// Methods errors.
PROCESS_ERROR( ClassFieldAccessInStaticMethod, "Accessing field \"{0}\" in static method." )
PROCESS_ERROR( ThisInNonclassFunction, "This in nonclass function \"{0}\"." )
PROCESS_ERROR( ThiscallMismatch, "Thiscall for function \"{0}\" does not match to thiscall in prototype." )
PROCESS_ERROR( AccessOfNonThisClassField, "Access field \"{0}\" of non-this class." )
PROCESS_ERROR( ThisUnavailable, "\"this\" unavailable." )
PROCESS_ERROR( BaseUnavailable, "\"base\" unavailable." )
PROCESS_ERROR( InvalidMethodForBodyGeneration, "Invalid method for body generation." )
PROCESS_ERROR( MethodBodyGenerationFailed, "Method body generation failed." )
PROCESS_ERROR( AccessingDeletedMethod, "Accessing deleted method." )

// Template errors.
PROCESS_ERROR( InvalidValueAsTemplateArgument, "Invalid value as template argument. Expected variable of type, got \"{0}\"." )
PROCESS_ERROR( InvalidTypeOfTemplateVariableArgument, "Invalid type for template variable-argument: \"{0}\"." )
PROCESS_ERROR( TemplateParametersDeductionFailed, "Template parameters deduction failed." )
PROCESS_ERROR( ValueIsNotTemplate, "Value is not template." )
PROCESS_ERROR( TemplateInstantiationRequired, "\"{0}\" template instantiation required." )
PROCESS_ERROR( MandatoryTemplateSignatureArgumentAfterOptionalArgument, "Mandatory template signature argument after optional argument." )
PROCESS_ERROR( TemplateArgumentIsNotDeducedYet, "\"{0}\" is not deduced yet." )
PROCESS_ERROR( TemplateArgumentNotUsedInSignature, "Template argument \"{0}\" not used in signature." )
PROCESS_ERROR( TypeTemplateRedefinition, "\"{0}\" redefinition - type template with such signature already exists in current namespace." )
PROCESS_ERROR( IncompleteMemberOfClassTemplate, "\"{0}\" is incomplete." )
PROCESS_ERROR( TemplateFunctionGenerationFailed, "Instantiation of function template \"{0}\" failed." )
PROCESS_ERROR( CouldNotSelectMoreSpicializedTypeTemplate, "Could not select more spicialized type template." )

// Reference checking
PROCESS_ERROR( ReferenceProtectionError, "Reference protection check for variable \"{0}\" failed." )
PROCESS_ERROR( InnerReferenceMutabilityChanging, "Changing inner reference \"{0}\" mutability." )
PROCESS_ERROR( DestroyedVariableStillHaveReferences, "Destroyed variable \"{0}\" still have reference(s)." )
PROCESS_ERROR( AccessingMovedVariable, "Accessing moved variable \"{0}\"." )
PROCESS_ERROR( ReturningUnallowedReference, "Returning unallowed reference." )
PROCESS_ERROR( SelfReferencePollution, "Reference self-pollution." )
PROCESS_ERROR( ArgReferencePollution, "Pollution of arg reference." )
PROCESS_ERROR( ConstructorThisReferencePollution, "Capturing \"this\" reference in constructor." )
PROCESS_ERROR( ReferencePollutionOfOuterLoopVariable, "Reference pollution for outer variables inside loop. \"{0}\" polluted by \"{1}\"." )
PROCESS_ERROR( OuterVariableMoveInsideLoop, "Outer loop variable \"{0}\" move inside loop." )
PROCESS_ERROR( ConditionalMove, "Variable \"{0}\" moved not in all if-else bracnes." )
PROCESS_ERROR( MovedVariableHaveReferences, "Moved variable \"{0}\" have references." )
PROCESS_ERROR( UnallowedReferencePollution, "Unallowed reference pollution." )
PROCESS_ERROR( ReferencePollutionForArgReference, "Pollution of inner reference of argument." )
PROCESS_ERROR( ExplicitReferencePollutionForCopyConstructor, "Explicit reference pollution for copy constructor. Reference pollution for copy constructors generated automatically." )
PROCESS_ERROR( ExplicitReferencePollutionForCopyAssignmentOperator, "Explicit reference pollution for copy assignment operator. Reference pollution for copy assignment operators generated automatically." )
PROCESS_ERROR( ExplicitReferencePollutionForEqualityCompareOperator, "Explicit reference pollution for equality compare operator. Reference pollution for such operators is not allowed." )
PROCESS_ERROR( ReferenceFieldOfTypeWithReferencesInside, "Reference field \"{0}\" have type, with other references inside." )

// Operators overloading
PROCESS_ERROR( OperatorDeclarationOutsideClass, "Operator declaration outside class. Operators can be declared only inside classes." )
PROCESS_ERROR( OperatorDoesNotHaveParentClassArguments, "Operator does not have parent class arguments. At least one argument of operator must have parent class type." )
PROCESS_ERROR( InvalidArgumentCountForOperator, "Invalid argument count for operator." )
PROCESS_ERROR( InvalidReturnTypeForOperator, "Invalid return type for operator, expected \"{0}\" ." )

// Enums
PROCESS_ERROR( UnderlayingTypeForEnumIsTooSmall, "Underlaying type for enum is too small - enum max value is {0}, but type max value is {1}." )

// Inheritance errors
PROCESS_ERROR( CanNotDeriveFromThisType, "Can not derive from \"{0}\"." )
PROCESS_ERROR( DuplicatedParentClass, "Parent class \"{0}\" is duplicated." )
PROCESS_ERROR( DuplicatedBaseClass, "Can not inherit from \"{0}\" because class already have base." )
PROCESS_ERROR( FieldsForInterfacesNotAllowed, "Fields for interfaces not allowed." )
PROCESS_ERROR( BaseClassForInterface, "Base class for interface." )
PROCESS_ERROR( ConstructorForInterface, "Constructor for interface." )
PROCESS_ERROR( ConstructingAbstractClassOrInterface, "Constructing object of class \"{0}\", which is abstract or interface." )
PROCESS_ERROR( MoveAssignForNonFinalPolymorphClass, "Perform move-assignment for non-final polymorph class \"{0}\"." )
PROCESS_ERROR( TakeForNonFinalPolymorphClass, "Taking value of non-final polymorph class \"{0}\"." )
PROCESS_ERROR( NonSyncTagAdditionInInheritance, "Class \"{0}\" is marked as \"non_sync\" or contains \"non_sync\" fields but its parent \"{1}\" is not \"non_sync\". Changing \"non_sync\" property in inheritance is forbidden." )

// Auto functions errors
PROCESS_ERROR( ExpectedBodyForAutoFunction, "Expected body for function \"{0}\", because return type declared as \"auto\"." )
PROCESS_ERROR( AutoFunctionInsideClassesNotAllowed, "\"auto\" for member function \"{0}\"." )

// Virtual functions errors
PROCESS_ERROR( VirtualForNonclassFunction, "Virtual for non-class function \"{0}\"." )
PROCESS_ERROR( VirtualForNonThisCallFunction, "Virtual for non-thiscall function \"{0}\"." )
PROCESS_ERROR( VirtualForNonpolymorphClass, "Function \"{0}\" can not be virtual, because it's class is not polymorph." )
PROCESS_ERROR( FunctionCanNotBeVirtual, "Function \"{0}\" can not be virtual." )
PROCESS_ERROR( VirtualRequired, "\"virtual\" required for function \"{0}\"." )
PROCESS_ERROR( OverrideRequired, "\"override\" required for function \"{0}\"." )
PROCESS_ERROR( FunctionDoesNotOverride, "Function \"{0}\" marked as \"override\", but does not override." )
PROCESS_ERROR( OverrideFinalFunction, "\"override\" for final function \"{0}\"." )
PROCESS_ERROR( FinalForFirstVirtualFunction, "\"final\" for first virtual function \"{0}\"." )
PROCESS_ERROR( BodyForPureVirtualFunction, "Body for pure virtual function \"{0}\"." )
PROCESS_ERROR( ClassContainsPureVirtualFunctions, "Class \"{0}\" is not interface or abstract and contains pure virtual functions." )
PROCESS_ERROR( NonPureVirtualFunctionInInterface, "Interface \"{0}\" contains non-pure virtual functions." )
PROCESS_ERROR( PureDestructor, "Pure destructor for class \"{0}\"." )
PROCESS_ERROR( VirtualForPrivateFunction, "Virtual for private function \"{0}\"." )
PROCESS_ERROR( VirtualForFunctionTemplate, "\"virtual\" for template function \"{0}\"." )
PROCESS_ERROR( VirtualForFunctionImplementation, "\"virtual\" for function implementation \"{0}\"." )
PROCESS_ERROR( VirtualMismatch, "\"virtual\" specifiers mismatch for function \"{0}\"." )
PROCESS_ERROR( FunctionOverridingWithReferencesNotationChange, "Function overriding causes changes in references notation (return references or references pollution)." )

// NoMangle
PROCESS_ERROR( NoMangleForNonglobalFunction, "\"nomangle\" for non-global function \"{0}\"." )
PROCESS_ERROR( NoMangleMismatch, "\"nomangle\" specifiers mismatch for function \"{0}\"." )

// Calling conventions
PROCESS_ERROR( UnknownCallingConvention, "Unknown calling convention \"{0}\"." )
PROCESS_ERROR( NonDefaultCallingConventionForClassMethod, "Only default calling convention allowed for this method." )

// Unsafe
PROCESS_ERROR( UnsafeFunctionCallOutsideUnsafeBlock, "Calling unsafe function outside unsafe block." )
PROCESS_ERROR( ExplicitAccessToThisMethodIsUnsafe, "Explicit access to method \"{0}\" is unsafe." )
PROCESS_ERROR( UnsafeReferenceCastOutsideUnsafeBlock, "Unsafe reference cast outside unsafe block." )
PROCESS_ERROR( MutableReferenceCastOutsideUnsafeBlock, "Mutable reference cast outside unsafe block." )
PROCESS_ERROR( UninitializedInitializerOutsideUnsafeBlock, "Unsafe initializer outside unsafe block." )
PROCESS_ERROR( GlobalMutableVariableAccessOutsideUnsafeBlock, "Accessing global mutable variable outside unsafe block." )
PROCESS_ERROR( ThisMethodCanNotBeUnsafe, "This method can not be unsafe." )
PROCESS_ERROR( UnsafeExpressionInGlobalContext, "Unsafe expression in global context." )

// Raw pointers
PROCESS_ERROR( ValueIsNotReference, "Value is not a reference. Expected mutable or immutable reference, got immediate value." )
PROCESS_ERROR( ValueIsNotPointer, "Value of type \"{0}\" is not a pointer." )
PROCESS_ERROR( RawPointerToReferenceConversionOutsideUnsafeBlock, "Raw pointer to reference conversion outside unsafe block." )
PROCESS_ERROR( DifferenceBetweenRawPointersWithZeroElementSize, "Can not calculate difference between pointers of type \"{0}\" with zero element size." )

// Generators
PROCESS_ERROR( YieldOutsideGenerator, "Yield operator is allowed only inside generator functions." )
PROCESS_ERROR( IfCoroAdvanceForNonCoroutineValue, "if_coro_advance used for non-coroutine value of type \"{0}\"." )
