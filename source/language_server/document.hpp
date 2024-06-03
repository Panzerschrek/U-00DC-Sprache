#pragma once
#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/ThreadPool.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "../compiler0/code_builder_lib/code_builder.hpp"
#include "../lex_synt_lib/source_graph_loader.hpp"
#include "completion.hpp"
#include "document_symbols.hpp"
#include "diagnostics.hpp"
#include "logger.hpp"
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

using DocumentClock= std::chrono::steady_clock;

class Document
{
public:
	Document(
		IVfs::Path path,
		DocumentBuildOptions build_options,
		// Shouldn't be thread-safe. Used only synchroniously to load source graph.
		IVfs& vfs,
		// Must be thread-safe. Used for embedding files.
		IVfsSharedPtr code_builder_vfs,
		Logger& log );

public: // Document text stuff.
	void UpdateText( const DocumentRange& range, std::string_view new_text );
	void SetText( std::string text );

	const std::string& GetCurrentText() const;

	// Returns text of last valid state or raw text if there is no last valid state.
	const std::string& GetTextForCompilation();

public: // State tracking.
	DocumentClock::time_point GetModificationTime() const;
	bool RebuildRequired() const;

	void OnPossibleDependentFileChanged( const IVfs::Path& file_path_normalized );

	bool RebuildIsRunning() const;
	bool RebuildFinished();
	void ResetRebuildFinishedFlag();

	// Wait synchronously until rebuild is finished. Use only in tests.
	void WaitUntilRebuildFinished();

public: // Diagnostics.

	// Diagnostics are generated not only for this document, but also for opened files.
	const DiagnosticsByDocument& GetDiagnostics() const;

public: // Requests.
	std::optional<Uri> GetFileForImportPoint( const DocumentPosition& position );

	std::optional<SrcLocInDocument> GetDefinitionPoint( const DocumentPosition& position );

	// Returns highlights only for this document.
	std::vector<DocumentRange> GetHighlightLocations( const DocumentPosition& position );

	std::vector<SrcLocInDocument> GetAllOccurrences( const DocumentPosition& position );

	Symbols GetSymbols();

	// Non-const this, since internal compiler state may be changed in completion.
	std::vector<CompletionItem> Complete( const DocumentPosition& position );

	std::vector<CodeBuilder::SignatureHelpItem> GetSignatureHelp( const DocumentPosition& position );

	// Assuming given SrcLoc is identifier start, get identifer end for it and construct result range.
	// Also performs mapping from SrcLoc for last valid state to current text state.
	std::optional<DocumentRange> GetIdentifierRange( const SrcLoc& src_loc ) const;

	// Same as abowe, but uses current state of document text.
	std::optional<DocumentRange> GetIdentifierCurrentRange( const SrcLoc& src_loc ) const;

public: // Other stuff.
	// Start rebuild. Rebuilding itself is performed in background thread.
	void StartRebuild( llvm::ThreadPool& thread_pool );

private:
	// This metod checks if compilation future has a new result. If so - it updates compiled state.
	void TryTakeBackgroundStateUpdate();

	// Map position in current document text to position in last valid state text.
	std::optional<TextLinearPosition> GetPositionInLastValidText( const DocumentPosition& position ) const;

	// Return SrcLoc for last valid state, based on input position of current document state.
	std::optional<SrcLoc> GetIdentifierStartSrcLoc( const DocumentPosition& position ) const;

private:
	struct CompiledState
	{
		size_t num_text_changes_at_compilation_task_start= 0; // Used only when updating state.
		std::string text;
		LineToLinearPositionIndex line_to_linear_position_index;
		CodeBuilder::SourceGraphPtr source_graph;
		std::unique_ptr<llvm::LLVMContext> llvm_context;
		std::unique_ptr<CodeBuilder> code_builder; // Still may be modified in const state because of indirection.
	};

	// Use shared_ptr, since llvm::ThreadPool returns only shared_future, that can return only immutable result.
	// So, we can't move-out result and take cheap copy of shared_ptr instead.
	using CompiledStatePtr= std::shared_ptr<const CompiledState>;

	// llvm::ThreadPool uses shared_future.
	using CompiledStateFuture= std::shared_future<CompiledStatePtr>;

private:
	const IVfs::Path path_;
	const DocumentBuildOptions build_options_;
	IVfs& vfs_;
	const IVfsSharedPtr code_builder_vfs_; // Must be thread-safe.
	Logger& log_;

	std::string text_;
	LineToLinearPositionIndex line_to_linear_position_index_; // Index is allways actual for current text.
	std::optional<TextChangesSequence> text_changes_since_compiled_state_;

	DocumentClock::time_point modification_time_;
	bool rebuild_required_= true;

	bool in_rebuild_call_= false;

	// Compiled state (source text + source graph + code builder).
	// It is updated relatively rarely - not for each text change.
	// It is impossible to update it for each change, because not each change produces syntaxically-correct program
	// and because update is too slow.
	CompiledStatePtr compiled_state_;

	CompiledStateFuture compilation_future_;
	bool rebuild_finished_= false;

	DiagnosticsByDocument diagnostics_;
};

} // namespace LangServer

} // namespace U
