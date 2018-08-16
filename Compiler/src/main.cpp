#include <cstdio>
#include <cstring>
#include <iostream>

#include <boost/filesystem/path.hpp>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Triple.h>
#include <llvm/AsmParser/Parser.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetMachine.h>
#include "pop_llvm_warnings.hpp"

#include "assert.hpp"
#include "code_builder.hpp"
#include "source_graph_loader.hpp"


static bool ReadFileRaw( const char* const name, std::string& out_file_content )
{
	std::FILE* const f= std::fopen( name, "rb" );
	if( f == nullptr )
		return false;

	std::fseek( f, 0, SEEK_END );
	const size_t file_size= size_t(std::ftell( f ));
	std::fseek( f, 0, SEEK_SET );

	out_file_content.resize(file_size);

	size_t read_total= 0u;
	bool read_error= false;
	do
	{
		const size_t read= std::fread( const_cast<char*>(out_file_content.data()) + read_total, 1, file_size - read_total, f );
		if( std::ferror(f) != 0 )
		{
			read_error= true;
			break;
		}
		if( read == 0 )
			break;

		read_total+= read;
	} while( read_total < file_size );

	std::fclose(f);

	return !read_error;
}

static bool ReadFile( const char* const name, U::ProgramString& out_file_content )
{
	std::string file_content_raw;
	if( !ReadFileRaw( name, file_content_raw ) )
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
			// TODO - maybe use here native format of path string?
			if( !ReadFile( result_path.string<std::string>().c_str(), result.file_content ) )
				return boost::none;

			result.full_file_path= ToProgramString( result_path.make_preferred().string<std::string>().c_str() );
			return std::move(result);
		}
		catch( const std::exception& e )
		{
			std::cout << e.what() << std::endl;
		}

		return boost::none;
	}

	virtual Path GetFullFilePath( const Path& file_path, const Path& full_parent_file_path ) override
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
			return ToProgramString( result_path.make_preferred().string<std::string>().c_str() );
		}
		catch( const std::exception& e )
		{
			std::cout << e.what() << std::endl;
		}

		return Path();
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

	std::vector<const char*> input_files;
	const char* output_file= nullptr;
	bool print_llvm_asm= false;
	bool produce_object_file= false;

	for( int i = 1; i < argc; )
	{
		if( std::strcmp( argv[i], "-o" ) == 0 )
		{
			if( i + 1 >= argc )
			{
				std::cout << "Expeted name after\"-o\"" << std::endl;
				return 1;
			}
			output_file= argv[ i + 1 ];
			i+= 2;
		}
		else if( std::strcmp( argv[i], "--print-llvm-asm" ) == 0 )
		{
			print_llvm_asm= true;
			++i;
		}
		else if( std::strcmp( argv[i], "--produce-object-file" ) == 0 )
		{
			produce_object_file= true;
			++i;
		}
		else if( std::strcmp( argv[i], "--help" ) == 0 )
		{
			std::cout << help_message << std::endl;
			return 0;
		}
		else
		{
			input_files.push_back( argv[i] );
			++i;
		}
	}

	if( input_files.empty() )
	{
		std::cout << "No input files" << std::endl;
		return 1;
	}
	if( output_file == nullptr )
	{
		std::cout << "No output file" << std::endl;
		return 1;
	}

	// Prepare target machine.
	// Currently can work only with native target.
	// TODO - allow compiler user to change target.
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmParser();
	llvm::InitializeNativeTargetAsmPrinter();
	const std::string target_triple_str= llvm::sys::getDefaultTargetTriple();

	std::unique_ptr<llvm::TargetMachine> target_machine;
	{
		std::string error_str;
		const llvm::Target* const target= llvm::TargetRegistry::lookupTarget( target_triple_str, error_str );
		if( target == nullptr )
		{
			std::cout << "Error, selecting target: " << error_str << std::endl;
			return 1;
		}

		std::string features_str; // TODO - set features
		// TODO - set optimization level, reloc model, etc.
		target_machine.reset(
			target->createTargetMachine(
				target_triple_str,
				llvm::sys::getHostCPUName(),
				features_str,
				llvm::TargetOptions() ) );

		if( target_machine == nullptr )
		{
			std::cout << "Error, creating target machine." << std::endl;
			return 1;
		}
	}

	// Compile multiple input files and link them together.
	U::SourceGraphLoader source_graph_loader( std::make_shared<U::VfsOverSystemFS>() );
	std::unique_ptr<llvm::Module> result_module;
	bool have_some_errors= false;
	for( const char* const input_file : input_files )
	{
		const U::SourceGraphPtr source_graph= source_graph_loader.LoadSource( U::ToProgramString( input_file ) );
		U_ASSERT( source_graph != nullptr );
		if( !source_graph->lexical_errors.empty() || !source_graph->syntax_errors.empty() )
		{
			have_some_errors= true;
			continue;
		}

		U::CodeBuilder::BuildResult build_result=
			U::CodeBuilder( target_triple_str, target_machine->createDataLayout() ).BuildProgram( *source_graph );

		for( const U::CodeBuilderError& error : build_result.errors )
			std::cout << U::ToStdString( source_graph->nodes_storage[error.file_pos.file_index ].file_path )
				<< ":" << error.file_pos.line << ":" << error.file_pos.pos_in_line << " " << U::ToStdString( error.text ) << "\n";

		if( !build_result.errors.empty() )
		{
			have_some_errors= true;
			continue;
		}

		if( result_module == nullptr )
			result_module= std::move( build_result.module );
		else
		{
			const bool not_ok=
				llvm::Linker::LinkModules( result_module.get() , build_result.module.get() );
			if( not_ok )
			{
				std::cout << "Error, linking file \"" << input_file << "\"" << std::endl;
				have_some_errors= true;
			}
		}
	}

	if( have_some_errors )
		return 1;

	{ // Link stdlib with result module.
		std::string file_content;
		if( !ReadFileRaw( "asm_funcs.bc", file_content ) )
		{
			std::cout << "Internal compiler error - stdlib read error." << std::endl;
			return 1;
		}

		const llvm::ErrorOr<std::unique_ptr<llvm::Module>> std_lib_module=
			llvm::parseBitcodeFile(
				llvm::MemoryBufferRef( file_content, "ustlib asm file" ),
				result_module->getContext() );

		if( !std_lib_module )
		{
			std::cout << "Internal compiler error - stdlib parse error." << std::endl;
			return 1;
		}

		std_lib_module.get()->setDataLayout( result_module->getDataLayout() );
		std_lib_module.get()->setTargetTriple( result_module->getTargetTriple() );

		std::string err_stream_str;
		llvm::raw_string_ostream err_stream( err_stream_str );
		if( llvm::verifyModule( *std_lib_module.get(), &err_stream ) )
		{
			std::cout << "Internal compiler error - stdlib verify error:\n" << err_stream.str() << std::endl;
			return 1;
		}

		llvm::Linker::LinkModules( result_module.get(), std_lib_module.get().get() );
	}

	if( print_llvm_asm )
	{
		llvm::raw_os_ostream stream(std::cout);
		result_module->print( stream, nullptr );
	}

	std::error_code file_error_code;
	llvm::raw_fd_ostream out_file_stream( output_file, file_error_code, llvm::sys::fs::F_None );

	if( produce_object_file )
	{
		llvm::PassRegistry& registry= *llvm::PassRegistry::getPassRegistry();
		llvm::initializeCore(registry);
		llvm::initializeCodeGen(registry);
		llvm::initializeLoopStrengthReducePass(registry);
		llvm::initializeLowerIntrinsicsPass(registry);
		llvm::initializeUnreachableBlockElimPass(registry);

		const llvm::TargetMachine::CodeGenFileType file_type= llvm::TargetMachine::CodeGenFileType::CGFT_ObjectFile;
		const bool no_verify= true;
		llvm::AnalysisID start_before_id= nullptr;
		llvm::AnalysisID start_after_id = nullptr;
		llvm::AnalysisID stop_after_id = nullptr;

		llvm::legacy::PassManager pass_manager;

		if( target_machine->addPassesToEmitFile(
				pass_manager,
				out_file_stream,
				file_type,
				no_verify,
				start_before_id,
				start_after_id,
				stop_after_id,
				nullptr) )
		{
			std::cout << "Error, creating file emit pass." << std::endl;
			return 1;
		}

		pass_manager.run(*result_module);
	}
	else
	{
		llvm::WriteBitcodeToFile( result_module.get(), out_file_stream );
	}

	out_file_stream.flush();
	if( out_file_stream.has_error() )
	{
		std::cout << "Error while writing output file \"" << output_file << "\"" << std::endl;
		return 1;
	}

	return 0;
}
