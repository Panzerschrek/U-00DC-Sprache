#include <iostream>

#include "../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/AsmParser/Parser.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/MC/SubtargetFeature.h>
#include "../code_builder_lib/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/source_graph_loader.hpp"
#include "../code_builder_lib/code_builder.hpp"
#include "../sprache_version/sprache_version.hpp"

namespace U
{

namespace
{

namespace fs= llvm::sys::fs;
namespace fsp= llvm::sys::path;

class VfsOverSystemFS final : public IVfs
{
	struct PrivateTag{};
	using fs_path= llvm::SmallString<256>;

public:
	static std::shared_ptr<VfsOverSystemFS> Create( const std::vector<std::string>& include_dirs )
	{
		std::vector<fs_path> result_include_dirs;

		bool all_ok= true;
		for( const std::string& include_dir : include_dirs )
		{
			fs_path dir_path= llvm::StringRef( include_dir );
			fs::make_absolute(dir_path);
			if( !fs::exists(dir_path) )
			{
				std::cout << "include dir \"" << include_dir << "\" does not exists." << std::endl;
				all_ok= false;
				continue;
			}
			if( !fs::is_directory(dir_path) )
			{
				std::cout << "\"" << include_dir << "\" is not a directory." << std::endl;
				all_ok= false;
				continue;
			}

			result_include_dirs.push_back( std::move(dir_path) );
		}

		if( !all_ok )
			return nullptr;

		return std::make_shared<VfsOverSystemFS>( std::move(result_include_dirs), PrivateTag() );
	}

	VfsOverSystemFS( std::vector<fs_path> include_dirs, PrivateTag )
		: include_dirs_(std::move(include_dirs))
	{}

public:
	virtual std::optional<LoadFileResult> LoadFileContent( const Path& file_path, const Path& full_parent_file_path ) override
	{
		fs_path result_path= GetFullFilePathInternal( file_path, full_parent_file_path );
		if( result_path.empty() )
			return std::nullopt;

		LoadFileResult result;

		llvm::ErrorOr< std::unique_ptr<llvm::MemoryBuffer> > file_mapped=
			llvm::MemoryBuffer::getFile( result_path );
		if( !file_mapped || *file_mapped == nullptr )
			return std::nullopt;

		result.file_content.assign( (*file_mapped)->getBufferStart(), (*file_mapped)->getBufferEnd() );
		result.full_file_path= result_path.str();
		return std::move(result);
	}

	virtual Path GetFullFilePath( const Path& file_path, const Path& full_parent_file_path ) override
	{
		return GetFullFilePathInternal( file_path, full_parent_file_path ).str();
	}

private:
	fs_path GetFullFilePathInternal( const Path& file_path, const Path& full_parent_file_path )
	{
		const fs_path file_path_r( file_path );
		fs_path result_path;

		if( full_parent_file_path.empty() )
		{
			result_path= file_path_r;
			fs::make_absolute(result_path);
		}
		else if( !file_path.empty() && file_path[0] == '/' )
		{
			// If file path is absolute, like "/some_lib/some_file.u" search file in include dirs.
			// Return real file system path to first existent file.
			for( const fs_path& include_dir : include_dirs_ )
			{
				fs_path full_file_path= include_dir;
				fsp::append( full_file_path, file_path_r );
				if( fs::exists( full_file_path ) && fs::is_regular_file( full_file_path ) )
				{
					result_path= full_file_path;
					break;
				}
			}
		}
		else
		{
			result_path= fsp::parent_path( full_parent_file_path );
			fsp::append( result_path, file_path_r );
		}
		return NormalizePath( result_path );
	}

