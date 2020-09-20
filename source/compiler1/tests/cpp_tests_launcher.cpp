#include <iostream>
#include <unordered_set>

#include "../../compilers_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../../compilers_common/pop_llvm_warnings.hpp"

#include "../../tests/tests_common.hpp"
#include "../../tests/cpp_tests/tests.hpp"
#include  "../tests_common/funcs_c.hpp"

namespace U
{

namespace
{

llvm::ManagedStatic<llvm::LLVMContext> g_llvm_context;

void ErrorHanlder(
	void* const data, // should be ErrorTestBuildResult
	const uint32_t line,
	const uint32_t column,
	const uint32_t error_code,
	const U1_StringView& error_text )
{
	CodeBuilderError error;
	error.file_pos= FilePos( 0u, line, column );
	error.code= CodeBuilderErrorCode(error_code);
	error.text= std::string( error_text.data, error_text.data + error_text.size );
	reinterpret_cast<ErrorTestBuildResult*>(data)->errors.push_back( std::move(error) );
}

// Returns "true" if should enable test.
bool FilterTest( const std::string& test_name )
{
	const std::string test_name_without_file_name= test_name.substr(test_name.find_last_of(':') + 1);

	static const std::unordered_set<std::string> c_test_to_disable
	{
		"MethodTest6",
	};

	if( c_test_to_disable.count( test_name_without_file_name ) != 0 )
		return false;

	static const std::unordered_set<std::string> c_test_to_enable
	{
		"DestructorMustReturnVoidTest0",
		"DestructorOutsideClassTest0",
		"DestructorsTest0",
		"DestructorsTest1",
		//"DestructorsTest2",
		"DestructorsTest3",
		"DestructorsTest4",
		//"DestructorsTest5",
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
		"DestructorsTest19_DestuctorForInterface",
		//"ExpectedConstantExpression_InTemplateSignatureArgument_Test0",
		"ExpectedConstantExpression_InTemplateSignatureArgument_Test1",
		"ExplicitArgumentsInDestructorTest1",
		"FunctionBodyDuplication_ForDestructors_Test0",
		"ImportMacro_Test0",
		"ImportsTest0",
		"ImportsTest1_FunctionPrototypeInOneFileAndBodyInAnother",
		"ImportsTest2_FunctionsWithDifferentSignaturesInDifferentFiles",
		"ImportsTest3_MultipleInportOfSameFile_Test0",
		"ImportsTest4_MultipleInportOfSameFile_Test1",
		"ImportsTest5_ImportContentOfNamespace",
		"ImportsTest6_ImportClass",
		"ImportsTest7_ImportFileWithFunctionPrototypeAfterFileWithFunctionBody",
		"ImportsTest8_ImportFileWithFunctionBodyAfterFileWithFunctionPrototype",
		"ImportsTest9_ImportFileWithClassPrototypeAfterFileWithClassBody",
		"ImportsTest10_ImportFileWithClassBodyAfterFileWithClassPrototype",
		"ImportsTest11_MultipleImportOfSameGeneratedTemplateClass",
		"ImportsTest12_MultipleImportOfSameGeneratedTemplateClass",
		"ImportsTest13_MultipleImportOfSameGeneratedTemplateClassInsideTemplateClass",
		"ImportsTest14_ImportFunctionTemplate0",
		"ImportsTest14_ImportFunctionTemplate1",
		"ImportsTest15_NewSymbolsNotVisibleInImportedFunctionTemplate",
		"ImportsTest16_NewSymbolsNotVisibleInImportedClassTemplate",
		"ImportsTest17_NewSymbolsNotVisibleInImportedClassTemplate",
		"MandatoryTemplateSignatureArgumentAfterOptionalArgument_Test0",
		"NameNotFound_ForClassTemplateArguments_Test0",
		"NameNotFound_ForClassTemplateDefaultSignatureArguments_Test0",
		"ReferenceCheckTest_FunctionWithSingleArgumentReturnsReferenceToInputVariable_0",
		"ReferenceCheckTest_FunctionWithSingleArgumentReturnsReferenceToInputVariable_1",
		"ReferenceCheckTest_ImmutableReferenceAfterMutableReferenceOnStack",
		"ReferenceCheckTest_MultipleImmutableReferencesPassedToFunctionShouldBeOk",
		"ReferenceCheckTest_MultipleImmutableReferencesShouldBeOk",
		"ReferenceCheckTest_MultipleMutableReferencesOnStack",
		"ReferenceCheckTest_MutableAndImmutableReferencesPassedToFunction",
		"ReferenceCheckTest_MutableReferenceAfterImmutableReferenceOnStack",
		"ValueIsNotTemplateTest0",
	};

	if( c_test_to_enable.count( test_name_without_file_name ) != 0 )
		return true;

	static const std::string c_tests_to_enable_pattern[]
	{
		"AccessingReferenceInsideMethodUsingImplicitThis",
		"AssignMutableReferenceInsideClass",
		"BasicReferenceInsideClassUsage",
		"ConstructorOutsideClassTest",
		"ConstructorMustReturnVoidTest",
		"DefaultConstructorNotFoundTest",
		"DestructorTest_ForCStyleForOperator",
		"DestructorOutsideClassTest0",
		"DuplicatedStructMemberInitializer",
		"ExpectedInitializer_InConstructors_Test",
		"FieldIsNotInitializedYetTest",
		"GeneratedCopyConstructorForStructsWithReferencesTest",
		"InitializationListInNonconstructorTest",
		"InitializerForNonfieldStructMember_InConstructors_Test0",
		"InvalidTypeOfTemplateVariableArgumentTest",
		"InvalidValueAsTemplateArgumentTest",
		"MethodsCallInConstructorInitializerListIsForbiddenTest0",
		"MultipleMutableReferencesInside",
		"MultipleReferencesInside",
		"NameIsNotTypeName_ForTypedef_Test",
		"NameNotFoundTest",
		"NameNotFound_ForTypedef_Test",
		"RecursiveCallTest",
		"Redefinition_ForTypedef_Test",
		"ReferenceClassFieldDeclaration",
		"TemplateArgumentIsNotDeducedYet",
		"TemplateArgumentNotUsedInSignature",
		"TemplateInstantiationRequiredTest",
		"TemplateParametersDeductionFailed",
		"ThisUnavailable_InConstructors_Test",
		"UsingKeywordAsName_ForTypedef_Test",

		"auto_variables_errors_test.cpp",
		"auto_variables_test.cpp",
		"code_builder_errors_test.cpp",
		"code_builder_test.cpp",
		"constexpr_errors_test.cpp",
		"constexpr_test.cpp",
		"constructors_test.cpp",
		"classes_declaration_errors_test.cpp",
		"classes_declaration_test.cpp",
		"class_templates_test.cpp",
		"enums_errors_test.cpp",
		"enums_test.cpp",
		"global_variables_errors_test.cpp",
		"global_variables_test.cpp",
		"halt_test.cpp",
		"imports_errors_test.cpp",
		"initializers_errors_test.cpp",
		"initializers_test.cpp",
		"mangling_test.cpp",
		"methods_errors_test.cpp",
		"methods_test.cpp",
		"mutability_errors_test.cpp",
		"namespaces_errors_test.cpp",
		"namespaces_test.cpp",
		"operators_overloading_errors_test.cpp",
		"operators_overloading_test.cpp",
		"operators_priority_test.cpp",
		"operators_test.cpp",
		"references_inside_structs_errors_test.cpp",
		"type_conversions_test.cpp",
		"typedefs_test.cpp",
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
	const U1_StringView text_view{ text, std::strlen(text) };

	llvm::LLVMContext& llvm_context= *g_llvm_context;

	llvm::DataLayout data_layout( GetTestsDataLayout() );

	auto ptr=
		U1_BuildProgram(
			text_view,
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout) );
	U_TEST_ASSERT( ptr != nullptr );

	return std::unique_ptr<llvm::Module>( reinterpret_cast<llvm::Module*>(ptr) );
}

ErrorTestBuildResult BuildProgramWithErrors( const char* const text )
{
	const U1_StringView text_view{ text, std::strlen(text) };

	llvm::LLVMContext& llvm_context= *g_llvm_context;

	llvm::DataLayout data_layout( GetTestsDataLayout() );

	ErrorTestBuildResult build_result;

	const bool ok=
		U1_BuildProgramWithErrors(
			text_view,
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout),
			ErrorHanlder,
			&build_result );
	U_TEST_ASSERT(ok);

