#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Module.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../compiler0/lex_synt_lib/lex_synt_error.hpp"
#include "../compiler0/lex_synt_lib/source_graph_loader.hpp"
#include "../compiler0/code_builder_lib/code_builder_errors.hpp"

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
CodeBuilderLaunchResult launchCodeBuilder(
	const IVfs::Path& input_file,
	const IVfsPtr& vfs,
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	bool generate_debug_info );

} // namespace U
