#include <iostream>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/AsmParser/Parser.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../lex_synt_lib_common/assert.hpp"
#include "../sprache_version/sprache_version.hpp"
#include  "code_builder_launcher.hpp"
#include "dep_file.hpp"
#include "vfs.hpp"

namespace U
{

namespace
{

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

void PrintErrors( const std::vector<IVfs::Path>& source_files, const CodeBuilderErrorsContainer& errors, const ErrorsFormat format )
{
	for( const CodeBuilderError& error : errors )
	{
		if( format == ErrorsFormat::MSVC )
		{
			if( error.code == CodeBuilderErrorCode::TemplateContext )
			{
				U_ASSERT( error.template_context != nullptr );

				PrintErrors( source_files, error.template_context->errors, format );

				std::cerr << source_files[ error.template_context->context_declaration_file_pos.GetFileIndex() ]
					<< "(" << error.template_context->context_declaration_file_pos.GetLine() << "): note: "
					<< "In instantiation of \"" << error.template_context->context_name
					<< "\" " << error.template_context->parameters_description
					<< "\n";

				std::cerr << source_files[ error.file_pos.GetFileIndex() ]
					<< "(" << error.file_pos.GetLine() << "): note: " << error.text << "\n";
			}
			else if( error.code == CodeBuilderErrorCode::MacroExpansionContext )
			{
				PrintErrors( source_files, error.template_context->errors, format );

				std::cerr << source_files[ error.template_context->context_declaration_file_pos.GetFileIndex() ]
					<< "(" << error.template_context->context_declaration_file_pos.GetLine() << "): note: "
					<< "In expansion of macro \"" << error.template_context->context_name << "\"\n";

				std::cerr << source_files[error.file_pos.GetFileIndex() ]
					<< "(" << error.file_pos.GetLine() << "): note: required from here\n";
			}
			else
			{
				std::cerr << source_files[ error.file_pos.GetFileIndex() ]
					<< "(" << error.file_pos.GetLine() << "): error: " << error.text << "\n";
			}
		}
		else
		{
			if( error.code == CodeBuilderErrorCode::TemplateContext )
			{
				U_ASSERT( error.template_context != nullptr );

				std::cerr << source_files[ error.template_context->context_declaration_file_pos.GetFileIndex() ] << ": "
					<< "In instantiation of \"" << error.template_context->context_name
					<< "\" " << error.template_context->parameters_description
					<< "\n";

				std::cerr << source_files[error.file_pos.GetFileIndex() ]
					<< ":" << error.file_pos.GetLine() << ":" << error.file_pos.GetColumn() << ": required from here: " << "\n";

				PrintErrors( source_files, error.template_context->errors, format );
			}
			else if( error.code == CodeBuilderErrorCode::MacroExpansionContext )
			{
				U_ASSERT( error.template_context != nullptr );

				std::cerr << source_files[ error.template_context->context_declaration_file_pos.GetFileIndex() ] << ": "
					<< "In expansion of macro \"" << error.template_context->context_name << "\"\n";

				std::cerr << source_files[ error.file_pos.GetFileIndex() ]
					<< ":" << error.file_pos.GetLine() << ":" << error.file_pos.GetColumn() << ": required from here: " << "\n";

				PrintErrors( source_files, error.template_context->errors, format );
			}
			else
			{
				std::cerr << source_files[ error.file_pos.GetFileIndex() ]
					<< ":" << error.file_pos.GetLine() << ":" << error.file_pos.GetColumn() << ": error: " << error.text << "\n";
			}
		}
	}
}

void PrintErrorsForTests( const std::vector<IVfs::Path>& source_files, const CodeBuilderErrorsContainer& errors )
{
	// For tests we print errors as "file.u 88 NameNotFound"
	for( const CodeBuilderError& error : errors )
		std::cout << source_files[error.file_pos.GetFileIndex() ]
			<< " " << error.file_pos.GetLine() << " " << CodeBuilderErrorCodeToString( error.code ) << "\n";
}

std::string QuoteDepTargetString( const std::string& str )
{
	std::string result;
	result.reserve( str.size() * 2u );

	for( const char c : str )
	{
		if( c == ' ' || c == '\t' || c == '\\' || c == '#' )
			result.push_back('\\');
		if( c == '$' )
			result.push_back('$');
		result.push_back(c);
	}

	return result;
}

bool WriteDepFile(
	const std::string& out_file_path,
	const std::vector<IVfs::Path>& deps_list, // Files list should not contain duplicates.
	const std::string& dep_file_path )
{
	std::string str= QuoteDepTargetString(out_file_path) + ":";
	for( const IVfs::Path& path : deps_list )
	{
		str+= " ";
		str+= QuoteDepTargetString(path);
		if( &path != &deps_list.back() )
			str+= " \\\n";
	}

	std::error_code file_error_code;
	llvm::raw_fd_ostream deps_file_stream( dep_file_path, file_error_code, llvm::sys::fs::F_None );
	deps_file_stream << str;

	deps_file_stream.flush();
	if( deps_file_stream.has_error() )
	{
		std::cerr << "Error while writing dep file \"" << dep_file_path << "\": " << file_error_code.message() << std::endl;
		return false;
	}

	return true;
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
	cl::cat(options_category) );

cl::opt<std::string> target_vendor(
	"target-vendor",
	cl::desc("Target vendor"),
	cl::cat(options_category) );

cl::opt<std::string> target_os(
	"target-os",
	cl::desc("Target OS"),
	cl::cat(options_category) );

cl::opt<std::string> target_environment(
	"target-environment",
	cl::desc("Target environment"),
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

cl::opt<std::string> dep_file_name(
	"MF",
	cl::desc("Output dependency file"),
	cl::value_desc("filename"),
	cl::Optional,
	cl::cat(options_category) );

cl::opt<bool> deps_tracking(
	"deps-tracking",
	cl::desc("Create dependency file for output file, do not rebuild output file if input files listed in output dependency file not changed"),
	cl::init(false),
	cl::cat(options_category) );

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
		[]( llvm::raw_ostream& )
		{
			std::cout << "Ü-Sprache version " << getFullVersion() << ", llvm version " << LLVM_VERSION_STRING << std::endl;
			llvm::InitializeAllTargets();
			PrintAvailableTargets();
		} );

