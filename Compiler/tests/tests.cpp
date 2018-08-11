#include <iostream>

#include <llvm/Support/ManagedStatic.h>

#include "../src/assert.hpp"
#include "../src/lexical_analyzer.hpp"
#include "../src/syntax_analyzer.hpp"
#include "../src/code_builder.hpp"
#include "../src/source_graph_loader.hpp"
#include "tests_common.hpp"

#include "tests.hpp"

namespace U
{

namespace
{

class MultiFileVfs final : public IVfs
{
public:
	explicit MultiFileVfs( std::vector<SourceEntry> sources )
		: sources_(std::move(sources))
	{}

	MultiFileVfs( ProgramString file_path, const char* text )
		: sources_( { SourceEntry{ file_path, text } } )
	{}

	virtual boost::optional<LoadFileResult> LoadFileContent( const Path& file_path, const Path& full_parent_file_path ) override
	{
		U_UNUSED( full_parent_file_path );
		for( const SourceEntry& source_entry : sources_ )
		{
			if( file_path == source_entry.file_path )
				return LoadFileResult{ file_path, DecodeUTF8( source_entry.text ) };
		}
		return boost::none;
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

std::unique_ptr<llvm::Module> BuildProgram( const char* const text )
{
	const ProgramString file_path= "_"_SpC;
	const SourceGraphPtr source_graph=
		SourceGraphLoader( std::make_shared<MultiFileVfs>( file_path, text ) ).LoadSource( file_path );

	U_TEST_ASSERT( source_graph != nullptr );
	U_TEST_ASSERT( source_graph->lexical_errors.empty() );
	U_TEST_ASSERT( source_graph->syntax_errors.empty() );
	U_TEST_ASSERT( source_graph->root_node_index < source_graph->nodes_storage.size() );

	ICodeBuilder::BuildResult build_result=
		CodeBuilder(
			Constants::tests_target_triple_str,
			llvm::DataLayout( Constants::tests_data_layout_str ) ).BuildProgram( *source_graph );

	for( const CodeBuilderError& error : build_result.errors )
		std::cout << error.file_pos.line << ":" << error.file_pos.pos_in_line << " " << ToStdString( error.text ) << "\n";

	U_TEST_ASSERT( build_result.errors.empty() );

	return std::move( build_result.module );
}

ICodeBuilder::BuildResult BuildProgramWithErrors( const char* const text )
{
	const ProgramString file_path= "_"_SpC;
	const SourceGraphPtr source_graph=
		SourceGraphLoader( std::make_shared<MultiFileVfs>( file_path, text ) ).LoadSource( file_path );

	U_TEST_ASSERT( source_graph != nullptr );
	U_TEST_ASSERT( source_graph->lexical_errors.empty() );
	U_TEST_ASSERT( source_graph->syntax_errors.empty() );

	return
		CodeBuilder(
			Constants::tests_target_triple_str,
			llvm::DataLayout( Constants::tests_data_layout_str ) ).BuildProgram( *source_graph );
}

std::unique_ptr<llvm::Module> BuildMultisourceProgram( std::vector<SourceEntry> sources, const ProgramString& root_file_path )
{
	const SourceGraphPtr source_graph=
		SourceGraphLoader( std::make_shared<MultiFileVfs>( std::move(sources) ) ).LoadSource( root_file_path );

	U_TEST_ASSERT( source_graph != nullptr );
	U_TEST_ASSERT( source_graph->lexical_errors.empty() );
	U_TEST_ASSERT( source_graph->syntax_errors.empty() );

	ICodeBuilder::BuildResult build_result=
		CodeBuilder(
			Constants::tests_target_triple_str,
			llvm::DataLayout( Constants::tests_data_layout_str ) ).BuildProgram( *source_graph );

	for( const CodeBuilderError& error : build_result.errors )
		std::cout << error.file_pos.line << ":" << error.file_pos.pos_in_line << " " << ToStdString( error.text ) << "\n";

	U_TEST_ASSERT( build_result.errors.empty() );

	return std::move( build_result.module );
}

ICodeBuilder::BuildResult BuildMultisourceProgramWithErrors( std::vector<SourceEntry> sources, const ProgramString& root_file_path )
{
	const SourceGraphPtr source_graph=
		SourceGraphLoader( std::make_shared<MultiFileVfs>( std::move(sources) ) ).LoadSource( root_file_path );

	U_TEST_ASSERT( source_graph != nullptr );
	U_TEST_ASSERT( source_graph->lexical_errors.empty() );
	U_TEST_ASSERT( source_graph->syntax_errors.empty() );

	return
		CodeBuilder(
			Constants::tests_target_triple_str,
			llvm::DataLayout( Constants::tests_data_layout_str ) ).BuildProgram( *source_graph );
}

EnginePtr CreateEngine( std::unique_ptr<llvm::Module> module, const bool needs_dump )
{
	U_TEST_ASSERT( module != nullptr );

	if( needs_dump )
		module->dump();

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

typedef std::vector<FuncData> FuncsContainer;

static FuncsContainer& GetFuncsContainer()
{
	static FuncsContainer funcs_container;
	return funcs_container;
}

TestId AddTestFuncPrivate( TestFunc* func, const char* const file_name, const char* const func_name )
{
	GetFuncsContainer().emplace_back( FuncData{ std::string(file_name) + ":" + func_name, func } );

	static TestId counter= 0u;
	return counter++;
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

	unsigned int failed= 0u;
	for(const FuncData& func_data : funcs_container )
	{
		try
		{
			func_data.func();
		}
		catch( const TestException& ex )
		{
			std::cout << "Test " << func_data.name << " failed: " << ex.what() << std::endl;
			failed++;
		};

		// We must kill ALL static internal llvm variables after each test.
		llvm::llvm_shutdown();
	}

	std::cout << std::endl << funcs_container.size() - failed << " tests passed\n" << failed << " tests failed" << std::endl;

	return -int(failed);
}
