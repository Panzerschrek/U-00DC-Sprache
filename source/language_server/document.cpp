#include "../code_builder_lib_common/source_file_contents_hash.hpp"
#include "../code_builder_lib_common/string_ref.hpp"
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
	DocumentDiagnostics& out_diagnostics )
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

void PopulateDiagnostics_r(
	const SourceGraph& source_graph,
	const CodeBuilderErrorsContainer& errors,
	const std::string_view program_text,
	const LineToLinearPositionIndex& line_to_linear_position_index,
	DiagnosticsByDocument& out_diagnostics )
{
	for( const CodeBuilderError& error : errors )
	{
		if( error.template_context != nullptr )
			PopulateDiagnostics_r( source_graph, error.template_context->errors, program_text, line_to_linear_position_index, out_diagnostics );

		auto range= GetErrorRange( error.src_loc, program_text, line_to_linear_position_index );
		if( range == std::nullopt )
			continue;

		const uint32_t file_index= error.src_loc.GetFileIndex();
		if( file_index >= source_graph.nodes_storage.size() )
			continue;

		DocumentDiagnostics& document_diagnostics= out_diagnostics[ Uri::FromFilePath( source_graph.nodes_storage[file_index].file_path ) ];

		DocumentDiagnostic diagnostic;
		diagnostic.range= std::move(*range);
		diagnostic.text= error.text;
		diagnostic.code= CodeBuilderErrorCodeToString( error.code );

		document_diagnostics.push_back( std::move(diagnostic) );
	}
}

void PopulateDiagnostics(
	const SourceGraph& source_graph,
	const CodeBuilderErrorsContainer& errors,
	const std::string_view program_text,
	const LineToLinearPositionIndex& line_to_linear_position_index,
	DiagnosticsByDocument& out_diagnostics )
{
	PopulateDiagnostics_r( source_graph, errors, program_text, line_to_linear_position_index, out_diagnostics );
}

// Returns true if found and changed.
bool FindAndChangeLexem( Lexems& lexems, const SrcLoc& src_loc, const Lexem::Type type, const Lexem::Type new_type )
{
	for( Lexem& lexem : lexems )
	{
		if( lexem.src_loc == src_loc && lexem.type == type )
		{
			lexem.type= new_type;
			return true;
		}
	}

	return false;
}

Synt::MacrosByContextMap TakeMacrosFromImports( const SourceGraph& source_graph )
{
	// Merge macroses of imported modules in order to parse document text properly.
	Synt::MacrosByContextMap merged_macroses;

	for( const size_t child_node_index : source_graph.nodes_storage.front().child_nodes_indices )
	{
		for( const auto& context_macro_map_pair : *source_graph.nodes_storage[child_node_index].ast.macros )
		{
			Synt::MacroMap& dst_map= merged_macroses[context_macro_map_pair.first];
			for( const auto& macro_map_pair : context_macro_map_pair.second )
				dst_map[macro_map_pair.first]= macro_map_pair.second;
		}
	}

	return merged_macroses;
}

} // namespace

Document::Document(
	IVfs::Path path,
	DocumentBuildOptions build_options,
	IVfs& vfs,
	IVfsSharedPtr code_builder_vfs, // Must be thread-safe. Used for embedding files.
	Logger& log )
	: path_(std::move(path))
	, build_options_(std::move(build_options))
	, vfs_(vfs)
	, code_builder_vfs_(std::move(code_builder_vfs))
	, log_(log)
{
	SetText("");
}

void Document::SetText( std::string text )
{
	text_= std::move(text);
	text_changes_since_compiled_state_= std::nullopt; // Can't perform changes tracking when text is completely changed.
	BuildLineToLinearPositionIndex( text_, line_to_linear_position_index_ );

	modification_time_= DocumentClock::now();
	rebuild_required_= true;
}

