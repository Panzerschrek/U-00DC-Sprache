#include <algorithm>

#include "../../lex_synt_lib_common/assert.hpp"

#include "source_graph_loader.hpp"

namespace U
{

namespace
{

size_t LoadNode_r(
	IVfs& vfs,
	const SourceFilePathHashigFunction source_file_path_hashing_function,
	const IVfs::Path& file_path,
	const IVfs::Path& parent_file_path,
	std::vector<std::string>& processed_files_stack,
	const SrcLoc& import_src_loc,
	SourceGraph& result )
{
	const IVfs::Path full_file_path= vfs.GetFullFilePath( file_path, parent_file_path );

	// Check for dependency loops.
	const auto prev_file_it= std::find( processed_files_stack.begin(), processed_files_stack.end(), full_file_path );
	if( prev_file_it != processed_files_stack.end() )
	{
		std::string imports_loop_str= "Import loop detected: ";
		for( auto it= prev_file_it; it != processed_files_stack.end(); ++it )
			imports_loop_str+= *it + " -> ";
		imports_loop_str+= full_file_path;

		result.errors.emplace_back( imports_loop_str, import_src_loc );

		return ~0u;
	}

	// Check, if already loaded.
	for( size_t i= 0u; i < result.nodes_storage.size(); ++i )
		if( result.nodes_storage[i].file_path == full_file_path )
			return i;

	const size_t node_index= result.nodes_storage.size();
	result.nodes_storage.emplace_back();
	result.nodes_storage[node_index].file_path= full_file_path;

	result.nodes_storage[node_index].category=
		vfs.IsFileFromSourcesDirectory( full_file_path )
			? SourceGraph::Node::Category::SourceOrInternalImport
			: SourceGraph::Node::Category::OtherImport;

	const std::optional<IVfs::FileContent> loaded_file= vfs.LoadFileContent( full_file_path );
	if( loaded_file == std::nullopt )
	{
		LexSyntError error_message( "Can not read file \"" + (full_file_path.empty() ? file_path : full_file_path) + "\"", import_src_loc );
		result.errors.push_back( std::move(error_message) );
		return ~0u;
	}

	// Check for allowing import after checking for file existence - in order to generate "file not found" error first.
	if( !vfs.IsImportingFileAllowed( full_file_path ) )
	{
		result.errors.emplace_back(
			"Importing file \"" + (full_file_path.empty() ? file_path : full_file_path) + "\" isn't allowed.",
			import_src_loc );
		return ~0u;
	}

	LexicalAnalysisResult lex_result= LexicalAnalysis( *loaded_file );

	for( LexSyntError error: lex_result.errors )
	{
		error.src_loc.SetFileIndex(uint32_t(node_index));
		result.errors.push_back( std::move(error) );
	}

	if( !lex_result.errors.empty() )
		return ~0u;

	for( Lexem& lexem :lex_result.lexems )
		lexem.src_loc.SetFileIndex(uint32_t(node_index));

	const std::vector<Synt::Import> imports= Synt::ParseImports( lex_result.lexems );

	result.nodes_storage[node_index].child_nodes_indices.resize( imports.size() );

	std::vector<Synt::MacrosPtr> imported_macroses;

	// Recursively load imports.
	processed_files_stack.push_back( full_file_path );
	for( size_t i= 0; i < result.nodes_storage[node_index].child_nodes_indices.size(); ++i )
	{
		const Synt::Import& import = imports[i];
		const size_t child_node_index=
			LoadNode_r(
				vfs,
				source_file_path_hashing_function,
				import.import_name,
				full_file_path,
				processed_files_stack,
				import.src_loc,
				result );
		if( child_node_index != ~0u )
		{
			if( const Synt::MacrosPtr macro= result.nodes_storage[child_node_index].ast.macros; macro != nullptr )
				imported_macroses.push_back( macro );
			result.nodes_storage[node_index].child_nodes_indices[i]= child_node_index;
		}
	}
	processed_files_stack.pop_back();

	// Merge macroses
	Synt::MacrosByContextMap merged_macroses;
	for( const Synt::MacrosPtr& macros : imported_macroses )
	{
		for( const auto& context_macro_map_pair : *macros )
		{
			Synt::MacroMap& dst_map= merged_macroses[context_macro_map_pair.first];
			for( const auto& macro_map_pair : context_macro_map_pair.second )
			{
				if( dst_map.find(macro_map_pair.first) != dst_map.end() &&
					macro_map_pair.second.src_loc != dst_map.find(macro_map_pair.first)->second.src_loc )
				{
					result.errors.emplace_back(
						"Macro \"" + macro_map_pair.first + "\" redefinition.",
						SrcLoc( uint32_t(node_index), 0u, 0u ) );
				}
				else
					dst_map[macro_map_pair.first]= macro_map_pair.second;
			}
		}
	}

	std::string file_path_hash= source_file_path_hashing_function( full_file_path );

	// Make syntax analysis, using imported macroses.
	Synt::SyntaxAnalysisResult synt_result=
		Synt::SyntaxAnalysis(
		lex_result.lexems,
		std::move(merged_macroses),
		result.macro_expansion_contexts,
		file_path_hash );

	result.errors.insert( result.errors.end(), synt_result.error_messages.begin(), synt_result.error_messages.end() );

	result.nodes_storage[node_index].ast= std::move( synt_result );
	result.nodes_storage[node_index].file_path_hash= std::move(file_path_hash);
	return node_index;
}

} // namespace

SourceGraph LoadSourceGraph(
	IVfs& vfs,
	const SourceFilePathHashigFunction source_file_path_hashing_function,
	const IVfs::Path& root_file_path,
	const std::string_view prelude_code )
{
	SourceGraph result;
	result.macro_expansion_contexts= std::make_shared<Synt::MacroExpansionContexts>();

	std::vector<std::string> processed_files_stack;
	LoadNode_r(
		vfs,
		source_file_path_hashing_function,
		root_file_path,
		"",
		processed_files_stack,
		SrcLoc(0, 0, 0),
		result );

	if( !prelude_code.empty() )
	{
		// Assume that prelude code has no imports and macroses.
		// With such assumption we can just parse prelude without bothering with imports/macros.

		const uint32_t prelude_node_index= uint32_t(result.nodes_storage.size());

		LexicalAnalysisResult lex_result= LexicalAnalysis( prelude_code );

		U_ASSERT( lex_result.errors.empty() ); // Prelude code should be valid.

		for( Lexem& lexem :lex_result.lexems )
			lexem.src_loc.SetFileIndex(uint32_t(prelude_node_index));

		// Use non-normalized path, to avoid name collisions with real files.
		std::string file_path= "compiler_generated_prelude";

		std::string file_path_hash= source_file_path_hashing_function( file_path );

		Synt::SyntaxAnalysisResult synt_result=
			Synt::SyntaxAnalysis(
				lex_result.lexems,
				Synt::MacrosByContextMap(),
				result.macro_expansion_contexts,
				file_path_hash );

		result.errors.insert( result.errors.end(), synt_result.error_messages.begin(), synt_result.error_messages.end() );

		// Add "import" of prelude node in nodes, that have no other imports (and only in these nodes).
		// There is no reason to import prelude in all modules.
		for( SourceGraph::Node& other_node : result.nodes_storage )
			if( other_node.child_nodes_indices.empty() )
				other_node.child_nodes_indices.push_back( prelude_node_index );

		SourceGraph::Node prelude_node;
		prelude_node.file_path= std::move(file_path);
		prelude_node.file_path_hash= std::move(file_path_hash);
		prelude_node.ast= std::move(synt_result);
		prelude_node.category= SourceGraph::Node::Category::BuiltInPrelude;

		result.nodes_storage.push_back( std::move(prelude_node) );
	}

	return result;
}

} // namespace U
