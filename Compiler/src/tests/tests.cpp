#include <iostream>

#include "../lexical_analyzer.hpp"
#include "../syntax_analyzer.hpp"
#include "../code_builder.hpp"

#include "tests.hpp"

namespace U
{

std::unique_ptr<llvm::Module> BuildProgram( const char* const text )
{
	const LexicalAnalysisResult lexical_analysis_result=
		LexicalAnalysis( ToProgramString( text ) );

	for( const std::string& lexical_error_message : lexical_analysis_result.error_messages )
		std::cout << lexical_error_message << "\n";
	U_TEST_ASSERT( lexical_analysis_result.error_messages.empty() );

	const SyntaxAnalysisResult syntax_analysis_result=
		SyntaxAnalysis( lexical_analysis_result.lexems );

	for( const std::string& syntax_error_message : syntax_analysis_result.error_messages )
		std::cout << syntax_error_message << "\n";
	U_TEST_ASSERT( syntax_analysis_result.error_messages.empty() );

	ICodeBuilder::BuildResult build_result=
		CodeBuilder().BuildProgram( syntax_analysis_result.program_elements );

	for( const CodeBuilderError& error : build_result.errors )
		std::cout << error.file_pos.line << ":" << error.file_pos.pos_in_line << " " << ToStdString( error.text ) << "\n";

	U_TEST_ASSERT( build_result.errors.empty() );

	return std::move( build_result.module );
}

ICodeBuilder::BuildResult BuildProgramWithErrors( const char* const text )
{
	const LexicalAnalysisResult lexical_analysis_result=
		LexicalAnalysis( ToProgramString( text ) );

	for( const std::string& lexical_error_message : lexical_analysis_result.error_messages )
		std::cout << lexical_error_message << "\n";
	U_TEST_ASSERT( lexical_analysis_result.error_messages.empty() );

	const SyntaxAnalysisResult syntax_analysis_result=
		SyntaxAnalysis( lexical_analysis_result.lexems );

	for( const SyntaxErrorMessage& syntax_error_message : syntax_analysis_result.error_messages )
		std::cout << syntax_error_message << "\n";
	U_TEST_ASSERT( syntax_analysis_result.error_messages.empty() );

	for( const std::string& syntax_error_message : syntax_analysis_result.error_messages )
		std::cout << syntax_error_message << "\n";
	U_TEST_ASSERT( syntax_analysis_result.error_messages.empty() );

	return
		CodeBuilder().BuildProgram( syntax_analysis_result.program_elements );
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