void Document::UpdateText( const DocumentRange& range, const std::string_view new_text )
{
	TryTakeBackgroundStateUpdate();

	const std::optional<TextLinearPosition> linear_position_start= DocumentPositionToLinearPosition( range.start, text_, line_to_linear_position_index_ );
	const std::optional<TextLinearPosition> linear_position_end  = DocumentPositionToLinearPosition( range.end  , text_, line_to_linear_position_index_ );
	if( linear_position_start == std::nullopt || linear_position_end == std::nullopt )
	{
		log_() << "Failed to convert range into offsets!" << std::endl;
		return;
	}
	if( *linear_position_end < *linear_position_start )
	{
		log_() << "Wrong range: end is less than start!" << std::endl;
		return;
	}

	text_.replace( size_t(*linear_position_start), size_t(*linear_position_end - *linear_position_start), new_text );
	BuildLineToLinearPositionIndex( text_, line_to_linear_position_index_ );

	// Save changes sequence.
	if( compiled_state_ != nullptr && text_changes_since_compiled_state_ != std::nullopt )
	{
		TextChange change;
		change.range_start= *linear_position_start;
		change.range_end= *linear_position_end;
		change.new_count= uint32_t(new_text.size());
		text_changes_since_compiled_state_->push_back( std::move(change) );
	}

	modification_time_= DocumentClock::now();
	rebuild_required_= true;
}

const std::string& Document::GetCurrentText() const
{
	return text_;
}

const std::string& Document::GetTextForCompilation()
{
	if( in_rebuild_call_ )
		return text_; // Force return raw text if this method was called indirectly from the "Rebuild" method.

	TryTakeBackgroundStateUpdate();

	// By providing text for compiled state we ensure dependent documents will build properly.
	// Also this allows to perform proper mapping of "SrcLoc" to current state of the text.
	return compiled_state_ == nullptr ? text_ : compiled_state_->text;
}

DocumentClock::time_point Document::GetModificationTime() const
{
	return modification_time_;
}

bool Document::RebuildRequired() const
{
	return rebuild_required_;
}

void Document::OnPossibleDependentFileChanged( const IVfs::Path& file_path_normalized )
{
	TryTakeBackgroundStateUpdate();

	if( file_path_normalized == path_ )
		return; // Do not process changes of itself.

	if( compiled_state_ == nullptr )
		return;

	for( const SourceGraph::Node& node : compiled_state_->source_graph->nodes_storage )
	{
		if( node.file_path == file_path_normalized )
		{
			// If this is one of dependent files - trigger delayed rebuild of this document.
			// TODO - maybe avoid updating maodification time and thus trigger immediate rebuild?
			modification_time_= DocumentClock::now();
			rebuild_required_= true;
			return;
		}
	}
}

bool Document::RebuildIsRunning() const
{
	return compilation_future_.valid();
}

bool Document::RebuildFinished()
{
	TryTakeBackgroundStateUpdate(); // If this method takes background state it may set finished flag to true.

	return rebuild_finished_;
}

void Document::ResetRebuildFinishedFlag()
{
	rebuild_finished_= false;
}

void Document::WaitUntilRebuildFinished()
{
	if( compilation_future_.valid() )
		compilation_future_.wait();
}

const DiagnosticsByDocument& Document::GetDiagnostics() const
{
	return diagnostics_;
}

