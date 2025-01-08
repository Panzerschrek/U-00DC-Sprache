#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "build_system_integration.hpp"

namespace U
{

namespace LangServer
{

namespace
{

std::optional<std::string> TryLoadWorkspaceInfoFile( const llvm::Twine file_path )
{
	const llvm::ErrorOr< std::unique_ptr<llvm::MemoryBuffer> > file_mapped= llvm::MemoryBuffer::getFile( file_path );
	if( !file_mapped || *file_mapped == nullptr )
		return std::nullopt;

	return std::string( (*file_mapped)->getBufferStart(), (*file_mapped)->getBufferEnd() );
}

std::optional<std::string> TryLoadWorkspaceInfoFileFromBuildTargetDirectory(
	Logger& logger, const llvm::Twine target_triple_directory )
{
	logger() << "Searching for a workspace info file in target triple directory \"" <<
		target_triple_directory.str() << "\"." << std::endl;

	// Keep this up-to-date with the build system code!
	static constexpr const char* const build_configurations[]
	{
		"release",
		"debug",
	};

	for( const char* const build_configuration : build_configurations )
	{
		logger() << "Searching for a workspace info file in build configuration directory \"" <<
			(target_triple_directory + "/" + build_configuration).str() << "\"." << std::endl;

		auto res= TryLoadWorkspaceInfoFile( target_triple_directory + "/" + build_configuration + "/language_server_workspace_info.json" );
		if( res != std::nullopt )
			return res;
	}
	return std::nullopt;
}

} // namespace

std::optional<std::string> TryLoadWorkspaceInfoFileFromBuildDirectory(
	Logger& logger, const llvm::Twine build_directory )
{
	logger() << "Searching for a workspace info file in root build directory \""
		<< build_directory.str() << "\"." << std::endl;

	// Process case where no target triple directory is present and
	// build confuiguration directories are created directly within build directory instead.
	auto res_in_build_root= TryLoadWorkspaceInfoFileFromBuildTargetDirectory( logger, build_directory );
	if( res_in_build_root != std::nullopt )
		return res_in_build_root;

	// Search in all directories within build root directory.
	// Assume that these are target triple directories.
	std::error_code ec;
	for( llvm::sys::fs::directory_iterator it(build_directory, ec), it_end;
		!ec && it != it_end;
		it = it.increment(ec))
	{
		if( llvm::sys::fs::is_directory( it->path() ) )
		{
			auto res_in_dir= TryLoadWorkspaceInfoFileFromBuildTargetDirectory( logger, it->path() );
			if( res_in_dir != std::nullopt )
				return res_in_dir;
		}
	}

	return std::nullopt;
}

} // namespace LangServer

} // namespace U
