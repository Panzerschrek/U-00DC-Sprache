#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/Path.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

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

bool IsPathWithinGivenDirectory( const llvm::StringRef path, const llvm::StringRef directory_path )
{
	auto given_path_it= llvm::sys::path::begin(path);
	const auto given_path_it_end= llvm::sys::path::end(path);

	auto directory_path_it= llvm::sys::path::begin(directory_path);
	const auto directory_path_it_end= llvm::sys::path::end(directory_path);
	while( directory_path_it != directory_path_it_end && given_path_it != given_path_it_end )
	{
		if( *given_path_it != *directory_path_it )
			break;
		++given_path_it;
		++directory_path_it;
	}

	return directory_path_it == directory_path_it_end;
}


} // namespace

VFSManager::VFSManager( Logger& log )
	: log_(log)
	, workspace_directories_groups_( LoadWorkspaceDirectoriesGroups( log_ ) )
{
}

IVfsSharedPtr VFSManager::GetVFSForDocument( const Uri& uri )
{
	// Search this file in workspace directories. If found - use includes specified.
	llvm::ArrayRef<std::string> workspace_includes;
	if( const auto file_path= uri.AsFilePath() )
	{
		for( const WorkspaceDirectoriesGroups& groups : workspace_directories_groups_ )
		{
			for( const WorkspaceDirectoriesGroup& group : groups )
			{
				for( const std::string& directory_path : group.directories )
				{
					if( IsPathWithinGivenDirectory( *file_path, directory_path ) )
					{
						log_() << "Found directory \"" << directory_path << "\" for document \""
							<< *file_path << "\"." << std::endl;
						workspace_includes= group.includes;
						goto end_workspace_includes_search;
					}
				}
			}
		}
	}
	end_workspace_includes_search:

	std::vector<std::string> include_dirs;
	include_dirs= Options::include_dir; // Append includes from options first.

	// TODO add ustlib path.
	// TODO - add build system include directories.

	// Append includes from workspace (if found).
	include_dirs.insert( include_dirs.end(), workspace_includes.begin(), workspace_includes.end() );

	// TODO - cache results.
	return std::make_unique<ThreadSafeVfsWrapper>(
		// Tolerate missing directories in language server. It's not that bad if a directory is missing.
		CreateVfsOverSystemFS( include_dirs, {}, false, true /* tolerate_errors */ ) );
}

} // namespace LangServer

} // namespace U