std::optional<Uri> Document::GetFileForImportPoint( const DocumentPosition& position )
{
	TryTakeBackgroundStateUpdate();

	// Try to extract imported file name for given point.
	// Extract current line for it and check if it starts with "import".
	// If so - try to parse string for this import.
	// If it is successfull - try to get full file path, using VFS.
	// This approach is relatively fast, but doesn't work for cases, when import keyword and import string are on different lines.

	const std::optional<TextLinearPosition> linear_position= GetPositionInLastValidText( position );
	if( linear_position == std::nullopt )
		return std::nullopt;

	const uint32_t line= LinearPositionToLine( compiled_state_->line_to_linear_position_index, *linear_position );
	U_ASSERT( line < compiled_state_->line_to_linear_position_index.size() );

	const TextLinearPosition line_offset= compiled_state_->line_to_linear_position_index[line];
	U_ASSERT( *linear_position >= line_offset );

	const std::string_view line_text= std::string_view( compiled_state_->text ).substr( line_offset );
	const TextLinearPosition offset_in_line= *linear_position - line_offset;

	static const char whitespaces[]= " \t"; // Since we process only single line, use only spaces and tabs.

	llvm::StringRef line_text_trimmed= StringViewToStringRef( line_text ).trim( whitespaces );

	if( line_text.data() + offset_in_line >= line_text_trimmed.data() + line_text_trimmed.size() )
		return std::nullopt; // Click point is in some whitespace.

	const llvm::StringRef import_str= "import";
	if( !line_text_trimmed.startswith( import_str ) )
		return std::nullopt; // Not an import.

	line_text_trimmed= line_text_trimmed.drop_front( import_str.size() );
	line_text_trimmed= line_text_trimmed.ltrim( whitespaces );

	if( line_text.data() + offset_in_line < line_text_trimmed.data() )
		return std::nullopt; // Click point is before string literal.

	if( !line_text_trimmed.startswith( "\"" ) )
		return std::nullopt; // Not a string.

	const std::optional<std::string> str= ParseStringLiteral( StringRefToStringView( line_text_trimmed ) );
	if( str == std::nullopt )
		return std::nullopt;

	const IVfs::Path path= vfs_.GetFullFilePath( *str, path_ );
	if( path.empty() )
		return std::nullopt;

	return Uri::FromFilePath( path );
}

std::optional<SrcLocInDocument> Document::GetDefinitionPoint( const DocumentPosition& position )
{
	TryTakeBackgroundStateUpdate();

	if( compiled_state_ == nullptr )
		return std::nullopt;

	const auto src_loc= GetIdentifierStartSrcLoc( position );
	if( src_loc == std::nullopt )
		return std::nullopt;

	if( const auto result_src_loc= compiled_state_->code_builder->GetDefinition( *src_loc ) )
	{
		SrcLocInDocument location;
		location.src_loc= *result_src_loc;

		const uint32_t file_index= result_src_loc->GetFileIndex();
		if( file_index < compiled_state_->source_graph->nodes_storage.size() )
			location.uri= Uri::FromFilePath( compiled_state_->source_graph->nodes_storage[ file_index ].file_path );
		else
			location.uri= Uri::FromFilePath( path_ ); // TODO - maybe return std::nullopt instead?

		return location;
	}

	return std::nullopt;
}

