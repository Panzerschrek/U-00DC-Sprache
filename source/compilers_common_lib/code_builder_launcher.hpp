#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Triple.h>
#include <llvm/IR/Module.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../lex_synt_lib_common/lex_synt_error.hpp"
#include "../code_builder_lib_common/mangling.hpp"
#include "../compiler0/lex_synt_lib/source_graph_loader.hpp"
#include "../code_builder_lib_common/code_builder_errors.hpp"

namespace U
{

struct CodeBuilderLaunchResult
{
	LexSyntErrors lex_synt_errors;
	CodeBuilderErrorsContainer code_builder_errors;

	std::vector<IVfs::Path> dependent_files;

	std::unique_ptr<llvm::Module> llvm_module;
};

// There are different implementations of this function for different implementations of CodeBuilderLib.
CodeBuilderLaunchResult LaunchCodeBuilder(
	const IVfs::Path& input_file,
	IVfs& vfs,
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const llvm::Triple& target_triple,
	bool generate_debug_info,
	bool generate_tbaa_metadata,
	bool enable_unused_names,
	ManglingScheme mangling_scheme,
	std::string_view prelude_code );

uint32_t GetCompilerGeneration();

} // namespace U
