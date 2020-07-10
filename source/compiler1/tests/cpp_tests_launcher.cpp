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
		"AdditionalSymbolsForIdentifiersTest0",
		"ArgumentsAssignmentTest",
		"ArraysTest0",
		"ArraysTest1",
		"BasicBinaryOperationsFloatTest",
		"BasicBinaryOperationsTest",
		"BitwiseNotTest",
		"BindValueToConstReferenceTest0",
		"BlocksTest",
		"BooleanBasicTest",
		"BreakOperatorTest",
		"BreakOutsideLoopTest",
		"CallTest0",
		"CallTest1",
		"CallTest4",
		"ComparisonFloatOperatorsTest",
		"ComparisonSignedOperatorsTest",
		"ComparisonUnsignedOperatorsTest",
		"ContinueOperatorTest",
		"ContinueOutsideLoopTest",
		"EqualityFloatOperatorsTest",
		"EqualityOperatorsTest",
		"IfOperatorTest",
		"LogicalBinaryOperationsTest",
		"LogicalNotTest",
		"NameIsNotTypeNameTest",
		"NameNotFoundTest_Minus1",
		"NameNotFoundTest0",
		"NameNotFoundTest1",
		"NameNotFoundTest2",
		"NoMatchBinaryOperatorForGivenTypesTest0",
		"NoMatchBinaryOperatorForGivenTypesTest1",
		"NoReturnInFunctionReturningNonVoidTest",
		"NumericConstantsTest0",
		"NumericConstantsTest1",
		"NumericConstantsTest2",
		"OperationNotSupportedForThisTypeTest1",
		"OperationNotSupportedForThisTypeTest2",
		"OperationNotSupportedForThisTypeTest3",
		"OperatorsPriorityTest",
		"RecursiveCallTest",
		"Redefinition0",
		"Redefinition1",
		"Redefinition3",
		"Redefinition5",
		"RedefinitionTest0",
		"RedefinitionTest1",
		"RemOperatorTest0",
		"RemOperatorTest2",
		"SimpliestProgramTest",
		"SimpleProgramTest",
		"TypesMismatchTest0",
		"TypesMismatchTest2",
		"TypesMismatchTest3",
		"TypesMismatchTest4",
		"TypesMismatchTest5",
		"UnaryMinusFloatTest",
		"UnaryMinusTest",
		"UnaryOperatorsOrderTest",
		"UnreachableCodeTest",
		"UsingKeywordAsName0",
		"UsingKeywordAsName1",
		"UsingKeywordAsName2",
		"UsingKeywordAsName3",
		"UsingKeywordAsName5",
		"UsingKeywordAsName6",
		"WhileOperatorTest",

		"auto_variables_test.cpp:AutoVariableTest0",
		"code_builder_test.cpp:ReferencesTest0",
		//"code_builder_test.cpp:ReferencesTest1",
		"code_builder_test.cpp:ReferencesTest2",
		"code_builder_test.cpp:ReferencesTest3",
		"code_builder_test.cpp:ReferencesTest4",
		"code_builder_test.cpp:ReferencesTest5",
		"code_builder_test.cpp:ReferencesTest6",
		"code_builder_test.cpp:ReferencesTest7",
		"code_builder_test.cpp:ReferencesTest8",
		"code_builder_test.cpp:ReferencesTest9",
		"code_builder_test.cpp:StructTest0",
		"code_builder_test.cpp:StructTest1",
		"code_builder_test.cpp:TypesMismatchTest1",
		"code_builder_test.cpp:VariablesTest0",
		"code_builder_test.cpp:VariablesTest1",
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
