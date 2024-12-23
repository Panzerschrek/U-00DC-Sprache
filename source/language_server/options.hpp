#pragma once
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/CommandLine.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

namespace U
{

namespace LangServer
{

namespace Options
{

namespace cl= llvm::cl;
inline cl::OptionCategory options_category( "Ü language server options" );

inline cl::opt<std::string> log_file_path(
	"log-file",
	cl::desc("Log file name"),
	cl::value_desc("filename"),
	cl::Optional,
	cl::cat(options_category) );

inline cl::opt<std::string> error_log_file_path(
	"error-log-file",
	cl::desc("Error log file name"),
	cl::value_desc("filename"),
	cl::Optional,
	cl::cat(options_category) );

inline cl::opt<uint32_t> num_threads(
	"num-threads",
	cl::desc("Number of threads for background tasks. Use 0 to create threads for all awailable CPU cores."),
	cl::value_desc("non-negative whole number"),
	cl::Optional,
	cl::cat(options_category) );

// Duplicate some compiler options.
// TODO - avoid this and use common header instead.

inline cl::list<std::string> include_dir(
	"include-dir",
	cl::Prefix,
	cl::desc("Add directory for search of \"import\" files. A prefix within the compiler VFS is specified after ::. This affects all files, opened within this instance of the LanguageServer."),
	cl::value_desc("dir"),
	cl::ZeroOrMore,
	cl::cat(options_category));

inline cl::opt<char> optimization_level(
	"O",
	cl::desc("Optimization level. [-O0, -O1, -O2, -O3, -Os or -Oz] (default = '-O0')"),
	cl::Prefix,
	cl::Optional,
	cl::init('0'),
	cl::cat(options_category) );

inline cl::opt<bool> generate_debug_info(
	"g",
	cl::desc("Generate debug information."),
	cl::init(false),
	cl::cat(options_category) );

inline cl::opt<std::string> architecture(
	"march",
	cl::desc("Architecture to generate code for (see --version)"),
	cl::cat(options_category) );

inline cl::opt<std::string> target_vendor(
	"target-vendor",
	cl::desc("Target vendor"),
	cl::cat(options_category) );

inline cl::opt<std::string> target_os(
	"target-os",
	cl::desc("Target OS"),
	cl::cat(options_category) );

inline cl::opt<std::string> target_environment(
	"target-environment",
	cl::desc("Target environment"),
	cl::cat(options_category) );
}

} // namespace Options

} // namespace U
