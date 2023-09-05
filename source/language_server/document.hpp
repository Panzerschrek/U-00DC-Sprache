#pragma once
#include <ostream>
#include "../compiler0/code_builder_lib/code_builder.hpp"
#include "../lex_synt_lib/source_graph_loader.hpp"
#include "completion.hpp"
#include "document_symbols.hpp"
#include "text_change.hpp"
#include "uri.hpp"

namespace U
{

namespace LangServer
{

struct DocumentBuildOptions
{
	llvm::DataLayout data_layout;
	llvm::Triple target_triple;
	std::string prelude;
};

struct DocumentDiagnostic
{
	DocumentRange range;
	std::string text;
};

class Document
{
public:
	Document( IVfs::Path path, DocumentBuildOptions build_options, IVfs& vfs, std::ostream& log );

	Document( const Document& )= delete;
	Document( Document&& )= default;
	Document& operator=( const Document& )= delete;
	Document& operator=( Document&& )= default;

public: // Document text stuff.
	void UpdateText( const DocumentRange& range, std::string_view new_text );
	void SetText( std::string text );

	const std::string& GetCurrentText() const;

	// Returns text of last valid state or raw text if there is no last valid state.
	const std::string& GetTextForCompilation() const;

public: // Diagnostics.
	llvm::ArrayRef<DocumentDiagnostic> GetDiagnostics() const;

public: // Requests.
	std::optional<SrcLocInDocument> GetDefinitionPoint( const DocumentPosition& position );

	// Returns highlights only for this document.
	std::vector<DocumentRange> GetHighlightLocations( const DocumentPosition& position );

	std::vector<SrcLocInDocument> GetAllOccurrences( const DocumentPosition& position );

	std::vector<Symbol> GetSymbols();

	std::vector<CompletionItem> Complete( const DocumentPosition& position );

	// Assuming given SrcLoc is identifier start, get identifer end for it and construct result range.
	// Also performs mapping from SrcLoc for last valid state to current text state.
	std::optional<DocumentRange> GetIdentifierRange( const SrcLoc& src_loc ) const;

public: // Other stuff.
	void Rebuild();

private:
	// Map position in current document text to position in last valid state text.
	std::optional<TextLinearPosition> GetPositionInLastValidText( const DocumentPosition& position ) const;

	// Return SrcLoc for last valid state, based on input position of current document state.
	std::optional<SrcLoc> GetIdentifierStartSrcLoc( const DocumentPosition& position ) const;

private:
	struct CompiledState
	{
		std::string text;
		LineToLinearPositionIndex line_to_linear_position_index;
		SourceGraph source_graph;
		std::unique_ptr<llvm::LLVMContext> llvm_context;
		std::unique_ptr<CodeBuilder> code_builder;
	};

private:
	const IVfs::Path path_;
	const DocumentBuildOptions build_options_;
	IVfs& vfs_;
	std::ostream& log_;

	std::string text_;
	LineToLinearPositionIndex line_to_linear_position_index_; // Index is allways actual for current text.
	std::optional<TextChangesSequence> text_changes_since_last_valid_state_;

	bool in_rebuild_call_= false;

	// State for last syntaxically-correct program.
	std::optional<CompiledState> last_valid_state_;

	std::vector<DocumentDiagnostic> diagnostics_;
};

} // namespace LangServer

} // namespace U
