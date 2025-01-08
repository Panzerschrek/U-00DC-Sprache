#include "../compilers_support_lib/vfs.hpp"
#include "options.hpp"
#include "vfs_manager.hpp"

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

VFSManager::VFSManager( Logger& log )
	: log_(log)
	, base_vfs_(
		std::make_unique<ThreadSafeVfsWrapper>(
			// Tolerate missing directories in language server. It's not that bad if a directory is missing.
			CreateVfsOverSystemFS( Options::include_dir, {}, false, true /* tolerate_errors */ ) ) )
	, workspace_directories_groups_( LoadWorkspaceDirectoriesGroups( log_ ) )
{
}

IVfsSharedPtr VFSManager::GetVFSForDocument( const Uri& uri )
{
	(void)uri;
	// TODO - create individual VFS instances based on workspace directories groups.
	return base_vfs_;
}

} // namespace LangServer

} // namespace U
