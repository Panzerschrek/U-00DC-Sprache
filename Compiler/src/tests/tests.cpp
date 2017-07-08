#include <iostream>

#include "../assert.hpp"
#include "../lexical_analyzer.hpp"
#include "../syntax_analyzer.hpp"

#include "tests.hpp"

namespace U
{

std::unique_ptr<llvm::Module> BuildProgram( const char* const text )
{
	const LexicalAnalysisResult lexical_analysis_result=
		LexicalAnalysis( ToProgramString( text ) );

	for( const std::string& lexical_error_message : lexical_analysis_result.error_messages )
		std::cout << lexical_error_message << "\n";
	U_ASSERT( lexical_analysis_result.error_messages.empty() );

	const SyntaxAnalysisResult syntax_analysis_result=
		SyntaxAnalysis( lexical_analysis_result.lexems );

	for( const std::string& syntax_error_message : syntax_analysis_result.error_messages )
		std::cout << syntax_error_message << "\n";
	U_ASSERT( syntax_analysis_result.error_messages.empty() );

	CodeBuilder::BuildResult build_result=
		CodeBuilder().BuildProgram( syntax_analysis_result.program_elements );

	for( const CodeBuilderError& error : build_result.errors )
		std::cout << error.file_pos.line << ":" << error.file_pos.pos_in_line << " " << ToStdString( error.text ) << "\n";

	U_ASSERT( build_result.errors.empty() );

	return std::move( build_result.module );
}

CodeBuilder::BuildResult BuildProgramWithErrors( const char* const text )
{
	const LexicalAnalysisResult lexical_analysis_result=
		LexicalAnalysis( ToProgramString( text ) );

	for( const std::string& lexical_error_message : lexical_analysis_result.error_messages )
		std::cout << lexical_error_message << "\n";
	U_ASSERT( lexical_analysis_result.error_messages.empty() );

	const SyntaxAnalysisResult syntax_analysis_result=
		SyntaxAnalysis( lexical_analysis_result.lexems );

	for( const SyntaxErrorMessage& syntax_error_message : syntax_analysis_result.error_messages )
		std::cout << syntax_error_message << "\n";
	U_ASSERT( syntax_analysis_result.error_messages.empty() );

	for( const std::string& syntax_error_message : syntax_analysis_result.error_messages )
		std::cout << syntax_error_message << "\n";
	U_ASSERT( syntax_analysis_result.error_messages.empty() );

	return
		CodeBuilder().BuildProgram( syntax_analysis_result.program_elements );
}

llvm::ExecutionEngine* CreateEngine( std::unique_ptr<llvm::Module> module, const bool needs_dump )
{
	U_ASSERT( module != nullptr );

	if( needs_dump )
		module->dump();

	llvm::EngineBuilder builder( std::move(module) );
	llvm::ExecutionEngine* const engine= builder.create();

	U_ASSERT( engine != nullptr );
	return engine;
}

} // namespace U
