#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/TargetParser/Host.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "../compilers_support_lib/prelude.hpp"
#include "../compilers_support_lib/vfs.hpp"
#include "document_position_utils.hpp"
#include "options.hpp"
#include "data_layout_stub.hpp"
#include "document_manager.hpp"

namespace U
{

namespace LangServer
{

namespace
{

// A wrapper which prevents unsynchronized VFS access.
// All methods are thread-safe.
class ThreadSafeVfsWrapper final : public IVfs
{
public:
	explicit ThreadSafeVfsWrapper( std::unique_ptr<IVfs> base )
		: base_( std::move(base) )
	{}

public:
	std::optional<FileContent> LoadFileContent( const Path& full_file_path ) override
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return base_->LoadFileContent( full_file_path );
	}

	Path GetFullFilePath( const Path& file_path, const Path& full_parent_file_path ) override
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return base_->GetFullFilePath( file_path, full_parent_file_path );
	}

	bool IsImportingFileAllowed( const Path& full_file_path ) override
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return base_->IsImportingFileAllowed( full_file_path );
	}

	bool IsFileFromSourcesDirectory( const Path& full_file_path ) override
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return base_->IsFileFromSourcesDirectory( full_file_path );
	}

private:
	std::mutex mutex_;
	const std::unique_ptr<IVfs> base_;
};


DocumentBuildOptions CreateBuildOptions( Logger& log )
{
	llvm::Triple target_triple( llvm::sys::getDefaultTargetTriple() );
	if( !Options::architecture.empty() && Options::architecture != "native" )
		target_triple.setArchName( Options::architecture );
	if( !Options::target_vendor.empty() )
		target_triple.setVendorName( Options::target_vendor );
	if( !Options::target_os.empty() )
		target_triple.setOSName( Options::target_os );
	if( !Options::target_environment.empty() )
		target_triple.setEnvironmentName( Options::target_environment );

	log() << "Using triple " << target_triple.str() << std::endl;

	DocumentBuildOptions build_options
	{
		CreateStubDataLayout( target_triple ),
		target_triple,
		"",
	};

	log() << "Created data layout " << build_options.data_layout.getStringRepresentation() << std::endl;

	const llvm::StringRef features;
	const llvm::StringRef cpu_name= llvm::sys::getHostCPUName();
	const uint32_t compiler_generation= 0;
	build_options.prelude=
		GenerateCompilerPreludeCode(
			build_options.target_triple,
			build_options.data_layout,
			features,
			cpu_name,
			Options::optimization_level,
			Options::generate_debug_info,
			compiler_generation );

	return build_options;
}

std::vector<WorkspaceDirectoriesGroups> LoadWorkspaceDirectoriesGroups( Logger& log )
{
	std::vector<WorkspaceDirectoriesGroups> result;

	for( const auto& build_dir : Options::build_dir )
	{
		auto file_contents_opt= TryLoadWorkspaceInfoFileFromBuildDirectory( log, build_dir );
		if( file_contents_opt != std::nullopt )
		{
			log() << "Found a project description file" << std::endl;
			auto file_parsed= ParseWorkspaceInfoFile( log, *file_contents_opt );
			if( file_parsed != std::nullopt )
			{
				log() << "Successfully parsed a project description file" << std::endl;
				result.push_back( std::move(*file_parsed) );
			}
			else
			{
				log() << "Failed to parse a project description file" << std::endl;
			}
		}
	}

	return result;
}

} // namespace

DocumentManager::DocumentManagerVfs::DocumentManagerVfs(
	Logger& log,
	IVfsSharedPtr base_vfs,
	std::shared_ptr<DocumentsContainer> documents_container )
	: log_( log )
	, base_vfs_( std::move(base_vfs) )
	, documents_container_( std::move(documents_container) )
{
}

std::optional<IVfs::FileContent> DocumentManager::DocumentManagerVfs::LoadFileContent( const Path& full_file_path )
{
	const Uri file_uri= Uri::FromFilePath( full_file_path );

	if( const auto it= documents_container_->documents.find( file_uri ); it != documents_container_->documents.end() )
		return it->second.GetTextForCompilation();
	if( const auto it= documents_container_->unmanaged_files.find( file_uri ); it != documents_container_->unmanaged_files.end() )
	{
		// TODO - detect changes in unmanaged files and reload them if it is necessary.
		if( it->second == std::nullopt )
			return std::nullopt;
		return it->second->content;
	}

	// Load unmanaged file.
	log_() << "Load unmanaged file " << full_file_path << std::endl;

	std::optional<UnmanagedFile>& unmanaged_file= documents_container_->unmanaged_files[file_uri];

	std::optional<IVfs::FileContent> content= base_vfs_->LoadFileContent( full_file_path );

	if( content == std::nullopt )
	{
		log_() << "Failed to load unmanaged file " << full_file_path << std::endl;
		return std::nullopt;
	}

	unmanaged_file= UnmanagedFile{};
	unmanaged_file->content= std::move(*content);
	unmanaged_file->line_to_linear_position_index= BuildLineToLinearPositionIndex( unmanaged_file->content );

	return unmanaged_file->content;
}

