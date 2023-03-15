#include <iostream>

#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/ManagedStatic.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../code_builder_lib/code_builder.hpp"
#include "../../lex_synt_lib_common/assert.hpp"
#include "../../code_builder_lib_common/source_file_contents_hash.hpp"
#include "../lex_synt_lib/lexical_analyzer.hpp"
#include "../lex_synt_lib/syntax_analyzer.hpp"
#include "../lex_synt_lib/source_graph_loader.hpp"
#include "../../tests/tests_common.hpp"

#include "../../tests/cpp_tests/tests.hpp"

namespace U
{

namespace
{

llvm::ManagedStatic<llvm::LLVMContext> g_llvm_context;

class MultiFileVfs final : public IVfs
{
public:
	explicit MultiFileVfs( std::vector<SourceEntry> sources )
		: sources_(std::move(sources))
	{}

	MultiFileVfs( std::string file_path, const char* text )
		: sources_( { SourceEntry{ file_path, text } } )
	{}

	virtual std::optional<FileContent> LoadFileContent( const Path& full_file_path ) override
	{
		for( const SourceEntry& source_entry : sources_ )
		{
			if( full_file_path == source_entry.file_path )
				return source_entry.text;
		}
		return std::nullopt;
	}

	virtual Path GetFullFilePath( const Path& file_path, const Path& full_parent_file_path ) override
	{
		U_UNUSED(full_parent_file_path);
		return file_path;
	}

private:
	const std::vector<SourceEntry> sources_;
};

void PrintLexSyntErrors( const SourceGraph& source_graph )
{
	std::vector<std::string> source_files;
	source_files.reserve( source_graph.nodes_storage.size() );
	for( const auto& node : source_graph.nodes_storage )
		source_files.push_back( node.file_path );

	PrintLexSyntErrors( source_files, source_graph.errors );
}

// Returns "true" if should enable test.
bool FilterTest( const std::string& test_name )
{
	static const std::unordered_set<std::string> c_tests_to_ignore
	{
	};

	const std::string test_name_without_file_name= test_name.substr(test_name.find_last_of(':') + 1);

	return c_tests_to_ignore.count(test_name_without_file_name) == 0;
}

} // namespace

static void PrinteErrors_r( const CodeBuilderErrorsContainer& errors )
{
	for( const CodeBuilderError& error : errors)
	{
		std::cout << error.src_loc.GetLine() << ":" << error.src_loc.GetColumn() << " " << error.text << "\n";
		if( error.template_context != nullptr )
			PrinteErrors_r( error.template_context->errors );
	}
}

static CodeBuilderOptions GetCodeBuilderOptionsForTests()
{
	CodeBuilderOptions options;
	options.build_debug_info= true;
	options.generate_tbaa_metadata= true;
	options.create_lifetimes= true;
	return options;
}

std::unique_ptr<llvm::Module> BuildProgram( const char* const text )
{
	const std::string file_path= "_";
	MultiFileVfs vfs( file_path, text );
	const SourceGraph source_graph= LoadSourceGraph( vfs, CalculateSourceFileContentsHash, file_path );

	PrintLexSyntErrors( source_graph );
	U_TEST_ASSERT( source_graph.errors.empty() );

	CodeBuilder::BuildResult build_result=
		CodeBuilder(
			*g_llvm_context,
			llvm::DataLayout( GetTestsDataLayout() ),
			GetTestsTargetTriple(),
			GetCodeBuilderOptionsForTests() ).BuildProgram( source_graph );

	PrinteErrors_r( build_result.errors );
	U_TEST_ASSERT( build_result.errors.empty() );

	return std::move( build_result.module );
}

ErrorTestBuildResult BuildProgramWithErrors( const char* const text )
{
	const std::string file_path= "_";
	MultiFileVfs vfs( file_path, text );
	const SourceGraph source_graph= LoadSourceGraph( vfs, CalculateSourceFileContentsHash, file_path );

	PrintLexSyntErrors( source_graph );
	U_TEST_ASSERT( source_graph.errors.empty() );

	return
		{ CodeBuilder(
			*g_llvm_context,
			llvm::DataLayout( GetTestsDataLayout() ),
			GetTestsTargetTriple(),
			GetCodeBuilderOptionsForTests() ).BuildProgram( source_graph ).errors };
}

std::unique_ptr<llvm::Module> BuildMultisourceProgram( std::vector<SourceEntry> sources, const std::string& root_file_path )
{
	MultiFileVfs vfs( std::move(sources) );
	const SourceGraph source_graph= LoadSourceGraph( vfs, CalculateSourceFileContentsHash, root_file_path );

	PrintLexSyntErrors( source_graph );
	U_TEST_ASSERT( source_graph.errors.empty() );

	CodeBuilder::BuildResult build_result=
		CodeBuilder(
			*g_llvm_context,
			llvm::DataLayout( GetTestsDataLayout() ),
			GetTestsTargetTriple(),
			GetCodeBuilderOptionsForTests() ).BuildProgram( source_graph );

	PrinteErrors_r( build_result.errors );
	U_TEST_ASSERT( build_result.errors.empty() );

	return std::move( build_result.module );
}

ErrorTestBuildResult BuildMultisourceProgramWithErrors( std::vector<SourceEntry> sources, const std::string& root_file_path )
{
	MultiFileVfs vfs( std::move(sources) );
	const SourceGraph source_graph= LoadSourceGraph( vfs, CalculateSourceFileContentsHash, root_file_path );

	PrintLexSyntErrors( source_graph );
	U_TEST_ASSERT( source_graph.errors.empty() );

	return
		{ CodeBuilder(
			*g_llvm_context,
			llvm::DataLayout( GetTestsDataLayout() ),
			GetTestsTargetTriple(),
			GetCodeBuilderOptionsForTests() ).BuildProgram( source_graph ).errors };
}

std::unique_ptr<llvm::Module> BuildProgramForLifetimesTest( const char* text )
{
	const std::string file_path= "_";
	MultiFileVfs vfs( file_path, text );
	const SourceGraph source_graph= LoadSourceGraph( vfs, CalculateSourceFileContentsHash, file_path );

	PrintLexSyntErrors( source_graph );
	U_TEST_ASSERT( source_graph.errors.empty() );

	CodeBuilderOptions options= GetCodeBuilderOptionsForTests();
	options.generate_lifetime_start_end_debug_calls= true;

	CodeBuilder::BuildResult build_result=
		CodeBuilder(
			*g_llvm_context,
			llvm::DataLayout( GetTestsDataLayout() ),
			GetTestsTargetTriple(),
			options ).BuildProgram( source_graph );

	PrinteErrors_r( build_result.errors );
	U_TEST_ASSERT( build_result.errors.empty() );

	return std::move( build_result.module );
}

std::unique_ptr<llvm::Module> BuildProgramForMSVCManglingTest( const char* text )
{
	const std::string file_path= "_";
	MultiFileVfs vfs( file_path, text );
	const SourceGraph source_graph= LoadSourceGraph( vfs, CalculateSourceFileContentsHash, file_path );

	PrintLexSyntErrors( source_graph );
	U_TEST_ASSERT( source_graph.errors.empty() );

	CodeBuilderOptions options= GetCodeBuilderOptionsForTests();
	options.mangling_scheme= ManglingScheme::MSVC64; // Test only 64-bit scheme

	CodeBuilder::BuildResult build_result=
		CodeBuilder(
			*g_llvm_context,
			llvm::DataLayout( GetTestsDataLayout() ),
			GetTestsTargetTriple(),
			options ).BuildProgram( source_graph );

	PrinteErrors_r( build_result.errors );
	U_TEST_ASSERT( build_result.errors.empty() );

	return std::move( build_result.module );
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

	std::cout << "Run " << funcs_container.size() << " tests" << std::endl << std::endl;

	uint32_t passed= 0u;
	uint32_t disabled= 0u;
	uint32_t failed= 0u;
	uint32_t filtered= 0u;

	for(const TestFuncData& func_data : funcs_container )
	{
		if( !FilterTest( func_data.name ) )
		{
			++filtered;
			continue;
		}

		try
		{
			func_data.func();
			++passed;
		}
		catch( const DisableTestException& )
		{
			std::cout << "Test " << func_data.name << " disabled\n";
			disabled++;
		}
		catch( const TestException& ex )
		{
			std::cout << "Test " << func_data.name << " failed: " << ex.what() << "\n" << std::endl;
			failed++;
		}
		catch( const ExecutionEngineException& ex )
		{
			std::cout << "Test " << func_data.name << " failed: " << ex.what() << "\n" << std::endl;
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
