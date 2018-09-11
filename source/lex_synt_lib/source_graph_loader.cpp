#include <iostream>

#include "assert.hpp"

#include "source_graph_loader.hpp"

namespace U
{

SourceGraphLoader::SourceGraphLoader( IVfsPtr vfs )
	: vfs_(std::move(vfs))
{
	U_ASSERT( vfs_ != nullptr );
}

SourceGraphPtr SourceGraphLoader::LoadSource( const IVfs::Path& root_file_path )
{
	SourceGraphPtr result( new SourceGraph );
	result->root_node_index= LoadNode_r( root_file_path, ""_SpC, *result );

	return result;
}

size_t SourceGraphLoader::LoadNode_r(
	const IVfs::Path& file_path,
	const IVfs::Path& parent_file_path,
	SourceGraph& result )
{
	const size_t node_index= result.nodes_storage.size();

	boost::optional<IVfs::LoadFileResult> loaded_file= vfs_->LoadFileContent( file_path, parent_file_path );
	if( loaded_file == boost::none )
	{
		Synt::SyntaxErrorMessage error_message;
		error_message.text= "Can not read file \""_SpC + file_path +"\""_SpC;
		error_message.file_pos= FilePos{ 0u, 0u, static_cast<unsigned short>(node_index) };

		std::cout << ToStdString(error_message.text) << std::endl;
		result.syntax_errors.push_back( std::move(error_message) );
		return ~0u;
	}

	LexicalAnalysisResult lex_result= LexicalAnalysis( loaded_file->file_content );
	for( const std::string& lexical_error_message : lex_result.error_messages )
		std::cout << lexical_error_message << "\n";
	result.lexical_errors.insert( result.lexical_errors.end(), lex_result.error_messages.begin(), lex_result.error_messages.end() );
	if( !lex_result.error_messages.empty() )
		return ~0u;

	for( Lexem& lexem :lex_result.lexems )
		lexem.file_pos.file_index= static_cast<unsigned short>(node_index);

	Synt::SyntaxAnalysisResult synt_result= Synt::SyntaxAnalysis( lex_result.lexems );
	for( const Synt::SyntaxErrorMessage& syntax_error_message : synt_result.error_messages )
		std::cout << std::to_string(syntax_error_message.file_pos.line) << ":" << std::to_string(syntax_error_message.file_pos.pos_in_line) << ": " << ToStdString( syntax_error_message.text ) << "\n";
	result.syntax_errors.insert( result.syntax_errors.end(), synt_result.error_messages.begin(), synt_result.error_messages.end() );
	if( !synt_result.error_messages.empty() )
		return ~0u;

	processed_files_stack_.push_back( file_path );
	// TODO - check loops

	result.nodes_storage.emplace_back();
	result.nodes_storage[node_index].file_path= loaded_file->full_file_path;
	result.nodes_storage[node_index].child_nodes_indeces.resize( synt_result.imports.size() );

	for( size_t i= 0; i < result.nodes_storage[node_index].child_nodes_indeces.size(); ++i )
	{
		// Search for already loaded file.
		bool prev_found= false;
		for( size_t j= 0u; j < result.nodes_storage.size(); ++j )
		{
			if( result.nodes_storage[j].file_path == vfs_->GetFullFilePath( synt_result.imports[i].import_name, loaded_file->full_file_path ) )
			{
				result.nodes_storage[node_index].child_nodes_indeces[i]= j;
				prev_found= true;
				break;
			}
		}
		if( prev_found )
			continue;

		const size_t child_node_index= LoadNode_r( synt_result.imports[i].import_name, loaded_file->full_file_path, result );
		if( child_node_index != ~0u )
			result.nodes_storage[node_index].child_nodes_indeces[i]= child_node_index;
	}

	processed_files_stack_.pop_back();

	result.nodes_storage[node_index].ast= std::move( synt_result );

	return node_index;
}

} // namespace U
