#include <cstdio>
#include <iostream>
#include <vector>

#include "lexical_analyzer.hpp"
#include "syntax_analyzer.hpp"

int main()
{
	std::cout << u8"Ü-Sprache Interpreter" << std::endl;

	std::FILE* f= std::fopen( "test_programs/one.u", "rb" );
	if( f == nullptr )
	{
		std::cout << "File not found" << std::endl;
		return -1;
	}

	std::fseek( f, 0, SEEK_END );
	std::vector<char> file_content( std::ftell(f) );

	std::fseek( f, 0, SEEK_SET );
	std::fread( file_content.data(), 1, file_content.size(), f );
	std::fclose( f );

	// Decode ascii to wide bytes.
	Interpreter::ProgramString program( file_content.size(), 0 );
	for( unsigned int n= 0; n < file_content.size(); n++ )
		program[n]= file_content[n];

	Interpreter::LexicalAnalysisResult lexical_result= Interpreter::LexicalAnalysis( program );

	for( const Interpreter::LexicalErrorMessage& error_message : lexical_result.error_messages )
		std::cout << error_message << "\n";

	for( const Interpreter::Lexem& lexem : lexical_result.lexems )
		std::cout << Interpreter::ToStdString( lexem.text ) << "\n";

	Interpreter::SyntaxAnalysisResult syntax_result= Interpreter::SyntaxAnalysis( lexical_result.lexems );

	for( const Interpreter::SyntaxErrorMessage& error_message : syntax_result.error_messages )
		std::cout << error_message << "\n";

	return 0;
}
