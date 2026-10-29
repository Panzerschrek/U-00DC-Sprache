#include "../../code_builder_lib_common/long_stable_hash.hpp"
#include "../../compilers_common_lib/code_builder_launcher.hpp"
#include "../../compilers_support_lib/vfs.hpp"
#include "../lex_synt_lib/source_graph_loader.hpp"
#include "../code_builder_lib/code_builder.hpp"

namespace U
{

class VfsHolderImpl final : public VfsHolder
{
public:
	IVfsSharedPtr vfs_;
};

std::unique_ptr<VfsHolder> CreateVfsHolder(
	const llvm::ArrayRef<std::string> include_dirs,
	const llvm::ArrayRef<std::string> source_dirs,
	const bool prevent_imports_outside_given_directories )
{
	std::unique_ptr<IVfs> vfs=
		CreateVfsOverSystemFS(
			include_dirs,
			source_dirs,
			prevent_imports_outside_given_directories,
			false /* tolerate_errors */ );

	if( vfs == nullptr )
		return nullptr;

	VfsHolderImpl vfs_holder_impl;
	vfs_holder_impl.vfs_= std::move(vfs);

	return std::make_unique<VfsHolderImpl>( std::move(vfs_holder_impl) );
}

CodeBuilderLaunchResult LaunchCodeBuilder(
	const IVfs::Path& input_file,
	const VfsHolder& vfs_holder,
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const llvm::Triple& target_triple,
	const bool generate_debug_info,
	const bool generate_tbaa_metadata,
	const bool allow_unused_names,
	const ManglingScheme mangling_scheme,
	const std::string_view prelude_code )
{
	const IVfsSharedPtr& vfs= static_cast<const VfsHolderImpl&>( vfs_holder ).vfs_;

	CodeBuilderLaunchResult result;

	SourceGraph source_graph= LoadSourceGraph( *vfs, CalculateLongStableHash, input_file, prelude_code );

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
	options.report_about_unused_names= !allow_unused_names;

	CodeBuilder::BuildResult build_result=
		CodeBuilder::BuildProgram(
			llvm_context,
			data_layout,
			target_triple,
			options,
			MakeRcPtr( std::move(source_graph) ),
			vfs );

	result.code_builder_errors= std::move( build_result.errors );
	result.llvm_module= std::move( build_result.module );

	// Add embedded files into the list of dependencies.
	for( IVfs::Path& path : build_result.embedded_files )
		result.dependent_files.push_back( std::move(path) );

	return result;
}

extern const uint32_t c_compiler_generation= 0;

} // namespace U
