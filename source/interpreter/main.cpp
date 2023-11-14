#include <iostream>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Linker/Linker.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../sprache_version/sprache_version.hpp"
#include "../code_builder_lib_common/source_file_contents_hash.hpp"
#include "../code_builder_lib_common/interpreter.hpp"
#include "../compiler0/code_builder_lib/code_builder.hpp"
#include "../compilers_support_lib/errors_print.hpp"
#include "../compilers_support_lib/prelude.hpp"
#include "../compilers_support_lib/ustlib.hpp"
#include "../compilers_support_lib/vfs.hpp"
#include "../tests/tests_common.hpp"

namespace U
{

namespace
{

// Use global instance in order to access it in registered functions.
std::unique_ptr<Interpreter> g_interpreter;

namespace InterpreterFuncs
{

llvm::GenericValue StdOutPrint( llvm::FunctionType*, const llvm::ArrayRef<llvm::GenericValue> args )
{
	if( args.size() < 2 )
	{
		std::cerr << "stdout_print called with invalid number of args." << std::endl;
		return llvm::GenericValue();
	}

	const uint64_t address= args[0].IntVal.getLimitedValue();
	const size_t size= size_t(args[1].IntVal.getLimitedValue());
	constexpr auto buffer_size= 1024;
	if( size < buffer_size )
	{
		char buffer[buffer_size];
		g_interpreter->ReadExecutinEngineData( buffer, address, size );
		buffer[size]= '\0';
		std::cout << buffer;
	}
	else
	{
		std::string buffer;
		buffer.resize(size + 1);
		g_interpreter->ReadExecutinEngineData( buffer.data(), address, size );
		buffer[size]= '\0';
		std::cout << buffer;
	}

	std::cout.flush();
	return llvm::GenericValue();
}

llvm::GenericValue MemCmp( llvm::FunctionType*, const llvm::ArrayRef<llvm::GenericValue> args )
{
	if( args.size() < 3 )
	{
		std::cerr << "memcmp called with invalid number of args." << std::endl;
		return llvm::GenericValue();
	}

	const uint64_t address0= args[0].IntVal.getLimitedValue();
	const uint64_t address1= args[1].IntVal.getLimitedValue();
	const size_t size= size_t( args[2].IntVal.getLimitedValue() );

	llvm::SmallVector<std::byte, 1024> buffer0, buffer1;
	buffer0.resize( size );
	buffer1.resize( size );
	g_interpreter->ReadExecutinEngineData( buffer0.data(), address0, size );
	g_interpreter->ReadExecutinEngineData( buffer1.data(), address1, size );

	const auto memcmp_result= std::memcmp( buffer0.data(), buffer1.data(), size );

	llvm::GenericValue result;
	result.IntVal= llvm::APInt( 32, uint64_t(memcmp_result) );

	return result;
}

llvm::GenericValue Abort( llvm::FunctionType*, const llvm::ArrayRef<llvm::GenericValue> )
{
	std::abort();
	return llvm::GenericValue();
}

} // namespace InterpreterFuncs

namespace JitFuncs
{

void StdOutPrint( const char* const ptr, const size_t size )
{
	constexpr auto buffer_size= 1024;
	if( size < buffer_size )
	{
		char buffer[buffer_size];
		std::memcpy( buffer, ptr, size );
		buffer[size]= '\0';
		std::cout << buffer;
	}
	else
	{
		std::string buffer;
		buffer.resize(size + 1);
		std::memcpy( buffer.data(), ptr, size );
		buffer[size]= '\0';
		std::cout << buffer;
	}

	std::cout.flush();
}

} // namespace JitFuncs

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

int Main( int argc, const char* argv[] )
{
	// HACK! Reset globals state from previous run (needed for emscripten).
	llvm::llvm_shutdown();

	static const llvm::InitLLVM llvm_initializer(argc, argv); // Static in order to construct exactly once (for emscripten).

	// Options.

	// Declare options locally in order to reset state after main.
	// This is needed in case if main function is re-entered (emscripten).

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

	cl::opt<bool> use_jit(
		"use-jit",
		cl::desc("Use JIT."),
		cl::init(false),
		cl::cat(options_category) );

	llvm::cl::SetVersionPrinter(
		[]( llvm::raw_ostream& )
		{
			std::cout << "Ü-Sprache version " << getFullVersion() << ", llvm version " << LLVM_VERSION_STRING << std::endl;
		} );

	llvm::cl::HideUnrelatedOptions( options_category );

	const auto description=
		"Ü-Sprache interpreter.\n"
		"Compiles provided files and emmideately executes result.\n";
	llvm::cl::ParseCommandLineOptions( argc, argv, description );

	std::string target_triple_str;
	llvm::Triple target_triple( llvm::sys::getProcessTriple() );
	const llvm::StringRef cpu_name= llvm::sys::getHostCPUName();
	const std::string features_str= GetNativeTargetFeaturesStr();
	std::unique_ptr<llvm::TargetMachine> target_machine;
	llvm::DataLayout data_layout("");
	if( use_jit )
	{
		// Initialize native target and create target machine only if using JIT.

		// LLVM stuff initialization.
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();

		// Prepare target machine.
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
				cpu_name,
				features_str,
				target_options,
				llvm::Reloc::PIC_,
				llvm::Optional<llvm::CodeModel::Model>(),
				llvm::CodeGenOpt::None ) );

