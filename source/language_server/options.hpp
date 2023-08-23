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

// Duplicate some compiler options.

inline cl::list<std::string> include_dir(
	"include-dir",
	cl::Prefix,
	cl::desc("Add directory for search of \"import\" files. This affects all files, opended within this instance of the LanguageServer."),
	cl::value_desc("dir"),
	cl::ZeroOrMore,
	cl::cat(options_category));
}

} // namespace Options

} // namespace U
