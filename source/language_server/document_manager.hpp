#pragma once
#include "document.hpp"
#include "vfs_manager.hpp"

namespace U
{

namespace LangServer
{

class DocumentManager
{
public:
	// Logger should live long enough!
	DocumentManager( Logger& log, std::string installation_directory );

	DocumentManager( const DocumentManager& )= delete;
	DocumentManager& operator=( const DocumentManager& )= delete;

	Document* Open( const Uri& uri, std::string text );
	Document* GetDocument( const Uri& uri );
	void Close( const Uri& uri );

	// Returns duration to next document update. This method may be called again after returned time is passed.
	// It is possible to call this method earlier, but it likely will not rebuild anything.
	// May return zero duration.
	DocumentClock::duration PerfromDelayedRebuild( llvm::ThreadPool& thread_pool );

public: // Diagnostics.
	bool DiagnosticsWereUpdated() const;
	void ResetDiagnosticsUpdatedFlag();
	const DiagnosticsBySourceDocument& GetDiagnostics() const;

public: // Wrappers for document founctionality. Use them to perform proper ranges mapping.

	std::optional<RangeInDocument> GetDefinitionPoint( const PositionInDocument& position );

	// Returns highligh locations only for given document.
	std::vector<DocumentRange> GetHighlightLocations( const PositionInDocument& position );

	std::vector<RangeInDocument> GetAllOccurrences( const PositionInDocument& position );

	Symbols GetSymbols( const Uri& uri );

	// Non-const this, since internal compiler state may be changed in completion.
	std::vector<CompletionItem> Complete( const PositionInDocument& position );

	std::vector<CodeBuilder::SignatureHelpItem> GetSignatureHelp( const PositionInDocument& position );

private:
	RangeInDocument GetDocumentIdentifierRangeOrDummy( const SrcLocInDocument& document_src_loc ) const;
	std::optional<DocumentRange> GetDocumentIdentifierRange( const SrcLocInDocument& document_src_loc ) const;

private:
	struct UnmanagedFile
	{
		IVfs::FileContent content;
		LineToLinearPositionIndex line_to_linear_position_index;
	};

	struct DocumentsContainer
	{
		// TODO - use unordered map.
		std::map<Uri, Document> documents;
		std::map<Uri, std::optional<UnmanagedFile>> unmanaged_files;
	};

	// Store documents in a shared_ptr to allow "DocumentManagerVfs" accessing them via copy of this shared_ptr.
	using DocumentsContainerPtr= std::shared_ptr<DocumentsContainer>;

	// VFS wrapper, that allows to read managed documents and also caches unmanaged files reads.
	// Use this to load source graph for documents.
	// It is not so efficient, because full lexical and synax analysis for all imports is performed for a document.
	// But there is no way to do another way, since core compiler structures like SourceGraph and SrcLoc use index-based sources identifying.
	class DocumentManagerVfs final : public IVfs
	{
	public:
		// Logger should live long enough!
		DocumentManagerVfs(
			Logger& log,
			IVfsSharedPtr base_vfs,
			DocumentsContainerPtr documents_container );

		std::optional<IVfs::FileContent> LoadFileContent( const Path& full_file_path ) override;

		IVfs::Path GetFullFilePath( const Path& file_path, const Path& full_parent_file_path ) override;

		virtual bool IsImportingFileAllowed( const Path& full_file_path ) override
		{
			// For now allow importing anything in language server.
			(void) full_file_path;
			return true;
		}

		virtual bool IsFileFromSourcesDirectory( const Path& full_file_path ) override
		{
			// For now consider all files as sources.
			(void) full_file_path;
			return true;
		}

	private:
		Logger& log_;
		const IVfsSharedPtr base_vfs_;
		const DocumentsContainerPtr documents_container_;
	};

private:
	Logger& log_;
	const DocumentBuildOptions build_options_;

	VFSManager vfs_manager_;

	const DocumentsContainerPtr documents_container_;

	DiagnosticsBySourceDocument all_diagnostics_;
	bool diagnostics_updated_= true;
};

} // namespace LangServer

} // namespace U
