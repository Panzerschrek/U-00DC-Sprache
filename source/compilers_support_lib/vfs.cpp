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

class VfsOverSystemFS final : public IVfs
{
public:
	explicit VfsOverSystemFS( std::vector<fs_path> include_dirs )
		: include_dirs_(std::move(include_dirs))
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
			for( const fs_path& include_dir : include_dirs_ )
			{
				fs_path full_file_path= include_dir;
				fsp::append( full_file_path, file_path_r );
				if( fs::exists( full_file_path ) && fs::is_regular_file( full_file_path ) )
				{
					result_path= full_file_path;
					break;
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
	const std::vector<fs_path> include_dirs_;
};

} // namespace

std::unique_ptr<IVfs> CreateVfsOverSystemFS( const std::vector<std::string>& include_dirs )
{
	std::vector<fs_path> result_include_dirs;
	result_include_dirs.reserve( include_dirs.size() );

	bool all_ok= true;
	for( const std::string& include_dir : include_dirs )
	{
		fs_path dir_path= llvm::StringRef( include_dir );
		fs::make_absolute(dir_path);
		if( !fs::exists(dir_path) )
		{
			std::cerr << "include dir \"" << include_dir << "\" does not exists." << std::endl;
			all_ok= false;
			continue;
		}
		if( !fs::is_directory(dir_path) )
		{
			std::cerr << "\"" << include_dir << "\" is not a directory." << std::endl;
			all_ok= false;
			continue;
		}

		result_include_dirs.push_back( std::move(dir_path) );
	}

	if( !all_ok )
		return nullptr;

	return std::make_unique<VfsOverSystemFS>( std::move(result_include_dirs) );
}

} // namespace U
