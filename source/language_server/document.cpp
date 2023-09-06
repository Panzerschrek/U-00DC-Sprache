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

namespace
{

std::optional<DocumentRange> GetErrorRange( const SrcLoc& src_loc, const std::string_view program_text, const LineToLinearPositionIndex& line_to_linear_position_index )
{
	const uint32_t line= src_loc.GetLine();
	if( line >= line_to_linear_position_index.size() )
		return std::nullopt;

	const std::string_view line_text= program_text.substr( line_to_linear_position_index[line] );

	const auto column_utf8= Utf32PositionToUtf8Position( line_text, src_loc.GetColumn() );
	if( column_utf8 == std::nullopt )
		return std::nullopt;

	// Use identifier end for position.
	const auto column_end_utf8= GetIdentifierEndForPosition( line_text, *column_utf8 );

	const auto column_utf16= Utf8PositionToUtf16Position( line_text, *column_utf8 );
	if( column_utf16 == std::nullopt )
		return std::nullopt;

	std::optional<uint32_t> column_end_utf16;
	if( column_end_utf8 != std::nullopt )
		column_end_utf16= Utf8PositionToUtf16Position( line_text, *column_end_utf8 );
	else
		column_end_utf16= *column_utf16 + 1; // Use character + 1 as range for non-identifiers.

	if( column_end_utf16 == std::nullopt )
		return std::nullopt;

	return DocumentRange{ { line, *column_utf16 }, { line, *column_end_utf16 } };
}

void PopulateDiagnostics(
	const LexSyntErrors& errors,
	const std::string_view program_text,
	const LineToLinearPositionIndex& line_to_linear_position_index,
	std::vector<DocumentDiagnostic>& out_diagnostics )
{
	out_diagnostics.reserve( out_diagnostics.size() + errors.size() );

	for( const LexSyntError& error : errors )
	{
		if( error.src_loc.GetFileIndex() != 0 )
			continue; // Ignore errors from imported files.

		auto range= GetErrorRange( error.src_loc, program_text, line_to_linear_position_index );
		if( range == std::nullopt )
			continue;

		DocumentDiagnostic diagnostic;
		diagnostic.range= std::move(*range);
		diagnostic.text= error.text;

		out_diagnostics.push_back( std::move(diagnostic) );
	}
}

void PopulateDiagnostics(
	const CodeBuilderErrorsContainer& errors,
	const std::string_view program_text,
	const LineToLinearPositionIndex& line_to_linear_position_index,
	std::vector<DocumentDiagnostic>& out_diagnostics )
{
	out_diagnostics.reserve( out_diagnostics.size() + errors.size() );

	for( const CodeBuilderError& error : errors )
	{
		auto range= GetErrorRange( error.src_loc, program_text, line_to_linear_position_index );
		if( range == std::nullopt )
			continue;

		DocumentDiagnostic diagnostic;
		diagnostic.range= std::move(*range);
		diagnostic.text= error.text;

		// TODO - fill other fields, like code and template/macro expansion context?

		out_diagnostics.push_back( std::move(diagnostic) );
	}
}

} // namespace

Document::Document( IVfs::Path path, DocumentBuildOptions build_options, IVfs& vfs, std::ostream& log )
	: path_(std::move(path)), build_options_(std::move(build_options)), vfs_(vfs), log_(log)
{
	(void)log_;
}

void Document::SetText( std::string text )
{
	text_= std::move(text);
	text_changes_since_last_valid_state_= std::nullopt; // Can't perform changes tracking when text is completely changed.
	BuildLineToLinearPositionIndex( text_, line_to_linear_position_index_ );
}

const std::string& Document::GetCurrentText() const
{
	return text_;
}

void Document::UpdateText( const DocumentRange& range, const std::string_view new_text )
{
	const std::optional<TextLinearPosition> linear_position_start= DocumentPositionToLinearPosition( range.start, text_ );
	const std::optional<TextLinearPosition> linear_position_end  = DocumentPositionToLinearPosition( range.end  , text_ );
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
	BuildLineToLinearPositionIndex( text_, line_to_linear_position_index_ );

	// Save changes sequence.
	if( last_valid_state_ != std::nullopt && text_changes_since_last_valid_state_ != std::nullopt )
	{
		TextChange change;
		change.range_start= *linear_position_start;
		change.range_end= *linear_position_end;
		change.new_count= uint32_t(new_text.size());
		text_changes_since_last_valid_state_->push_back( std::move(change) );
	}
}

