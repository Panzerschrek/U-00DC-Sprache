#include <iostream>

#include "../../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../../code_builder_lib/pop_llvm_warnings.hpp"

#include "../../tests/tests_common.hpp"
#include "../../tests/cpp_tests/tests.hpp"
#include  "../tests_common/funcs_c.hpp"

namespace U
{

namespace
{

llvm::ManagedStatic<llvm::LLVMContext> g_llvm_context;

// Returns "true" if should enable test.
bool FilterTest( const std::string& test_name )
{
	static const std::unordered_set<std::string> c_test_to_enable
	{
		"ArrayAsTemplateSignatureParameter_Test0",
		"ArrayAsTemplateSignatureParameter_Test1",
		"ArrayAsTemplateSignatureParameter_Test2",
		"BracketExpressionAsTemplateSignatureParameter_Test0",
		"ClassmethodsManglingTest",
		"ClassTemplateTest0",
		"ClassTemplateTest1",
		"ClassTemplateTest2",
		"ClassTemplateTest3",
		"ClassTemplateTest4",
		"ClassTemplateTest5",
		"ClassTemplateTest7",
		"ClassTemplateTest14",
		"ClassTemplateTest15",
		"ClassTemplateTest16",
		"DestructorMustReturnVoidTest0",
		"DestructorOutsideClassTest0",
		"DestructorsTest0",
		"DestructorsTest1",
		"DestructorsTest2",
		"DestructorsTest3",
		"DestructorsTest4",
		"DestructorsTest5",
		"DestructorsTest6",
		"DestructorsTest7",
		"DestructorsTest8",
		"DestructorsTest9",
		"DestructorsTest10",
		"DestructorsTest12_ShouldCorrectlyReturnValueFromDestructibleStruct",
		"DestructorsTest13_ShouldBeDesdtroyedAfterUsage0",
		"DestructorsTest14_ShouldBeDesdtroyedAfterUsage1",
		"DestructorsTest15_ShouldBeDesdtroyedAfterUsage2",
		"DestructorsTest16_ShouldBeDesdtroyedAfterUsage3",
		"DestructorsTest17_ShouldBeDesdtroyedAfterUsage4",
		"DestructorsTest18_ShouldBeDesdtroyedAfterUsage5",
		"ExplicitArgumentsInDestructorTest1",
		"FunctionBodyDuplication_ForDestructors_Test0",
		"FunctionsParametersManglingTest",
		"FundamentalTypesManglingTest",
		"GlobalVariablesManglingTest0",
		"NamesCompressionTest",
		"NamespacesManglingTest",
		"NumericConstantAsTemplateSignatureParameter_Test0",
		"NumericConstantAsTemplateSignatureParameter_Test1",
		"NumericConstantAsTemplateSignatureParameter_Test2",
		"ShortClassTemplateForm_Test0",
		"ShortClassTemplateForm_Test1",
		"ShortClassTemplateForm_Test2",
		"ShortClassTemplateForm_Test3",
		"ShortClassTemplateForm_Test4",
		"VariableExpressionAsTemplateSignatureParameter_Test0",
		"VariableExpressionAsTemplateSignatureParameter_Test1",
	};

	const std::string test_name_without_file_name= test_name.substr(test_name.find_last_of(':') + 1);
	if( c_test_to_enable.count( test_name_without_file_name ) != 0 )
		return true;

	static const std::string c_tests_to_enable_pattern[]
	{
		"AccessingReferenceInsideMethodUsingImplicitThis",
		"AdditionalSymbolsForIdentifiersTest0",
		"AdditiveOperationsTest",
		"ArgumentsAssignmentTest",
		"ArrayIndexOutOfBoundsTest0",
		"ArrayInitializerForFundamentalTypesTest",
		"ArrayInitializerForNonArrayTest",
		"ArrayInitializersCountMismatchTest",
		"ArrayOutOfBoundsShouldHalt",
		"ArraysTest",
		"AssignMutableReferenceInsideClass",
		"AssignToImmutableReferenceInsideStruct_Test",
		"BasicBinaryOperationsFloatTest",
		"BasicBinaryOperationsTest",
		"BasicReferenceInsideClassUsage",
		"BitwiseNotTest",
		"BindingConstReferenceToNonconstReferenceTest",
		"BindingConstReferenceToNonConstReference_InReferenceFieldInitialization_Test",
		"BindValueToConstReferenceTest0",
		"BlocksTest",
		"BooleanBasicTest",
		"BreakOperatorTest",
		"BreakOutsideLoopTest",
		"CallTest",
		"ComparisonFloatOperatorsTest",
		"ComparisonSignedOperatorsTest",
		"ComparisonUnsignedOperatorsTest",
		"ConstantExpressionResultIsUndefinedTest0",
		"ConstexprReferenceTest",
		"ConstexprTest",
		"ConstructorInitializerForFundamentalTypesTest",
		"ConstructorInitializerForReferencesTest0",
		"ConstructorInitializerForUnsupportedTypeTest0",
		"ConstructorOutsideClassTest",
		"ConstructorMustReturnVoidTest",
		"ConstructorTest",
		"ContinueOperatorTest",
		"ContinueOutsideLoopTest",
		"CopyAssignmentOperatorForStructsWithReferencesDeleted",
		"CouldNotOverloadFunctionTest",
		"CouldNotSelectOverloadedFunction",
		"DefaultConstructorNotFoundTest",
		"DefaultInitializationForStructMembersTest",
		"DestructorOutsideClassTest0",
		"DuplicatedStructMemberInitializerTest0",
		"DuplicatedStructMemberInitializer_InConstructors_Test0",
		"EmptyInitializerTest0",
		"EnumAsClassFiled",
		"EnumsCompareTest",
		"EnumsAssignmentAndReturnTest",
		"EnumsDeclarationTest",
		"EnumsRestrictionsTest",
		"EnumToIntConversionTest",
		"EnumValueAsArgument",
		"EnumValueRefTest",
		"EqualityFloatOperatorsTest",
		"EqualityOperatorsTest",
		"ExpectedInitializerTest",
		"ExpectedVariableAsArgumentTest0",
		"ExpectedVariableInAdditiveAssignmentTest0",
		"ExpectedVariableInArraySizeTest0",
		"ExpectedVariableInAssignmentTest0",
		"ExpectedVariableInBinaryOperatorTest0",
		"ExpectedVariableInIncrementOrDecrementTest0",
		"ExpectedVariable_InStructReferenceInitialization",
		"ExpressionInitializerTest0",
		"ExpressionInitializerTest1",
		"ExpressionInitializerTest2",
		"ExpressionInitializerTest3",
		"ExpectedConstantExpressionTest",
		"ExpectedInitializer_InConstructors_Test0",
		"ExpectedReferenceValue_ForConstexpr_Test",
		"ExpectedReferenceValue_InStructReferenceInitialization",
		"FieldIsNotInitializedYetTest0",
		"FieldIsNotInitializedYetTest1",
		"FunctionBodyDuplicationTest",
		"FunctionDeclarationOutsideItsScopeTest",
		"FunctionsOverloadingTest",
		"FunctionPrototypeDuplicationTest",
		"FunctionPrototypeTest",
		"FundamentalTypesHaveConstructorsWithExactlyOneParameterTest",
		"GeneratedCopyConstructorForStructsWithReferencesTest",
		"GlobalStaticAssert_Test0",
		"GlobalStaticAssert_Test1",
		"GlobalVariableMustBeConstexpr_Test0",
		"GlobalVariablesTest0",
		"GlobalVariablesTest2_ShouldTakeAddressOfGlobalVariable",
		"GlobalVariablesTest3_ShouldTakeAddressOfGlobalAutoVariable",
		"GlobalVariablesTest4_GlobalConstantArray",
		"GlobalVariablesTest5_GlobalReferences",
		"GlobalVariablesTest6_GlobalVariableUsedInOtherGlobalVariableInitializer",
		"GlobalVariablesTest7_ImutGlobalVariablesWithConstantInitializers",
		"GlobalVariablesTest8_GlobalVariablesInsideNamespaces",
		"GlobalVariablesTest9_GlobalVariablesInsideClasses",
		"Halt",
		"IfOperatorTest",
		"ImmutableClassField_Test",
		"ImplicitConstexprTest",
		"IncrementTest",
		"InitializationListInNonconstructorTest",
		"InitializationOfEnumVariables",
		"InitializerDisabledBecauseClassHaveExplicitNoncopyConstructorsTest0",
		"InitializerForNonfieldStructMember_InConstructors_Test0",
		"InnerClassTest0",
		"InnerClassTest1",
		"InnerClassTest2",
		"InvalidTypeForAutoVariableTest",
		"InvalidTypeForConstantExpressionVariableTest0",
		"LazyLogical",
		"lazyLogicalCombinedTest",
		"LeftShift",
		"LogicalBinaryOperationsTest",
		"LogicalNotTest",
		"MethodsCallInConstructorInitializerListIsForbiddenTest0",
		"MultipleMutableReferencesInside",
		"MultipleReferencesInside",
		"NameIsNotTypeNameTest",
		"NameIsNotTypeName_ForTypedef_Test",
		"NameIsNotTypeName_ForUnderlayingEnumType_Test",
		"NameNotFoundTest",
		"NameNotFound_ForEnumElement",
		"NameNotFound_ForTypedef_Test",
		"NameNotFound_ForUnderlayingEnumType_Test",
		"NamespacesTest",
		"NoMatchBinaryOperatorForGivenTypesTest",
		"NoReturnInFunctionReturningNonVoidTest",
		"NumericConstantsTest",
		"OperationNotSupportedForThisTypeTest",
		"OperatorsPriorityTest",
		"RecursiveCallTest",
		"Redefinition0",
		"Redefinition1",
		"Redefinition2",
		"Redefinition3",
		"Redefinition5",
		"Redefinition_ForEnumMembers",
		"Redefinition_ForEnums",
		"RedefinitionTest",
		"Redefenition_ForNamespaces_Test0",
		"Redefinition_ForTypedef_Test",
		"ReferenceClassFiledDeclaration",
		"ReferencesHaveConstructorsWithExactlyOneParameterTest",
		"RemOperatorTest",
		"RightShift",
		"SimpliestProgramTest",
		"SimpleProgramTest",
		"StaticAssertExpression",
		"StaticAssertInsideClass_Test0",
		"StaticAssertInsideClass_Test1",
		"StaticAssertTest",
		"StructInitializerForNonStructTest0",
		"StructNamedInitializersTest",
		"StructsWithReferencesHaveNoGeneratedDefaultConstructor",
		"TempVariableConstructionTest",
		"ThiscallMismatch_Test0",
		"ThisUnavailable_InConstructors_Test",
		"TwodimensionalArrayInitializerTest0",
		"TypeConversionTest",
		"TypedefsTest0",
		"TypedefsTest1_TypedefForFundamentalType",
		"TypedefsTest2_TypedefForFundamentalTypeInsideClass",
		"TypedefsTest3_TypedefForStruct",
		"TypedefsTest5_TypedefForArray",
		"TypedefsTest6_TypedefForTwodimensionalArray",
		"TypesMismatchTest0",
		"TypesMismatchTest2",
		"TypesMismatchTest3",
		"TypesMismatchTest4",
		"TypesMismatchTest5",
		"TypesMismatchTest6",
		"TypesMismatchTest7",
		"TypesMismatchTest8",
		"TypesMismatchTest9",
		"TypesMismatchTest10",
		"TypesMismatchTest11",
		"TypesMismatch_ForUnderlayingEnumType_Test",
		"UnaryMinusFloatTest",
		"UnaryMinusTest",
		"UnaryOperatorsOrderTest",
		"UnderlayingTypeIsTooSmall_Test",
		"UnreachableCodeTest",
		"UnsupportedInitializerForReferenceTest0",
		"UsingKeywordAsName0",
		"UsingKeywordAsName1",
		"UsingKeywordAsName2",
		"UsingKeywordAsName3",
		"UsingKeywordAsName5",
		"UsingKeywordAsName6",
		"UsingKeywordAsName_ForTypedef_Test",
		"ViariableInitializerIsNotConstantExpressionTest",
		"WhileOperatorTest",
		"ZeroInitConstexprTest",
		"ZeroInitializerForReferenceField_Test",
		"ZeroInitilaizerTest",

		"auto_variables_errors_test.cpp:BindingConstReferenceToNonconstReferenceTest",
		"auto_variables_errors_test.cpp:ExpectedReferenceValueTest",
		"auto_variables_test.cpp:AutoVariableTest",
		"code_builder_test.cpp:ReferencesTest",
		"code_builder_test.cpp:StructTest0",
		"code_builder_test.cpp:StructTest1",
		"code_builder_test.cpp:TypesMismatchTest1",
		"code_builder_test.cpp:VariablesTest0",
		"code_builder_test.cpp:VariablesTest1",
		"code_builder_errors_test.cpp:ExpectedReferenceValueTest",
		"methods_test.cpp:MethodTest",
		"operators_test.cpp:DecrementTest",
	};

	if( std::find_if(
			std::begin(c_tests_to_enable_pattern),
			std::end(c_tests_to_enable_pattern),
			[&]( const std::string& pattern ) { return test_name.find( pattern ) != std::string::npos; } )
		!= std::end(c_tests_to_enable_pattern) )
		return true;

	return false;
}

} // namespace

std::unique_ptr<llvm::Module> BuildProgram( const char* const text )
{
	llvm::LLVMContext& llvm_context= *g_llvm_context;

	llvm::DataLayout data_layout( GetTestsDataLayout() );

	auto ptr=
		U1_BuildProgram(
			text,
			std::strlen(text),
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout) );
	U_TEST_ASSERT( ptr != nullptr );

