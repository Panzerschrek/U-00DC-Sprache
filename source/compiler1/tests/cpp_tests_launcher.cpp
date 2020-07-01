#include <iostream>

#include "../../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../../code_builder_lib/pop_llvm_warnings.hpp"

#include "../../tests/tests_common.hpp"
#include "../../tests/cpp_tests/tests.hpp"
#include  "funcs_c.hpp"

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
		"ArgumentsAssignmentTest",
		"BlocksTest0",
		"NameNotFoundTest_Minus1",
		"NumericConstantsTest0",
		"OperatorsPriorityTest0",
		"OperatorsPriorityTest2",
		"OperatorsPriorityTest4",
		"OperatorsPriorityTest5",
		"Redefinition5",
		"SimpliestProgramTest",
		"SimpleProgramTest",

		"auto_variables_test.cpp:AutoVariableTest0",
		"code_builder_test.cpp:VariablesTest0",
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

	auto ptr= U1_BuildProgram( text, std::strlen(text), reinterpret_cast<LLVMContextRef>(&llvm_context) );
	U_TEST_ASSERT( ptr != nullptr );

	return std::unique_ptr<llvm::Module>( reinterpret_cast<llvm::Module*>(ptr) );
}

ICodeBuilder::BuildResult BuildProgramWithErrors( const char* const text )
{
	llvm::LLVMContext& llvm_context= *g_llvm_context;

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
			reinterpret_cast<LLVMContextRef>(&llvm_context),
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
int main()
{
	using namespace U;

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
