#pragma once
#include <map>
#include "../lex_synt_lib/i_vfs.hpp"
#include "build_system_integration.hpp"
#include "uri.hpp"

namespace U
{

namespace LangServer
{

// Class for creation of VFS instances, with possible tweaking for each document.
class VFSManager
{
public:
	VFSManager( Logger& log, std::string installation_directory );

	// Get a VFS instance for given document.
	// The same instance may be returned for different documents.
	// Result instance is thread-safe.
	IVfsSharedPtr GetVFSForDocument( const Uri& uri );

private:
	const WorkspaceDirectoriesGroup* FindDirectoriesGroupForFile( const std::string& file_path ) const;

	// Search for a default build directory for given file and try loading worksapce info file from such a directory.
	void TryLoadDirectoriesGroupForFile( const std::string& file_path );

private:
	using WorkspaceDirectoriesGroupsByBuildDirectory= std::unordered_map<std::string, WorkspaceDirectoriesGroups>;
	using IncludesList= std::vector<std::string>;

private:
	Logger& log_;
	const std::string installation_directory_;

	WorkspaceDirectoriesGroupsByBuildDirectory workspace_directories_groups_;

	// TODO - use unordered_map
	std::map<IncludesList, IVfsSharedPtr> vfs_cache_;
};

} // namespace LangServer

} // namespace U
