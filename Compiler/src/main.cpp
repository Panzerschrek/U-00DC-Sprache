#include <cstdio>
#include <cstring>
#include <iostream>

#include <boost/filesystem/path.hpp>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include "pop_llvm_warnings.hpp"

#include "assert.hpp"
#include "code_builder.hpp"
#include "source_graph_loader.hpp"

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

namespace U
{

namespace fs= boost::filesystem;

class VfsOverSystemFS final : public IVfs
{
public:
	virtual boost::optional<LoadFileResult> LoadFileContent( const Path& file_path, const Path& full_parent_file_path ) override
	{
		try
		{
			const fs::path file_path_r( ToStdString(file_path) );
			fs::path result_path;
			if( full_parent_file_path.empty() || file_path_r.is_absolute() )
				result_path= file_path_r;
			else
			{
				const fs::path base_dir= fs::path( ToStdString(full_parent_file_path) ).parent_path();
				result_path= base_dir / file_path_r;
			}

			LoadFileResult result;
			if( !ReadFile( result_path.c_str(), result.file_content ) )
				return boost::none;

			return std::move(result);
		}
		catch( const std::exception& e )
		{
			std::cout << e.what() << std::endl;
		}

		return boost::none;
	}
};

} // namespace U

int main( const int argc, const char* const argv[])
{
	static const char help_message[]=
	R"(
Ü-Sprache compiler
Usage:
	Compiler -i [input_file] -o [output_ir_file] [--print-llvm-asm])";

	if( argc <= 1 )
	{
		std::cout << help_message << std::endl;
		return 1;
	}

	const char* input_file= nullptr;
	const char* output_file= nullptr;
	bool print_llvm_asm= false;

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
		else if( std::strcmp( argv[i], "--print-llvm-asm" ) == 0 )
		{
			print_llvm_asm= true;
		}
		else if( std::strcmp( argv[i], "--help" ) == 0 )
		{
			std::cout << help_message << std::endl;
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

	// Source graph loading (inluding lex & synth).
	U::SourceGraphLoader source_gramph_loader( std::make_shared<U::VfsOverSystemFS>() );
	const U::SourceGraphPtr source_graph= source_gramph_loader.LoadSource( U::ToProgramString( input_file ) );
	U_ASSERT( source_graph != nullptr );
	if( !source_graph->lexical_errors.empty() || !source_graph->syntax_errors.empty() )
		return 1;

	// Code build
	U::CodeBuilder::BuildResult build_result=
		U::CodeBuilder().BuildProgram( *source_graph );

	for( const U::CodeBuilderError& error : build_result.errors )
		std::cout << input_file << ":" << error.file_pos.line << ":" << error.file_pos.pos_in_line << " " << U::ToStdString( error.text ) << "\n";

	if( !build_result.errors.empty() )
		return 1;

	if( print_llvm_asm )
	{
		llvm::raw_os_ostream stream(std::cout);
		build_result.module->print(stream, nullptr);
	}

	// LLVM ir dump
	std::error_code file_error_code;
	llvm::raw_fd_ostream file( output_file , file_error_code, llvm::sys::fs::F_None );

	llvm::WriteBitcodeToFile( build_result.module.get(), file );
	file.flush();

	if( file.has_error() )
	{
		std::cout << "Error while writing output file \"" << output_file << "\"" << std::endl;
		return 1;
	}

	return 0;
}
