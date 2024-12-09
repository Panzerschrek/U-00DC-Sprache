#include <iostream>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "vfs.hpp"

namespace U
{

namespace
{

namespace fs= llvm::sys::fs;
namespace fsp= llvm::sys::path;

using fs_path= llvm::SmallString<256>;

struct PrefixedIncludeDir
{
	fs_path vfs_path;
	fs_path host_fs_path;
};

class VfsOverSystemFS final : public IVfs
{
public:
	VfsOverSystemFS(
		std::vector<PrefixedIncludeDir> prefixed_include_dirs, bool prevent_imports_outside_given_directories )
		: include_dirs_(std::move(prefixed_include_dirs))
		, prevent_imports_outside_given_directories_(prevent_imports_outside_given_directories)
	{}

public: // IVfs
	virtual std::optional<FileContent> LoadFileContent( const Path& full_file_path ) override
	{
		const llvm::ErrorOr< std::unique_ptr<llvm::MemoryBuffer> > file_mapped=
			llvm::MemoryBuffer::getFile( full_file_path );
		if( !file_mapped || *file_mapped == nullptr )
			return std::nullopt;

		return std::string( (*file_mapped)->getBufferStart(), (*file_mapped)->getBufferEnd() );
	}

	virtual Path GetFullFilePath( const Path& file_path, const Path& full_parent_file_path ) override
	{
		const fs_path file_path_r( file_path );
		fs_path result_path;

		if( full_parent_file_path.empty() )
		{
			result_path= file_path_r;
			fs::make_absolute(result_path);
		}
		else if( !file_path.empty() && file_path[0] == '/' )
		{
			// If file path is absolute, like "/some_lib/some_file.u" search file in include dirs.
			// Return real file system path to first existent file.
			for( const PrefixedIncludeDir& prefixed_include_dir : include_dirs_ )
			{
				auto given_path_it= llvm::sys::path::begin(file_path);
				++given_path_it; // Skip first "/".
				const auto given_path_it_end= llvm::sys::path::end(file_path);

				auto prefix_it= llvm::sys::path::begin(prefixed_include_dir.vfs_path);
				const auto prefix_it_end= llvm::sys::path::end(prefixed_include_dir.vfs_path);

				while( prefix_it != prefix_it_end && given_path_it != given_path_it_end )
				{
					if( *given_path_it != *prefix_it )
						break;
					++given_path_it;
					++prefix_it;
				}

				if( prefix_it == prefix_it_end )
				{
					// Given path is a subdirectory inside "vfs_path".
					fs_path full_file_path= prefixed_include_dir.host_fs_path;
					fsp::append( full_file_path, given_path_it, given_path_it_end );
					if( fs::exists( full_file_path ) && fs::is_regular_file( full_file_path ) )
					{
						result_path= full_file_path;
						break;
					}
				}
			}
		}
		else
		{
			result_path= fsp::parent_path( full_parent_file_path );
			fsp::append( result_path, file_path_r );
		}
		return NormalizePath( result_path ).str().str();
	}

	virtual bool IsImportingFileAllowed( const Path& full_file_path ) override
	{
		if( !prevent_imports_outside_given_directories_ )
			return true; // Importing anything is allowed.

		for( const PrefixedIncludeDir& prefixed_include_dir : include_dirs_ )
		{
			auto given_path_it= llvm::sys::path::begin(full_file_path);
			++given_path_it; // Skip first "/".
			const auto given_path_it_end= llvm::sys::path::end(full_file_path);

			auto allowed_path_it= llvm::sys::path::begin(prefixed_include_dir.host_fs_path);
			const auto allowed_path_it_end= llvm::sys::path::end(prefixed_include_dir.host_fs_path);
			while( allowed_path_it != allowed_path_it_end && given_path_it != given_path_it_end )
			{
				if( *given_path_it != *allowed_path_it )
					break;
				++given_path_it;
				++allowed_path_it;
			}

			if( allowed_path_it == allowed_path_it_end )
				return true; // Given full file path is inside this include directory.
		}

		// Import outside allowed directories.
		return false;
	}

private:
	static fs_path NormalizePath( const fs_path& p )
	{
		fs_path result;
		for( auto it= llvm::sys::path::begin(p), it_end= llvm::sys::path::end(p); it != it_end; ++it)
		{
			if( it->size() == 1 && *it == "." )
				continue;
			if( it->size() == 2 && *it == ".." )
				llvm::sys::path::remove_filename( result );
			else
				llvm::sys::path::append( result, *it );
		}
		llvm::sys::path::native(result);
		return result;
	}

private:
	const std::vector<PrefixedIncludeDir> include_dirs_;
	const bool prevent_imports_outside_given_directories_;
};

} // namespace

std::unique_ptr<IVfs> CreateVfsOverSystemFS(
	const llvm::ArrayRef<std::string> include_dirs,
	const bool prevent_imports_outside_given_directories )
{
	const char* const separator= "::";
	const size_t separator_size= std::strlen( separator );

	std::vector<PrefixedIncludeDir> result_include_dirs;
	result_include_dirs.reserve( include_dirs.size() );

	bool all_ok= true;
	for( const std::string& include_dir : include_dirs )
	{
		std::string fs_dir;
		std::string vfs_mount_path;
		if( const auto separator_pos= include_dir.find( separator ); separator_pos != std::string::npos )
		{
			// Prefixed includ dir.
			if( include_dir.find( separator, separator_pos + separator_size ) != std::string::npos )
			{
				std::cerr << "Duplicated separator " << separator << " in include dir string \"" << include_dir << "\"" << std::endl;
				all_ok= false;
				continue;
			}

			fs_dir= include_dir.substr( 0, separator_pos );
			vfs_mount_path= include_dir.substr( separator_pos + separator_size );

			// Remove trailing/leading slashes.
			while( !vfs_mount_path.empty() && (vfs_mount_path.front() == '/' || vfs_mount_path.front() == '\\' ) )
				vfs_mount_path.erase(vfs_mount_path.begin());
			while( !vfs_mount_path.empty() && (vfs_mount_path.back() == '/' || vfs_mount_path.back() == '\\' ) )
				vfs_mount_path.pop_back();
		}
		else
		{
			// No prefix - mount to VFS root.
			fs_dir= include_dir;
			vfs_mount_path= "";
		}

		fs_path dir_path= llvm::StringRef( fs_dir );
		fs::make_absolute(dir_path);
		if( !fs::exists(dir_path) )
		{
			std::cerr << "include dir \"" << fs_dir << "\" does not exists." << std::endl;
			all_ok= false;
			continue;
		}
		if( !fs::is_directory(dir_path) )
		{
			std::cerr << "\"" << fs_dir << "\" is not a directory." << std::endl;
			all_ok= false;
			continue;
		}

		result_include_dirs.push_back( PrefixedIncludeDir{ fs_path( llvm::StringRef(vfs_mount_path) ), std::move(dir_path) } );
	}

	if( !all_ok )
		return nullptr;

	return
		std::make_unique<VfsOverSystemFS>(
			std::move(result_include_dirs),
			prevent_imports_outside_given_directories );
}

} // namespace U