	llvm::cl::HideUnrelatedOptions( Options::options_category );
	llvm::cl::ParseCommandLineOptions( argc, argv, "Ü-Sprache compiler\n" );

	if( Options::deps_tracking && DepFile::NothingChanged( Options::output_file_name, argc, argv ) )
		return 0;

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

	// LLVM stuff initialization.
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmPrinters();
	llvm::InitializeAllAsmParsers();

	{
		llvm::PassRegistry& registry= *llvm::PassRegistry::getPassRegistry();
		llvm::initializeCore(registry);
		llvm::initializeTransformUtils(registry);
		llvm::initializeScalarOpts(registry);
		llvm::initializeVectorization(registry);
		llvm::initializeInstCombine(registry);
		llvm::initializeAggressiveInstCombine(registry);
		llvm::initializeIPO(registry);
		llvm::initializeInstrumentation(registry);
		llvm::initializeAnalysis(registry);
		llvm::initializeCodeGen(registry);
		llvm::initializeTarget(registry);
	}

	// Prepare target machine.
	std::string target_triple_str;
	std::unique_ptr<llvm::TargetMachine> target_machine;
	{
		const llvm::Target* target= nullptr;

		llvm::Triple target_triple( llvm::sys::getDefaultTargetTriple() );

		if( !Options::architecture.empty() && Options::architecture != "native" )
			target_triple.setArchName( Options::architecture );
		if( !Options::target_vendor.empty() )
			target_triple.setVendorName( Options::target_vendor );
		if( !Options::target_os.empty() )
			target_triple.setOSName( Options::target_os );
		if( !Options::target_environment.empty() )
			target_triple.setEnvironmentName( Options::target_environment );

		target_triple_str= target_triple.normalize();

		std::string error_str;
		target= llvm::TargetRegistry::lookupTarget( target_triple_str, error_str );
		if( target == nullptr )
		{
			std::cerr << "Error, selecting target: " << error_str << std::endl;
			PrintAvailableTargets();
			return 1;
		}

		const std::string cpu_name= ( Options::architecture == "native" && Options::target_cpu.empty() )
			? llvm::sys::getHostCPUName()
			: Options::target_cpu;

		const std::string features_str= ( Options::architecture == "native" && Options::target_attributes.empty() )
			? GetNativeTargetFeaturesStr()
			: GetFeaturesStr( Options::target_attributes );

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

	const bool is_msvc= target_machine->getTargetTriple().getEnvironment() == llvm::Triple::MSVC;
	const auto errors_format= is_msvc ? ErrorsFormat::MSVC : ErrorsFormat::GCC;

	// Compile multiple input files and link them together.
	llvm::LLVMContext llvm_context;
	std::unique_ptr<llvm::Module> result_module;
	std::vector<IVfs::Path> deps_list;

	{
		const auto vfs= CreateVfsOverSystemFS( Options::include_dir );
		if( vfs == nullptr )
			return 1u;

		bool have_some_errors= false;
		for( const std::string& input_file : Options::input_files )
		{
			CodeBuilderLaunchResult code_builder_launch_result=
				LaunchCodeBuilder( input_file, vfs, llvm_context, data_layout, Options::generate_debug_info );

			deps_list.insert( deps_list.end(), code_builder_launch_result.dependent_files.begin(), code_builder_launch_result.dependent_files.end() );

			PrintLexSyntErrors( code_builder_launch_result.dependent_files, code_builder_launch_result.lex_synt_errors, errors_format );

			if( Options::tests_output )
				PrintErrorsForTests( code_builder_launch_result.dependent_files, code_builder_launch_result.code_builder_errors );
			else
				PrintErrors( code_builder_launch_result.dependent_files, code_builder_launch_result.code_builder_errors, errors_format );

			if( !code_builder_launch_result.lex_synt_errors.empty() ||
				!code_builder_launch_result.code_builder_errors.empty() ||
				code_builder_launch_result.llvm_module == nullptr )
			{
				have_some_errors= true;
				continue;
			}

			if( result_module == nullptr )
				result_module= std::move( code_builder_launch_result.llvm_module );
			else
			{
				const bool not_ok=
					llvm::Linker::linkModules( *result_module, std::move(code_builder_launch_result.llvm_module) );
				if( not_ok )
				{
					std::cout << "Error, linking file \"" << input_file << "\"" << std::endl;
					have_some_errors= true;
				}
			}
		}

		if( have_some_errors )
			return 1;
	}

	{
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
				std::cerr << "Internal compiler error - stdlib module parse error" << std::endl;
				return 1;
			}

			std_lib_module.get()->setDataLayout( data_layout );

			std::string err_stream_str;
			llvm::raw_string_ostream err_stream( err_stream_str );
			if( llvm::verifyModule( *std_lib_module.get(), &err_stream ) )
			{
				std::cerr << "Internal compiler error - stdlib module verify error:\n" << err_stream.str() << std::endl;
				return 1;
			}

			llvm::Linker::linkModules( *result_module, std::move(std_lib_module.get()) );
		}
	}