	return std::unique_ptr<llvm::Module>( reinterpret_cast<llvm::Module*>(ptr) );
}

ICodeBuilder::BuildResult BuildProgramWithErrors( const char* const text )
{
	llvm::LLVMContext& llvm_context= *g_llvm_context;

	llvm::DataLayout data_layout( GetTestsDataLayout() );

	ICodeBuilder::BuildResult build_result;
	const auto error_handler=
	[](
		void* const data,
		const uint32_t line,
		const uint32_t column,
		const uint32_t error_code,
		const char* const error_text,
		const size_t error_text_length )
	{
		CodeBuilderError error;
		error.file_pos= FilePos( 0u, line, column );
		error.code= CodeBuilderErrorCode(error_code);
		error.text= std::string( error_text, error_text + error_text_length );
		reinterpret_cast<ICodeBuilder::BuildResult*>(data)->errors.push_back( std::move(error) );
	};

	const bool ok=
		U1_BuildProgramWithErrors(
			text,
			std::strlen(text),
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout),
			error_handler,
			&build_result );
	U_TEST_ASSERT(ok);

	return build_result;
}

std::unique_ptr<llvm::Module> BuildMultisourceProgram( std::vector<SourceEntry> sources, const std::string& root_file_path )
{
	(void)sources;
	(void)root_file_path;
	DISABLE_TEST;
}