const std::string& Document::GetTextForCompilation() const
{
	if( in_rebuild_call_ )
		return text_; // Force return raw text if this method was called indirectly from the "Rebuild" method.

	// By providing text for last valid state we ensure dependent documents will build properly.
	// Also this allows to perform proper mapping of "SrcLoc" to current state of the text.
	return last_valid_state_ == std::nullopt ? text_ : last_valid_state_->text;
}

llvm::ArrayRef<DocumentDiagnostic> Document::GetDiagnostics() const
{
	return diagnostics_;
}

std::optional<SrcLocInDocument> Document::GetDefinitionPoint( const DocumentPosition& position ) const
{
	if( last_valid_state_ == std::nullopt )
		return std::nullopt;

	const auto src_loc= GetIdentifierStartSrcLoc( position );
	if( src_loc == std::nullopt )
	{
		log_ << "Failed to get indentifier start" << std::endl;
		return std::nullopt;
	}

	if( const auto result_src_loc= last_valid_state_->code_builder->GetDefinition( *src_loc ) )
	{
		SrcLocInDocument location;
		location.src_loc= *result_src_loc;

		const uint32_t file_index= result_src_loc->GetFileIndex();
		if( file_index < last_valid_state_->source_graph.nodes_storage.size() )
			location.uri= Uri::FromFilePath( last_valid_state_->source_graph.nodes_storage[ file_index ].file_path );
		else
			location.uri= Uri::FromFilePath( path_ ); // TODO - maybe return std::nullopt instead?

		return location;
	}

	return std::nullopt;
}

std::vector<DocumentRange> Document::GetHighlightLocations( const DocumentPosition& position ) const
{
	if( last_valid_state_ == std::nullopt )
		return {};

	const auto src_loc= GetIdentifierStartSrcLoc( position );
	if( src_loc == std::nullopt )
		return {};

	const std::vector<SrcLoc> occurrences= last_valid_state_->code_builder->GetAllOccurrences( *src_loc );

	std::vector<DocumentRange> result;
	result.reserve( occurrences.size() );

	if( occurrences.empty() )
		return {};

	for( const SrcLoc& result_src_loc : occurrences )
	{
		if( result_src_loc.GetFileIndex() != 0 )
			continue; // Filter out symbols from imported files.

		if( auto range= GetIdentifierRange( result_src_loc ) )
			result.push_back( std::move(*range) );
	}

	return result;
}

std::vector<SrcLocInDocument> Document::GetAllOccurrences( const DocumentPosition& position ) const
{
	if( last_valid_state_ == std::nullopt )
		return {};

	const auto src_loc= GetIdentifierStartSrcLoc( position );
	if( src_loc == std::nullopt )
	{
		log_ << "Failed to get indentifier start" << std::endl;
		return {};
	}

	const std::vector<SrcLoc> occurrences= last_valid_state_->code_builder->GetAllOccurrences( *src_loc );

	// TODO - improve this.
	// We need to extract occurences in other opended documents and maybe search for other files.

	std::vector<SrcLocInDocument> result;
	result.reserve( occurrences.size() );

	for( const SrcLoc& result_src_loc : occurrences )
	{
		SrcLocInDocument location;
		location.src_loc= result_src_loc;

		const uint32_t file_index= result_src_loc.GetFileIndex();
		if( file_index < last_valid_state_->source_graph.nodes_storage.size() )
			location.uri= Uri::FromFilePath( last_valid_state_->source_graph.nodes_storage[ file_index ].file_path );
		else
			location.uri= Uri::FromFilePath( path_ ); // TODO - maybe skip this item instead?

		result.push_back( std::move(location) );
	}

	return result;
}

