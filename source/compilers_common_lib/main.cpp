#include <iostream>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/AsmParser/Parser.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/CodeGen/TargetPassConfig.h>
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

#include "../lex_synt_lib_common/assert.hpp"
#include "../sprache_version/sprache_version.hpp"
#include  "code_builder_launcher.hpp"
#include "dep_file.hpp"
#include "errors_print.hpp"
#include "make_dep_file.hpp"
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

void AddModuleGlobalConstant( llvm::Module& module, llvm::Constant* const initializer, const std::string& name )
{
	const auto prev_variable= module.getGlobalVariable( name );
	if( prev_variable != nullptr )
	{
		if( prev_variable->getInitializer() != initializer )
			std::cout << "Warning, overrideing Module constant \"" << name << "\" value" << std::endl;
		return;
	}

	const auto variable=
		new llvm::GlobalVariable(
			module,
			initializer->getType(),
			true, // is_constant
			llvm::GlobalValue::ExternalLinkage,
			initializer,
			name );
	llvm::Comdat* const comdat= module.getOrInsertComdat( variable->getName() );
	comdat->setSelectionKind( llvm::Comdat::Any );
	variable->setComdat( comdat );
}

namespace Options
{

namespace cl= llvm::cl;

cl::OptionCategory options_category( "Ü compier options" );

cl::list<std::string> input_files(
	cl::Positional,
	cl::desc("<source0> [... <sourceN>]"),
	cl::value_desc("input files"),
	cl::OneOrMore,
	cl::cat(options_category) );

enum class InputFileType{ Source, BC, LL };
cl::opt< InputFileType > input_files_type(
	"input-filetype",
	cl::init(InputFileType::Source),
	cl::desc("Choose a input files type:"),
	cl::values(
		clEnumValN( InputFileType::Source, "source", "Reead Ü source files" ),
		clEnumValN( InputFileType::BC, "bc", "Read an llvm bitcode ('.bc') files" ),
		clEnumValN( InputFileType::LL, "ll", "Read an llvm asm ('.ll') files" )),
	cl::cat(options_category) );

cl::opt<std::string> output_file_name(
	"o",
	cl::desc("Output filename"),
	cl::value_desc("filename"),
	cl::Optional,
	cl::cat(options_category) );

cl::list<std::string> include_dir(
	"include-dir",
	cl::Prefix,
	cl::desc("Add directory for search of \"import\" files"),
	cl::value_desc("dir"),
	cl::ZeroOrMore,
	cl::cat(options_category));

enum class FileType{ BC, LL, Obj, Asm, Null };
cl::opt< FileType > file_type(
	"filetype",
	cl::init(FileType::Obj),
	cl::desc("Choose a file type (not all types are supported by all targets):"),
	cl::values(
		clEnumValN( FileType::BC, "bc", "Emit an llvm bitcode ('.bc') file" ),
		clEnumValN( FileType::LL, "ll", "Emit an llvm asm ('.ll') file" ),
		clEnumValN( FileType::Obj, "obj", "Emit a native object ('.o') file" ),
		clEnumValN( FileType::Asm, "asm", "Emit an assembly ('.s') file" ),
		clEnumValN( FileType::Null, "null", "Emit no output file. Usable for compilation check." ) ),
	cl::cat(options_category) );

cl::opt<bool> override_data_layout(
	"override-data-layout",
	cl::desc("Override data layout of input LL or BC module."),
	cl::init(false),
	cl::cat(options_category) );

cl::opt<bool> override_target_triple(
	"override-target-triple",
	cl::desc("Override data layout of input LL or BC module."),
	cl::init(false),
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

const auto mangling_scheme_auto= ManglingScheme(100);

cl::opt<ManglingScheme> mangling_scheme(
	"mangling-scheme",
	cl::init(mangling_scheme_auto),
	cl::desc("Override mangling scheme."),
	cl::values(
		clEnumValN( mangling_scheme_auto, "auto", "Choose mangling scheme based on target triple." ),
		clEnumValN( ManglingScheme::ItaniumABI, "itanium-abi", "Itanium ABI mangling scheme." ),
		clEnumValN( ManglingScheme::MSVC, "msvc", "MSVC mangling scheme (exact scheme is determined by target pointer size).." ),
		clEnumValN( ManglingScheme::MSVC32, "msvc32", "MSVC 32-bit mangling scheme." ),
		clEnumValN( ManglingScheme::MSVC64, "msvc64", "MSVC 64-bit mangling scheme." ) ),
	cl::cat(options_category) );

cl::opt<bool> data_sections(
	"data-sections",
	cl::desc("Emit data into separate sections"),
	cl::init(false),
	cl::cat(options_category) );

cl::opt<bool> function_sections(
	"function-sections",
	cl::desc("Emit functions into separate sections"),
	cl::init(false),
	cl::cat(options_category) );

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
	cl::desc("Print LLVM code (faster optimizations)."),
	cl::init(false),
	cl::cat(options_category) );

cl::opt<bool> print_llvm_asm_initial(
	"print-llvm-asm-initial",
	cl::desc("Print LLVM code (initial)."),
	cl::init(false),
	cl::cat(options_category) );

cl::opt<bool> print_prelude_code(
	"print-prelude-code",
	cl::desc("Print compiler-generated prelude code."),
	cl::init(false),
	cl::cat(options_category) );

enum class HaltMode{ Trap, Abort, ConfigurableHandler, Unreachable, };
cl::opt< HaltMode > halt_mode(
	"halt-mode",
	cl::init(HaltMode::Trap),
	cl::desc("Halt handling mode:"),
	cl::values(
		clEnumValN( HaltMode::Trap, "trap", "Produce trap instruction (default)." ),
		clEnumValN( HaltMode::Abort, "abort", "Call C \"abort\" function." ),
		clEnumValN( HaltMode::ConfigurableHandler, "configurable_handler", "Produce call to configurable \"_U_halt_handler\" function." ),
		clEnumValN( HaltMode::Unreachable, "unreachable", "Treat \"halt\" as unreachable instruction. Behavior is undefined if \"halt\" happens." ) ),
	cl::cat(options_category) );

cl::opt<bool> no_libc_alloc(
	"no-libc-alloc",
	cl::desc("Disable usage of libc allocation functions."),
	cl::init(false),
	cl::cat(options_category) );

cl::opt<bool> verify_module(
	"verify-module",
	cl::desc("Run verification for result llvm module (before optimization passes). Allows to find linkage errors and (possible) internal compiler errors."),
	cl::init(false),
	cl::cat(options_category) );

cl::opt<bool> internalize(
	"internalize",
	cl::desc("Internalize symbols with public visibility (functions, global variables) except \"main\" and symobols listed in \"--internalize-preserve\" option. Usefull for whole program optimization."),
	cl::init(false),
	cl::cat(options_category) );

cl::list<std::string> internalize_preserve(
	"internalize-preserve",
	cl::CommaSeparated,
	cl::desc("Used together with \"--internalize option\". Preserve listed symbols."),
	cl::value_desc("symbol1, symbol2, symbolN,..."),
	cl::Optional,
	cl::cat(options_category) );

enum class LTOMode{ None, PreLink, Link };
cl::opt< LTOMode > lto_mode(
	"lto-mode",
	cl::init(LTOMode::None),
	cl::desc("LTO mode:"),
	cl::values(
		clEnumValN( LTOMode::None, "none", "Do not apply LTO (default)." ),
		clEnumValN( LTOMode::PreLink, "prelink", "Run Pre-link LTO pipeline. Usable for initial modules for later LTO link stage." ),
		clEnumValN( LTOMode::Link, "link", "Run link LTO pipeline. Input llvm modules should be optimized with link stage before this." ) ),
	cl::cat(options_category) );

} // namespace Options