	static fs_path NormalizePath( const fs_path& p )
	{
		fs_path result;
		for( auto it= llvm::sys::path::begin(p), it_end= llvm::sys::path::end(p); it != it_end; ++it)
		{
			if( it->size() == 1 && *it == "." )
				continue;
			if( it->size() == 2 && *it == ".." )
				llvm::sys::path::remove_filename( result );
			else
				llvm::sys::path::append( result, *it );
		}
		return result;
	}

private:
	const std::vector<fs_path> include_dirs_;
};

std::string GetFeaturesStr( const llvm::ArrayRef<std::string> features_list )
{
	llvm::SubtargetFeatures features;
	for( auto& f : features_list )
		features.AddFeature( f, true );
	return features.getString();
}

std::string GetNativeTargetFeaturesStr()
{
	llvm::SubtargetFeatures features;

	llvm::StringMap<bool> host_features;
	if( llvm::sys::getHostCPUFeatures(host_features) )
	{
		for( auto& f : host_features )
			features.AddFeature( f.first(), f.second );
	}

	return features.getString();
}

void PrintErrors( const SourceGraph& source_graph, const CodeBuilderErrorsContainer& errors )
{
	for( const CodeBuilderError& error : errors )
	{
		if( error.code == CodeBuilderErrorCode::TemplateContext )
		{
			U_ASSERT( error.template_context != nullptr );

			std::cerr << source_graph.nodes_storage[ error.template_context->template_declaration_file_pos.GetFileIndex() ].file_path << ": "
				<< "In instantiation of \"" << error.template_context->template_name
				<< "\" " << error.template_context->parameters_description
				<< "\n";

			std::cerr << source_graph.nodes_storage[error.file_pos.GetFileIndex() ].file_path
				<< ":" << error.file_pos.GetLine() << ":" << error.file_pos.GetColumn() << ": required from here: " << "\n";
		}
		else
		{
			std::cerr << source_graph.nodes_storage[error.file_pos.GetFileIndex() ].file_path
				<< ":" << error.file_pos.GetLine() << ":" << error.file_pos.GetColumn() << ": error: " << error.text << "\n";
		}

		if( error.template_context != nullptr )
			PrintErrors( source_graph, error.template_context->errors );
	}
}

void PrintAvailableTargets()
{
	std::string targets_list;
	for( const llvm::Target& target : llvm::TargetRegistry::targets() )
	{
		if( !targets_list.empty() )
			targets_list+= ", ";
		targets_list+= target.getName();
	}
	std::cout << "Available targets: " << targets_list << std::endl;
}

namespace Options
{

namespace cl= llvm::cl;

cl::OptionCategory options_category( "Ü compier options" );

cl::list<std::string> input_files(
	cl::Positional,
	cl::desc("<source0> [... <sourceN>]"),
	cl::value_desc("iinput files"),
	cl::OneOrMore,
	cl::cat(options_category) );

cl::opt<std::string> output_file_name(
	"o",
	cl::desc("Output filename"),
	cl::value_desc("filename"),
	cl::Required,
	cl::cat(options_category) );

cl::list<std::string> include_dir(
	"include-dir",
	cl::Prefix,
	cl::desc("<dir0> [... <dirN>]"),
	cl::value_desc("directory for search of \"import\" files"),
	cl::ZeroOrMore,
	cl::cat(options_category));

enum class FileType{ BC, LL, Obj, Asm };
cl::opt< FileType > file_type(
	"filetype",
	cl::init(FileType::Obj),
	cl::desc("Choose a file type (not all types are supported by all targets):"),
	cl::values(
		clEnumValN( FileType::BC, "bc", "Emit an llvm bitcode ('.bc') file" ),
		clEnumValN( FileType::LL, "ll", "Emit an llvm asm ('.ll') file" ),
		clEnumValN( FileType::Obj, "obj", "Emit a native object ('.o') file" ),
		clEnumValN( FileType::Asm, "asm", "Emit an assembly ('.s') file" )),
	cl::cat(options_category) );

cl::opt<char> optimization_level(
	"O",
	cl::desc("Optimization level. [-O0, -O1, -O2, -O3, -Os or -Oz] (default = '-O0')"),
	cl::Prefix,
	cl::Optional,
	cl::init('0'),
	cl::cat(options_category) );

cl::opt<bool> generate_debug_info(
	"g",
	cl::desc("Generate debug information."),
	cl::init(false),
	cl::cat(options_category) );

cl::opt<std::string> architecture(
	"march",
	cl::desc("Architecture to generate code for (see --version)"),
	cl::init("native"),
	cl::cat(options_category) );

cl::opt<std::string> target_cpu(
	"mcpu",
	cl::desc("Target a specific cpu type (-mcpu=help for details)"),
	cl::value_desc("cpu-name"),
	cl::init(""),
	cl::cat(options_category) );

cl::list<std::string> target_attributes(
	"mattr",
	cl::CommaSeparated,
	cl::desc("Target specific attributes (-mattr=help for details)"),
	cl::value_desc("a1,+a2,-a3,..."),
	cl::cat(options_category) );

cl::opt<llvm::Reloc::Model> relocation_model(
	"relocation-model",
	cl::desc("Choose relocation model"),
	cl::init(llvm::Reloc::PIC_),
	cl::values(
		clEnumValN( llvm::Reloc::Static, "static", "Non-relocatable code" ),
		clEnumValN( llvm::Reloc::PIC_, "pic", "Fully relocatable, position independent code" ),
		clEnumValN( llvm::Reloc::DynamicNoPIC, "dynamic-no-pic", "Relocatable external references, non-relocatable code" ),
		clEnumValN( llvm::Reloc::ROPI, "ropi", "Code and read-only data relocatable, accessed PC-relative" ),
		clEnumValN( llvm::Reloc::RWPI, "rwpi", "Read-write data relocatable, accessed relative to static base" ),
		clEnumValN( llvm::Reloc::ROPI_RWPI, "ropi-rwpi", "Combination of ropi and rwpi") ),
	cl::cat(options_category) );

cl::opt<llvm::CodeModel::Model> code_model(
	"code-model",
	cl::desc("Choose code model"),
	cl::values(
		clEnumValN( llvm::CodeModel::Tiny, "tiny", "Tiny code model" ),
		clEnumValN( llvm::CodeModel::Small, "small", "Small code model" ),
		clEnumValN( llvm::CodeModel::Kernel, "kernel", "Kernel code model" ),
		clEnumValN( llvm::CodeModel::Medium, "medium", "Medium code model" ),
		clEnumValN( llvm::CodeModel::Large, "large", "Large code model") ),
	cl::cat(options_category));

cl::opt<bool> tests_output(
	"tests-output",
	cl::desc("Print code builder errors in test mode."),
	cl::init(false),
	cl::cat(options_category) );

cl::opt<bool> print_llvm_asm(
	"print-llvm-asm",
	cl::desc("Print LLVM code."),
	cl::init(false),
	cl::cat(options_category) );

} // namespace Options

int Main( int argc, const char* argv[] )
{
	const llvm::InitLLVM llvm_initializer(argc, argv);

	// Options
	llvm::cl::SetVersionPrinter(
		[]( llvm::raw_ostream& ) {
			std::cout << "Ü-Sprache version " << getFullVersion() << ", llvm version " << LLVM_VERSION_STRING << std::endl;
			llvm::InitializeAllTargets();
			PrintAvailableTargets();
		} );

	llvm::cl::HideUnrelatedOptions( Options::options_category );
	llvm::cl::ParseCommandLineOptions( argc, argv, "Ü-Sprache compiler\n" );

	// Select optimization level.
	unsigned int optimization_level= 0u;
	unsigned int size_optimization_level= 0u;
		 if( Options::optimization_level == '0' )
		optimization_level= 0u;
	else if( Options::optimization_level == '1' )
		optimization_level= 1u;
	else if( Options::optimization_level == '2' )
		optimization_level= 2u;
	else if( Options::optimization_level == '3' )
		optimization_level= 3u;
	else if( Options::optimization_level == 's' )
	{
		size_optimization_level= 1u;
		optimization_level= 2u;
	}
	else if( Options::optimization_level == 'z' )
	{
		size_optimization_level= 2u;
		optimization_level= 2u;
	}
	else
	{
		std::cout << "Unknown optimization: " << Options::optimization_level << std::endl;
		return 1;
	}

	// Prepare target machine.
	std::string target_triple_str;
	std::unique_ptr<llvm::TargetMachine> target_machine;
	{
		llvm::InitializeAllTargets();
		llvm::InitializeAllTargetMCs();
		llvm::InitializeAllAsmPrinters();
		llvm::InitializeAllAsmParsers();

		const llvm::Target* target= nullptr;
		std::string error_str, features_str, cpu_name;
		if( Options::architecture == "native" )
		{
			target_triple_str= llvm::sys::getDefaultTargetTriple();
			target= llvm::TargetRegistry::lookupTarget( target_triple_str, error_str );
			features_str= GetNativeTargetFeaturesStr();
			cpu_name= llvm::sys::getHostCPUName();
		}
		else
		{
			llvm::Triple traget_triple;
			target= llvm::TargetRegistry::lookupTarget( Options::architecture, traget_triple, error_str );
			target_triple_str= traget_triple.getTriple();
			features_str= GetFeaturesStr( Options::target_attributes );
			cpu_name= Options::target_cpu;
		}

		if( target == nullptr )
		{
			std::cerr << "Error, selecting target: " << error_str << std::endl;
			PrintAvailableTargets();
			return 1;
		}

		llvm::TargetOptions target_options;

		llvm::CodeGenOpt::Level code_gen_optimization_level;
		if( optimization_level >= 2u || size_optimization_level > 0u )
			code_gen_optimization_level= llvm::CodeGenOpt::Default;
		else if( optimization_level == 1u )
			code_gen_optimization_level= llvm::CodeGenOpt::Less;
		else if( optimization_level == 3u )
			code_gen_optimization_level= llvm::CodeGenOpt::Aggressive;
		else
			code_gen_optimization_level= llvm::CodeGenOpt::None;

		target_machine.reset(
			target->createTargetMachine(
				target_triple_str,
				cpu_name,
				features_str,
				target_options,
				Options::relocation_model.getValue(),
				Options::code_model.getNumOccurrences() > 0 ? Options::code_model.getValue() : llvm::Optional<llvm::CodeModel::Model>(),
				code_gen_optimization_level ) );

		if( target_machine == nullptr )
		{
			std::cout << "Error, creating target machine." << std::endl;
			return 1;
		}
	}
	const llvm::DataLayout data_layout= target_machine->createDataLayout();

	const auto vfs= VfsOverSystemFS::Create( Options::include_dir );
	if( vfs == nullptr )
		return 1u;

	// Compile multiple input files and link them together.
	SourceGraphLoader source_graph_loader( vfs );
	llvm::LLVMContext llvm_context;
	std::unique_ptr<llvm::Module> result_module;
	bool have_some_errors= false;
	for( const std::string& input_file : Options::input_files )
	{
		const SourceGraphPtr source_graph= source_graph_loader.LoadSource( input_file );
		U_ASSERT( source_graph != nullptr );
		if( source_graph->have_errors || !source_graph->lexical_errors.empty() || !source_graph->syntax_errors.empty() )
		{
			have_some_errors= true;
			continue;
		}

		CodeBuilder::BuildResult build_result=
			CodeBuilder(
				llvm_context,
				target_triple_str,
				data_layout,
				Options::generate_debug_info ).BuildProgram( *source_graph );

		if( Options::tests_output )
		{
			// For tests we print errors as "file.u 88 NameNotFound"
			for( const CodeBuilderError& error : build_result.errors )
				std::cout << source_graph->nodes_storage[error.file_pos.GetFileIndex() ].file_path
					<< " " << error.file_pos.GetLine() << " " << CodeBuilderErrorCodeToString( error.code ) << "\n";
		}
		else
		{
			PrintErrors( *source_graph, build_result.errors );
		}

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
				llvm::Linker::linkModules( *result_module, std::move(build_result.module) );
			if( not_ok )
			{
				std::cout << "Error, linking file \"" << input_file << "\"" << std::endl;
				have_some_errors= true;
			}
		}
	}