ICodeBuilder::BuildResult BuildMultisourceProgramWithErrors( std::vector<SourceEntry> sources, const std::string& root_file_path )
{
	(void)sources;
	(void)root_file_path;
	DISABLE_TEST;
}

EnginePtr CreateEngine( std::unique_ptr<llvm::Module> module, const bool needs_dump )
{
	U_TEST_ASSERT( module != nullptr );

	if( needs_dump )
	{
		llvm::raw_os_ostream stream(std::cout);
		module->print( stream, nullptr );
	}

	llvm::EngineBuilder builder( std::move(module) );
	llvm::ExecutionEngine* const engine= builder.create();

	// llvm engine builder uses "new" operator inside it.
	// So, we can correctly use unique_ptr for engine, because unique_ptr uses "delete" operator in destructor.

	U_TEST_ASSERT( engine != nullptr );
	return EnginePtr(engine);
}

bool HaveError( const std::vector<CodeBuilderError>& errors, const CodeBuilderErrorCode code, const unsigned int line )
{
	for( const CodeBuilderError& error : errors )
	{
		if( error.code == code && error.file_pos.GetLine() == line )
			return true;
	}
	return false;
}

} // namespace U

// Entry point for tests executable.
int main(int argc, char* argv[])
{
	using namespace U;

	const llvm::InitLLVM llvm_initializer(argc, argv);

	const TestsFuncsContainer& funcs_container= GetTestsFuncsContainer();

	std::cout << "Run " << funcs_container.size() << " Ü tests" << std::endl << std::endl;

	unsigned int passed= 0u;
	unsigned int disabled= 0u;
	unsigned int failed= 0u;
	unsigned int filtered= 0u;
	for(const TestFuncData& func_data : funcs_container )
	{
		if( !FilterTest( func_data.name ) )
		{
			filtered++;
			continue;
		}

		try
		{
			func_data.func();
			++passed;
		}
		catch( const DisableTestException& )
		{
			// std::cout << "Test " << func_data.name << " disabled\n";
			disabled++;
		}
		catch( const TestException& ex )
		{
			std::cout << "Test " << func_data.name << " failed: " << ex.what() << "\n" << std::endl;
			failed++;
		};

		// We must kill ALL static internal llvm variables after each test.
		llvm::llvm_shutdown();
	}

	std::cout << std::endl <<
		passed << " tests passed\n" <<
		disabled << " tests disabled\n" <<
		filtered << " tests filtered\n" <<
		failed << " tests failed" << std::endl;

	return -int(failed);
}