std::vector<Symbol> Document::GetSymbols() const
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

	const uint32_t line= position.line;
	if( line >= line_to_linear_position_index_.size() )
	{
		log_ << "Line is greater than document end" << std::endl;
		return {};
	}
	const std::string_view line_text= std::string_view(text_).substr( line_to_linear_position_index_[ line ] );

	const auto column_utf8= Utf16PositionToUtf8Position( line_text, position.character );
	if( column_utf8 == std::nullopt )
	{
		log_ << "Can't obtain column" << std::endl;
		return {};
	}
	if( *column_utf8 == 0 )
	{
		log_ << "Can't complete at column 0" << std::endl;
		return {};
	}
	const TextLinearPosition column_utf8_minus_one= *column_utf8 - 1u;

	SrcLoc src_loc;
	if( line_text[ column_utf8_minus_one ] == '.' )
	{
		log_ << "Complete for ." << std::endl;

		const auto column= Utf8PositionToUtf32Position( line_text, column_utf8_minus_one );
		if( column == std::nullopt )
		{
			log_ << "Failed to get utf32 position" << std::endl;
			return {};
		}
		src_loc= SrcLoc( 0, line, *column );

		bool found= false;
		for( Lexem& lexem : lex_result.lexems )
		{
			if( lexem.src_loc == src_loc && lexem.type == Lexem::Type::Dot )
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
	else if( line_text[ column_utf8_minus_one ] == ':' && column_utf8_minus_one > 0 && line_text[ column_utf8_minus_one - 1 ] == ':' )
	{
		log_ << "Complete for ::" << std::endl;

		const auto column= Utf8PositionToUtf32Position( line_text, column_utf8_minus_one - 1 ); // -1 to reach start of "::"
		if( column == std::nullopt )
		{
			log_ << "Failed to get utf32 position" << std::endl;
			return {};
		}
		src_loc= SrcLoc( 0, line, *column );

		bool found= false;
		for( Lexem& lexem : lex_result.lexems )
		{
			if( lexem.src_loc == src_loc && lexem.type == Lexem::Type::Scope )
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

		const std::optional<TextLinearPosition> idenifier_start_utf8= GetIdentifierStartForPosition( line_text, column_utf8_minus_one );
		if( idenifier_start_utf8 == std::nullopt )
		{
			log_ << "Failed to find identifer start" << std::endl;
			return {};
		}

		const auto column= Utf8PositionToUtf32Position( line_text, *idenifier_start_utf8 );
		if( column == std::nullopt )
		{
			log_ << "Failed to get utf32 position" << std::endl;
			return {};
		}
		src_loc= SrcLoc( 0, line, *column );

		bool found= false;
		for( Lexem& lexem : lex_result.lexems )
		{
			if( lexem.src_loc == src_loc && lexem.type == Lexem::Type::Identifier )
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
		FindCompletionSyntaxElement( src_loc.GetLine(), src_loc.GetColumn(), synt_result.program_elements );
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

std::optional<DocumentRange> Document::GetIdentifierRange( const SrcLoc& src_loc ) const
{
	if( last_valid_state_ == std::nullopt || text_changes_since_last_valid_state_ == std::nullopt )
		return std::nullopt;

	const uint32_t line= src_loc.GetLine();
	if( line >= last_valid_state_->line_to_linear_position_index.size() )
		return std::nullopt;

	const TextLinearPosition line_start= last_valid_state_->line_to_linear_position_index[ line ];
	const std::string_view line_text= std::string_view(last_valid_state_->text).substr( line_start );

	const auto utf8_column= Utf32PositionToUtf8Position( line_text, src_loc.GetColumn() );
	if( utf8_column == std::nullopt )
		return std::nullopt;

	const std::optional<TextLinearPosition> utf8_column_end= GetIdentifierEndForPosition( line_text, *utf8_column );
	if( utf8_column_end == std::nullopt )
		return std::nullopt;

	const std::optional<uint32_t> position_mapped= MapOldPositionToNewPosition( *text_changes_since_last_valid_state_, line_start + *utf8_column );
	const std::optional<uint32_t> position_end_mapped= MapOldPositionToNewPosition( *text_changes_since_last_valid_state_, line_start + *utf8_column_end );
	if( position_mapped == std::nullopt || position_end_mapped == std::nullopt )
		return std::nullopt;

	if( *position_end_mapped < *position_mapped )
		return std::nullopt;

	const uint32_t current_line= LinearPositionToSrcLoc( line_to_linear_position_index_, *position_mapped ).GetLine();
	const uint32_t current_end_line= LinearPositionToSrcLoc( line_to_linear_position_index_, *position_end_mapped ).GetLine();
	if( current_line >= line_to_linear_position_index_.size() || current_end_line >= line_to_linear_position_index_.size() )
		return std::nullopt;


	const TextLinearPosition current_line_start_position= line_to_linear_position_index_[current_line];
	const std::optional<uint32_t> character=
		Utf8PositionToUtf16Position(
			std::string_view(text_).substr( current_line_start_position ),
			*position_mapped - current_line_start_position );

	const TextLinearPosition current_end_line_start_position= line_to_linear_position_index_[current_end_line];
	const std::optional<uint32_t> character_end=
		Utf8PositionToUtf16Position(
			std::string_view(text_).substr( current_end_line_start_position ),
			*position_end_mapped - current_end_line_start_position );

	if( character == std::nullopt || character_end == std::nullopt )
		return std::nullopt;

	if( *character_end < character )
		return std::nullopt;

	DocumentRange range;
	range.start= DocumentPosition{ current_line, *character };
	range.end= DocumentPosition{ current_end_line, *character_end };
	return range;
}

void Document::Rebuild()
{
	diagnostics_.clear();

	U_ASSERT( !in_rebuild_call_ );
	in_rebuild_call_= true;
	SourceGraph source_graph= LoadSourceGraph( vfs_, CalculateSourceFileContentsHash, path_, build_options_.prelude );
	in_rebuild_call_= false;

	if( !source_graph.errors.empty() )
	{
		PopulateDiagnostics( source_graph.errors, text_, line_to_linear_position_index_, diagnostics_ );
		return;
	}

	if( source_graph.nodes_storage.empty() )
		return;

	// Take syntax errors only from this document.
	const LexSyntErrors& synt_errors= source_graph.nodes_storage.front().ast.error_messages;
	if( !synt_errors.empty() )
	{
		PopulateDiagnostics( synt_errors, text_, line_to_linear_position_index_, diagnostics_ );
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

	PopulateDiagnostics( code_builder->TakeErrors(), text_, line_to_linear_position_index_, diagnostics_ );

	last_valid_state_= std::nullopt; // Reset previous explicitely to free resources of previos state first.
	last_valid_state_= CompiledState{
		text_,
		line_to_linear_position_index_,
		std::move(source_graph),
		std::move(llvm_context),
		std::move(code_builder) };

	// Reset sequence of changes by last valid state update.
	if( text_changes_since_last_valid_state_ == std::nullopt )
		text_changes_since_last_valid_state_= TextChangesSequence();
	else
		text_changes_since_last_valid_state_->clear();
}

std::optional<TextLinearPosition> Document::GetPositionInLastValidText( const DocumentPosition& position ) const
{
	if( last_valid_state_ == std::nullopt || text_changes_since_last_valid_state_ == std::nullopt )
		return std::nullopt;

	if( position.line >= line_to_linear_position_index_.size() )
		return std::nullopt;
	const uint32_t line_offset= line_to_linear_position_index_[ position.line ];

	const std::optional<uint32_t> column_offset=
		Utf16PositionToUtf8Position( std::string_view(text_).substr( line_offset ), position.character );
	if( column_offset == std::nullopt )
		return std::nullopt;

	const std::optional<uint32_t> last_valid_text_position=
		MapNewPositionToOldPosition( *text_changes_since_last_valid_state_, line_offset + *column_offset );
	if( last_valid_text_position == std::nullopt )
		return std::nullopt;

	if( *last_valid_text_position >= last_valid_state_->text.size() )
		return std::nullopt;

	return last_valid_text_position;
}

std::optional<SrcLoc> Document::GetIdentifierStartSrcLoc( const DocumentPosition& position ) const
{
	if( last_valid_state_ == std::nullopt )
		return std::nullopt;

	const std::optional<TextLinearPosition> linear_position= GetPositionInLastValidText( position );
	if( linear_position == std::nullopt )
	{
		log_ << "Failed to get last valid document position" << std::endl;
		return std::nullopt;
	}

	const uint32_t line= LinearPositionToSrcLoc( last_valid_state_->line_to_linear_position_index, *linear_position ).GetLine();

	return GetSrcLocForIndentifierStartPoisitionInText( last_valid_state_->text, line, *linear_position );
}

} // namespace LangServer

} // namespace U