	if( have_some_errors )
		return 1;

// Generated by "bin2c.cmake" arrays.
#include "asm_funcs.h"
#include "asm_funcs_32.h"
#include "asm_funcs_64.h"
	
	// Prepare stdlib modules set.
	const llvm::StringRef asm_funcs_modules[]=
	{
		llvm::StringRef( reinterpret_cast<const char*>(c_asm_funcs_file_content), sizeof(c_asm_funcs_file_content) ),
		( data_layout.getPointerSizeInBits() == 32u
			? llvm::StringRef( reinterpret_cast<const char*>(c_asm_funcs_32_file_content), sizeof(c_asm_funcs_32_file_content) )
			: llvm::StringRef( reinterpret_cast<const char*>(c_asm_funcs_64_file_content), sizeof(c_asm_funcs_64_file_content) ) ),
	};

	// Link stdlib with result module.
	for( const llvm::StringRef& asm_funcs_module : asm_funcs_modules )
	{
		llvm::Expected<std::unique_ptr<llvm::Module>> std_lib_module=
			llvm::parseBitcodeFile(
				llvm::MemoryBufferRef( asm_funcs_module, "ustlib asm file" ),
				result_module->getContext() );

		if( !std_lib_module )
		{
			std::cout << "Internal compiler error - stdlib module parse error" << std::endl;
			return 1;
		}

		std_lib_module.get()->setDataLayout( data_layout );
		std_lib_module.get()->setTargetTriple( target_triple_str );

		std::string err_stream_str;
		llvm::raw_string_ostream err_stream( err_stream_str );
		if( llvm::verifyModule( *std_lib_module.get(), &err_stream ) )
		{
			std::cout << "Internal compiler error - stdlib module verify error:\n" << err_stream.str() << std::endl;
			return 1;
		}

		llvm::Linker::linkModules( *result_module, std::move(std_lib_module.get()) );
	}

