#include "../../code_builder_lib_common/long_stable_hash.hpp"
#include "../../compilers_common_lib/code_builder_launcher.hpp"
#include "../../compilers_support_lib/vfs.hpp"
#include "../lex_synt_lib/source_graph_loader.hpp"
#include "../code_builder_lib/code_builder.hpp"

namespace U
{

CodeBuilderLaunchResult LaunchCodeBuilder(
	const llvm::ArrayRef<std::string> include_dirs,
	const llvm::ArrayRef<std::string> source_dirs,
	const bool prevent_imports_outside_given_directories,
	const llvm::ArrayRef<std::string> input_files,
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const llvm::Triple& target_triple,
	const bool generate_debug_info,
	const bool generate_tbaa_metadata,
	const bool allow_unused_names,
	const ManglingScheme mangling_scheme,
	const std::string_view prelude_code )
{
	CodeBuilderLaunchResult result;
	result.llvm_modules.resize( input_files.size() );

	const IVfsSharedPtr vfs=
		CreateVfsOverSystemFS(
			include_dirs,
			source_dirs,
			prevent_imports_outside_given_directories,
			false /* tolerate_errors */ );

	if( vfs == nullptr )
		return result;

	CodeBuilderOptions options;
	options.build_debug_info= generate_debug_info;
	options.mangling_scheme= mangling_scheme;
	options.generate_tbaa_metadata= generate_tbaa_metadata;
	options.report_about_unused_names= !allow_unused_names;

	for( size_t i= 0; i < input_files.size(); ++i )
	{
		SourceGraph source_graph= LoadSourceGraph( *vfs, CalculateLongStableHash, input_files[i], prelude_code );

		result.dependent_files.reserve( result.dependent_files.size() + source_graph.nodes_storage.size() );
		for( const SourceGraph::Node& node : source_graph.nodes_storage )
			result.dependent_files.push_back( node.file_path );

		result.lex_synt_errors= std::move(source_graph.errors);
		if( result.lex_synt_errors.empty() )
		{
			CodeBuilder::BuildResult build_result=
				CodeBuilder::BuildProgram(
					llvm_context,
					data_layout,
					target_triple,
					options,
					MakeRcPtr( std::move(source_graph) ),
					vfs );

			result.llvm_modules[i]= std::move( build_result.module );

			result.code_builder_errors.reserve( result.code_builder_errors.size() + build_result.errors.size() );
			for( CodeBuilderError& e : build_result.errors )
				result.code_builder_errors.push_back( std::move(e) );

			// Add embedded files into the list of dependencies.
			for( IVfs::Path& path : build_result.embedded_files )
				result.dependent_files.push_back( std::move(path) );
		}
	}

	return result;
}

extern const uint32_t c_compiler_generation= 0;

} // namespace U