std::vector<DocumentRange> Document::GetHighlightLocations( const DocumentPosition& position )
{
	TryTakeBackgroundStateUpdate();

	if( compiled_state_ == nullptr )
		return {};

	const auto src_loc= GetIdentifierStartSrcLoc( position );
	if( src_loc == std::nullopt )
		return {};

	const std::vector<SrcLoc> occurrences= compiled_state_->code_builder->GetAllOccurrences( *src_loc );

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

std::vector<SrcLocInDocument> Document::GetAllOccurrences( const DocumentPosition& position )
{
	TryTakeBackgroundStateUpdate();

	if( compiled_state_ == nullptr )
		return {};

	const auto src_loc= GetIdentifierStartSrcLoc( position );
	if( src_loc == std::nullopt )
		return {};

	const std::vector<SrcLoc> occurrences= compiled_state_->code_builder->GetAllOccurrences( *src_loc );

	// TODO - improve this.
	// We need to extract occurences in other opended documents and maybe search for other files.

	std::vector<SrcLocInDocument> result;
	result.reserve( occurrences.size() );

	for( const SrcLoc& result_src_loc : occurrences )
	{
		SrcLocInDocument location;
		location.src_loc= result_src_loc;

		const uint32_t file_index= result_src_loc.GetFileIndex();
		if( file_index < compiled_state_->source_graph->nodes_storage.size() )
			location.uri= Uri::FromFilePath( compiled_state_->source_graph->nodes_storage[ file_index ].file_path );
		else
			location.uri= Uri::FromFilePath( path_ ); // TODO - maybe skip this item instead?

		result.push_back( std::move(location) );
	}

	return result;
}

Symbols Document::GetSymbols()
{
	TryTakeBackgroundStateUpdate();

	if( compiled_state_ != nullptr )
	{
		// Normal case - use last valid state of syntax tree in order to build symbols.
		return
			BuildSymbols(
				compiled_state_->source_graph->nodes_storage.front().ast,
				// Map src_loc in compiled state to range in current state of the document.
				[this]( const SrcLoc& src_loc ) { return GetIdentifierRange(src_loc); } );
	}

	// Backup for cases when document is not compiled yet.
	// Since first document build may be delayed we need to provide symbols just after document was opened.
	const SourceGraph source_graph= LoadSourceGraph( vfs_, CalculateSourceFileContentsHash, path_, build_options_.prelude );

	if( source_graph.nodes_storage.empty() )
		return {};

	return
		BuildSymbols(
			source_graph.nodes_storage.front().ast,
			// Use current state of the document text to get ranges for src_loc.
			[this]( const SrcLoc& src_loc ) { return GetIdentifierCurrentRange(src_loc); } );
}

std::vector<CompletionItem> Document::Complete( const DocumentPosition& position )
{
	TryTakeBackgroundStateUpdate();

	if( compiled_state_ == nullptr || compiled_state_->source_graph->nodes_storage.empty() )
	{
		log_() << "Can't complete - document is not compiled" << std::endl;
		return {};
	}

	// Perform lexical analysis and other manipulations for current version of the document text.

	const uint32_t line= position.line;
	if( line >= line_to_linear_position_index_.size() )
	{
		log_() << "Line is greater than document end" << std::endl;
		return {};
	}
	const std::string_view line_text= std::string_view(text_).substr( line_to_linear_position_index_[ line ] );

	const auto column_utf8= Utf16PositionToUtf8Position( line_text, position.character );
	if( column_utf8 == std::nullopt )
	{
		log_() << "Can't obtain column" << std::endl;
		return {};
	}
	if( *column_utf8 == 0 )
	{
		log_() << "Can't complete at column 0" << std::endl;
		return {};
	}
	const TextLinearPosition column_utf8_minus_one= *column_utf8 - 1u;

	Lexems lexems= LexicalAnalysis( text_ ).lexems;

	SrcLoc src_loc;
	if( line_text[ column_utf8_minus_one ] == '.' )
	{
		const auto column= Utf8PositionToUtf32Position( line_text, column_utf8_minus_one );
		if( column == std::nullopt )
		{
			log_() << "Failed to get utf32 position" << std::endl;
			return {};
		}
		src_loc= SrcLoc( 0, line, *column );

		if( !FindAndChangeLexem( lexems, src_loc, Lexem::Type::Dot, Lexem::Type::CompletionDot ) )
		{
			log_() << "Can't find . lexem" << std::endl;
			return {};
		}
	}
	else if( line_text[ column_utf8_minus_one ] == ':' && column_utf8_minus_one > 0 && line_text[ column_utf8_minus_one - 1 ] == ':' )
	{
		const auto column= Utf8PositionToUtf32Position( line_text, column_utf8_minus_one - 1 ); // -1 to reach start of "::"
		if( column == std::nullopt )
		{
			log_() << "Failed to get utf32 position" << std::endl;
			return {};
		}
		src_loc= SrcLoc( 0, line, *column );

		if( !FindAndChangeLexem( lexems, src_loc, Lexem::Type::Scope, Lexem::Type::CompletionScope ) )
		{
			log_() << "Can't find :: lexem" << std::endl;
			return {};
		}
	}
	else
	{
		const std::optional<TextLinearPosition> idenifier_start_utf8= GetIdentifierStartForPosition( line_text, column_utf8_minus_one );
		if( idenifier_start_utf8 == std::nullopt )
		{
			log_() << "Failed to find identifer start" << std::endl;
			return {};
		}

		const auto column= Utf8PositionToUtf32Position( line_text, *idenifier_start_utf8 );
		if( column == std::nullopt )
		{
			log_() << "Failed to get utf32 position" << std::endl;
			return {};
		}
		src_loc= SrcLoc( 0, line, *column );

		if( !FindAndChangeLexem( lexems, src_loc, Lexem::Type::Identifier, Lexem::Type::CompletionIdentifier ) )
		{
			log_() << "Can't find identifier lexem" << std::endl;
			return {};
		}
	}

	// Perform syntaxis parsing of current text.
	// In most cases it will fail, but it will still parse text until first error.
	// Here we assume, that first error is at least at point of completion or further.

	const auto synt_result=
		Synt::SyntaxAnalysis(
			lexems,
			TakeMacrosFromImports( *compiled_state_->source_graph ),
			std::make_shared<Synt::MacroExpansionContexts>(),
			CalculateSourceFileContentsHash( text_ ) );

	// Lookup global thing, where element with "completion*" lexem is located, together with path to it.
	const SyntaxTreeLookupResultOpt lookup_result=
		FindCompletionSyntaxElement( src_loc.GetLine(), src_loc.GetColumn(), synt_result.program_elements );
	if( lookup_result == std::nullopt )
	{
		log_() << "Failed to find parsed syntax element" << std::endl;
		return {};
	}

	// Use existing compiled program to perform names lookup.
	// Do not try to compile current text, because it is broken and completion will not return what should be returned.
	// Also it is too slow to recompile program for each completion.
	const std::vector<CodeBuilder::CompletionItem> completion_result=
		std::visit(
			[&]( const auto& el ) { return compiled_state_->code_builder->Complete( lookup_result->prefix, *el ); },
			lookup_result->global_item );

	std::vector<CompletionItem> result_transformed;
	result_transformed.reserve( completion_result.size() );
	for( const CodeBuilder::CompletionItem& item : completion_result )
		result_transformed.push_back(
			CompletionItem{ item.name, item.sort_text, item.detail, TranslateCompletionItemKind( item.kind ) } );

	return result_transformed;
}

std::vector<CodeBuilder::SignatureHelpItem> Document::GetSignatureHelp( const DocumentPosition& position )
{
	TryTakeBackgroundStateUpdate();

	if( compiled_state_ == nullptr || compiled_state_->source_graph->nodes_storage.empty() )
	{
		log_() << "Can't get signature help - document is not compiled" << std::endl;
		return {};
	}

	// Perform lexical analysis and other manipulations for current version of the document text.

	const uint32_t line= position.line;
	if( line >= line_to_linear_position_index_.size() )
	{
		log_() << "Line is greater than document end" << std::endl;
		return {};
	}
	const std::string_view line_text= std::string_view(text_).substr( line_to_linear_position_index_[ line ] );

	const auto column_utf8= Utf16PositionToUtf8Position( line_text, position.character );
	if( column_utf8 == std::nullopt )
	{
		log_() << "Can't obtain column" << std::endl;
		return {};
	}
	if( *column_utf8 == 0 )
	{
		log_() << "Can't get signature at column 0" << std::endl;
		return {};
	}
	const TextLinearPosition column_utf8_minus_one= *column_utf8 - 1u;

	const auto column= Utf8PositionToUtf32Position( line_text, column_utf8_minus_one );
	if( column == std::nullopt )
	{
		log_() << "Failed to get utf32 position" << std::endl;
		return {};
	}

	const char symbol= line_text[ column_utf8_minus_one ];
	Lexems lexems= LexicalAnalysis( text_ ).lexems;
	const SrcLoc src_loc( 0, line, *column );
	SrcLoc src_loc_for_search= src_loc;
	if( symbol == '(' )
	{
		if( !FindAndChangeLexem( lexems, src_loc, Lexem::Type::BracketLeft, Lexem::Type::SignatureHelpBracketLeft ) )
		{
			log_() << "Can't find '(' lexem" << std::endl;
			return {};
		}
	}
	else if( symbol == ',' )
	{
		// Re-trigger signature help with ','
		if( !FindAndChangeLexem( lexems, src_loc, Lexem::Type::Comma, Lexem::Type::SignatureHelpComma ) )
		{
			log_() << "Can't find ',' lexem" << std::endl;
			return {};
		}
	}
	else if( symbol == ')' )
	{
		src_loc_for_search= SrcLoc( 0, src_loc.GetLine(), src_loc.GetColumn() + 1 );

		// Re-trigger signature help when ')' is typed.
		// Doing so we can reset signature help if perogrammer finished call operator typing.
		// Also we can re-trigger outer call signature help.
		bool found= false;
		for( auto it= lexems.begin(); it != lexems.end(); ++it )
		{
			if( it->src_loc == src_loc && it->type == Lexem::Type::BracketRight )
			{
				// Insert dummy ',' after ')' in order to trigger signature help for outer call.
				Lexem lexem{ ",", src_loc_for_search, Lexem::Type::SignatureHelpComma };
				lexems.insert( std::next(it), std::move(lexem) );
				found= true;
				break;
			}
		}

		if( !found )
		{
			log_() << "Can't find ')' lexem" << std::endl;
			return {};
		}
	}
	else
	{
		log_() << "Can't get signature help for symbol " << symbol << std::endl;
		return {};
	}

	// Perform syntaxis parsing of current text.
	// In most cases it will fail, but it will still parse text until first error.
	// Here we assume, that first error is at least at point of completion or further.

	const auto synt_result=
		Synt::SyntaxAnalysis(
			lexems,
			TakeMacrosFromImports( *compiled_state_->source_graph ),
			std::make_shared<Synt::MacroExpansionContexts>(),
			CalculateSourceFileContentsHash( text_ ) );

	// Lookup global thing, where element with SignatureHelpBracketLeft lexem is located, together with path to it.
	const SyntaxTreeLookupResultOpt lookup_result=
		FindCompletionSyntaxElement( src_loc_for_search.GetLine(), src_loc_for_search.GetColumn(), synt_result.program_elements );
	if( lookup_result == std::nullopt )
	{
		if( symbol != ')' )
			log_() << "Failed to find parsed syntax element" << std::endl;
		return {};
	}

	// Use existing compiled program to perform signature help.
	// Do not try to compile current text, because it is broken and completion will not return what should be returned.
	// Also it is too slow to recompile program for each signature help.
	return
		std::visit(
			[&]( const auto& el ) { return compiled_state_->code_builder->GetSignatureHelp( lookup_result->prefix, *el ); },
			lookup_result->global_item );
}

std::optional<DocumentRange> Document::GetIdentifierRange( const SrcLoc& src_loc ) const
{
	if( compiled_state_ == nullptr || text_changes_since_compiled_state_ == std::nullopt )
		return std::nullopt;

	const uint32_t line= src_loc.GetLine();
	if( line >= compiled_state_->line_to_linear_position_index.size() )
		return std::nullopt;

	const TextLinearPosition line_start= compiled_state_->line_to_linear_position_index[ line ];
	const std::string_view line_text= std::string_view(compiled_state_->text).substr( line_start );

	const auto utf8_column= Utf32PositionToUtf8Position( line_text, src_loc.GetColumn() );
	if( utf8_column == std::nullopt )
		return std::nullopt;

	const std::optional<TextLinearPosition> utf8_column_end= GetIdentifierEndForPosition( line_text, *utf8_column );
	if( utf8_column_end == std::nullopt )
		return std::nullopt;

	const std::optional<uint32_t> position_mapped= MapOldPositionToNewPosition( *text_changes_since_compiled_state_, line_start + *utf8_column );
	const std::optional<uint32_t> position_end_mapped= MapOldPositionToNewPosition( *text_changes_since_compiled_state_, line_start + *utf8_column_end );
	if( position_mapped == std::nullopt || position_end_mapped == std::nullopt )
		return std::nullopt;

	if( *position_end_mapped < *position_mapped )
		return std::nullopt;

	const uint32_t current_line= LinearPositionToLine( line_to_linear_position_index_, *position_mapped );
	const uint32_t current_end_line= LinearPositionToLine( line_to_linear_position_index_, *position_end_mapped );
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

std::optional<DocumentRange> Document::GetIdentifierCurrentRange( const SrcLoc& src_loc ) const
{
	return SrcLocToDocumentIdentifierRange( src_loc, text_, line_to_linear_position_index_ );
}

void Document::StartRebuild( llvm::ThreadPool& thread_pool )
{
	TryTakeBackgroundStateUpdate();
	if( compilation_future_.valid() )
	{
		// Compilation is already runnung.
		// Do not reset rebuild flag during active compilation.
		// It allows to rebuild document again to apply changes made during current rebuild.
		return;
	}

	// Reset rebuild flag. Even if rebuild fails, there is no reason to try another rebuild, unless document (or its dependencies) changed.
	rebuild_required_= false;

	// Perform lexical and syntax analysis synchronous.
	// It is not possible to do this in background thread, since "LoadSourceGraph" uses VFS, which usage is not thread-safe.
	// And there is a little reason to try to change this, since this step is relatively fast (no more than 100 ms on reasonable large file with a lot of imports).

	diagnostics_.clear();

	U_ASSERT( !in_rebuild_call_ );
	in_rebuild_call_= true;
	SourceGraph source_graph= LoadSourceGraph( vfs_, CalculateSourceFileContentsHash, path_, build_options_.prelude );
	in_rebuild_call_= false;

	if( !source_graph.errors.empty() )
	{
		PopulateDiagnostics( source_graph.errors, text_, line_to_linear_position_index_, diagnostics_[Uri::FromFilePath(path_)] );
		rebuild_finished_= true;
		return;
	}

	if( source_graph.nodes_storage.empty() )
	{
		rebuild_finished_= true;
		return;
	}

	// Take syntax errors only from this document.
	const LexSyntErrors& synt_errors= source_graph.nodes_storage.front().ast.error_messages;
	if( !synt_errors.empty() )
	{
		PopulateDiagnostics( synt_errors, text_, line_to_linear_position_index_,  diagnostics_[Uri::FromFilePath(path_)] );
		rebuild_finished_= true;
		return;
	}

	// Do not compile code if imports are not correct.
	for( const SourceGraph::Node& node : source_graph.nodes_storage )
	{
		if( !node.ast.error_messages.empty() )
		{
			rebuild_finished_= true;
			return;
		}
	}

	// If this is first rebuild - initialize changes tracking, in order to track changes, made during assynchronous compilation.
	if( compiled_state_ == nullptr && text_changes_since_compiled_state_ == std::nullopt )
		text_changes_since_compiled_state_= TextChangesSequence();

	// Save number of changes since compiled state.
	// Later, when taking result of update state we erase all changes until this point, but preserve changes made during update task running.
	const size_t num_text_changes_at_compilation_task_start=
		text_changes_since_compiled_state_== std::nullopt ? 0 : text_changes_since_compiled_state_->size();

	// Start background compilation task, since compilation itself is relatively slow (a couple of seconds for reasonable large file).
	// It is safe to do this, since compilation itself uses no data dependencies.
	// Doing so we allow to execute some methods (completiong, highlighting, etc.) during compilation - without blocking whole language server.

	auto update_func=
		[
			num_text_changes_at_compilation_task_start,
			text= text_,
			code_builder_vfs= code_builder_vfs_, // The only thing which may be mutated in background thread. So, it should be thread-safe.
			line_to_linear_position_index= line_to_linear_position_index_,
			source_graph= std::make_shared<const SourceGraph>( std::move(source_graph) ),
			build_options= build_options_ // Capture copy of build options in case this update func outlives this class instance.
		]
		() mutable // Mutable in order to move captured variables.
		{
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
					build_options.data_layout,
					build_options.target_triple,
					options,
					source_graph,
					std::move(code_builder_vfs) );

			// Reduce a bit memory footprint.
			code_builder->DeleteFunctionsBodies();

			return std::make_shared<const CompiledState>(
				CompiledState{
					num_text_changes_at_compilation_task_start,
					std::move(text),
					std::move(line_to_linear_position_index),
					std::move(source_graph),
					std::move(llvm_context),
					std::move(code_builder) } );
		};

	compilation_future_=
		thread_pool.async(
			// Hack! llvm::ThreadPool uses std::function inside, which requires lambda to be copy-constructible.
			// So, wrap actual lambda into wrapper with shared_ptr - copy shared pointer, not lambda (with captured variables) instead.
			[ lambda_ptr= std::make_shared< decltype(update_func) >( std::move(update_func) ) ]
			{
				return (*lambda_ptr)();
			} );
}

void Document::TryTakeBackgroundStateUpdate()
{
	if( !compilation_future_.valid() )
		return; // Task has not started or we already took the result.

	// Use 0 timeout in order to return (hopefully) immediately.
	// Use wait, since there is no (for now) simple status/is_ready method.
	const std::future_status status= compilation_future_.wait_for( std::chrono::milliseconds(0) );
	if( status != std::future_status::ready )
		return;

	compiled_state_= nullptr;
	compiled_state_= compilation_future_.get();

	// Make future invalid - mark it as empty.
	compilation_future_= CompiledStateFuture();
	rebuild_finished_= true;

	if( compiled_state_ != nullptr )
	{
		if( text_changes_since_compiled_state_ == std::nullopt )
			text_changes_since_compiled_state_= TextChangesSequence();
		else
		{
			// Reset changes made before update task start (for actual for task start moment).
			// Preserve changes made during task running.
			U_ASSERT( text_changes_since_compiled_state_->size() >= compiled_state_->num_text_changes_at_compilation_task_start );
			text_changes_since_compiled_state_->erase(
				text_changes_since_compiled_state_->begin(),
				text_changes_since_compiled_state_->begin() + std::vector<TextChange>::iterator::difference_type( compiled_state_->num_text_changes_at_compilation_task_start ) );
		}

		PopulateDiagnostics(
			*compiled_state_->source_graph,
			compiled_state_->code_builder->TakeErrors(),
			compiled_state_->text,
			compiled_state_->line_to_linear_position_index,
			diagnostics_ );
	}
}

std::optional<TextLinearPosition> Document::GetPositionInLastValidText( const DocumentPosition& position ) const
{
	if( compiled_state_ == nullptr || text_changes_since_compiled_state_ == std::nullopt )
		return std::nullopt;

	if( position.line >= line_to_linear_position_index_.size() )
		return std::nullopt;
	const uint32_t line_offset= line_to_linear_position_index_[ position.line ];

	const std::optional<uint32_t> column_offset=
		Utf16PositionToUtf8Position( std::string_view(text_).substr( line_offset ), position.character );
	if( column_offset == std::nullopt )
		return std::nullopt;

	const std::optional<uint32_t> last_valid_text_position=
		MapNewPositionToOldPosition( *text_changes_since_compiled_state_, line_offset + *column_offset );
	if( last_valid_text_position == std::nullopt )
		return std::nullopt;

	if( *last_valid_text_position >= compiled_state_->text.size() )
		return std::nullopt;

	return last_valid_text_position;
}

std::optional<SrcLoc> Document::GetIdentifierStartSrcLoc( const DocumentPosition& position ) const
{
	if( compiled_state_ == nullptr )
		return std::nullopt;

	const std::optional<TextLinearPosition> linear_position= GetPositionInLastValidText( position );
	if( linear_position == std::nullopt )
		return std::nullopt;

	// Assume, that identifier can't be multiline - start of the identifier is always in the same line as any position within it.

	const uint32_t line= LinearPositionToLine( compiled_state_->line_to_linear_position_index, *linear_position );
	U_ASSERT( line < compiled_state_->line_to_linear_position_index.size() );

	const TextLinearPosition line_offset= compiled_state_->line_to_linear_position_index[line];
	U_ASSERT( *linear_position >= line_offset );

	const std::string_view line_text= std::string_view( compiled_state_->text ).substr( line_offset );

	const std::optional<TextLinearPosition> column_utf8= GetIdentifierStartForPosition( line_text, *linear_position - line_offset );
	if( column_utf8 == std::nullopt )
		return std::nullopt;

	const std::optional<uint32_t> column_utf32= Utf8PositionToUtf32Position( line_text, *column_utf8 );
	if( column_utf32 == std::nullopt )
		return std::nullopt;

	return SrcLoc( 0, line, *column_utf32 );
}

} // namespace LangServer

} // namespace U