	if( optimization_level > 0u || size_optimization_level > 0u )
	{
		llvm::legacy::FunctionPassManager function_pass_manager( result_module.get() );
		llvm::legacy::PassManager pass_manager;

		// Setup target-dependent optimizations.
		pass_manager.add( llvm::createTargetTransformInfoWrapperPass( target_machine->getTargetIRAnalysis() ) );

		{
			llvm::PassManagerBuilder pass_manager_builder;
			pass_manager_builder.OptLevel = optimization_level;
			pass_manager_builder.SizeLevel = size_optimization_level;

			if( optimization_level == 0u )
				pass_manager_builder.Inliner= nullptr;
			else
				pass_manager_builder.Inliner= llvm::createFunctionInliningPass( optimization_level, size_optimization_level, false );

			pass_manager_builder.populateFunctionPassManager(function_pass_manager);
			pass_manager_builder.populateModulePassManager(pass_manager);
		}

		// Run per-function optimizations.
		function_pass_manager.doInitialization();
		for( llvm::Function& func : *result_module )
			function_pass_manager.run(func);
		function_pass_manager.doFinalization();

		// Run optimizations for module.
		pass_manager.run( *result_module );
	}

	if( Options::print_llvm_asm )
	{
		llvm::raw_os_ostream stream(std::cout);
		result_module->print( stream, nullptr );
	}

