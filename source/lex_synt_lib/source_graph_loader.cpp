#include <algorithm>
#include <iostream>

#include "assert.hpp"

#include "source_graph_loader.hpp"

namespace U
{

extern const char c_build_in_macros_text[]=
#include "built_in_macros.h"
;

static Synt::MacrosPtr PrepareBuiltInMacros()
{
	const LexicalAnalysisResult lex_result= LexicalAnalysis( DecodeUTF8( c_build_in_macros_text ) );
	U_ASSERT( lex_result.error_messages.empty() );

	Synt::SyntaxAnalysisResult synt_result= Synt::SyntaxAnalysis( lex_result.lexems, std::make_shared<Synt::MacrosByContextMap>() );
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
	result->root_node_index= LoadNode_r( root_file_path, "", *result );

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
		ProgramString imports_loop_str;
		for( auto it= prev_file_it; it != processed_files_stack_.end(); ++it )
			imports_loop_str+= *it + " -> ";
		imports_loop_str+= full_file_path;
		std::cerr << ToUTF8( parent_file_path ) << ": 1:1: Import loop detected: " << ToUTF8( imports_loop_str ) << std::endl;
		result.have_errors= true;
		return ~0u;
	}

	// Check, if already loaded.
	for( size_t i= 0u; i < result.nodes_storage.size(); ++i )
		if( result.nodes_storage[i].file_path == full_file_path )
			return i;

	const size_t node_index= result.nodes_storage.size();

	std::optional<IVfs::LoadFileResult> loaded_file= vfs_->LoadFileContent( file_path, parent_file_path );
	if( loaded_file == std::nullopt )
	{
		Synt::SyntaxErrorMessage error_message;
		error_message.text= "Can not read file \"" + file_path + "\"";
		error_message.file_pos= FilePos{ 0u, 0u, static_cast<unsigned short>(node_index) };

		std::cerr << ToUTF8(error_message.text) << std::endl;
		result.syntax_errors.push_back( std::move(error_message) );
		result.have_errors= true;
		return ~0u;
	}

	LexicalAnalysisResult lex_result= LexicalAnalysis( loaded_file->file_content );
	for( const std::string& lexical_error_message : lex_result.error_messages )
		std::cerr << ToUTF8(full_file_path) << ": error: " << lexical_error_message << "\n";
	result.lexical_errors.insert( result.lexical_errors.end(), lex_result.error_messages.begin(), lex_result.error_messages.end() );
	if( !lex_result.error_messages.empty() )
	{
		result.have_errors= true;
		return ~0u;
	}

	for( Lexem& lexem :lex_result.lexems )
		lexem.file_pos.file_index= static_cast<unsigned short>(node_index);

	const std::vector<Synt::Import> imports= Synt::ParseImports( lex_result.lexems );

	result.nodes_storage.emplace_back();
	result.nodes_storage[node_index].file_path= full_file_path;
	result.nodes_storage[node_index].child_nodes_indeces.resize( imports.size() );

	std::vector<Synt::MacrosPtr> imported_macroses;

	// Recursively load imports.
	processed_files_stack_.push_back( full_file_path );
	for( size_t i= 0; i < result.nodes_storage[node_index].child_nodes_indeces.size(); ++i )
	{
		const size_t child_node_index= LoadNode_r( imports[i].import_name, full_file_path, result );
		if( child_node_index != ~0u )
		{
			imported_macroses.push_back( result.nodes_storage[child_node_index].ast.macros );
			result.nodes_storage[node_index].child_nodes_indeces[i]= child_node_index;
		}
	}
	processed_files_stack_.pop_back();

	// Merge macroses
	Synt::MacrosPtr merged_macroses= std::make_shared<Synt::MacrosByContextMap>( *built_in_macros_ );
	for( const Synt::MacrosPtr& macros : imported_macroses )
	{
		for( const auto& context_macro_map_pair : *macros )
		{
			Synt::MacroMap& dst_map= (*merged_macroses)[context_macro_map_pair.first];
			for( const auto& macro_map_pair : context_macro_map_pair.second )
			{
				if( dst_map.find(macro_map_pair.first) != dst_map.end() &&
					macro_map_pair.second.file_pos != dst_map.find(macro_map_pair.first)->second.file_pos )
				{
					Synt::SyntaxErrorMessage error_message;
					error_message.text= "Macro \"" + macro_map_pair.first + "\" redefinition.";
					error_message.file_pos= FilePos{ 0u, 0u, static_cast<unsigned short>(node_index) };

					std::cout << ToUTF8(error_message.text) << std::endl;
					result.syntax_errors.push_back( std::move(error_message) );
				}
				else
					dst_map[macro_map_pair.first]= macro_map_pair.second;
			}
		}
	}

	// Make syntax analysis, using imported macroses.
	Synt::SyntaxAnalysisResult synt_result= Synt::SyntaxAnalysis( lex_result.lexems, std::move(merged_macroses) );
	for( const Synt::SyntaxErrorMessage& syntax_error_message : synt_result.error_messages )
		std::cerr << ToUTF8(full_file_path) << ":"
			<< std::to_string(syntax_error_message.file_pos.line) << ":" << std::to_string(syntax_error_message.file_pos.pos_in_line) << ": error: " << ToUTF8( syntax_error_message.text ) << "\n";

	result.syntax_errors.insert( result.syntax_errors.end(), synt_result.error_messages.begin(), synt_result.error_messages.end() );
	if( !synt_result.error_messages.empty() )
	{
		result.have_errors= true;
		return ~0u;
	}

	result.nodes_storage[node_index].ast= std::move( synt_result );
	return node_index;
}

} // namespace U
