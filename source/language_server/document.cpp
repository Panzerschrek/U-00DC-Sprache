#include "../code_builder_lib_common/source_file_contents_hash.hpp"
#include "../compiler0/lex_synt_lib/lex_utils.hpp"
#include "../compiler0/lex_synt_lib/syntax_analyzer.hpp"
#include "document.hpp"

namespace U
{

namespace LangServer
{

namespace
{

DocumentPosition SrcLocToDocumentPosition( const SrcLoc& src_loc )
{
	return DocumentPosition{ src_loc.GetLine(), src_loc.GetColumn() };
}

} // namespace

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

std::optional<RangeInDocument> Document::GetDefinitionPoint( const SrcLoc& src_loc )
{
	if( last_valid_state_ == std::nullopt )
		return std::nullopt;

	// Find lexem, where position is located.
	const Lexem* const lexem= GetLexemForPosition( src_loc.GetLine(), src_loc.GetColumn(), last_valid_state_->lexems );
	if( lexem == nullptr )
		return std::nullopt;

	if( const auto src_loc= last_valid_state_->code_builder->GetDefinition( lexem->src_loc ) )
	{
		RangeInDocument range;
		range.range.start= SrcLocToDocumentPosition(*src_loc);
		range.range.end= SrcLocToDocumentPosition( GetLexemEnd( src_loc->GetLine(), src_loc->GetColumn(), last_valid_state_->lexems ) );

		const uint32_t file_index= src_loc->GetFileIndex();
		if( file_index < last_valid_state_->source_graph.nodes_storage.size() )
			range.uri= Uri::FromFilePath( last_valid_state_->source_graph.nodes_storage[ file_index ].file_path );
		else
			range.uri= Uri::FromFilePath( path_ ); // TODO - maybe return std::nullopt instead?

		return range;
	}

	return std::nullopt;
}

std::vector<DocumentRange> Document::GetHighlightLocations( const SrcLoc& src_loc )
{
	if( last_valid_state_ == std::nullopt )
		return {};

	// Find lexem, where position is located.
	const Lexem* const lexem= GetLexemForPosition( src_loc.GetLine(), src_loc.GetColumn(), last_valid_state_->lexems );
	if( lexem == nullptr )
		return {};

	if( lexem->type != Lexem::Type::Identifier )
	{
		// There is no reason to highlight non-identifiers.
		// TODO - maybe highlight at least overloaded operators?
		return {};
	}

	const std::vector<SrcLoc> occurrences= last_valid_state_->code_builder->GetAllOccurrences( lexem->src_loc );

	std::vector<DocumentRange> result;
	result.reserve( occurrences.size() );

	for( const SrcLoc& src_loc : occurrences )
	{
		if( src_loc.GetFileIndex() != 0 )
			continue; // Filter out symbols from imported files.

		DocumentRange range;
		range.start= SrcLocToDocumentPosition(src_loc);
		range.end= SrcLocToDocumentPosition( GetLexemEnd( src_loc.GetLine(), src_loc.GetColumn(), last_valid_state_->lexems ) );

		result.push_back( std::move(range) );
	}

	return result;
}

std::vector<RangeInDocument> Document::GetAllOccurrences( const SrcLoc& src_loc )
{
	if( last_valid_state_ == std::nullopt )
		return {};

	// Find lexem, where position is located.
	const Lexem* const lexem= GetLexemForPosition( src_loc.GetLine(), src_loc.GetColumn(), last_valid_state_->lexems );
	if( lexem == nullptr )
		return {};

	if( lexem->type != Lexem::Type::Identifier )
	{
		// There is no reason to process non-identifiers.
		return {};
	}

	const std::vector<SrcLoc> occurrences= last_valid_state_->code_builder->GetAllOccurrences( lexem->src_loc );

	// TODO - improve this.
	// We need to extract occurences in other opended documents and maybe search for other files.

	std::vector<RangeInDocument> result;
	result.reserve( occurrences.size() );

	for( const SrcLoc& src_loc : occurrences )
	{
		RangeInDocument range;
		range.range.start= SrcLocToDocumentPosition(src_loc);
		range.range.end= SrcLocToDocumentPosition( GetLexemEnd( src_loc.GetLine(), src_loc.GetColumn(), last_valid_state_->lexems ) );

		const uint32_t file_index= src_loc.GetFileIndex();
		if( file_index < last_valid_state_->source_graph.nodes_storage.size() )
			range.uri= Uri::FromFilePath( last_valid_state_->source_graph.nodes_storage[ file_index ].file_path );
		else
			range.uri= Uri::FromFilePath( path_ ); // TODO - maybe skip this item instead?

		result.push_back( std::move(range) );
	}

	return result;
}

std::vector<Symbol> Document::GetSymbols()
{
	if( last_valid_state_ == std::nullopt )
		return {};

	return BuildSymbols( last_valid_state_->source_graph.nodes_storage.front().ast.program_elements );
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

	// Re-do lexical analysis, since source graph loading function doesn't saves it.
	Lexems lexems= LexicalAnalysis( text_ ).lexems;

	last_valid_state_= std::nullopt;
	last_valid_state_= CompiledState{ std::move( lexems ), std::move( source_graph ), std::move(llvm_context), std::move(code_builder) };
}

} // namespace LangServer

} // namespace U
