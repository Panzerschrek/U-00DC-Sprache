#include <iostream>
#include <unordered_set>

#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/ManagedStatic.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../code_builder_lib_common/async_calls_inlining.hpp"
#include "../../tests/cpp_tests/cpp_tests.hpp"
#include "../../tests/tests_common.hpp"
#include  "../launchers_common/funcs_c.hpp"

namespace U
{

namespace
{

llvm::ManagedStatic<llvm::LLVMContext> g_llvm_context;

U1_UserHandle ErrorHanlder(
	const U1_UserHandle data, // should be "std::vector<CodeBuilderError>"
	const uint32_t file_index,
	const uint32_t line,
	const uint32_t column,
	const uint32_t error_code,
	const U1_StringView& error_text )
{
	CodeBuilderError error;
	error.src_loc= SrcLoc( file_index, line, column );
	error.code= CodeBuilderErrorCode(error_code);
	error.text= std::string( error_text.data, error_text.data + error_text.size );

	const auto errors_container= reinterpret_cast<std::vector<CodeBuilderError>*>(data);
	errors_container->push_back( std::move(error) );
	return reinterpret_cast<U1_UserHandle>(&errors_container->back());
}

U1_UserHandle TemplateErrorsContextHandler(
	const U1_UserHandle data, // should be "CodeBuilderError*"
	const uint32_t file_index,
	const uint32_t line,
	const uint32_t column,
	const U1_StringView& context_name,
	const U1_StringView& args_description )
{
	const auto out_error= reinterpret_cast<CodeBuilderError*>(data);
	out_error->template_context= std::make_shared<TemplateErrorsContext>();
	out_error->template_context->context_declaration_src_loc= SrcLoc( file_index, line, column );
	out_error->template_context->context_name= std::string( context_name.data, context_name.data + context_name.size );
	out_error->template_context->parameters_description= std::string( args_description.data, args_description.data + args_description.size );

	return reinterpret_cast<U1_UserHandle>( & out_error->template_context->errors );
}

const U1_ErrorsHandlingCallbacks g_error_handling_callbacks
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
		"LambdasMangling_Test9",
		"LambdasMangling_Test10",
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
			llvm::wrap(&data_layout),
			false );
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
			false,
			g_error_handling_callbacks,
			reinterpret_cast<U1_UserHandle>(&build_result.errors) );
	U_TEST_ASSERT(ok);

	return build_result;
}

std::unique_ptr<llvm::Module> BuildMultisourceProgram( std::vector<SourceEntry> sources, const std::string& root_file_path, const bool report_about_unused_names )
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
			llvm::wrap(&data_layout),
			report_about_unused_names );
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
			reinterpret_cast<U1_UserHandle>(&build_result.errors) );
	U_TEST_ASSERT( ok );

	return build_result;
}

std::unique_ptr<llvm::Module> BuildProgramForLifetimesTest( const char* text )
{
	const U1_StringView text_view{ text, std::strlen(text) };

	llvm::LLVMContext& llvm_context= *g_llvm_context;

	llvm::DataLayout data_layout( GetTestsDataLayout() );

	auto ptr=
		U1_BuildProgramForLifetimesTest(
			text_view,
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout),
			false );
	U_TEST_ASSERT( ptr != nullptr );

	return std::unique_ptr<llvm::Module>( reinterpret_cast<llvm::Module*>(ptr) );
}

std::unique_ptr<llvm::Module> BuildProgramForMSVCManglingTest( const char* text )
{
	const U1_StringView text_view{ text, std::strlen(text) };

	llvm::LLVMContext& llvm_context= *g_llvm_context;

	llvm::DataLayout data_layout( GetTestsDataLayout() );

	auto ptr=
		U1_BuildProgramForMSVCManglingTest(
			text_view,
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout) );
	U_TEST_ASSERT( ptr != nullptr );

	return std::unique_ptr<llvm::Module>( reinterpret_cast<llvm::Module*>(ptr) );
}

std::unique_ptr<llvm::Module> BuildProgramForAsyncFunctionsInliningTest( const char* const text )
{
	auto module= BuildProgram( text );
	if( module == nullptr )
		return nullptr;

	InlineAsyncCalls( *module );

	return module;
}

bool HaveError( const std::vector<CodeBuilderError>& errors, const CodeBuilderErrorCode code, const uint32_t line )
{
	for( const CodeBuilderError& error : errors )
	{
		if( error.code == code && error.src_loc.GetLine() == line )
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

	uint32_t passed= 0u;
	uint32_t disabled= 0u;
	uint32_t failed= 0u;
	uint32_t filtered= 0u;
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
		}
		catch( const HaltException& )
		{
			std::cout << "Test " << func_data.name << " halted" << std::endl;
			failed++;
		}
		catch( const ExecutionEngineException& ex )
		{
			std::cout << "Test " << func_data.name << " failed:";
			for( const std::string& e : ex.errors )
				std::cout << "\n" << e;
			std::cout << std::endl;
			failed++;
		}

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
