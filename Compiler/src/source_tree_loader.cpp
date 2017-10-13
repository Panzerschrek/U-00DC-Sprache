#include <iostream>

#include "assert.hpp"

#include "source_tree_loader.hpp"

namespace U
{

SourceTreeLoader::SourceTreeLoader( IVfsPtr vfs )
	: vfs_(std::move(vfs))
{
	U_ASSERT( vfs_ != nullptr );
}

SourceTreePtr SourceTreeLoader::LoadSource( const IVfs::Path& root_file_path )
{
	SourceTreePtr result( new SourceTree );
	result->root_node_index= LoadNode_r( root_file_path, *result );

	return result;
}

size_t SourceTreeLoader::LoadNode_r( const IVfs::Path& file_path, SourceTree& result )
{
	const ProgramString path_normalized= vfs_->NormalizePath( file_path );
	boost::optional<ProgramString> file_content= vfs_->LoadFileContent( path_normalized );
	if( file_content == boost::none )
	{
		std::cout << "Con not open file \"" << ToStdString( path_normalized ) << "\"" << std::endl;
		return ~0u;
	}

	const LexicalAnalysisResult lex_result= LexicalAnalysis( *file_content );
	for( const std::string& lexical_error_message : lex_result.error_messages )
		std::cout << lexical_error_message << "\n";
	result.lexical_errors.insert( result.lexical_errors.end(), lex_result.error_messages.begin(), lex_result.error_messages.end() );
	if( !lex_result.error_messages.empty() )
		return ~0u;

	SyntaxAnalysisResult synt_result= SyntaxAnalysis( lex_result.lexems );
	for( const std::string& syntax_error_message : synt_result.error_messages )
		std::cout << syntax_error_message << "\n";
	result.syntax_errors.insert( result.syntax_errors.end(), synt_result.error_messages.begin(), synt_result.error_messages.end() );
	if( !synt_result.error_messages.empty() )
		return ~0u;

	processed_files_stack_.push_back( path_normalized );
	// TODO - check loops

	// TODO - handle case of non-tree graph

	const size_t node_index= result.nodes_storage.size();
	result.nodes_storage.emplace_back();

	result.nodes_storage[node_index].file_path= path_normalized;

	result.nodes_storage[node_index].child_nodes_indeces.resize( synt_result.imports.size() );
	for( size_t i= 0; i  < result.nodes_storage[node_index].child_nodes_indeces.size(); ++i )
	{
		const size_t child_node_index= LoadNode_r( synt_result.imports[i].import_name, result );
		if( child_node_index != ~0u )
			result.nodes_storage[node_index].child_nodes_indeces[i]= child_node_index;
	}

	processed_files_stack_.pop_back();

	result.nodes_storage[node_index].ast= std::move( synt_result );

	return node_index;
}

} // namespace U