		if( target_machine == nullptr )
		{
			std::cerr << "Error, creating target machine." << std::endl;
			return 1;
		}

		data_layout= target_machine->createDataLayout();
	}
	else
		data_layout= llvm::DataLayout( GetTestsDataLayout() );

	const auto vfs= CreateVfsOverSystemFS( include_dir );
	if( vfs == nullptr )
		return 1u;

	const std::string prelude_code=
		GenerateCompilerPreludeCode(
			target_triple,
			data_layout,
			features_str,
			cpu_name,
			'0',
			false,
			0 );

	llvm::LLVMContext llvm_context;
	std::unique_ptr<llvm::Module> result_module;

	const auto errors_format= ErrorsFormat::GCC;

	bool have_some_errors= false;
	for( const std::string& input_file : input_files )
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
			CodeBuilder::BuildProgram(
				llvm_context,
				data_layout,
				target_triple,
				CodeBuilderOptions(),
				source_graph );

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

	if( !LinkUstLibModules(
			*result_module,
			HaltMode::Abort,
			false, // Enable libc allocations
			true // Disable stdout implementations - use our own stdout function
				) )
		return 1;

	// TODO - run here optimizations?

	// TODO - support main with empty args or argc+argv.
	const auto expected_main_function_type= llvm::FunctionType::get( llvm::Type::getInt32Ty(llvm_context), {}, false );
	const auto main_function_llvm= result_module->getFunction(entry_point_name);
	if( main_function_llvm == nullptr )
	{
		std::cerr << "Can't find entry point!" << std::endl;
		return 1;
	}

	// Check if entry point function signatire is correct.
	if( main_function_llvm->getFunctionType() != expected_main_function_type )
	{
		std::cerr << "Entry point has invalid signature! Expected (fn() : i32)." << std::endl;
		return 1;
	}

	if( use_jit )
	{
		llvm::EngineBuilder builder(std::move(result_module));
		std::string engine_creation_error_string;
		builder.setEngineKind(llvm::EngineKind::JIT);
		builder.setMemoryManager(std::make_unique<llvm::SectionMemoryManager>());
		builder.setErrorStr( &engine_creation_error_string );
		const std::unique_ptr<llvm::ExecutionEngine> engine(builder.create(target_machine.release())); // Engine takes ownership over target machine.

		if( engine == nullptr )
		{
			std::cerr << "Can't create engine: " << engine_creation_error_string << std::endl;
			return 1;
		}

		engine->addGlobalMapping( "ust_stdout_print_impl", reinterpret_cast<uint64_t>(reinterpret_cast<void*>( &JitFuncs::StdOutPrint ) ) );
		// No need to add other functions here - llvm interpreter supports other required functions (memory functions, exit, abort).

		using MainFunctionType= int(*)();
		const auto main_function= reinterpret_cast<MainFunctionType>(engine->getFunctionAddress(entry_point_name));
		if( main_function == nullptr )
		{
			std::cerr << "Can't find entry point!" << std::endl;

			return 1;
		}

		return main_function();
	}
	else
	{
		g_interpreter= std::make_unique<Interpreter>( data_layout );

		// Memory functions are supported internally (inside interpreter), for other needed functions register our own handlers.
		g_interpreter->RegisterCustomFunction( "ust_stdout_print_impl", InterpreterFuncs::StdOutPrint );
		g_interpreter->RegisterCustomFunction( "memcmp", InterpreterFuncs::MemCmp );
		g_interpreter->RegisterCustomFunction( "abort", InterpreterFuncs::Abort );

		const Interpreter::ResultGeneric result= g_interpreter->EvaluateGeneric( main_function_llvm, {} );

		for( const std::string& err : result.errors )
			std::cerr << "Execution error: " << err << std::endl;

		g_interpreter= nullptr;

		if( !result.errors.empty() )
			return 1;

		return int(result.result.IntVal.getLimitedValue());
	}
}

} // namespace

} // namespace U

int main( const int argc, const char* argv[] )
{
	// Place actual "main" body inside "U" namespace.
	return U::Main( argc, argv );
}
