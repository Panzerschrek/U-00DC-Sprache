#include <iostream>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/AsmParser/Parser.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/InitializePasses.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/GlobalDCE.h>
#include <llvm/Transforms/IPO/Internalize.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../sprache_version/sprache_version.hpp"
#include "../code_builder_lib_common/source_file_contents_hash.hpp"
#include "../compiler0/code_builder_lib/code_builder.hpp"
#include "../compilers_support_lib/errors_print.hpp"
#include "../compilers_support_lib/prelude.hpp"
#include "../compilers_support_lib/ustlib.hpp"
#include "../compilers_support_lib/vfs.hpp"

namespace U
{

namespace
{

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

using MainFunctionType= int(*)();

namespace Options
{

namespace cl= llvm::cl;

cl::OptionCategory options_category( "Ü interpreter options" );

cl::list<std::string> input_files(
	cl::Positional,
	cl::desc("<source0> [... <sourceN>]"),
	cl::value_desc("input files"),
	cl::OneOrMore,
	cl::cat(options_category) );

cl::list<std::string> include_dir(
	"include-dir",
	cl::Prefix,
	cl::desc("Add directory for search of \"import\" files"),
	cl::value_desc("dir"),
	cl::ZeroOrMore,
	cl::cat(options_category));

cl::opt<std::string> entry_point_name(
	"entry",
	cl::desc("Name of entry point function. Default is \"main\"."),
	cl::init("main"),
	cl::cat(options_category));

} // namespace Options

int Main( int argc, const char* argv[] )
{
	const llvm::InitLLVM llvm_initializer(argc, argv);

	// Options
	llvm::cl::SetVersionPrinter(
		[]( llvm::raw_ostream& )
		{
			std::cout << "Ü-Sprache version " << getFullVersion() << ", llvm version " << LLVM_VERSION_STRING << std::endl;
		} );

	llvm::cl::HideUnrelatedOptions( Options::options_category );

	const auto description=
		"Ü-Sprache interpreter.\n"
		"Compiles provided files and emmideately executes result.\n"
		"Uses JIT.\n";
	llvm::cl::ParseCommandLineOptions( argc, argv, description );

	// LLVM stuff initialization.
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();

	// Prepare target machine.
	std::string target_triple_str;
	llvm::Triple target_triple( llvm::sys::getProcessTriple() );
	std::unique_ptr<llvm::TargetMachine> target_machine;
	{
		target_triple_str= target_triple.normalize();

		std::string error_str;
		const llvm::Target* const target= llvm::TargetRegistry::lookupTarget( target_triple_str, error_str );
		if( target == nullptr )
		{
			std::cerr << "Error, selecting target: " << error_str << std::endl;
			return 1;
		}

		llvm::TargetOptions target_options;

		target_machine.reset(
			target->createTargetMachine(
				target_triple_str,
				llvm::sys::getHostCPUName(),
				GetNativeTargetFeaturesStr(),
				target_options,
				llvm::Reloc::PIC_,
				llvm::Optional<llvm::CodeModel::Model>(),
				llvm::CodeGenOpt::None ) );

		if( target_machine == nullptr )
		{
			std::cerr << "Error, creating target machine." << std::endl;
			return 1;
		}
	}
	const llvm::DataLayout data_layout= target_machine->createDataLayout();

	const auto vfs= CreateVfsOverSystemFS( Options::include_dir );
	if( vfs == nullptr )
		return 1u;

	const std::string prelude_code=
		GenerateCompilerPreludeCode(
			target_triple,
			data_layout,
			target_machine->getTargetFeatureString(),
			target_machine->getTargetCPU(),
			'0',
			false,
			0 );

	llvm::LLVMContext llvm_context;
	std::unique_ptr<llvm::Module> result_module;

	const auto errors_format= ErrorsFormat::GCC;

	bool have_some_errors= false;
	for( const std::string& input_file : Options::input_files )
	{
		const SourceGraph source_graph= LoadSourceGraph( *vfs, CalculateSourceFileContentsHash, input_file, prelude_code );

		std::vector<IVfs::Path> dependent_files;
		dependent_files.reserve( source_graph.nodes_storage.size() );
		for( const SourceGraph::Node& node : source_graph.nodes_storage )
			dependent_files.push_back( node.file_path );

		PrintLexSyntErrors( dependent_files, source_graph.errors, errors_format );
		if( !source_graph.errors.empty() )
		{
			have_some_errors= true;
			continue;
		}

		CodeBuilder::BuildResult build_result=
			CodeBuilder(
				llvm_context,
				data_layout,
				target_triple,
				CodeBuilderOptions() ).BuildProgram( source_graph );

		PrintErrors( dependent_files, build_result.errors, errors_format );

		if( !build_result.errors.empty() || build_result.module == nullptr )
		{
			have_some_errors= true;
			continue;
		}

		if( result_module == nullptr )
			result_module= std::move( build_result.module );
		else
		{
			const bool not_ok= llvm::Linker::linkModules( *result_module, std::move(build_result.module) );
			if( not_ok )
			{
				std::cerr << "Error, linking file \"" << input_file << "\"" << std::endl;
				have_some_errors= true;
			}
		}
	}

	if( have_some_errors )
		return 1;

	if( !LinkUstLibModules( *result_module, HaltMode::Abort, false ) )
		return 1;

	// TODO - run here optimizations?

	llvm::EngineBuilder builder(std::move(result_module));
	builder.setEngineKind(llvm::EngineKind::JIT);
	builder.setMemoryManager(std::make_unique<llvm::SectionMemoryManager>());
	const std::unique_ptr<llvm::ExecutionEngine> engine(builder.create(target_machine.release())); // Engine takes ownership over target machine.

	// TODO - support main with empty args or argc+argv.
	const auto main_function= reinterpret_cast<MainFunctionType>(engine->getFunctionAddress(Options::entry_point_name));
	if( main_function == nullptr )
	{
		std::cerr << "Can't find entry point!" << std::endl;

		return 1;
	}

	return main_function();
}

} // namespace

} // namespace U

int main( const int argc, const char* argv[] )
{
	// Place actual "main" body inside "U" namespace.
	return U::Main( argc, argv );
}
