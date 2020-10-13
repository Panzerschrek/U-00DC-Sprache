#include <iostream>
#include <unordered_set>

#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../tests/tests_common.hpp"
#include "../../tests/cpp_tests/tests.hpp"
#include  "../tests_common/funcs_c.hpp"

namespace U
{

namespace
{

llvm::ManagedStatic<llvm::LLVMContext> g_llvm_context;

UserHandle ErrorHanlder(
	const UserHandle data, // should be "std::vector<CodeBuilderError>"
	const uint32_t line,
	const uint32_t column,
	const uint32_t error_code,
	const U1_StringView& error_text )
{
	CodeBuilderError error;
	error.file_pos= FilePos( 0u, line, column );
	error.code= CodeBuilderErrorCode(error_code);
	error.text= std::string( error_text.data, error_text.data + error_text.size );

	const auto errors_container= reinterpret_cast<std::vector<CodeBuilderError>*>(data);
	errors_container->push_back( std::move(error) );
	return reinterpret_cast<UserHandle>(&errors_container->back());
}

UserHandle TemplateErrorsContextHandler(
	const UserHandle data, // should be "CodeBuilderError*"
	const uint32_t line,
	const uint32_t column,
	const U1_StringView& context_name,
	const U1_StringView& args_description )
{
	const auto out_error= reinterpret_cast<CodeBuilderError*>(data);
	out_error->template_context= std::make_shared<TemplateErrorsContext>();
	out_error->template_context->context_declaration_file_pos= FilePos( 0u, line, column );
	out_error->template_context->context_name= std::string( context_name.data, context_name.data + context_name.size );
	out_error->template_context->parameters_description= std::string( args_description.data, args_description.data + args_description.size );

	return reinterpret_cast<UserHandle>( & out_error->template_context->errors );
}

const ErrorsHandlingCallbacks g_error_handling_callbacks
{
	ErrorHanlder,
	TemplateErrorsContextHandler,
};

// Returns "true" if should enable test.
bool FilterTest( const std::string& test_name )
{
	const std::string test_name_without_file_name= test_name.substr(test_name.find_last_of(':') + 1);

	static const std::unordered_set<std::string> c_test_to_disable
	{
		"ImportInClassFieldInitializer_Test0",
	};

	return c_test_to_disable.count( test_name_without_file_name ) == 0;
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
			g_error_handling_callbacks,
			reinterpret_cast<UserHandle>(&build_result.errors) );
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
			g_error_handling_callbacks,
			reinterpret_cast<UserHandle>(&build_result.errors) );
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
