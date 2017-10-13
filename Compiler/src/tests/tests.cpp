#include <iostream>

#include "../lexical_analyzer.hpp"
#include "../syntax_analyzer.hpp"
#include "../code_builder.hpp"
#include "../source_tree_loader.hpp"

#include "tests.hpp"

namespace U
{

namespace
{

class SingleFileVfs final : public IVfs
{
public:
	SingleFileVfs( const ProgramString& file_name, const char* const file_content )
		: file_name_(file_name)
		, file_content_(file_content)
	{}

	virtual boost::optional<ProgramString> LoadFileContent( const Path& path ) override
	{
		if( path == file_name_ )
			return ToProgramString( file_content_ );
		return boost::none;
	}

	virtual Path NormalizePath( const Path& path ) override
	{
		// TODO
		return path;
	}

private:
	const ProgramString file_name_;
	const char* const file_content_;
};

} // namespace

std::unique_ptr<llvm::Module> BuildProgram( const char* const text )
{
	const ProgramString file_path= "_"_SpC;
	const SourceTreePtr source_tree=
		SourceTreeLoader( std::make_shared<SingleFileVfs>( file_path, text ) ).LoadSource( file_path);

	U_TEST_ASSERT( source_tree != nullptr );
	U_TEST_ASSERT( source_tree->lexical_errors.empty() );
	U_TEST_ASSERT( source_tree->syntax_errors.empty() );

	ICodeBuilder::BuildResult build_result= CodeBuilder().BuildProgram( *source_tree );

	for( const CodeBuilderError& error : build_result.errors )
		std::cout << error.file_pos.line << ":" << error.file_pos.pos_in_line << " " << ToStdString( error.text ) << "\n";

	U_TEST_ASSERT( build_result.errors.empty() );

	return std::move( build_result.module );
}

ICodeBuilder::BuildResult BuildProgramWithErrors( const char* const text )
{
	const ProgramString file_path= "_"_SpC;
	const SourceTreePtr source_tree=
		SourceTreeLoader( std::make_shared<SingleFileVfs>( file_path, text ) ).LoadSource( file_path);

	U_TEST_ASSERT( source_tree != nullptr );
	U_TEST_ASSERT( source_tree->lexical_errors.empty() );
	U_TEST_ASSERT( source_tree->syntax_errors.empty() );

	return CodeBuilder().BuildProgram( *source_tree );
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

void RunAllTests()
{
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
	}

	std::cout << std::endl << funcs_container.size() - failed << " tests passed\n" << failed << " tests failed" << std::endl;
}

} // namespace U