bool MustPreserveGlobalValue( const llvm::GlobalValue& global_value )
{
	const llvm::StringRef name= global_value.getName();

	if( name == "main" )
		return true; // Always preserve "main" - default entry point of executable files.

	// TODO - use StringMap or something like that instead.
	for( const std::string& name_fro_preserve : Options::internalize_preserve )
		if( name == name_fro_preserve )
			return true;

	return false;
}

std::string GenerateCompilerPreludeCode(
	const llvm::Triple& target_triple,
	const std::string_view& features,
	const std::string_view cpu_name )
{
	std::string result;

	// TODO - carefully choose namespace name.
	result += "namespace compiler\n";
	result += "{\n";
	{
		// Info about compiler itself.
		result += "auto& version = \"";
		result += getSpracheVersion();
		result += "\";\n";

		result += "auto& git_revision = \"";
		result += getGitRevision();
		result += "\";\n";

		result += "var size_type generation = ";
		result += std::to_string(GetCompilerGeneration());
		result += "s;\n";

		// Options.
		result += "namespace options\n";
		result += "{\n";
		{
			result += "var char8 optimization_level = \"";
			result += Options::optimization_level;
			result += "\"c8;\n";

			result += "var bool generate_debug_info = ";
			result += Options::generate_debug_info ? "true" : "false";
			result += ";\n";

			result += "auto& cpu_name = \"";
			result += cpu_name;
			result += "\";\n";

			// Features
			{
				const llvm::SubtargetFeatures features_parsed( features );
				const auto features_list= features_parsed.getFeatures();

				result += "var tup[ ";
				for( const std::string& feature : features_list )
				{
					result += "[ char8, ";
					result += std::to_string(feature.size());
					result += " ]";
					if( &feature != &features_list.back() )
						result += ", ";
				}
				result += " ]";

				result += " features[ ";
				for( const std::string& feature : features_list )
				{
					result += "\"";
					result += feature;
					result+= "\"";
					if( &feature != &features_list.back() )
						result += ", ";
				}
				result += " ];\n";
			}
		}
		result += "}\n";

		// Target triple.
		result += "namespace target\n";
		result += "{\n";
		{
			result += "auto& str = \"";
			result += target_triple.str();
			result += "\";\n";

			result += "auto& arch = \"";
			result += target_triple.getArchName();
			result += "\";\n";

			result += "auto& vendor = \"";
			result += target_triple.getVendorName();
			result += "\";\n";

			result += "auto& os = \"";
			result += target_triple.getOSName();
			result += "\";\n";

			result += "auto& environment = \"";
			result += target_triple.getEnvironmentName();
			result += "\";\n";

			result += "auto& os_and_environment = \"";
			result += target_triple.getOSAndEnvironmentName();
			result += "\";\n";
		}
		result += "}\n";
	}
	result += "}\n";

	if( Options::print_prelude_code )
		std::cout << result << std::endl;

	return result;
}

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

	if( Options::output_file_name.empty() && Options::file_type != Options::FileType::Null )
	{
		std::cerr << "No output file specified" << std::endl;
		return 1;
	}

	if( Options::deps_tracking && DepFile::NothingChanged( Options::output_file_name, argc, argv ) )
		return 0;

	// Select optimization level.
	llvm::OptimizationLevel optimization_level= llvm::OptimizationLevel::O0;
		 if( Options::optimization_level == '0' )
		optimization_level= llvm::OptimizationLevel::O0;
	else if( Options::optimization_level == '1' )
		optimization_level= llvm::OptimizationLevel::O1;
	else if( Options::optimization_level == '2' )
		optimization_level= llvm::OptimizationLevel::O2;
	else if( Options::optimization_level == '3' )
		optimization_level= llvm::OptimizationLevel::O3;
	else if( Options::optimization_level == 's' )
		optimization_level= llvm::OptimizationLevel::Os;
	else if( Options::optimization_level == 'z' )
		optimization_level= llvm::OptimizationLevel::Oz;
	else
	{
		std::cerr << "Unknown optimization: " << Options::optimization_level << std::endl;
		return 1;
	}

	// Build TBAA metadata only if we perform optimizations, based on this metadata.
	const bool generate_tbaa_metadata= optimization_level.getSpeedupLevel() > 0;

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
	llvm::Triple target_triple( llvm::sys::getDefaultTargetTriple() );

	std::unique_ptr<llvm::TargetMachine> target_machine;
	{
		const llvm::Target* target= nullptr;

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

		llvm::TargetOptions target_options;
		target_options.DataSections = Options::data_sections;
		target_options.FunctionSections = Options::function_sections;

		auto code_gen_optimization_level= llvm::CodeGenOpt::None;
		if ( optimization_level.getSizeLevel() > 0 )
			code_gen_optimization_level= llvm::CodeGenOpt::Default;
		else if( optimization_level.getSpeedupLevel() == 0 )
			code_gen_optimization_level= llvm::CodeGenOpt::None;
		else if( optimization_level.getSpeedupLevel() == 1 )
			code_gen_optimization_level= llvm::CodeGenOpt::Less;
		else if( optimization_level.getSpeedupLevel() == 2 )
			code_gen_optimization_level= llvm::CodeGenOpt::Default;
		else if( optimization_level.getSpeedupLevel() == 3 )
			code_gen_optimization_level= llvm::CodeGenOpt::Aggressive;

		const std::string features=
			( Options::architecture == "native" && Options::target_attributes.empty() )
				? GetNativeTargetFeaturesStr()
				: GetFeaturesStr( Options::target_attributes );

		const std::string cpu_name= (( Options::architecture == "native" && Options::target_cpu.empty() )
			? llvm::sys::getHostCPUName()
			: Options::target_cpu).str();

		target_machine.reset(
			target->createTargetMachine(
				target_triple_str,
				cpu_name,
				features,
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

	const ManglingScheme mangling_scheme=
		Options::mangling_scheme == Options::mangling_scheme_auto
		? (is_msvc ? ManglingScheme::MSVC : ManglingScheme::ItaniumABI)
		: Options::mangling_scheme;

	llvm::LLVMContext llvm_context;
	std::unique_ptr<llvm::Module> result_module;
	std::vector<IVfs::Path> deps_list;

	if( Options::input_files_type == Options::InputFileType::Source )
	{
		// Compile multiple input files and link them together.

		const auto vfs= CreateVfsOverSystemFS( Options::include_dir );
		if( vfs == nullptr )
			return 1u;

		const std::string prelude_code=
			GenerateCompilerPreludeCode(
				target_triple,
				target_machine->getTargetFeatureString(),
				target_machine->getTargetCPU() );

		bool have_some_errors= false;
		for( const std::string& input_file : Options::input_files )
		{
			CodeBuilderLaunchResult code_builder_launch_result=
				LaunchCodeBuilder(
					input_file,
					*vfs,
					llvm_context,
					data_layout,
					target_triple,
					Options::generate_debug_info,
					generate_tbaa_metadata,
					mangling_scheme,
					prelude_code );

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
				const bool not_ok= llvm::Linker::linkModules( *result_module, std::move(code_builder_launch_result.llvm_module) );
				if( not_ok )
				{
					std::cerr << "Error, linking file \"" << input_file << "\"" << std::endl;
					have_some_errors= true;
				}
			}
		}

		if( have_some_errors )
			return 1;

		// Add various flags only for source files build result. Input "ll" or "bc" modules must already have such flags.
		if( Options::generate_debug_info )
		{
			// Dwarf debug info - default format.
			if( is_msvc )
				result_module->addModuleFlag( llvm::Module::Warning, "CodeView", 1 );
			else
				result_module->addModuleFlag( llvm::Module::Warning, "Debug Info Version", 3 );
		}

		// Add module flags and global constants for compiler version and generation.
		{
			const auto constant= llvm::ConstantDataArray::getString( llvm_context, getFullVersion() );
			result_module->addModuleFlag( llvm::Module::Warning, "Sprache compiler version", constant );
			AddModuleGlobalConstant( *result_module, constant, "__U_sprache_compiler_version" );
		}
		{
			const auto constant=
				llvm::ConstantInt::get(
					llvm::IntegerType::getInt32Ty(llvm_context),
					uint64_t(GetCompilerGeneration()), false );
			result_module->addModuleFlag( llvm::Module::Warning, "Sprache compiler generation", constant );
			AddModuleGlobalConstant( *result_module, constant, "__U_sprache_compiler_generation" );
		}
	}
	else if(
		Options::input_files_type == Options::InputFileType::BC ||
		Options::input_files_type == Options::InputFileType::LL )
	{
		// Load and link together multiple BC or LL files.

		bool have_some_errors= false;
		for( const std::string& input_file : Options::input_files )
		{
			std::unique_ptr<llvm::Module> module;
			if ( Options::input_files_type == Options::InputFileType::BC )
			{
				const llvm::ErrorOr< std::unique_ptr<llvm::MemoryBuffer> > file_mapped= llvm::MemoryBuffer::getFile( input_file );
				if( !file_mapped || *file_mapped == nullptr )
				{
					std::cerr << "Can't load file \"" << input_file << "\\" << std::endl;
					have_some_errors= true;
					continue;
				}

				llvm::Expected<std::unique_ptr<llvm::Module>> module_opt= llvm::parseBitcodeFile( **file_mapped, llvm_context );

				if( !module_opt )
				{
					std::cerr << "Failed to parse BC module for file \"" << input_file << "\"" << std::endl;
					have_some_errors= true;
					continue;
				}
				module= std::move(*module_opt);
			}
			else
			{
				llvm::SMDiagnostic diagnostic( input_file, llvm::SourceMgr::DK_Error, "" );
				module= llvm::parseAssemblyFile( input_file, diagnostic, llvm_context );

				if( module == nullptr )
				{
					llvm::raw_os_ostream stream( std::cerr );
					diagnostic.print( "", stream );
					have_some_errors= true;
					continue;
				}
			}

			if( Options::override_data_layout )
				module->setDataLayout( data_layout );
			else if( module->getDataLayout() != data_layout )
			{
				std::cerr << "Unexpoected data layout of file \"" << input_file << "\", expected \"" << data_layout.getStringRepresentation() << "\", got \"" << module->getDataLayout().getStringRepresentation() << "\"" << std::endl;
				have_some_errors= true;
				continue;
			}
			if( Options::override_target_triple )
				module->setTargetTriple( target_triple_str );
			else if( module->getTargetTriple() != target_triple_str )
			{
				std::cerr << "Unexpoected target triple of file \"" << input_file << "\", expected \"" << target_triple_str << "\", got \"" << module->getTargetTriple() << "\"" << std::endl;
				have_some_errors= true;
				continue;
			}
			{
				std::string err_stream_str;
				llvm::raw_string_ostream err_stream( err_stream_str );
				if( llvm::verifyModule( *module, &err_stream ) )
				{
					std::cerr << "Module verify error for file \"" << input_file << "\":\n" << err_stream.str() << std::endl;
					have_some_errors= true;
					continue;
				}
			}

			deps_list.push_back( input_file );

			if( result_module == nullptr )
				result_module= std::move( module );
			else
			{
				const bool not_ok= llvm::Linker::linkModules( *result_module, std::move(module) );
				if( not_ok )
				{
					std::cerr << "Error, linking file \"" << input_file << "\"" << std::endl;
					have_some_errors= true;
				}
			}
		}

		if( have_some_errors )
			return 1;
	}
	else
		U_ASSERT(false);

	{
		// Generated by "bin2c.cmake" arrays.
		#include "bc_files_headers/alloc_32.h"
		#include "bc_files_headers/alloc_64.h"
		#include "bc_files_headers/alloc_dummy.h"
		#include "bc_files_headers/atomic.h"
		#include "bc_files_headers/coro.h"
		#include "bc_files_headers/checked_math.h"
		#include "bc_files_headers/halt_abort.h"
		#include "bc_files_headers/halt_configurable.h"
		#include "bc_files_headers/halt_trap.h"
		#include "bc_files_headers/halt_unreachable.h"
		#include "bc_files_headers/math.h"
		#include "bc_files_headers/memory_32.h"
		#include "bc_files_headers/memory_64.h"
		#include "bc_files_headers/volatile.h"

		// Prepare stdlib modules set.
		#define STRING_REF(x) llvm::StringRef( reinterpret_cast<const char*>(c_##x##_file_content), sizeof(c_##x##_file_content) )

		llvm::StringRef halt_module= STRING_REF(halt_trap);
		switch(Options::halt_mode)
		{
		case Options::HaltMode::Trap:
			halt_module= STRING_REF(halt_trap);
			break;
		case Options::HaltMode::Abort:
			halt_module= STRING_REF(halt_abort);
			break;
		case Options::HaltMode::ConfigurableHandler:
			halt_module= STRING_REF(halt_configurable);
			break;
		case Options::HaltMode::Unreachable:
			halt_module= STRING_REF(halt_unreachable);
			break;
		};

		const bool is_32_bit= data_layout.getPointerSizeInBits() == 32u ;

		const llvm::StringRef asm_funcs_modules[]=
		{
			STRING_REF(atomic),
			STRING_REF(coro),
			STRING_REF(checked_math),
			halt_module,
			STRING_REF(math),
			Options::no_libc_alloc ? STRING_REF(alloc_dummy) : ( is_32_bit ? STRING_REF(alloc_32) : STRING_REF(alloc_64) ),
			is_32_bit ? STRING_REF(memory_32) : STRING_REF(memory_64),
			STRING_REF(volatile),
		};
		#undef STRING_REF

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

	if( Options::verify_module )
	{
		std::string err_stream_str;
		llvm::raw_string_ostream err_stream( err_stream_str );
		if( llvm::verifyModule( *result_module, &err_stream ) )
		{
			std::cerr << "Module verify error:\n" << err_stream.str() << std::endl;
			return 1;
		}
	}

	// Dump llvm code before optimization passes.
	if( Options::print_llvm_asm_initial )
	{
		llvm::raw_os_ostream stream(std::cout);
		result_module->print( stream, nullptr );
	}

	// Create and run optimization passes.
	{
		llvm::PipelineTuningOptions tuning_options;
		tuning_options.LoopUnrolling= optimization_level.getSpeedupLevel() > 0;
		tuning_options.LoopVectorization= optimization_level.getSpeedupLevel() > 1 && optimization_level.getSizeLevel() < 2;
		tuning_options.SLPVectorization= optimization_level.getSpeedupLevel() > 1 && optimization_level.getSizeLevel() < 2;

		llvm::PassBuilder pass_builder( target_machine.get(), tuning_options );

		// Register all the basic analyses with the managers.
		llvm::LoopAnalysisManager loop_analysis_manager;
		llvm::FunctionAnalysisManager function_analysis_manager;
		llvm::CGSCCAnalysisManager cg_analysis_manager;
		llvm::ModuleAnalysisManager module_analysis_manager;
		pass_builder.registerModuleAnalyses(module_analysis_manager);
		pass_builder.registerCGSCCAnalyses(cg_analysis_manager);
		pass_builder.registerFunctionAnalyses(function_analysis_manager);
		pass_builder.registerLoopAnalyses(loop_analysis_manager);
		pass_builder.crossRegisterProxies(
			loop_analysis_manager,
			function_analysis_manager,
			cg_analysis_manager,
			module_analysis_manager);

		// Add callbacks for early passes creation.
		pass_builder.registerPipelineStartEPCallback(
			[](llvm::ModulePassManager& module_pass_manager, const llvm::OptimizationLevel )
		{
			// Internalize (if needed).
			if( Options::internalize )
				module_pass_manager.addPass( llvm::InternalizePass( MustPreserveGlobalValue ) );
		} );

		// Create the pass manager.
		llvm::ModulePassManager module_pass_manager;
		if( optimization_level == llvm::OptimizationLevel::O0 )
			module_pass_manager= pass_builder.buildO0DefaultPipeline( optimization_level );
		else if( Options::lto_mode == Options::LTOMode::PreLink )
			module_pass_manager= pass_builder.buildLTOPreLinkDefaultPipeline( optimization_level );
		else if( Options::lto_mode == Options::LTOMode::Link )
			module_pass_manager= pass_builder.buildLTODefaultPipeline( optimization_level, nullptr );
		else
			module_pass_manager= pass_builder.buildPerModuleDefaultPipeline( optimization_level );

		// Optimize the IR!
		module_pass_manager.run( *result_module, module_analysis_manager );
	}

	// Dump llvm code after optimization passes.
	if( Options::print_llvm_asm )
	{
		llvm::raw_os_ostream stream(std::cout);
		result_module->print( stream, nullptr );
	}

	std::error_code file_error_code;
	llvm::raw_fd_ostream out_file_stream( Options::output_file_name, file_error_code );

	// Create pass manager for output passes.
	llvm::legacy::PassManager pass_manager;

	switch( Options::file_type )
	{
	case Options::FileType::Obj:
		if( target_machine->addPassesToEmitFile( pass_manager, out_file_stream, nullptr, llvm::CGFT_ObjectFile ) )
		{
			std::cerr << "Error, creating file emit pass." << std::endl;
			return 1;
		}
		break;

	case Options::FileType::Asm:
		if( target_machine->addPassesToEmitFile( pass_manager, out_file_stream, nullptr, llvm::CGFT_AssemblyFile ) )
		{
			std::cerr << "Error, creating file emit pass." << std::endl;
			return 1;
		}
		break;

	case Options::FileType::BC:
		pass_manager.add( llvm::createBitcodeWriterPass( out_file_stream ) );
		break;

	case Options::FileType::LL:
		result_module->print( out_file_stream, nullptr );
		break;

	case Options::FileType::Null:
		// Do nothing.
		break;
	}

	// Run codegen/output passes.
	pass_manager.run(*result_module);

	// Check if output file is ok.
	out_file_stream.flush();
	if( !Options::output_file_name.empty() && out_file_stream.has_error() )
	{
		std::cerr << "Error while writing output file \"" << Options::output_file_name << "\": " << file_error_code.message() << std::endl;
		return 1;
	}

	// Left only unique paths in dependencies list.
	DeduplicateDepsList(deps_list);

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
