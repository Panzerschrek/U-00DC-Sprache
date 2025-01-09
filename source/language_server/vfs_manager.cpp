#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/FileSystem.h>
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

bool PathComponentsAreEqual( const llvm::StringRef l, const llvm::StringRef r )
{
#ifdef WIN32
	// Handle Windows paths with forward/back-slash mess and case insensitivity.

	if( l.size() != r.size() )
		return false;

	for( size_t i= 0; i < l.size(); ++i )
	{
		const char c_l= l[i];
		const char c_r= r[i];
		if( c_l == c_r )
			continue; // Same char - fine.

		if( ( c_l == '/' || c_l == '\\' ) && ( c_r == '/' || c_r == '\\' ) )
			continue; // Both separators - fine.

		if( llvm::toLower( c_l ) == llvm::toLower( c_r ) )
			continue; // ASCII lowercase values are equal - fine. TODO - handle non-ascii letters.

		return false; // Different chars.
	}

	return true;

#else
	// Non-Windows paths - just compare them.
	return l == r;
#endif
}

bool IsPathWithinGivenDirectory( const llvm::StringRef path, const llvm::StringRef directory_path )
{
	auto given_path_it= llvm::sys::path::begin(path);
	const auto given_path_it_end= llvm::sys::path::end(path);

	auto directory_path_it= llvm::sys::path::begin(directory_path);
	const auto directory_path_it_end= llvm::sys::path::end(directory_path);
	while( directory_path_it != directory_path_it_end && given_path_it != given_path_it_end )
	{
		if( !PathComponentsAreEqual( *given_path_it, *directory_path_it ) )
			break;
		++given_path_it;
		++directory_path_it;
	}

	return directory_path_it == directory_path_it_end;
}

} // namespace

VFSManager::VFSManager( Logger& log )
	: log_(log)
{
	// Load workspace info files from build directories provided via command line options.
	for( const auto& build_dir : Options::build_dir )
	{
		const auto file_contents_opt= TryLoadWorkspaceInfoFileFromBuildDirectory( log, build_dir );
		if( file_contents_opt != std::nullopt )
		{
			log() << "Found a project description file" << std::endl;
			auto file_parsed= ParseWorkspaceInfoFile( log, *file_contents_opt );
			if( file_parsed != std::nullopt )
			{
				log() << "Successfully parsed a project description file" << std::endl;
				workspace_directories_groups_.emplace( build_dir, std::move(*file_parsed) );
			}
			else
			{
				log() << "Failed to parse a project description file" << std::endl;
			}
		}
	}
}

IVfsSharedPtr VFSManager::GetVFSForDocument( const Uri& uri )
{
	// Search this file in workspace directories. If found - use includes specified.
	llvm::ArrayRef<std::string> workspace_includes;
	if( const auto file_path= uri.AsFilePath() )
	{
		// First perform search of already loaded workspace directories.
		if( const WorkspaceDirectoriesGroup* const directories_group= FindDirectoriesGroupForFile( *file_path ) )
			workspace_includes= directories_group->includes;
		else
		{
			// If search fails - try to load workspace info for given file and perform search again.
			TryLoadDirectoriesGroupForFile( *file_path );
			if( const WorkspaceDirectoriesGroup* const directories_group= FindDirectoriesGroupForFile( *file_path ) )
				workspace_includes= directories_group->includes;
		}
	}

	IncludesList includes;
	includes= Options::include_dir; // Append includes from options first.

	// TODO add ustlib path.
	// TODO - add build system include directories.

	// Append includes from workspace (if found).
	includes.insert( includes.end(), workspace_includes.begin(), workspace_includes.end() );

	if( const auto it= vfs_cache_.find( includes ); it != vfs_cache_.end() )
		return it->second;

	log_() << "Create new VFS instance for document \"" << uri.ToString() << std::endl;

	auto vfs=
		std::make_shared<ThreadSafeVfsWrapper>(
			// Tolerate missing directories in language server. It's not that bad if a directory is missing.
			CreateVfsOverSystemFS( includes, {}, false, true /* tolerate_errors */ ) );

	vfs_cache_.emplace( std::move(includes), vfs );
	return vfs;
}

const WorkspaceDirectoriesGroup* VFSManager::FindDirectoriesGroupForFile( const std::string& file_path ) const
{
	for( const auto& groups_of_a_build_directory : workspace_directories_groups_ )
	{
		for( const WorkspaceDirectoriesGroup& group : groups_of_a_build_directory.second )
		{
			for( const std::string& directory_path : group.directories )
			{
				if( IsPathWithinGivenDirectory( file_path, directory_path ) )
				{
					log_() << "Found directory \"" << directory_path << "\" for document \""
						<< file_path << "\"." << std::endl;
					return &group;
				}
			}
		}
	}

	return nullptr;
}

void VFSManager::TryLoadDirectoriesGroupForFile( const std::string& file_path )
{
	// Iterate over parent directories and search for default build directory with name "build".

	auto it= llvm::sys::path::begin(file_path), it_end= llvm::sys::path::end(file_path);
	if( it == it_end )
		return;

	std::string directory_path;
	for( auto it= llvm::sys::path::begin(file_path), it_end= llvm::sys::path::end(file_path); std::next(it) != it_end; ++it)
	{
		directory_path+= *it;
		if( directory_path.empty() || directory_path.back() != '/' )
			directory_path+= "/";

		const std::string default_build_directory_path= directory_path + "build";

		if( !llvm::sys::fs::is_directory(default_build_directory_path) )
			continue; // Doesn't exist or not a directory.

		const auto file_contents_opt= TryLoadWorkspaceInfoFileFromBuildDirectory( log_, default_build_directory_path );
		if( file_contents_opt != std::nullopt )
		{
			log_() << "Found a project workspace info in build directory \"" << default_build_directory_path << "\"." << std::endl;
			auto file_parsed= ParseWorkspaceInfoFile( log_, *file_contents_opt );
			if( file_parsed != std::nullopt )
			{
				log_() << "Successfully parsed a project description file." << std::endl;
				workspace_directories_groups_.emplace( default_build_directory_path, std::move(*file_parsed) );
				return;
			}
			else
			{
				log_() << "Failed to parse a project description file." << std::endl;
			}
		}
	}
}

} // namespace LangServer

} // namespace U
