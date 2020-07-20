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
	static const std::string c_tests_to_enable_pattern[]
	{
		"AccessingReferenceInsideMethodUsingImplicitThis",
		"AdditionalSymbolsForIdentifiersTest0",
		"AdditiveOperationsTest0",
		"AdditiveOperationsTest1",
		"AdditiveOperationsTest2",
		"ArgumentsAssignmentTest",
		"ArrayIndexOutOfBoundsTest0",
		"ArrayInitializerForFundamentalTypesTest0",
		"ArrayInitializerForFundamentalTypesTest1",
		"ArrayInitializerForNonArrayTest0",
		"ArrayInitializerForNonArrayTest1",
		"ArrayInitializersCountMismatchTest0",
		"ArrayInitializersCountMismatchTest1",
		"ArraysTest0",
		"ArraysTest1",
		"AssignMutableReferenceInsideClass",
		"AssignToImmutableReferenceInsideStruct_Test0",
		"AssignToImmutableReferenceInsideStruct_Test1",
		"BasicBinaryOperationsFloatTest",
		"BasicBinaryOperationsTest",
		"BasicReferenceInsideClassUsage",
		"BitwiseNotTest",
		"BindingConstReferenceToNonConstReference_InReferenceFieldInitialization_Test0",
		"BindingConstReferenceToNonConstReference_InReferenceFieldInitialization_Test1",
		"BindValueToConstReferenceTest0",
		"BlocksTest",
		"BooleanBasicTest",
		"BreakOperatorTest",
		"BreakOutsideLoopTest",
		"CallTest0",
		"CallTest1",
		"CallTest3",
		"CallTest4",
		"CallTest5",
		"CallTest6",
		"CallTest7",
		"CallTest8",
		"CallTest9",
		"ComparisonFloatOperatorsTest",
		"ComparisonSignedOperatorsTest",
		"ComparisonUnsignedOperatorsTest",
		"ConstantExpressionResultIsUndefinedTest0",
		"ConstexprReferenceTest0",
		"ConstexprReferenceTest1",
		"ConstexprReferenceTest2",
		"ConstexprTest1",
		"ConstexprTest2",
		"ConstexprTest3",
		"ConstexprTest4",
		"ConstexprTest5",
		"ConstexprTest6",
		"ConstexprTest7",
		"ConstexprTest8",
		"ConstexprTest9",
		"ConstexprTest10",
		"ConstexprTest11",
		"ConstexprTest12",
		"ConstructorInitializerForFundamentalTypesTest0",
		"ConstructorInitializerForFundamentalTypesTest1",
		"ConstructorInitializerForReferencesTest0",
		"ConstructorInitializerForUnsupportedTypeTest0",
		"ConstructorOutsideClassTest0",
		"ConstructorOutsideClassTest1",
		"ConstructorMustReturnVoidTest0",
		"ConstructorMustReturnVoidTest1",
		"ConstructorTest",
		"ContinueOperatorTest",
		"ContinueOutsideLoopTest",
		"CopyAssignmentOperatorForStructsWithReferencesDeleted",
		"CouldNotOverloadFunctionTest1",
		"CouldNotOverloadFunctionTest2",
		"CouldNotOverloadFunctionTest3",
		"CouldNotSelectOverloadedFunction0",
		"CouldNotSelectOverloadedFunction1",
		"DefaultConstructorNotFoundTest0",
		"DefaultConstructorNotFoundTest1",
		"DefaultConstructorNotFoundTest2",
		"DefaultInitializationForStructMembersTest0",
		"DefaultInitializationForStructMembersTest1",
		"DestructorOutsideClassTest0",
		"DuplicatedStructMemberInitializerTest0",
		"DuplicatedStructMemberInitializer_InConstructors_Test0",
		"EmptyInitializerTest0",
		"EqualityFloatOperatorsTest",
		"EqualityOperatorsTest",
		"ExpectedInitializerTest",
		"ExpressionInitializerTest0",
		"ExpressionInitializerTest1",
		"ExpressionInitializerTest2",
		"ExpressionInitializerTest3",
		"ExpectedConstantExpressionTest0",
		"ExpectedConstantExpressionTest1",
		"ExpectedInitializer_InConstructors_Test0",
		"ExpectedReferenceValue_ForConstexpr_Test0",
		"ExpectedReferenceValue_ForConstexpr_Test1",
		"ExpectedReferenceValue_InStructReferenceInitialization",
		"FieldIsNotInitializedYetTest0",
		"FieldIsNotInitializedYetTest1",
		"FunctionBodyDuplicationTest0",
		"FunctionBodyDuplicationTest1",
		"FunctionDeclarationOutsideItsScopeTest",
		"FunctionsOverloadingTest0",
		"FunctionsOverloadingTest1",
		"FunctionsOverloadingTest2",
		"FunctionsOverloadingTest3",
		"FunctionPrototypeDuplicationTest0",
		"FunctionPrototypeDuplicationTest1",
		"FunctionPrototypeTest0",
		"FunctionPrototypeTest1",
		"FundamentalTypesHaveConstructorsWithExactlyOneParameterTest0",
		"FundamentalTypesHaveConstructorsWithExactlyOneParameterTest1",
		"GeneratedCopyConstructorForStructsWithReferencesTest",
		"HaltTest0",
		"HaltTest1_ShouldHaltInsteadOfReturn",
		"HaltTest2_ShouldHaltWithDeepCallStack",
		"HaltTest3_CodeAfterHaltMustBeUnreachable",
		"HaltTest4_HaltIsLikeReturn",
		"IfOperatorTest",
		"ImplicitConstexprTest0",
		"ImplicitConstexprTest1",
		"ImplicitConstexprTest2",
		"IncrementTest0",
		"IncrementTest1",
		"InitializationListInNonconstructorTest0",
		"InitializationListInNonconstructorTest1",
		"InitializerDisabledBecauseClassHaveExplicitNoncopyConstructorsTest0",
		"InitializerForNonfieldStructMember_InConstructors_Test0",
		"InnerClassTest0",
		"InnerClassTest1",
		"InnerClassTest2",
		"InvalidTypeForAutoVariableTest0",
		"InvalidTypeForAutoVariableTest1",
		"InvalidTypeForConstantExpressionVariableTest0",
		"LazyLogicalAndTest0",
		"LazyLogicalAndTest1",
		"LazyLogicalAndTest2",
		"lazyLogicalCombinedTest",
		"LazyLogicalOrTest0",
		"LazyLogicalOrTest1",
		"LazyLogicalOrTest2",
		"LeftShiftAndAssignTest0",
		"LeftShiftTest0",
		"LeftShiftTest1",
		"LogicalBinaryOperationsTest",
		"LogicalNotTest",
		"MethodsCallInConstructorInitializerListIsForbiddenTest0",
		"MultipleMutableReferencesInside",
		"MultipleReferencesInside",
		"NameIsNotTypeNameTest",
		"NameNotFoundTest_Minus1",
		"NameNotFoundTest0",
		"NameNotFoundTest1",
		"NameNotFoundTest2",
		"NamespacesTest",
		"NoMatchBinaryOperatorForGivenTypesTest0",
		"NoMatchBinaryOperatorForGivenTypesTest1",
		"NoReturnInFunctionReturningNonVoidTest",
		"NumericConstantsTest0",
		"NumericConstantsTest1",
		"NumericConstantsTest2",
		"OperationNotSupportedForThisTypeTest",
		"OperatorsPriorityTest",
		"RecursiveCallTest",
		"Redefinition0",
		"Redefinition1",
		"Redefinition3",
		"Redefinition5",
		"RedefinitionTest0",
		"RedefinitionTest1",
		"Redefenition_ForNamespaces_Test0",
		"ReferenceClassFiledDeclaration",
		"ReferencesHaveConstructorsWithExactlyOneParameterTest0",
		"ReferencesHaveConstructorsWithExactlyOneParameterTest1",
		"RemOperatorTest0",
		"RemOperatorTest1",
		"RemOperatorTest2",
		"RightShiftAndAssignTest0",
		"RightShiftTest0",
		"RightShiftTest1",
		"SimpliestProgramTest",
		"SimpleProgramTest",
		"StaticAssertExpressionIsNotConstantTest0",
		"StaticAssertExpressionIsNotConstantTest1",
		"StaticAssertExpressionMustHaveBoolTypeTest0",
		"StaticAssertExpressionMustHaveBoolTypeTest1",
		"StaticAssertExpressionMustHaveBoolTypeTest2",
		"StaticAssertTest0",
		"StaticAssertTest1",
		"StaticAssertTest2",
		"StructInitializerForNonStructTest0",
		"StructNamedInitializersTest0",
		"StructNamedInitializersTest1",
		"StructNamedInitializersTest2",
		"StructNamedInitializersTest3",
		"StructsWithReferencesHaveNoGeneratedDefaultConstructor",
		"TempVariableConstructionTest0",
		"TempVariableConstructionTest1",
		"ThiscallMismatch_Test0",
		"ThisUnavailable_InConstructors_Test0",
		"ThisUnavailable_InConstructors_Test1",
		"TwodimensionalArrayInitializerTest0",
		"TypeConversionTest",
		"TypesMismatchTest0",
		"TypesMismatchTest2",
		"TypesMismatchTest3",
		"TypesMismatchTest4",
		"TypesMismatchTest5",
		"UnaryMinusFloatTest",
		"UnaryMinusTest",
		"UnaryOperatorsOrderTest",
		"UnreachableCodeTest",
		"UnsupportedInitializerForReferenceTest0",
		"UsingKeywordAsName0",
		"UsingKeywordAsName1",
		"UsingKeywordAsName2",
		"UsingKeywordAsName3",
		"UsingKeywordAsName5",
		"UsingKeywordAsName6",
		"ViariableInitializerIsNotConstantExpressionTest0",
		"ViariableInitializerIsNotConstantExpressionTest1",
		"ViariableInitializerIsNotConstantExpressionTest2",
		"ViariableInitializerIsNotConstantExpressionTest3",
		"WhileOperatorTest",
		"ZeroInitConstexprTest0",
		"ZeroInitConstexprTest1",
		"ZeroInitializerForReferenceField_Test0",
		"ZeroInitializerForReferenceField_Test1",
		"ZeroInitilaizerTest",

		"auto_variables_errors_test.cpp:BindingConstReferenceToNonconstReferenceTest0",
		"auto_variables_errors_test.cpp:BindingConstReferenceToNonconstReferenceTest1",
		"auto_variables_errors_test.cpp:ExpectedReferenceValueTest0",
		"auto_variables_errors_test.cpp:ExpectedReferenceValueTest1",
		"auto_variables_test.cpp:AutoVariableTest0",
		"auto_variables_test.cpp:AutoVariableTest1",
		"auto_variables_test.cpp:AutoVariableTest2",
		"auto_variables_test.cpp:AutoVariableTest3",
		"auto_variables_test.cpp:AutoVariableTest4",
		"auto_variables_test.cpp:AutoVariableTest5",
		"code_builder_test.cpp:ReferencesTest",
		"code_builder_test.cpp:StructTest0",
		"code_builder_test.cpp:StructTest1",
		"code_builder_test.cpp:TypesMismatchTest1",
		"code_builder_test.cpp:VariablesTest0",
		"code_builder_test.cpp:VariablesTest1",
		"code_builder_errors_test.cpp:ExpectedReferenceValueTest0",
		"code_builder_errors_test.cpp:ExpectedReferenceValueTest2",
		"code_builder_errors_test.cpp:ExpectedReferenceValueTest3",
		"code_builder_errors_test.cpp:ExpectedReferenceValueTest4",
		"code_builder_errors_test.cpp:ExpectedReferenceValueTest5",
		"code_builder_errors_test.cpp:ExpectedReferenceValueTest6",
		"constexpr_test.cpp:ConstexprTest0",
		"methods_test.cpp:MethodTest0",
		//"methods_test.cpp:MethodTest1",
		"methods_test.cpp:MethodTest2",
		"methods_test.cpp:MethodTest3",
		"methods_test.cpp:MethodTest4",
		"methods_test.cpp:MethodTest5",
		"methods_test.cpp:MethodTest6",
		"methods_test.cpp:MethodTest7",
		"methods_test.cpp:MethodTest8",
		"methods_test.cpp:MethodTest9",
		"methods_test.cpp:MethodTest10",
		"operators_test.cpp:DecrementTest0",
		"operators_test.cpp:DecrementTest1",
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
