#include "../../compilers_common_lib/code_builder_launcher.hpp"
#include "../lex_synt_lib/source_graph_loader.hpp"
#include "../code_builder_lib/code_builder.hpp"

namespace U
{

CodeBuilderLaunchResult LaunchCodeBuilder(
	const IVfs::Path& input_file,
	const IVfsPtr& vfs,
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const bool generate_debug_info )
{
	CodeBuilderLaunchResult result;

	const SourceGraphPtr source_graph= SourceGraphLoader(vfs).LoadSource( input_file );

	result.dependent_files.reserve( source_graph->nodes_storage.size() );
	for( const SourceGraph::Node& node : source_graph->nodes_storage )
		result.dependent_files.push_back( node.file_path );

	result.lex_synt_errors= std::move(source_graph->errors);
	if( !result.lex_synt_errors.empty() )
		return result;

	CodeBuilder::BuildResult build_result=
		CodeBuilder(
			llvm_context,
			data_layout,
			generate_debug_info ).BuildProgram( *source_graph );

	result.code_builder_errors= std::move( build_result.errors );
	result.llvm_module= std::move( build_result.module );

	return result;
}

uint32_t GetCompilerGeneration()
{
	return 0u;
}

} // namespace U
