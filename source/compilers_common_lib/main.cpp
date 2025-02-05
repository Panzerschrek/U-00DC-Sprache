#include <iostream>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/AsmParser/Parser.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/InitializePasses.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/SubtargetFeature.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/ConstantMerge.h>
#include <llvm/Transforms/IPO/GlobalDCE.h>
#include <llvm/Transforms/IPO/Internalize.h>
#include <llvm/Transforms/IPO/MergeFunctions.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../code_builder_lib_common/async_calls_inlining.hpp"
#include "../code_builder_lib_common/string_ref.hpp"
#include "../compilers_support_lib/errors_print.hpp"
#include "../compilers_support_lib/prelude.hpp"
#include "../compilers_support_lib/ustlib.hpp"
#include "../compilers_support_lib/vfs.hpp"
#include "../lex_synt_lib_common/assert.hpp"
#include "../sprache_version/sprache_version.hpp"
#include  "code_builder_launcher.hpp"
#include "linker.hpp"
#include "make_dep_file.hpp"

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

namespace Options
{

const llvm::codegen::RegisterCodeGenFlags use_common_llvm_code_gen_flags;

namespace cl= llvm::cl;

cl::OptionCategory options_category( "Ü compier options" );

cl::list<std::string> input_files(
	cl::Positional,
	cl::desc("<source0> [... <sourceN>]"),
	cl::value_desc("input files"),
	cl::ZeroOrMore,
	cl::cat(options_category) );

enum class InputFileType{ Source, BC, LL };
cl::opt< InputFileType > input_files_type(
	"input-filetype",
	cl::init(InputFileType::Source),
	cl::desc("Choose an input files type:"),
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
	cl::desc("Add directory for search of \"import\" files. A prefix within the compiler VFS is specified after ::."),
	cl::value_desc("dir"),
	cl::ZeroOrMore,
	cl::cat(options_category));

cl::list<std::string> source_dir(
	"source-dir",
	cl::Prefix,
	cl::desc("Mark this directory as source directory, importing files from which is allowed.\
 Allows to import files relative to the given source file if \"prevent-imports-outside-given-directories\" option is used.\
 Also symbols declared in files imported from a source directory considered to be internal for a build target."),
	cl::value_desc("dir"),
	cl::ZeroOrMore,
	cl::cat(options_category));

cl::opt<bool> prevent_imports_outside_given_directories(
	"prevent-imports-outside-given-directories",
	cl::desc("Prevent imports outside given include directories and source directories."),
	cl::init(false),
	cl::cat(options_category) );

cl::opt<bool> override_data_layout(
	"override-data-layout",
	cl::desc("Override data layout of input LL or BC module."),
	cl::init(false),
	cl::cat(options_category) );

cl::opt<bool> override_target_triple(
	"override-target-triple",
	cl::desc("Override target triple of input LL or BC module."),
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

cl::opt<bool> allow_unused_names(
	"allow-unused-names",
	cl::desc("Allow declaration of unused names (variables, type aliases, etc.)."),
	cl::init(false),
	cl::cat(options_category) );

cl::opt<std::string> target_arch(
	"target-arch",
	cl::desc("Target architecture"),
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

const auto mangling_scheme_auto= ManglingScheme(100);

cl::opt<ManglingScheme> mangling_scheme(
	"mangling-scheme",
	cl::init(mangling_scheme_auto),
	cl::desc("Override mangling scheme."),
	cl::values(
		clEnumValN( mangling_scheme_auto, "auto", "Choose mangling scheme based on target triple." ),
		clEnumValN( ManglingScheme::ItaniumABI, "itanium-abi", "Itanium ABI mangling scheme." ),
		clEnumValN( ManglingScheme::MSVC, "msvc", "MSVC mangling scheme (exact scheme is determined by target pointer size)." ),
		clEnumValN( ManglingScheme::MSVC32, "msvc32", "MSVC 32-bit mangling scheme." ),
		clEnumValN( ManglingScheme::MSVC64, "msvc64", "MSVC 64-bit mangling scheme." ) ),
	cl::cat(options_category) );

cl::opt<std::string> dep_file_name(
	"MF",
	cl::desc("Output dependency file"),
	cl::value_desc("filename"),
	cl::Optional,
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

cl::opt< HaltMode > halt_mode(
	"halt-mode",
	cl::init(HaltMode::Trap),
	cl::desc("Halt handling mode:"),
	cl::values(
		clEnumValN( HaltMode::Trap, "trap", "Produce trap instruction (default)." ),
		clEnumValN( HaltMode::Abort, "abort", "Call C \"abort\" function." ),
		clEnumValN( HaltMode::ConfigurableHandler, "configurable_handler", "Produce call to configurable \"_U_halt_handler\" function." ),
		clEnumValN( HaltMode::Unreachable, "unreachable", "Treat \"halt\" as unreachable instruction. behavior is undefined if \"halt\" happens." ) ),
	cl::cat(options_category) );

cl::opt<bool> no_system_alloc(
	"no-system-alloc",
	cl::desc("Disable usage of system allocation functions."),
	cl::init(false),
	cl::cat(options_category) );

cl::opt<bool> disable_async_calls_inlining(
	"disable-async-calls-inlining",
	cl::desc("Disable force-inlining for async function calls (via await)."),
	cl::init(false),
	cl::cat(options_category) );

cl::opt<bool> verify_module(
	"verify-module",
	cl::desc("Run verification for result llvm module (before optimization passes). Allows to find linkage errors and (possible) internal compiler errors."),
	cl::init(false),
	cl::cat(options_category) );

cl::opt<bool> internalize(
	"internalize",
	cl::desc("Internalize symbols with public linkage (functions, global variables) except \"main\" and symobols listed in \"--internalize-preserve\" option. Usefull for whole program optimization."),
	cl::init(false),
	cl::cat(options_category) );

cl::opt<bool> internalize_hidden(
	"internalize-hidden",
	cl::desc("Internalize symbols with hidden visibility."),
	cl::init(false),
	cl::cat(options_category) );

cl::list<std::string> internalize_preserve(
	"internalize-preserve",
	cl::CommaSeparated,
	cl::desc("Used together with \"--internalize option\". Preserve listed symbols."),
	cl::value_desc("symbol1, symbol2, symbolN,..."),
	cl::Optional,
	cl::cat(options_category) );

cl::list<std::string> internalize_symbols_from(
	"internalize-symbols-from",
	cl::Prefix,
	cl::desc("Internalize symbols from given input file. File name must be specified exactly as it specified in input files option."),
	cl::value_desc("file"),
	cl::ZeroOrMore,
	cl::cat(options_category));

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

cl::list<std::string> linker_args(
	"Wl",
	cl::value_desc("linker args"),
	cl::desc("Pass a comma-separated list of arguments to the internal linker (LLD). Used only if the internal linker is used (for executable or shared library output)."),
	cl::ZeroOrMore,
	cl::Prefix,
	cl::CommaSeparated,
	cl::cat(options_category) );

cl::opt<std::string> sysroot(
	"sysroot",
	cl::desc("System root directory path. Needed for things like libraries search paths for linking."),
	cl::value_desc("path"),
	cl::Optional,
	cl::cat(options_category) );

} // namespace Options

bool MustPreserveGlobalValue( const llvm::GlobalValue& global_value )
{
	const llvm::StringRef name= global_value.getName();

	if( name == "main" )
		return true; // Always preserve "main" - default entry point of executable files.

	if( name == "wmain" )
		return true; // Preserve also "wmain" - for Windows executables using entry point with wchar_t arguments.

	// TODO - use StringMap or something like that instead.
	for( const std::string& name_for_preserve : Options::internalize_preserve )
		if( name == name_for_preserve )
			return true;

	return false;
}

void InternalizeHiddenSymbols( llvm::Module& module )
{
	const auto internalize=
		[]( llvm::GlobalObject& v )
		{
			if( !v.isDeclaration() && v.getVisibility() == llvm::GlobalValue::HiddenVisibility )
			{
				v.setLinkage( llvm::GlobalValue::PrivateLinkage );
				if( const auto comdat = v.getComdat() )
				{
					v.setComdat( nullptr );
					// HACK! LLVM Doesn't remove unused comdat. Make this manually.
					if( comdat->getName() == v.getName() )
						v.getParent()->getComdatSymbolTable().erase( comdat->getName() );
				}
			}
		};

	for( llvm::Function& function : module.functions() )
		internalize( function );

	for( llvm::GlobalVariable& global_variable : module.globals() )
		internalize( global_variable );
}

struct ExternalSymbolsInfo
{
	// Symbols with external linkage have uniqie names, so, we can identify them by name.
	std::vector<std::string> functions;
	std::vector<std::string> variables;
};

void CollectExternalSymbolsForInternalizatioin(
	const llvm::Module& module,
	const std::string& input_file_name,
	ExternalSymbolsInfo& external_symbols_info )
{
	bool should_internalize= false;
	for( const auto& file_name : Options::internalize_symbols_from )
	{
		if( input_file_name == file_name )
		{
			should_internalize= true;
			break;
		}
	}

	if( !should_internalize )
		return;

	for( const llvm::Function& function : module.functions() )
		if( !function.isDeclaration() && !function.hasLocalLinkage() )
			external_symbols_info.functions.push_back( function.getName().str() );

	for( const llvm::GlobalVariable& global_variable : module.globals() )
		if( !global_variable.isDeclaration() && !global_variable.hasLocalLinkage() )
			external_symbols_info.variables.push_back( global_variable.getName().str() );
}

void InternalizeCollectedSymbols( llvm::Module& module, const ExternalSymbolsInfo& external_symbols_info )
{
	for( const std::string& function_name : external_symbols_info.functions )
	{
		if( const auto function= module.getFunction( function_name ) )
		{
			function->setLinkage( llvm::GlobalValue::PrivateLinkage );
			if( const auto comdat = function->getComdat() )
			{
				function->setComdat( nullptr );
				// HACK! LLVM Doesn't remove unused comdat. Make this manually.
				if( comdat->getName() == function->getName() )
					function->getParent()->getComdatSymbolTable().erase( comdat->getName() );
			}
		}
	}

	for( const std::string& variable_name : external_symbols_info.variables )
	{
		if( const auto variable= module.getGlobalVariable( variable_name ) )
		{
			variable->setLinkage( llvm::GlobalValue::PrivateLinkage );
			if( const auto comdat = variable->getComdat() )
			{
				variable->setComdat( nullptr );
				// HACK! LLVM Doesn't remove unused comdat. Make this manually.
				if( comdat->getName() == variable->getName() )
					variable->getParent()->getComdatSymbolTable().erase( comdat->getName() );
			}
		}
	}
}

void SetupDLLExport( llvm::Module& module )
{
	const auto set_dllexport=
		[]( llvm::GlobalValue& v )
		{
			if( v.isDeclaration() )
				return; // Skip declarations.

			const llvm::GlobalValue::LinkageTypes linkage= v.getLinkage();
			if( linkage == llvm::GlobalValue::InternalLinkage ||
				linkage == llvm::GlobalValue::PrivateLinkage )
				return; // Set dllexport only for public symbols.

			if( v.getVisibility() == llvm::GlobalValue::DefaultVisibility )
				v.setDLLStorageClass( llvm::GlobalValue::DLLExportStorageClass );
			else
			{} // Do not export "hidden" symbols from dll.
		};

	for( llvm::Function& function : module.functions() )
		set_dllexport( function );

	for( llvm::GlobalVariable& global_variable : module.globals() )
		set_dllexport( global_variable );
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

	// Replace codegen::filetype option with our own.
	llvm::cl::TopLevelSubCommand->OptionsMap.erase( "filetype" );

	enum class FileType{ BC, LL, Obj, Asm, Exe, Dll, Null };
	llvm::cl::opt< FileType > file_type(
		"filetype",
		llvm::cl::init(FileType::Obj),
		llvm::cl::desc("Choose a file type:"),
		llvm::cl::values(
			clEnumValN( FileType::BC, "bc", "Emit an llvm bitcode ('.bc') file" ),
			clEnumValN( FileType::LL, "ll", "Emit an llvm asm ('.ll') file" ),
			clEnumValN( FileType::Obj, "obj", "Emit a native object ('.o') file" ),
			clEnumValN( FileType::Asm, "asm", "Emit an assembly ('.s') file" ),
			clEnumValN( FileType::Exe, "exe", "Emit a native executable file" ),
			clEnumValN( FileType::Dll, "dll", "Emit a native shared library ('.so', '.dll') file" ),
			clEnumValN( FileType::Null, "null", "Emit no output file. Usable for compilation check." ) ),
		llvm::cl::cat(Options::options_category) );

	llvm::cl::ParseCommandLineOptions( argc, argv, "Ü-Sprache compiler\n" );

	// Remove Ü options just after parsing in order to avoid parsing them second time in the linker.
	// This is needed because COFF linker calls "ParseCommandLineOptions".
	Options::input_files.removeArgument();
	Options::input_files_type.removeArgument();
	Options::output_file_name.removeArgument();
	Options::include_dir.removeArgument();
	Options::source_dir.removeArgument();
	Options::prevent_imports_outside_given_directories.removeArgument();
	Options::override_data_layout.removeArgument();
	Options::override_target_triple.removeArgument();
	Options::optimization_level.removeArgument();
	Options::generate_debug_info.removeArgument();
	Options::allow_unused_names.removeArgument();
	Options::target_arch.removeArgument();
	Options::target_vendor.removeArgument();
	Options::target_os.removeArgument();
	Options::target_environment.removeArgument();
	Options::mangling_scheme.removeArgument();
	Options::dep_file_name.removeArgument();
	Options::tests_output.removeArgument();
	Options::print_llvm_asm.removeArgument();
	Options::print_llvm_asm_initial.removeArgument();
	Options::print_prelude_code.removeArgument();
	Options::halt_mode.removeArgument();
	Options::no_system_alloc.removeArgument();
	Options::verify_module.removeArgument();
	Options::internalize.removeArgument();
	Options::internalize_hidden.removeArgument();
	Options::internalize_preserve.removeArgument();
	Options::internalize_symbols_from.removeArgument();
	Options::lto_mode.removeArgument();
	Options::linker_args.removeArgument();

	if( Options::output_file_name.empty() && file_type != FileType::Null )
	{
		std::cerr << "No output file specified" << std::endl;
		return 1;
	}

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
		llvm::initializeIPO(registry);
		llvm::initializeAnalysis(registry);
		llvm::initializeCodeGen(registry);
		llvm::initializeTarget(registry);
	}

	// Prepare target machine.
	std::string target_triple_str;
	llvm::Triple target_triple( llvm::sys::getDefaultTargetTriple() );
	std::unique_ptr<llvm::TargetMachine> target_machine;
	{
		if( !Options::target_arch.empty() )
			target_triple.setArchName( Options::target_arch );
		if( !Options::target_vendor.empty() )
			target_triple.setVendorName( Options::target_vendor );
		if( !Options::target_os.empty() )
			target_triple.setOSName( Options::target_os );
		if( !Options::target_environment.empty() )
			target_triple.setEnvironmentName( Options::target_environment );

		std::string error_str;
		const llvm::Target* target= llvm::TargetRegistry::lookupTarget( llvm::codegen::getMArch(), target_triple, error_str );
		if( target == nullptr )
		{
			std::cerr << "Error, selecting target: " << error_str << std::endl;
			PrintAvailableTargets();
			return 1;
		}

		target_triple_str= target_triple.normalize();

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

		target_machine.reset(
			target->createTargetMachine(
				target_triple_str,
				llvm::codegen::getMCPU(),
				llvm::codegen::getFeaturesStr(),
				llvm::codegen::InitTargetOptionsFromCodeGenFlags( target_triple ),
				llvm::codegen::getExplicitRelocModel(),
				llvm::codegen::getExplicitCodeModel(),
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
	ExternalSymbolsInfo external_symbols_info;

	if( Options::input_files_type == Options::InputFileType::Source )
	{
		// Compile multiple input files and link them together.

		const IVfsSharedPtr vfs=
			CreateVfsOverSystemFS(
				Options::include_dir,
				Options::source_dir,
				Options::prevent_imports_outside_given_directories,
				false /* tolerate_errors */ );
		if( vfs == nullptr )
			return 1u;

		const std::string prelude_code=
			GenerateCompilerPreludeCode(
				target_triple,
				data_layout,
				target_machine->getTargetFeatureString(),
				target_machine->getTargetCPU(),
				Options::optimization_level,
				Options::generate_debug_info,
				c_compiler_generation );

		if( Options::print_prelude_code )
			std::cout << prelude_code << std::endl;

		bool have_some_errors= false;
		for( const std::string& input_file : Options::input_files )
		{
			CodeBuilderLaunchResult code_builder_launch_result=
				LaunchCodeBuilder(
					input_file,
					vfs,
					llvm_context,
					data_layout,
					target_triple,
					Options::generate_debug_info,
					generate_tbaa_metadata,
					Options::allow_unused_names,
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

			CollectExternalSymbolsForInternalizatioin( *code_builder_launch_result.llvm_module, input_file, external_symbols_info );

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
					std::cerr << "Can't load file \"" << input_file << "\"" << std::endl;
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

			CollectExternalSymbolsForInternalizatioin( *module, input_file, external_symbols_info );

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

	if( result_module == nullptr )
	{
		// No input files. Allow this and produce empty module.
		result_module= std::make_unique<llvm::Module>( "null", llvm_context );
		result_module->setDataLayout( data_layout );
		result_module->setTargetTriple( target_triple.normalize() );
	}

	// Set debug info version - it's needed for proper debug information parsing for loading of ll/bc modules.
	if( Options::generate_debug_info &&
		( file_type == FileType::BC || file_type == FileType::LL ) &&
		result_module->getModuleFlag( "Debug Info Version" ) == nullptr )
	{
		result_module->addModuleFlag(
			llvm::Module::Warning,
			"Debug Info Version",
			llvm::LLVMConstants::DEBUG_METADATA_VERSION );
	}

	if( !LinkUstLibModules( *result_module, Options::halt_mode, Options::no_system_alloc ) )
		return 1;

	// Dump llvm code before optimization passes.
	if( Options::print_llvm_asm_initial )
	{
		llvm::raw_os_ostream stream(std::cout);
		result_module->print( stream, nullptr );
	}

	// Run async calls inlining.
	// Enable it for O1, O2, O3, Os, but not O0 and Oz.
	// It's also important to perform inlining only for compilation from Ü sources directly.
	// Trying to perform such optimization for already compiled ll/bc files is useless or may lead to errors.
	if( optimization_level.isOptimizingForSpeed() && optimization_level.getSizeLevel() <= 1 &&
		! Options::disable_async_calls_inlining &&
		Options::input_files_type == Options::InputFileType::Source )
		InlineAsyncCalls( *result_module );

	// Internalize symbols from input files listed in "internalize-symbols-from" option.
	InternalizeCollectedSymbols( *result_module, external_symbols_info );

	// Internalize hidden symbols, if necessary.
	// Do it before optimization, to encourange inlining of internalized functions.
	if( Options::internalize_hidden )
		InternalizeHiddenSymbols( *result_module );

	// Perform verification after code generation/linking and after special optimizations and internalizations, but before running LLVM optimizations pipeline.
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

	// Create and run optimization passes.
	{
		llvm::PipelineTuningOptions tuning_options;
		tuning_options.LoopInterleaving= optimization_level.getSpeedupLevel() > 0;
		tuning_options.LoopUnrolling= optimization_level.getSpeedupLevel() > 0;
		tuning_options.LoopVectorization= optimization_level.getSpeedupLevel() > 1 && optimization_level.getSizeLevel() < 2;
		tuning_options.SLPVectorization= optimization_level.getSpeedupLevel() > 1 && optimization_level.getSizeLevel() < 2;

		// Do not care about function address uniqueness.
		tuning_options.MergeFunctions= optimization_level.getSpeedupLevel() > 0 || optimization_level.getSizeLevel() > 0;

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

		auto add_start_passes_callback=
				[](llvm::ModulePassManager& module_pass_manager, const llvm::OptimizationLevel o )
				{
					// Internalize (if needed).
					if( Options::internalize )
						module_pass_manager.addPass( llvm::InternalizePass( MustPreserveGlobalValue ) );

					if( o.isOptimizingForSpeed() || o.isOptimizingForSize() )
					{
						// Run manually constant merge pass and then function merge pass.
						// Do this in order to deduplicate code, which may be duplicated in case of LTO linking.
						// Constants merge as first pass is necessary before functions merging,
						// since function merging checks functions equality and
						// functions accessing different constants with same value are considered to be different.
						module_pass_manager.addPass( llvm::ConstantMergePass() );

						if( LLVM_VERSION_MAJOR >= 23 )
						{
							// HACK!
							// Functions merging in LLVM is still broken, because TBAA metadata on "store" instrctions,
							// which leads to wrong merges and misoptimizations.
							// See https://github.com/llvm/llvm-project/issues/123462.
							// TODO - enable it when this bug will be fixed.
							module_pass_manager.addPass( llvm::MergeFunctionsPass() );
						}
					}
				};

		// Create the pass manager.
		llvm::ModulePassManager module_pass_manager;
		if( optimization_level == llvm::OptimizationLevel::O0 )
		{
			pass_builder.registerPipelineStartEPCallback( add_start_passes_callback );
			module_pass_manager= pass_builder.buildO0DefaultPipeline( optimization_level );
		}
		else if( Options::lto_mode == Options::LTOMode::PreLink )
		{
			pass_builder.registerPipelineStartEPCallback( add_start_passes_callback );

			module_pass_manager= pass_builder.buildLTOPreLinkDefaultPipeline( optimization_level );
		}
		else if( Options::lto_mode == Options::LTOMode::Link )
		{
			// LTO pipeline uses different callbacks at start.
			pass_builder.registerFullLinkTimeOptimizationEarlyEPCallback( add_start_passes_callback );
			module_pass_manager= pass_builder.buildLTODefaultPipeline( optimization_level, nullptr );
		}
		else
		{
			pass_builder.registerPipelineStartEPCallback( add_start_passes_callback );
			module_pass_manager= pass_builder.buildPerModuleDefaultPipeline( optimization_level );
		}

		// Optimize the IR!
		module_pass_manager.run( *result_module, module_analysis_manager );
	}

	// Translate "visibility(default)" into "dllexport" for Windows dynamic libraries.
	if( file_type == FileType::Dll && target_machine->getTargetTriple().getOS() == llvm::Triple::Win32 )
		SetupDLLExport( *result_module );

	// Dump llvm code after optimization passes.
	if( Options::print_llvm_asm )
	{
		llvm::raw_os_ostream stream(std::cout);
		result_module->print( stream, nullptr );
	}

	if( !Options::output_file_name.empty() && file_type != FileType::Null )
	{
		// Create directories for output file.
		const llvm::StringRef parent_dir= llvm::sys::path::parent_path( Options::output_file_name );
		if( !parent_dir.empty() )
		{
			// Ignore errors here. If something goes wrong, an error will be generated later - on attempt to create output file.
			llvm::sys::fs::create_directories( parent_dir, /* IgnoreExisting */ true );
		}
	}

	switch( file_type )
	{
	case FileType::Obj:
	case FileType::Asm:
		{
			std::error_code file_error_code;
			llvm::raw_fd_ostream out_file_stream( Options::output_file_name, file_error_code );

			llvm::legacy::PassManager pass_manager;

			if( target_machine->addPassesToEmitFile( pass_manager, out_file_stream, nullptr, file_type == FileType::Obj ? llvm::CGFT_ObjectFile : llvm::CGFT_AssemblyFile ) )
			{
				std::cerr << "Error, creating file emit pass." << std::endl;
				return 1;
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
		}
		break;

	case FileType::BC:
		{
			std::error_code file_error_code;
			llvm::raw_fd_ostream out_file_stream( Options::output_file_name, file_error_code );

			llvm::WriteBitcodeToFile( *result_module, out_file_stream );

			// Check if output file is ok.
			out_file_stream.flush();
			if( !Options::output_file_name.empty() && out_file_stream.has_error() )
			{
				std::cerr << "Error while writing output file \"" << Options::output_file_name << "\": " << file_error_code.message() << std::endl;
				return 1;
			}
		}
		break;

	case FileType::LL:
		{
			std::error_code file_error_code;
			llvm::raw_fd_ostream out_file_stream( Options::output_file_name, file_error_code );

			result_module->print( out_file_stream, nullptr );

			out_file_stream.flush();
			if( !Options::output_file_name.empty() && out_file_stream.has_error() )
			{
				std::cerr << "Error while writing output file \"" << Options::output_file_name << "\": " << file_error_code.message() << std::endl;
				return 1;
			}
		}
		break;

	case FileType::Exe:
	case FileType::Dll:
		{
			const std::string temp_object_file_name= Options::output_file_name + "_temp.o";
			{
				llvm::legacy::PassManager pass_manager;

				std::error_code file_error_code;
				llvm::raw_fd_ostream temp_object_file_stream( temp_object_file_name, file_error_code );

				if( target_machine->addPassesToEmitFile( pass_manager, temp_object_file_stream, nullptr, llvm::CGFT_ObjectFile ) )
				{
					std::cerr << "Error, creating file emit pass." << std::endl;
					return 1;
				}

				// Run codegen/output passes.
				pass_manager.run(*result_module);

				// Check if output file is ok.
				temp_object_file_stream.flush();
				if( !temp_object_file_name.empty() && temp_object_file_stream.has_error() )
				{
					std::cerr << "Error while writing output file \"" << temp_object_file_name<< "\": " << file_error_code.message() << std::endl;
					return 1;
				}
			}

			const bool produce_shared_library= file_type == FileType::Dll;
			// Remove unreferenced symbols in builds with optimization buth also without debug information.
			const bool remove_unreferenced_symbols= optimization_level != llvm::OptimizationLevel::O0 && ! Options::generate_debug_info;

			const bool linker_ok= RunLinker(
				argv[0],
				Options::linker_args,
				Options::sysroot,
				target_triple,
				temp_object_file_name,
				Options::output_file_name,
				produce_shared_library,
				remove_unreferenced_symbols,
				Options::generate_debug_info );

			llvm::sys::fs::remove( temp_object_file_name, true );
			if( !linker_ok )
			{
				std::cerr << "Linker execution failed" << std::endl;
				return 1;
			}
		}
		break;

	case FileType::Null:
		// Do nothing.
		break;
	}

	// Left only unique paths in dependencies list.
	DeduplicateAndFilterDepsList(deps_list);

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
