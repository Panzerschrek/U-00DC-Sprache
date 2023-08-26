#include "../code_builder_lib_common/source_file_contents_hash.hpp"
#include "../compiler0/lex_synt_lib/lex_utils.hpp"
#include "document_position_utils.hpp"
#include "document.hpp"

namespace U
{

namespace LangServer
{

Document::Document( IVfs::Path path, DocumentBuildOptions build_options, IVfs& vfs, std::ostream& log )
	: path_(std::move(path)), build_options_(std::move(build_options)), vfs_(vfs), log_(log)
{
	(void)log_;
}

LexSyntErrors Document::GetLexErrors() const
{
	return lex_errors_;
}

LexSyntErrors Document::GetSyntErrors() const
{
	return synt_errors_;
}

CodeBuilderErrorsContainer Document::GetCodeBuilderErrors() const
{
	return code_builder_errors_;
}

std::optional<PositionInDocument> Document::GetDefinitionPoint( const SrcLoc& src_loc )
{
	if( last_valid_state_ == std::nullopt )
		return std::nullopt;

	const auto src_loc_corrected= GetIdentifierStartSrcLoc( src_loc, text_, last_valid_state_->line_to_linear_position_index );
	if( src_loc_corrected == std::nullopt )
		return std::nullopt;

	if( const auto result_src_loc= last_valid_state_->code_builder->GetDefinition( *src_loc_corrected ) )
	{
		PositionInDocument position;
		position.position= SrcLocToDocumentPosition( *result_src_loc );

		const uint32_t file_index= result_src_loc->GetFileIndex();
		if( file_index < last_valid_state_->source_graph.nodes_storage.size() )
			position.uri= Uri::FromFilePath( last_valid_state_->source_graph.nodes_storage[ file_index ].file_path );
		else
			position.uri= Uri::FromFilePath( path_ ); // TODO - maybe return std::nullopt instead?

		return position;
	}

	return std::nullopt;
}

std::vector<DocumentRange> Document::GetHighlightLocations( const SrcLoc& src_loc )
{
	if( last_valid_state_ == std::nullopt )
		return {};

	const auto src_loc_corrected= GetIdentifierStartSrcLoc( src_loc, text_, last_valid_state_->line_to_linear_position_index );
	if( src_loc_corrected == std::nullopt )
		return {};

	const std::vector<SrcLoc> occurrences= last_valid_state_->code_builder->GetAllOccurrences( *src_loc_corrected );

	std::vector<DocumentRange> result;
	result.reserve( occurrences.size() );

	for( const SrcLoc& result_src_loc : occurrences )
	{
		if( result_src_loc.GetFileIndex() != 0 )
			continue; // Filter out symbols from imported files.

		// It is fine to use text of this file to determine end position, since highlighting works only within the document.
		const auto result_end_src_loc= GetIdentifierEndSrcLoc( result_src_loc, text_, last_valid_state_->line_to_linear_position_index );
		if( result_end_src_loc == std::nullopt )
			continue;

		DocumentRange range;
		range.start= SrcLocToDocumentPosition( result_src_loc );
		range.end= SrcLocToDocumentPosition( *result_end_src_loc );

		result.push_back( std::move(range) );
	}

	return result;
}

std::vector<PositionInDocument> Document::GetAllOccurrences( const SrcLoc& src_loc )
{
	if( last_valid_state_ == std::nullopt )
		return {};

	const auto src_loc_corrected= GetIdentifierStartSrcLoc( src_loc, text_, last_valid_state_->line_to_linear_position_index );
	if( src_loc_corrected == std::nullopt )
		return {};

	const std::vector<SrcLoc> occurrences= last_valid_state_->code_builder->GetAllOccurrences( *src_loc_corrected );

	// TODO - improve this.
	// We need to extract occurences in other opended documents and maybe search for other files.

	std::vector<PositionInDocument> result;
	result.reserve( occurrences.size() );

	for( const SrcLoc& result_src_loc : occurrences )
	{
		PositionInDocument position;
		position.position= SrcLocToDocumentPosition( result_src_loc );

		const uint32_t file_index= result_src_loc.GetFileIndex();
		if( file_index < last_valid_state_->source_graph.nodes_storage.size() )
			position.uri= Uri::FromFilePath( last_valid_state_->source_graph.nodes_storage[ file_index ].file_path );
		else
			position.uri= Uri::FromFilePath( path_ ); // TODO - maybe skip this item instead?

		result.push_back( std::move(position) );
	}

	return result;
}

std::vector<Symbol> Document::GetSymbols()
{
	if( last_valid_state_ == std::nullopt )
		return {};

	return BuildSymbols( last_valid_state_->source_graph.nodes_storage.front().ast.program_elements );
}

std::optional<DocumentPosition> Document::GetIdentifierEndPosition( const DocumentPosition& start_position ) const
{
	if( last_valid_state_ == std::nullopt )
		return std::nullopt;

	const auto end_src_loc= GetIdentifierEndSrcLoc( DocumentPositionToSrcLoc( start_position ), text_, last_valid_state_->line_to_linear_position_index );
	if( end_src_loc == std::nullopt )
		return std::nullopt;

	return SrcLocToDocumentPosition( *end_src_loc );
}

void Document::SetText( std::string text )
{
	if( text == text_ )
		return;

	text_= text;
	Rebuild();
}

const std::string& Document::GetText() const
{
	return text_;
}

void Document::Rebuild()
{
	lex_errors_.clear();
	synt_errors_.clear();
	code_builder_errors_.clear();

	SourceGraph source_graph= LoadSourceGraph( vfs_, CalculateSourceFileContentsHash, path_, build_options_.prelude );

	lex_errors_= std::move( source_graph.errors );
	if( !lex_errors_.empty() )
		return;

	if( source_graph.nodes_storage.empty() )
		return;

	// Take syntax errors only from this document.
	synt_errors_.swap( source_graph.nodes_storage.front().ast.error_messages );
	if( !synt_errors_.empty() )
	{
		for( const auto& error : synt_errors_ )
			std::cout << "error: " << error.text << std::endl;
		return;
	}

	// Do not compile code if imports are not correct.
	for( const SourceGraph::Node& node : source_graph.nodes_storage )
	{
		if( !node.ast.error_messages.empty() )
			return;
	}

	// TODO - maybe avoid recreating context or even share it across multiple documents?
	auto llvm_context= std::make_unique<llvm::LLVMContext>();

	// Disable almost all code builder options.
	// We do not need to generate code here - only assist developer (retrieve errors, etc.).
	CodeBuilderOptions options;
	options.build_debug_info= false;
	options.create_lifetimes= false;
	options.generate_lifetime_start_end_debug_calls= false;
	options.generate_tbaa_metadata= false;
	options.report_about_unused_names= false;

	options.collect_definition_points= true;

	auto code_builder=
		CodeBuilder::BuildProgramAndLeaveInternalState(
			*llvm_context,
			build_options_.data_layout,
			build_options_.target_triple,
			options,
			source_graph );

	code_builder_errors_= code_builder->TakeErrors();

	auto line_to_linear_position_index= BuildLineToLinearPositionIndex( text_ );

	last_valid_state_= std::nullopt;
	last_valid_state_= CompiledState{ std::move(line_to_linear_position_index), std::move( source_graph ), std::move(llvm_context), std::move(code_builder) };
}

} // namespace LangServer

} // namespace U