	std::error_code file_error_code;
	llvm::raw_fd_ostream out_file_stream( Options::output_file_name, file_error_code, llvm::sys::fs::F_None );

	if( Options::file_type == Options::FileType::BC )
		llvm::WriteBitcodeToFile( *result_module, out_file_stream );
	else if( Options::file_type == Options::FileType::LL )
		result_module->print( out_file_stream, nullptr );
	else
	{
		llvm::PassRegistry& registry= *llvm::PassRegistry::getPassRegistry();
		llvm::initializeCore(registry);
		llvm::initializeCodeGen(registry);
		llvm::initializeIPO(registry);
		llvm::initializeLoopStrengthReducePass(registry);
		llvm::initializeLowerIntrinsicsPass(registry);
		llvm::initializeScalarOpts(registry);
		llvm::initializeVectorization(registry);
		llvm::initializeScalarizeMaskedMemIntrinPass(registry);
		llvm::initializeExpandReductionsPass(registry);
		llvm::initializeHardwareLoopsPass(registry);

		llvm::TargetMachine::CodeGenFileType file_type= llvm::TargetMachine::CGFT_Null;
		switch( Options::file_type )
		{
		case Options::FileType::Obj: file_type= llvm::TargetMachine::CGFT_ObjectFile; break;
		case Options::FileType::Asm: file_type= llvm::TargetMachine::CGFT_AssemblyFile; break;
		case Options::FileType::BC:
		case Options::FileType::LL:
		U_ASSERT(false);
		};

		llvm::legacy::PassManager pass_manager;

		if( target_machine->addPassesToEmitFile(
				pass_manager,
				out_file_stream,
				nullptr,
				file_type ) )
		{
			std::cout << "Error, creating file emit pass." << std::endl;
			return 1;
		}

		pass_manager.run(*result_module);
	}

	out_file_stream.flush();
	if( out_file_stream.has_error() )
	{
		std::cout << "Error while writing output file \"" << Options::output_file_name << "\"" << std::endl;
		return 1;
	}

	return 0;
}

} // namespace U

} // namespace

int main( const int argc, const char* argv[] )
{
	// Place actual "main" body inside "U" namespace.
	return U::Main( argc, argv );
}
