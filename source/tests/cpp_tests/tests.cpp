#include <iostream>

#include "../../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../../code_builder_lib/pop_llvm_warnings.hpp"

#include "../../code_builder_lib/code_builder.hpp"
#include "../../lex_synt_lib/assert.hpp"
#include "../../lex_synt_lib/lexical_analyzer.hpp"
#include "../../lex_synt_lib/syntax_analyzer.hpp"
#include "../../lex_synt_lib/source_graph_loader.hpp"
#include "../tests_common.hpp"

#include "tests.hpp"

namespace U
{

namespace
{

const bool g_build_debug_info= true;

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

	virtual std::optional<LoadFileResult> LoadFileContent( const Path& file_path, const Path& full_parent_file_path ) override
	{
		U_UNUSED( full_parent_file_path );
		for( const SourceEntry& source_entry : sources_ )
		{
			if( file_path == source_entry.file_path )
				return LoadFileResult{ file_path, source_entry.text };
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

} // namespace

static void PrinteErrors_r( const CodeBuilderErrorsContainer& errors )
{
	for( const CodeBuilderError& error : errors)
	{
		std::cout << error.file_pos.line << ":" << error.file_pos.column << " " << error.text << "\n";
		if( error.template_context != nullptr )
			PrinteErrors_r( error.template_context->errors );
	}
}

std::unique_ptr<llvm::Module> BuildProgram( const char* const text )
{
	const std::string file_path= "_";
	const SourceGraphPtr source_graph=
		SourceGraphLoader( std::make_shared<MultiFileVfs>( file_path, text ) ).LoadSource( file_path );

	U_TEST_ASSERT( source_graph != nullptr );
	U_TEST_ASSERT( source_graph->lexical_errors.empty() );
	U_TEST_ASSERT( source_graph->syntax_errors.empty() );
	U_TEST_ASSERT( source_graph->root_node_index < source_graph->nodes_storage.size() );

	ICodeBuilder::BuildResult build_result=
		CodeBuilder(
			*g_llvm_context,
			llvm::sys::getProcessTriple(),
			llvm::DataLayout( GetTestsDataLayout() ),
			g_build_debug_info ).BuildProgram( *source_graph );

	PrinteErrors_r( build_result.errors );
	U_TEST_ASSERT( build_result.errors.empty() );

	return std::move( build_result.module );
}

ICodeBuilder::BuildResult BuildProgramWithErrors( const char* const text )
{
	const std::string file_path= "_";
	const SourceGraphPtr source_graph=
		SourceGraphLoader( std::make_shared<MultiFileVfs>( file_path, text ) ).LoadSource( file_path );

	U_TEST_ASSERT( source_graph != nullptr );
	U_TEST_ASSERT( source_graph->lexical_errors.empty() );
	U_TEST_ASSERT( source_graph->syntax_errors.empty() );

	return
		CodeBuilder(
			*g_llvm_context,
			llvm::sys::getProcessTriple(),
			llvm::DataLayout( GetTestsDataLayout() ),
			g_build_debug_info ).BuildProgram( *source_graph );
}

std::unique_ptr<llvm::Module> BuildMultisourceProgram( std::vector<SourceEntry> sources, const std::string& root_file_path )
{
	const SourceGraphPtr source_graph=
		SourceGraphLoader( std::make_shared<MultiFileVfs>( std::move(sources) ) ).LoadSource( root_file_path );

	U_TEST_ASSERT( source_graph != nullptr );
	U_TEST_ASSERT( source_graph->lexical_errors.empty() );
	U_TEST_ASSERT( source_graph->syntax_errors.empty() );

	ICodeBuilder::BuildResult build_result=
		CodeBuilder(
			*g_llvm_context,
			llvm::sys::getProcessTriple(),
			llvm::DataLayout( GetTestsDataLayout() ),
			g_build_debug_info ).BuildProgram( *source_graph );

	PrinteErrors_r( build_result.errors );
	U_TEST_ASSERT( build_result.errors.empty() );

	return std::move( build_result.module );
}

ICodeBuilder::BuildResult BuildMultisourceProgramWithErrors( std::vector<SourceEntry> sources, const std::string& root_file_path )
{
	const SourceGraphPtr source_graph=
		SourceGraphLoader( std::make_shared<MultiFileVfs>( std::move(sources) ) ).LoadSource( root_file_path );

	U_TEST_ASSERT( source_graph != nullptr );
	U_TEST_ASSERT( source_graph->lexical_errors.empty() );
	U_TEST_ASSERT( source_graph->syntax_errors.empty() );

	return
		CodeBuilder(
			*g_llvm_context,
			llvm::sys::getProcessTriple(),
			llvm::DataLayout( GetTestsDataLayout() ),
			g_build_debug_info ).BuildProgram( *source_graph );
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

struct FuncData
{
	std::string name;
	TestFunc* func;
};

using FuncsContainer= std::vector<FuncData>;

static FuncsContainer& GetFuncsContainer()
{
	static FuncsContainer funcs_container;
	return funcs_container;
}

TestId AddTestFuncPrivate( TestFunc* func, const char* const file_name, const char* const func_name )
{
	GetFuncsContainer().emplace_back( FuncData{ std::string(file_name) + ":" + func_name, func } );
	return TestId();
}

bool HaveError( const std::vector<CodeBuilderError>& errors, const CodeBuilderErrorCode code, const unsigned int line )
{
	for( const CodeBuilderError& error : errors )
	{
		if( error.code == code && error.file_pos.line == line )
			return true;
	}
	return false;
}

} // namespace U

// Entry point for tests executable.
int main()
{
	using namespace U;

	FuncsContainer& funcs_container= GetFuncsContainer();

	std::cout << "Run " << funcs_container.size() << " tests" << std::endl << std::endl;

	unsigned int passed= 0u;
	unsigned int disabled= 0u;
	unsigned int failed= 0u;
	for(const FuncData& func_data : funcs_container )
	{
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
		};

		// We must kill ALL static internal llvm variables after each test.
		llvm::llvm_shutdown();
	}

	std::cout << std::endl <<
		passed << " tests passed\n" <<
		disabled << " tests disabled\n" <<
		failed << " tests failed" << std::endl;

	return -int(failed);
}
