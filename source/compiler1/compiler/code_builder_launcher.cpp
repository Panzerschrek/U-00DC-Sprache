#include "../../compilers_common_lib/code_builder_launcher.hpp"

namespace U
{

CodeBuilderLaunchResult launchCodeBuilder(
	const IVfs::Path& input_file,
	const IVfsPtr& vfs,
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const bool generate_debug_info )
{
	(void)input_file;
	(void)vfs;
	(void)llvm_context;
	(void)data_layout;
	(void)generate_debug_info;

	CodeBuilderLaunchResult result;

	// TODO

	return result;
}

} // namespace U
