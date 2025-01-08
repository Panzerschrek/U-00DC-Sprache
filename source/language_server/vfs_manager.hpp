#pragma once
#include "../lex_synt_lib/source_graph_loader.hpp"
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
	explicit VFSManager( Logger& log );

	// Get a VFS instance for given document.
	// The same instance may be returned for different documents.
	// Result instance is thread-safe.
	IVfsSharedPtr GetVFSForDocument( const Uri& uri );

private:
	Logger& log_;
	const IVfsSharedPtr base_vfs_; // Thread-safe.

	const std::vector<WorkspaceDirectoriesGroups> workspace_directories_groups_;
};

} // namespace LangServer

} // namespace U