	// Setup various module properties.
	result_module->setTargetTriple( target_triple_str );

	if( Options::generate_debug_info )
	{
		// Dwarf debug info - default format.
		if( is_msvc )
			result_module->addModuleFlag( llvm::Module::Warning, "CodeView", 1 );
		else
			result_module->addModuleFlag( llvm::Module::Warning, "Debug Info Version", 3 );
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

			// vectorization/unroll is same as in "opt"
			pass_manager_builder.DisableUnrollLoops= optimization_level == 0;
			pass_manager_builder.LoopVectorize= optimization_level > 1 && size_optimization_level < 2;
			pass_manager_builder.SLPVectorize= optimization_level > 1 && size_optimization_level < 2;

			target_machine->adjustPassManager(pass_manager_builder);

			if (llvm::TargetPassConfig* const target_pass_config= static_cast<llvm::LLVMTargetMachine &>(*target_machine).createPassConfig(pass_manager))
				pass_manager.add(target_pass_config);

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

	// Dump llvm code after optimization pass.
	if( Options::print_llvm_asm )
	{
		llvm::raw_os_ostream stream(std::cout);
		result_module->print( stream, nullptr );
	}

	// Write result file.
	{
		std::error_code file_error_code;
		llvm::raw_fd_ostream out_file_stream( Options::output_file_name, file_error_code, llvm::sys::fs::F_None );

		if( Options::file_type == Options::FileType::BC )
			llvm::WriteBitcodeToFile( *result_module, out_file_stream );
		else if( Options::file_type == Options::FileType::LL )
			result_module->print( out_file_stream, nullptr );
		else
		{
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
			if( target_machine->addPassesToEmitFile( pass_manager, out_file_stream, nullptr, file_type ) )
			{
				std::cerr << "Error, creating file emit pass." << std::endl;
				return 1;
			}

			pass_manager.run(*result_module);
		}

		out_file_stream.flush();
		if( out_file_stream.has_error() )
		{
			std::cerr << "Error while writing output file \"" << Options::output_file_name << "\": " << file_error_code.message() << std::endl;
			return 1;
		}
	}

	// Left only unique paths in dependencies list.
	std::sort( deps_list.begin(), deps_list.end() );
	deps_list.erase( std::unique( deps_list.begin(), deps_list.end() ), deps_list.end() );

	if( Options::deps_tracking )
		DepFile::Write( Options::output_file_name, argc, argv, deps_list );

	if( !Options::dep_file_name.empty() &&
		!WriteDepFile( Options::output_file_name, deps_list, Options::dep_file_name ) )
		return 1;

	return 0;
}

} // namespace

} // namespace U

int main( const int argc, const char* argv[] )
{
	// Place actual "main" body inside "U" namespace.
	return U::Main( argc, argv );
}