	return build_result;
}

std::unique_ptr<llvm::Module> BuildMultisourceProgram( std::vector<SourceEntry> sources, const std::string& root_file_path )
{
	std::vector<U1_SourceFile> source_files;
	source_files.reserve(sources.size());
	for (const SourceEntry& entry : sources)
	{
		U1_SourceFile f{};
		f.file_path.data= entry.file_path.data();
		f.file_path.size= entry.file_path.size();
		f.file_content.data= entry.text;
		f.file_content.size= std::strlen(entry.text);
		source_files.push_back(f);
	}

	const U1_StringView root_file_path_view{ root_file_path.data(), root_file_path.size() };

	llvm::LLVMContext& llvm_context= *g_llvm_context;

	llvm::DataLayout data_layout( GetTestsDataLayout() );

	const auto ptr=
		U1_BuildMultisourceProgram(
			source_files.data(), source_files.size(),
			root_file_path_view,
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout) );
	U_TEST_ASSERT( ptr != nullptr );

	return std::unique_ptr<llvm::Module>( reinterpret_cast<llvm::Module*>(ptr) );
}

ErrorTestBuildResult BuildMultisourceProgramWithErrors( std::vector<SourceEntry> sources, const std::string& root_file_path )
{
	std::vector<U1_SourceFile> source_files;
	source_files.reserve(sources.size());
	for (const SourceEntry& entry : sources)
	{
		U1_SourceFile f{};
		f.file_path.data= entry.file_path.data();
		f.file_path.size= entry.file_path.size();
		f.file_content.data= entry.text;
		f.file_content.size= std::strlen(entry.text);
		source_files.push_back(f);
	}

	const U1_StringView root_file_path_view{ root_file_path.data(), root_file_path.size() };

	llvm::LLVMContext& llvm_context= *g_llvm_context;

	llvm::DataLayout data_layout( GetTestsDataLayout() );

	ErrorTestBuildResult build_result;

	const bool ok=
		U1_BuildMultisourceProgramWithErrors(
			source_files.data(), source_files.size(),
			root_file_path_view,
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout),
			ErrorHanlder,
			&build_result );
	U_TEST_ASSERT( ok );

	return build_result;
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
