#include <algorithm>

#include "../../lex_synt_lib_common/assert.hpp"

#include "source_graph_loader.hpp"

namespace U
{

static Synt::MacrosPtr PrepareBuiltInMacros()
{
	#include "built_in_macros.hpp"
	const LexicalAnalysisResult lex_result= LexicalAnalysis( c_built_in_macros );
	U_ASSERT( lex_result.errors.empty() );

	const Synt::SyntaxAnalysisResult synt_result=
		Synt::SyntaxAnalysis(
			lex_result.lexems,
			Synt::MacrosByContextMap(),
			std::make_shared<Synt::MacroExpansionContexts>() );
	U_ASSERT( synt_result.error_messages.empty() );

	return synt_result.macros;
}

SourceGraphLoader::SourceGraphLoader( IVfsPtr vfs )
	: built_in_macros_(PrepareBuiltInMacros())
	, vfs_(std::move(vfs))
{
	U_ASSERT( built_in_macros_ != nullptr );
	U_ASSERT( vfs_ != nullptr );
}

SourceGraphPtr SourceGraphLoader::LoadSource( const IVfs::Path& root_file_path )
{
	auto result = std::make_unique<SourceGraph>();
	result->macro_expansion_contexts= std::make_shared<Synt::MacroExpansionContexts>();
	LoadNode_r( root_file_path, "", *result );

	return result;
}

size_t SourceGraphLoader::LoadNode_r(
	const IVfs::Path& file_path,
	const IVfs::Path& parent_file_path,
	SourceGraph& result )
{
	const IVfs::Path full_file_path= vfs_->GetFullFilePath( file_path, parent_file_path );

	// Check for dependency loops.
	const auto prev_file_it= std::find( processed_files_stack_.begin(), processed_files_stack_.end(), full_file_path );
	if( prev_file_it != processed_files_stack_.end() )
	{
		std::string imports_loop_str= "Import loop detected: ";
		for( auto it= prev_file_it; it != processed_files_stack_.end(); ++it )
			imports_loop_str+= *it + " -> ";
		imports_loop_str+= full_file_path;

		result.errors.emplace_back( imports_loop_str, SrcLoc( 0u, 0u, 0u ) );

		return ~0u;
	}

	// Check, if already loaded.
	for( size_t i= 0u; i < result.nodes_storage.size(); ++i )
		if( result.nodes_storage[i].file_path == full_file_path )
			return i;

	const size_t node_index= result.nodes_storage.size();
	result.nodes_storage.emplace_back();
	result.nodes_storage[node_index].file_path= full_file_path;

	const std::optional<IVfs::FileContent> loaded_file= vfs_->LoadFileContent( full_file_path );
	if( loaded_file == std::nullopt )
	{
		LexSyntError error_message( "Can not read file \"" + full_file_path + "\"", SrcLoc( uint32_t(node_index), 0u, 0u ) );
		result.errors.push_back( std::move(error_message) );
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

	result.nodes_storage[node_index].child_nodes_indeces.resize( imports.size() );

	std::vector<Synt::MacrosPtr> imported_macroses;

	// Recursively load imports.
	processed_files_stack_.push_back( full_file_path );
	for( size_t i= 0; i < result.nodes_storage[node_index].child_nodes_indeces.size(); ++i )
	{
		const size_t child_node_index= LoadNode_r( imports[i].import_name, full_file_path, result );
		if( child_node_index != ~0u )
		{
			if( const Synt::MacrosPtr macro= result.nodes_storage[child_node_index].ast.macros; macro != nullptr )
				imported_macroses.push_back( macro );
			result.nodes_storage[node_index].child_nodes_indeces[i]= child_node_index;
		}
	}
	processed_files_stack_.pop_back();

	// Merge macroses
	Synt::MacrosByContextMap merged_macroses= *built_in_macros_;
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

	// Make syntax analysis, using imported macroses.
	Synt::SyntaxAnalysisResult synt_result=
		Synt::SyntaxAnalysis(
		lex_result.lexems,
		std::move(merged_macroses),
		result.macro_expansion_contexts );

	result.errors.insert( result.errors.end(), synt_result.error_messages.begin(), synt_result.error_messages.end() );

	result.nodes_storage[node_index].ast= std::move( synt_result );
	return node_index;
}

} // namespace U
