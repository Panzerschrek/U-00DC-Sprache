#define NOMINMAX
#include <windows.h>

#include <iostream>
#include <unordered_set>


#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/ManagedStatic.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../code_builder_lib_common/async_calls_inlining.hpp"
#include "../../tests/cpp_tests/cpp_tests.hpp"
#include "../../tests/tests_common.hpp"
#include  "../imports/funcs_c.hpp"

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
	out_error->template_context->args_description= std::string( args_description.data, args_description.data + args_description.size );

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
	};

	return c_test_to_disable.count( test_name_without_file_name ) == 0;
}

} // namespace

std::unique_ptr<llvm::Module> BuildProgram( const std::string_view text )
{
	const U1_StringView text_view{ text.data(), text.size() };

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

ErrorTestBuildResult BuildProgramWithErrors( const std::string_view text )
{
	const U1_StringView text_view{ text.data(), text.size() };

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
		f.file_content.data= entry.text.data();
		f.file_content.size= entry.text.size();
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
		f.file_content.data= entry.text.data();
		f.file_content.size= entry.text.size();
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

std::unique_ptr<llvm::Module> BuildProgramForLifetimesTest( const std::string_view text )
{
	const U1_StringView text_view{ text.data(), text.size() };

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

std::unique_ptr<llvm::Module> BuildProgramForMSVCManglingTest( const std::string_view text )
{
	const U1_StringView text_view{ text.data(), text.size() };

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

std::unique_ptr<llvm::Module> BuildProgramForAsyncFunctionsInliningTest( const std::string_view text )
{
	const U1_StringView text_view{ text.data(), text.size() };

	llvm::LLVMContext& llvm_context= *g_llvm_context;

	llvm::DataLayout data_layout( GetTestsDataLayout() );

	llvm::Module* const ptr=
		llvm::unwrap( U1_BuildProgramForAsyncCallsInliningTest(
			text_view,
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout) ) );
	U_TEST_ASSERT( ptr != nullptr );

	InlineAsyncCalls( *ptr );

	return std::unique_ptr<llvm::Module>( ptr );
}

bool HasError( const std::vector<CodeBuilderError>& errors, const CodeBuilderErrorCode code, const uint32_t line )
{
	for( const CodeBuilderError& error : errors )
	{
		if( error.code == code && error.src_loc.GetLine() == line )
			return true;
	}
	return false;
}

enum class TestStatus : uint8_t
{
	NotRun,
	InProgress,
	Filtered,
	Disabled,
	Finished,
};

} // namespace U

// Entry point for tests executable.
int main( const int argc, const char** argv )
{
	using namespace U;

	const TestsFuncsContainer& funcs_container= GetTestsFuncsContainer();

	if( argc <= 1 )
	{
		wchar_t module_name[ 1024 ];
		GetModuleFileNameW( nullptr, module_name, 1024 );

		std::cout << "Run " << funcs_container.size() << " Ü tests" << std::endl << std::endl;

		const std::wstring file_mapping_name= L"U_test_mapping_" + std::to_wstring( GetCurrentProcessId() );

		const HANDLE memory_handle=
			CreateFileMappingW(
				INVALID_HANDLE_VALUE,
				nullptr,
				PAGE_READWRITE,
				0,
				DWORD( funcs_container.size() * sizeof(TestStatus) ),
				file_mapping_name.data() );

		if( memory_handle == nullptr )
		{
			std::cerr << "Failed to create file mapping!" << std::endl;
			return -1;
		}

		void* const memory_mapped=
			MapViewOfFile( memory_handle, FILE_MAP_ALL_ACCESS, 0, 0, funcs_container.size() * sizeof(TestStatus) );

		if( memory_mapped == nullptr )
		{
			std::cerr << "Failed to map shared memory!" << std::endl;
			return -1;
		}

		const auto shared_test_status_values= static_cast<TestStatus*>( memory_mapped );

		// Set initial status for all tests.
		for( size_t i= 0; i < funcs_container.size(); ++i )
			shared_test_status_values[i]= TestStatus::NotRun;

		// Perform several attemts of running a child process.
		std::wstring command_line= std::wstring(L"\"") + module_name + L"\" \"" + file_mapping_name + L"\"";

		const size_t c_max_retries= 32;
		for( size_t try_index= 0; try_index < c_max_retries; ++try_index )
		{
			STARTUPINFOW startup_info{};
			startup_info.cb= sizeof(STARTUPINFOW);

			PROCESS_INFORMATION process_information{};

			const BOOL res=
				CreateProcessW(
					module_name,
					command_line.data(),
					nullptr,
					nullptr,
					0,
					0,
					0,
					nullptr,
					&startup_info,
					&process_information );

			if( res == 0 )
			{
				const auto e= GetLastError();
				std::cout << "Process creation error: " << e << std::endl;
				return -1;
			}

			if( WaitForSingleObject( process_information.hProcess, INFINITE ) != WAIT_OBJECT_0 )
			{
				const auto e= GetLastError();
				std::cerr << "WaitForSingleObject failed with error " << e << std::endl;
				return -1;
			}

			DWORD exit_code= 99999;
			if( GetExitCodeProcess( process_information.hProcess, &exit_code ) == 0 )
			{
				const auto e= GetLastError();
				std::cerr << "GetExitCodeProcess failed with error " << e << std::endl;
				return -1;
			}

			if( exit_code == 0 )
			{
				break; // Finished all (remaining) tests with no problems.
			}
			else
			{
				// Something failed.
				// Continue execution of other tests after failed one.
				std::cout << "Tests failure detected (exit code " << exit_code << "), re-run the child process, retries remained: " << c_max_retries - try_index << std::endl;
				continue;
			}
		}

		// Read status for each test from the shared memory.
		// It's safe to do here, since the child process (which modifies it) is already finished.

		uint32_t passed= 0u;
		uint32_t disabled= 0u;
		uint32_t failed= 0u;
		uint32_t filtered= 0u;

		for( size_t i= 0; i < funcs_container.size(); ++i )
		{
			const TestFuncData& func_data= funcs_container[i];
			switch( shared_test_status_values[i] )
			{
			case TestStatus::NotRun:
				std::cout << "Test " << func_data.name << " not run  - wtf?" << std::endl;
				++failed;
				break;
			case TestStatus::InProgress:
				std::cout << "Test " << func_data.name << " failed" << std::endl;
				++failed;
				break;
			case TestStatus::Filtered:
				++filtered;
				break;
			case TestStatus::Finished:
				++passed;
				break;
			case TestStatus::Disabled:
				std::cout << "Test " << func_data.name << " disabled" << std::endl;
				++disabled;
				break;
			}
		}

		std::cout << std::endl <<
			passed << " tests passed\n" <<
			disabled << " tests disabled\n" <<
			filtered << " tests filtered\n" <<
			failed << " tests failed" << std::endl;

		// Don't care unmapping shared memory and closing mapping, we exit anyway.

		return ( failed == 0 ? 0 : -1 );
	}
	else
	{
		const HANDLE memory_handle= OpenFileMappingA( FILE_MAP_ALL_ACCESS, FALSE, argv[1] );

		if( memory_handle == nullptr )
		{
			std::cerr << "Failed to open file mapping!" << std::endl;
			return -1;
		}

		void* const memory_mapped=
			MapViewOfFile( memory_handle, FILE_MAP_ALL_ACCESS, 0, 0, funcs_container.size() * sizeof(TestStatus) );

		if( memory_mapped == nullptr )
		{
			std::cerr << "Failed to map shared memory!" << std::endl;
			return -1;
		}

		const auto shared_test_status_values= static_cast<TestStatus*>( memory_mapped );

		for( size_t i= 0; i < funcs_container.size(); ++i )
		{
			TestStatus& status= shared_test_status_values[i];

			if( status != TestStatus::NotRun )
			{
				// Ignore tests which are finished.
				// Also ignore tests marked as "in progress", since they are likely failed.
				continue;
			}

			status= TestStatus::InProgress;

			const TestFuncData& func_data= funcs_container[i];

			if( !FilterTest( func_data.name ) )
			{
				status= TestStatus::Filtered;
				continue;
			}

			try
			{
				func_data.func();

				llvm::llvm_shutdown(); // We must kill ALL static internal llvm variables after each test.

				status= TestStatus::Finished;
			}
			catch( const DisableTestException& )
			{
				std::cout << "Test " << func_data.name << " disabled\n";
				status= TestStatus::Disabled;
			}
			catch( const TestException& ex )
			{
				std::cout << "Test " << func_data.name << " failed: " << ex.what() << "\n" << std::endl;
				return -1;
			}
			catch( const HaltException& )
			{
				std::cout << "Test " << func_data.name << " halted" << std::endl;
				return -1;
			}
			catch( const ExecutionEngineException& ex )
			{
				std::cout << "Test " << func_data.name << " failed:";
				for( const std::string& e : ex.errors )
					std::cout << "\n" << e;
				std::cout << std::endl;
				return -1;
			}
		}

		// Don't care unmapping shared memory and closing mapping, we exit anyway.

		return 0; // All is fine - exit with zero code.
	}
}