IVfs::Path DocumentManager::DocumentManagerVfs::GetFullFilePath( const Path& file_path, const Path& full_parent_file_path )
{
	return base_vfs_->GetFullFilePath( file_path, full_parent_file_path );
}

DocumentManager::DocumentManager( Logger& log )
	: log_(log)
	// Base vfs is used also in background threads to load embedded files. So, make it thread-safe.
	, base_vfs_(
		std::make_unique<ThreadSafeVfsWrapper>(
			// Tolerate missing directories in language server. It's not that bad if a directory is missing.
			CreateVfsOverSystemFS( Options::include_dir, {}, false, true /* tolerate_errors */ ) ) )
	// TODO - use individual VFS for different files.
	// TODO - create different build options for different files.
	, build_options_( CreateBuildOptions(log_) )
	, workspace_directories_groups_( LoadWorkspaceDirectoriesGroups( log ) )
	, documents_container_( std::make_shared<DocumentsContainer>() )
{}

Document* DocumentManager::Open( const Uri& uri, std::string text )
{
	std::optional<std::string> file_path= uri.AsFilePath();
	if( file_path == std::nullopt )
	{
		log_() << "Can't convert URI into file path!" << std::endl;
		return nullptr;
	}

	documents_container_->unmanaged_files.erase( uri ); // Now we manage this file.

	auto base_vfs= base_vfs_; // TODO - create tweaked base vfs for each document.

	// First add docuemnt into a map.
	const auto it_bool_pair=
		documents_container_->documents.insert(
			std::make_pair(
				uri,
				Document(
					std::move( *file_path ),
					build_options_,
					std::make_shared<DocumentManagerVfs>( log_, base_vfs, documents_container_ ),
					// Pass base VFS for CodeBuilder VFS, because we require here thread-safe VFS class instance.
					// This isn't ideal, because no managed files can be loaded in such cases.
					// There is also no caching.
					// But it is good enough, because eliminates complicated synchronization issues.
					base_vfs,
					log_ ) ) );

	Document& document= it_bool_pair.first->second;
	document.SetText( std::move(text) );

	// Do not build document right now - perform delayed rebuild later.

	return &document;
}

Document* DocumentManager::GetDocument( const Uri& uri )
{
	const auto it= documents_container_->documents.find( uri );
	if( it == documents_container_->documents.end() )
		return nullptr;
	return &it->second;
}

void DocumentManager::Close( const Uri& uri )
{
	documents_container_->documents.erase( uri );
	all_diagnostics_.erase( uri );
}

DocumentClock::duration DocumentManager::PerfromDelayedRebuild( llvm::ThreadPool& thread_pool )
{
	// Check for finished async rebuilds of documents.
	for( auto& document_pair : documents_container_->documents )
	{
		const Uri& uri= document_pair.first;
		Document& document= document_pair.second;
		if( document.RebuildFinished() )
		{
			document.ResetRebuildFinishedFlag();

			// Notify other documents about change in order to trigger rebuilding of dependent documents.
			if( const auto file_path= uri.AsFilePath() )
			{
				for( auto& other_document_pair : documents_container_->documents )
					other_document_pair.second.OnPossibleDependentFileChanged( *file_path );
			}

			// Take diagnostics.
			all_diagnostics_[ uri ]= document.GetDiagnostics();
			diagnostics_updated_= true;
		}
	}

	const auto rebuild_delay= std::chrono::milliseconds(1000); // TODO - make it configurable.
	const auto current_time= DocumentClock::now();

	// Start documents rebuilding (if necessary).
	for( auto& document_pair : documents_container_->documents )
	{
		Document& document= document_pair.second;
		if( document.RebuildRequired() )
		{
			const auto modification_time= document.GetModificationTime();
			if( modification_time <= current_time && (current_time - modification_time) >= rebuild_delay )
				document.StartRebuild( thread_pool );

		}
	}

	// Calculate minimal time to next document rebuild.
	// Start with reasonably great value.
	DocumentClock::duration wait_time= std::chrono::duration_cast<DocumentClock::duration>( std::chrono::seconds(5) );
	for( auto& document_pair : documents_container_->documents )
	{
		Document& document= document_pair.second;
		if( document.RebuildIsRunning() )
		{
			// Rebuild is running right now.
			// It is impossible to know exactly how much it will be running, so, return resonable-small time to next check.
			wait_time= std::min( wait_time, std::chrono::duration_cast<DocumentClock::duration>( std::chrono::milliseconds(75) ) );
		}
		else if( document.RebuildRequired() )
		{
			const auto update_time= document.GetModificationTime() + rebuild_delay;
			if( current_time <= update_time )
				wait_time= std::min( wait_time, update_time - current_time );
		}
	}

	return wait_time;
}

