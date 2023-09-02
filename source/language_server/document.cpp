#include "../code_builder_lib_common/source_file_contents_hash.hpp"
#include "../compiler0/lex_synt_lib/lex_utils.hpp"
#include "../compiler0/lex_synt_lib/syntax_analyzer.hpp"
#include "../lex_synt_lib_common/assert.hpp"
#include "syntax_tree_lookup.hpp"
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


void Document::SetText( std::string text )
{
	text_= std::move(text);
}

void Document::UpdateText( const DocumentRange& range, const std::string_view new_text )
{
	const std::optional<uint32_t> linear_position_start= DocumentPositionToLinearPosition( range.start, text_ );
	const std::optional<uint32_t> linear_position_end  = DocumentPositionToLinearPosition( range.end  , text_ );
	if( linear_position_start == std::nullopt || linear_position_end == std::nullopt )
	{
		log_ << "Failed to convert range into offsets!" << std::endl;
		return;
	}
	if( *linear_position_end < *linear_position_start )
	{
		log_ << "Wrong range: end is less than start!" << std::endl;
		return;
	}

	text_.replace( size_t(*linear_position_start), size_t(*linear_position_end - *linear_position_start), new_text );
}

const std::string& Document::GetText() const
{
	return text_;
}

std::optional<PositionInDocument> Document::GetDefinitionPoint( const DocumentPosition& position )
{
	if( last_valid_state_ == std::nullopt )
		return std::nullopt;

	const std::optional<TextLinearPosition> linear_position= DocumentPositionToLinearPosition( position, text_ );
	if( linear_position == std::nullopt )
	{
		log_ << "Failed to decode position" << std::endl;
		return std::nullopt;
	}
	std::optional<TextLinearPosition> identifier_start_linear_position= GetIdentifierStartForPosition( text_, *linear_position );
	if( identifier_start_linear_position == std::nullopt )
	{
		log_ << "Failed to find identifier start" << std::endl;
		return std::nullopt;
	}

	// Assume, that identifier can't be multiline.
	if( *identifier_start_linear_position > *linear_position )
	{
		log_ << "Find wrong identifier start!" << std::endl;
		return std::nullopt;
	}

	// Find line start.
	TextLinearPosition line_start_position= *identifier_start_linear_position;
	while( line_start_position >= 1 )
	{
		if( text_[ line_start_position - 1 ] == '\n' )
			break;
		--line_start_position;
	}

	// Count column in UTF-32 code point units.
	TextLinearPosition code_point_column= 0;
	{
		const char* s= text_.data() + line_start_position;
		const char* const s_end= text_.data() + *identifier_start_linear_position;
		while( s < s_end )
		{
			ReadNextUTF8Char( s, s_end );
			++code_point_column;
		}
	}

	const SrcLoc src_loc( 0, position.line, code_point_column );

	if( const auto result_src_loc= last_valid_state_->code_builder->GetDefinition( src_loc ) )
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

std::vector<DocumentRange> Document::GetHighlightLocations( const DocumentPosition& position )
{
	if( last_valid_state_ == std::nullopt )
		return {};

	const auto src_loc_corrected= GetIdentifierStartSrcLoc( DocumentPositionToSrcLoc(position), text_, last_valid_state_->line_to_linear_position_index );
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

std::vector<PositionInDocument> Document::GetAllOccurrences( const DocumentPosition& position )
{
	if( last_valid_state_ == std::nullopt )
		return {};

	const auto src_loc_corrected= GetIdentifierStartSrcLoc( DocumentPositionToSrcLoc(position), text_, last_valid_state_->line_to_linear_position_index );
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

std::vector<CompletionItem> Document::Complete( const DocumentPosition& position )
{
	log_ << "Completion request " << position.line << ":" << position.character << std::endl;

	if( last_valid_state_ == std::nullopt || last_valid_state_->source_graph.nodes_storage.empty() )
	{
		log_ << "Can't complete - document is not compiled" << std::endl;
		return {};
	}

	// Perform lexical analysis for current text.
	LexicalAnalysisResult lex_result= LexicalAnalysis( text_ );
	const LineToLinearPositionIndex line_to_linear_position_index= BuildLineToLinearPositionIndex( text_ );

	const uint32_t column= position.character;
	if( column == 0 )
	{
		log_ << "Can't complete at column 0" << std::endl;
		return {};
	}
	const uint32_t column_minus_one= column - 1u;

	const uint32_t line= position.line;
	if( line >= line_to_linear_position_index.size() )
	{
		log_ << "Line is greater than document end" << std::endl;
		return {};
	}

	const uint32_t char_position= line_to_linear_position_index[ line ] + column_minus_one;
	if( char_position >= text_.size() )
	{
		log_ << "Wrong linear position inside text" << std::endl;
		return {};
	}

	SrcLoc src_loc_corected;
	if( text_[ char_position ] == '.' )
	{
		log_ << "Complete for ." << std::endl;

		src_loc_corected= SrcLoc( 0, line, column_minus_one );

		bool found= false;
		for( Lexem& lexem : lex_result.lexems )
		{
			if( lexem.src_loc == src_loc_corected && lexem.type == Lexem::Type::Dot )
			{
				lexem.type= Lexem::Type::CompletionDot;
				found= true;
				break;
			}
		}

		if( !found )
		{
			log_ << "Can't find . lexem" << std::endl;
			return {};
		}
	}
	else if( text_[ char_position ] == ':' && char_position > 0 && text_[ char_position - 1 ] == ':' )
	{
		log_ << "Complete for ::" << std::endl;

		src_loc_corected= SrcLoc( 0, line, column_minus_one - 1 ); // -1 to reach start of "::"

		bool found= false;
		for( Lexem& lexem : lex_result.lexems )
		{
			if( lexem.src_loc == src_loc_corected && lexem.type == Lexem::Type::Scope )
			{
				lexem.type= Lexem::Type::CompletionScope;
				found= true;
				break;
			}
		}

		if( !found )
		{
			log_ << "Can't find :: lexem" << std::endl;
			return {};
		}
	}
	else
	{
		log_ << "Complete for identifier" << std::endl;

		const auto idenifier_start_src_loc= GetIdentifierStartSrcLoc( SrcLoc( 0, line, column_minus_one ), text_, line_to_linear_position_index );
		if( idenifier_start_src_loc == std::nullopt )
		{
			log_ << "Failed to find identifer start" << std::endl;
			return {};
		}
		src_loc_corected= *idenifier_start_src_loc;

		bool found= false;
		for( Lexem& lexem : lex_result.lexems )
		{
			if( lexem.src_loc == src_loc_corected && lexem.type == Lexem::Type::Identifier )
			{
				log_ << "Complete text \"" << lexem.text << "\"" << std::endl;
				lexem.type= Lexem::Type::CompletionIdentifier;
				found= true;
				break;
			}
		}

		if( !found )
		{
			log_ << "Can't find identifier lexem" << std::endl;
			return {};
		}
	}

	// Perform syntaxis parsing of current text.
	// In most cases it will fail, but it will still parse text until first error.
	// Here we assume, that first error is at least at point of completion or further.

	Synt::MacrosByContextMap merged_macroses;
	{
		const auto& child_nodes_indeces= last_valid_state_->source_graph.nodes_storage.front().child_nodes_indeces;
		if( child_nodes_indeces.empty() )
		{
			// Load built-in macroses only if this document has no imports. Otherwise built-in macroses will be taken from imports.
			merged_macroses= *PrepareBuiltInMacros( CalculateSourceFileContentsHash );
		}

		// Merge macroses of imported modules in order to parse document text properly.
		for( const size_t child_node_index : child_nodes_indeces )
		{
			for( const auto& context_macro_map_pair : *last_valid_state_->source_graph.nodes_storage[child_node_index].ast.macros )
			{
				Synt::MacroMap& dst_map= merged_macroses[context_macro_map_pair.first];
				for( const auto& macro_map_pair : context_macro_map_pair.second )
					dst_map[macro_map_pair.first]= macro_map_pair.second;
			}
		}
	}

	const auto synt_result=
		Synt::SyntaxAnalysis(
			lex_result.lexems,
			merged_macroses,
			std::make_shared<Synt::MacroExpansionContexts>(),
			CalculateSourceFileContentsHash( text_ ) );

	// Lookup global thing, where element with "completion*" lexem is located, together with path to it.
	const SyntaxTreeLookupResultOpt lookup_result=
		FindCompletionSyntaxElement( src_loc_corected.GetLine(), src_loc_corected.GetColumn(), synt_result.program_elements );
	if( lookup_result == std::nullopt )
	{
		log_ << "Failed to find parsed syntax element" << std::endl;
		return {};
	}

	log_ << "Find syntax element of kind " << lookup_result->element.index() << std::endl;

	// Use existing compiled program to perform names lookup.
	// Do not try to compile current text, because it is broken and completion will not return what should be returned.
	// Also it is too slow to recompile program for each completion.

	const GlobalItem& global_item= lookup_result->global_item;
	std::vector<CodeBuilder::CompletionItem> completion_result;
	if( const auto program_element= std::get_if<const Synt::ProgramElement*>( &global_item ) )
	{
		U_ASSERT( *program_element != nullptr );
		completion_result= last_valid_state_->code_builder->Complete( lookup_result->prefix, **program_element );
	}
	else if( const auto class_element= std::get_if<const Synt::ClassElement*>( &global_item ) )
	{
		U_ASSERT( *class_element != nullptr );
		completion_result= last_valid_state_->code_builder->Complete( lookup_result->prefix, **class_element );
	}
	else U_ASSERT( false );

	log_ << "Completion found " << completion_result.size() << " results" << std::endl;

	std::vector<CompletionItem> result_transformed;
	result_transformed.reserve( completion_result.size() );
	for( const CodeBuilder::CompletionItem& item : completion_result )
		result_transformed.push_back(
			CompletionItem{ item.name, item.sort_text, item.detail, TranslateCompletionItemKind( item.kind ) } );

	return result_transformed;
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

	// Specific options for the Language Server.
	options.collect_definition_points= true;
	options.skip_building_generated_functions= true;

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
