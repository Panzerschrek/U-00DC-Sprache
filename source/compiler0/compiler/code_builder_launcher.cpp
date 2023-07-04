#include "../../code_builder_lib_common/source_file_contents_hash.hpp"
#include "../../compilers_common_lib/code_builder_launcher.hpp"
#include "../lex_synt_lib/source_graph_loader.hpp"
#include "../code_builder_lib/code_builder.hpp"

namespace U
{

CodeBuilderLaunchResult LaunchCodeBuilder(
	const IVfs::Path& input_file,
	IVfs& vfs,
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const llvm::Triple& target_triple,
	const bool generate_debug_info,
	const bool generate_tbaa_metadata,
	const ManglingScheme mangling_scheme,
	const std::string_view prelude_code )
{
	CodeBuilderLaunchResult result;

	const SourceGraph source_graph= LoadSourceGraph( vfs, CalculateSourceFileContentsHash, input_file, prelude_code );

	result.dependent_files.reserve( source_graph.nodes_storage.size() );
	for( const SourceGraph::Node& node : source_graph.nodes_storage )
		result.dependent_files.push_back( node.file_path );

	result.lex_synt_errors= std::move(source_graph.errors);
	if( !result.lex_synt_errors.empty() )
		return result;

	CodeBuilderOptions options;
	options.build_debug_info= generate_debug_info;
	options.mangling_scheme= mangling_scheme;
	options.generate_tbaa_metadata= generate_tbaa_metadata;

	CodeBuilder::BuildResult build_result=
		CodeBuilder(
			llvm_context,
			data_layout,
			target_triple,
			options ).BuildProgram( source_graph );

	result.code_builder_errors= std::move( build_result.errors );
	result.llvm_module= std::move( build_result.module );

	return result;
}

uint32_t GetCompilerGeneration()
{
	return 0u;
}

} // namespace U
