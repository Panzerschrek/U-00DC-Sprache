#include <cstdio>
#include <cstring>
#include <iostream>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include "pop_llvm_warnings.hpp"

#include "code_builder.hpp"
#include "lexical_analyzer.hpp"
#include "program_string.hpp"
#include "syntax_analyzer.hpp"

static bool ReadFile( const char* const name, U::ProgramString& out_file_content )
{
	std::FILE* const f= std::fopen( name, "rb" );
	if( f == nullptr )
		return false;

	std::fseek( f, 0, SEEK_END );
	const unsigned int file_size= std::ftell( f );
	std::fseek( f, 0, SEEK_SET );

	std::vector<char> file_content_raw( file_size );

	unsigned int read_total= 0u;
	bool read_error= false;
	do
	{
		const int read= std::fread( static_cast<char*>(file_content_raw.data()) + read_total, 1, file_size - read_total, f );
		if( read < 0 )
		{
			read_error= true;
			break;
		}
		if( read == 0 )
			break;

		read_total+= read;
	} while( read_total < file_size );

	std::fclose(f);

	if( read_error )
		return false;

	out_file_content= U::DecodeUTF8( file_content_raw );
	return true;
}

int main( const int argc, const char* const argv[])
{
	std::cout << u8"Ü-Sprache Compiler" << std::endl;

	if( argc <= 1 )
	{
		std::cout << "No input" << std::endl;
		return 1;
	}

	const char* input_file= nullptr;
	const char* output_file= nullptr;

	for( int i = 1; i < argc; i++ )
	{
		if( std::strcmp( argv[i], "-i" ) == 0 )
		{
			if( i + 1 >= argc )
			{
				std::cout << "Expeted name after\"-i\"" << std::endl;
				return 1;
			}
			input_file= argv[ i + 1 ];
		}
		else
		if( std::strcmp( argv[i], "-o" ) == 0 )
		{
			if( i + 1 >= argc )
			{
				std::cout << "Expeted name after\"-o\"" << std::endl;
				return 1;
			}
			output_file= argv[ i + 1 ];
		}
		else if( std::strcmp( argv[i], "--help" ) == 0 )
		{
			std::cout << "Usage:\n" << "-i [input_file] -o [output_ir_file]" << std::endl;
			return 0;
		}
	}

	if( input_file == nullptr )
	{
		std::cout << "No input file" << std::endl;
		return 1;
	}
	if( output_file == nullptr )
	{
		std::cout << "No output file" << std::endl;
		return 1;
	}

	U::ProgramString input_file_content;
	if( ! ReadFile( input_file, input_file_content ) )
	{
		std::cout << "Can not read input file \"" << input_file << "\"" << std::endl;
		return false;
	}

	// lex
	const U::LexicalAnalysisResult lexical_analysis_result=
		U::LexicalAnalysis( input_file_content );

	for( const std::string& lexical_error_message : lexical_analysis_result.error_messages )
		std::cout << lexical_error_message << "\n";

	if( !lexical_analysis_result.error_messages.empty() )
		return 1;

	// Syntax
	const U::SyntaxAnalysisResult syntax_analysis_result=
		U::SyntaxAnalysis( lexical_analysis_result.lexems );

	for( const std::string& syntax_error_message : syntax_analysis_result.error_messages )
		std::cout << syntax_error_message << "\n";

	if( !syntax_analysis_result.error_messages.empty() )
		return 1;

	U::CodeBuilder::BuildResult build_result=
		U::CodeBuilder().BuildProgram( syntax_analysis_result.program_elements );

	for( const U::CodeBuilderError& error : build_result.errors )
		std::cout << error.file_pos.line << ":" << error.file_pos.pos_in_line << " " << U::ToStdString( error.text ) << "\n";

	if( !build_result.errors.empty() )
		return 1;

	// LLVM ir dump
	std::error_code file_error_code;
	llvm::raw_fd_ostream file( output_file , file_error_code, llvm::sys::fs::F_None );

	llvm::WriteBitcodeToFile( build_result.module.get(), file );
	file.flush();

	// TODO - maybe add some file write error messages?

	return 0;
}
