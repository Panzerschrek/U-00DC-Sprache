#include "../../code_builder_lib_common/long_stable_hash.hpp"
#include "../../compilers_common_lib/code_builder_launcher.hpp"
#include "../../compilers_support_lib/vfs.hpp"
#include "../code_builder_lib/errors_print.hpp"
#include "../code_builder_lib/code_builder.hpp"
#include "../lex_synt_lib/source_graph_loader.hpp"

namespace U
{

namespace
{

std::vector<std::string> BuildDependentFilesList( const SourceGraph& source_graph )
{
	std::vector<std::string> list;
	list.reserve( source_graph.nodes_storage.size() );
	for( const SourceGraph::Node& node : source_graph.nodes_storage )
		list.push_back( node.file_path );

	return list;
}

} // namespace

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
	const ErrorsFormat errors_format,
	bool tests_output,
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

		if( !source_graph.errors.empty() )
		{
			PrintLexSyntErrors( BuildDependentFilesList( source_graph ), source_graph.errors, errors_format );

			result.has_errors= true;
		}
		else
		{
			const auto source_graph_ptr= MakeRcPtr( std::move(source_graph) );

			CodeBuilder::BuildResult build_result=
				CodeBuilder::BuildProgram(
					llvm_context,
					data_layout,
					target_triple,
					options,
					source_graph_ptr,
					vfs );

			result.llvm_modules[i]= std::move( build_result.module );

			if( !build_result.errors.empty() )
			{
				const auto dependent_files_list= BuildDependentFilesList( *source_graph_ptr );
				if( tests_output )
					PrintErrorsForTests( dependent_files_list, build_result.errors );
				else
					PrintErrors( dependent_files_list, build_result.errors, errors_format );

				result.has_errors= true;
			}

			// Add embedded files into the list of dependencies.
			for( IVfs::Path& path : build_result.embedded_files )
				result.dependent_files.push_back( std::move(path) );
		}
	}

	return result;
}

extern const uint32_t c_compiler_generation= 0;

} // namespace U
