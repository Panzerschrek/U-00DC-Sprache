#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/TargetParser/Triple.h>
#include <llvm/IR/Module.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../lex_synt_lib_common/errors_format.hpp"
#include "../code_builder_lib_common/mangling.hpp"
#include "../code_builder_lib_common/code_builder_errors.hpp"

namespace U
{

struct CodeBuilderLaunchResult
{
	bool has_errors= false;

	// This list isn't ordered and may contain duplicates.
	std::vector<std::string> dependent_files;

	std::vector< std::unique_ptr<llvm::Module> > llvm_modules; // Has size equal to given files. May contain nulls.
};

// There are different implementations of this function for different implementations of CodeBuilderLib.
CodeBuilderLaunchResult LaunchCodeBuilder(
	llvm::ArrayRef<std::string> include_dirs,
	llvm::ArrayRef<std::string> source_dirs,
	bool prevent_imports_outside_given_directories,
	llvm::ArrayRef<std::string> input_files,
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const llvm::Triple& target_triple,
	bool generate_debug_info,
	bool generate_tbaa_metadata,
	bool allow_unused_names,
	ManglingScheme mangling_scheme,
	std::string_view prelude_code,
	const ErrorsFormat errors_format,
	bool tests_output );

// Contains value of current compiler generation (0, 1, 2, etc.).
// Is constant, but not "constexpr", because this constant is defined outside this header.
extern const uint32_t c_compiler_generation;

} // namespace U
