#pragma once
#include "document.hpp"

namespace U
{

namespace LangServer
{

class DocumentManager
{
public:
	explicit DocumentManager( Logger& log );

	Document* Open( const Uri& uri, std::string text );
	Document* GetDocument( const Uri& uri );
	void Close( const Uri& uri );

	// Returns duration to next document update. This method may be called again after returned time is passed.
	// It is possible to call this method earlier, but it likely will not rebuild anything.
	// May return zero time.
	DocumentClock::duration PerfromDelayedRebuild();

public: // Wrappers for document founctionality. Use them to perform proper ranges mapping.

	std::optional<RangeInDocument> GetDefinitionPoint( const PositionInDocument& position ) const;

	// Returns highligh locations only for given document.
	std::vector<DocumentRange> GetHighlightLocations( const PositionInDocument& position ) const;

	std::vector<RangeInDocument> GetAllOccurrences( const PositionInDocument& position ) const;

	std::vector<Symbol> GetSymbols( const Uri& uri ) const;

	// Non-const this, since internal compiler state may be changed in completion.
	std::vector<CompletionItem> Complete( const PositionInDocument& position );

private:
	RangeInDocument GetDocumentIdentifierRangeOrDummy( const SrcLocInDocument& document_src_loc ) const;
	std::optional<DocumentRange> GetDocumentIdentifierRange( const SrcLocInDocument& document_src_loc ) const;

private:
	struct UnmanagedFile
	{
		IVfs::FileContent content;
		LineToLinearPositionIndex line_to_linear_position_index;
	};

	// VFS wrapper, that allows to read managed documents and also caches unmanaged files reads.
	// Use this to load source graph for documents.
	// It is not so efficient, because full lexical and synax analysis for all imports is performed for a document.
	// But there is no way to do another way, since core compiler structures like SourceGraph and SrcLoc use index-based sources identifying.
	class DocumentManagerVfs final : public IVfs
	{
	public:
		DocumentManagerVfs( DocumentManager& document_manager, Logger& log );

		std::optional<IVfs::FileContent> LoadFileContent( const Path& full_file_path ) override;

		// Empty "full_parent_file_path" means root file.
		IVfs::Path GetFullFilePath( const Path& file_path, const Path& full_parent_file_path ) override;

	private:
		DocumentManager& document_manager_;
		const std::unique_ptr<IVfs> base_vfs_;
	};

private:
	Logger& log_;
	DocumentManagerVfs vfs_;
	const DocumentBuildOptions build_options_;
	// TODO - use unordered map.
	std::map<Uri, Document> documents_;
	std::map<Uri, std::optional<UnmanagedFile>> unmanaged_files_;
};

} // namespace LangServer

} // namespace U