bool DocumentManager::DiagnosticsWereUpdated() const
{
	return diagnostics_updated_;
}

void DocumentManager::ResetDiagnosticsUpdatedFlag()
{
	diagnostics_updated_= false;
}

const DiagnosticsBySourceDocument& DocumentManager::GetDiagnostics() const
{
	return all_diagnostics_;
}

std::optional<RangeInDocument> DocumentManager::GetDefinitionPoint( const PositionInDocument& position )
{
	const auto it= documents_container_->documents.find( position.uri );
	if( it == documents_container_->documents.end() )
	{
		log_() << "Can't find document" << position.uri.ToString() << std::endl;
		return std::nullopt;
	}

	Document& document= it->second;

	if( auto uri_opt= document.GetFileForImportPoint( position.position ) )
		return RangeInDocument{ DocumentRange{ { 1, 0 }, { 1, 0 } }, std::move(*uri_opt) };

	if( const auto result_position= document.GetDefinitionPoint( position.position ) )
		return GetDocumentIdentifierRangeOrDummy( *result_position );

	return std::nullopt;
}

std::vector<DocumentRange> DocumentManager::GetHighlightLocations( const PositionInDocument& position )
{
	const auto it= documents_container_->documents.find( position.uri );
	if( it == documents_container_->documents.end() )
	{
		log_() << "Can't find document" << position.uri.ToString() << std::endl;
		return {};
	}

	return it->second.GetHighlightLocations( position.position );
}

std::vector<RangeInDocument> DocumentManager::GetAllOccurrences( const PositionInDocument& position )
{
	const auto it= documents_container_->documents.find( position.uri );
	if( it == documents_container_->documents.end() )
	{
		log_() << "Can't find document" << position.uri.ToString() << std::endl;
		return {};
	}

	const std::vector<SrcLocInDocument> occurences= it->second.GetAllOccurrences( position.position );

	std::vector<RangeInDocument> result;
	result.reserve( occurences.size() );
	for( const SrcLocInDocument& document_src_loc : occurences )
		result.push_back( GetDocumentIdentifierRangeOrDummy( document_src_loc ) );

	return result;
}

Symbols DocumentManager::GetSymbols( const Uri& uri )
{
	const auto it= documents_container_->documents.find( uri );
	if( it == documents_container_->documents.end() )
	{
		log_() << "Can't find document" << uri.ToString() << std::endl;
		return {};
	}

	return it->second.GetSymbols();
}

std::vector<CompletionItem> DocumentManager::Complete( const PositionInDocument& position )
{
	const auto it= documents_container_->documents.find( position.uri );
	if( it == documents_container_->documents.end() )
	{
		log_() << "Can't find document" << position.uri.ToString() << std::endl;
		return {};
	}

	return it->second.Complete( position.position );
}

std::vector<CodeBuilder::SignatureHelpItem> DocumentManager::GetSignatureHelp( const PositionInDocument& position )
{
	const auto it= documents_container_->documents.find( position.uri );
	if( it == documents_container_->documents.end() )
	{
		log_() << "Can't find document" << position.uri.ToString() << std::endl;
		return {};
	}

	return it->second.GetSignatureHelp( position.position );
}

RangeInDocument DocumentManager::GetDocumentIdentifierRangeOrDummy( const SrcLocInDocument& document_src_loc ) const
{
	if( auto range= GetDocumentIdentifierRange( document_src_loc ) )
		return RangeInDocument{ std::move(*range), document_src_loc.uri };

	// Something went wrong. Fill some dummy.
	// Convert UTF-32 column to UTF-16 character. This is wrong, but better than nothing.
	DocumentRange range;
	range.start= DocumentPosition{ document_src_loc.src_loc.GetLine(), document_src_loc.src_loc.GetColumn() };
	range.end= DocumentPosition{ range.start.line, range.start.character + 1 };
	return RangeInDocument{ std::move(range), document_src_loc.uri };
}

std::optional<DocumentRange> DocumentManager::GetDocumentIdentifierRange( const SrcLocInDocument& document_src_loc ) const
{
	if( const auto it= documents_container_->documents.find( document_src_loc.uri ); it != documents_container_->documents.end() )
		return it->second.GetIdentifierRange( document_src_loc.src_loc );

	if( const auto it= documents_container_->unmanaged_files.find( document_src_loc.uri ); it != documents_container_->unmanaged_files.end() )
	{
		if( it->second != std::nullopt )
		{
			const UnmanagedFile& unmanaged_file= *it->second;
			return SrcLocToDocumentIdentifierRange( document_src_loc.src_loc, unmanaged_file.content, unmanaged_file.line_to_linear_position_index );
		}
	}

	return std::nullopt;
}

} // namespace LangServer

} // namespace U
