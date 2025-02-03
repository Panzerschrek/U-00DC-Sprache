#pragma once
#include <memory>
#include <optional>
#include <string>

namespace U
{

class IVfs
{
public:
	using Path= std::string;
	using FileContent= std::string;

	virtual ~IVfs()= default;

	virtual std::optional<FileContent> LoadFileContent( const Path& full_file_path )= 0;

	// Empty "full_parent_file_path" means root file.
	virtual Path GetFullFilePath( const Path& file_path, const Path& full_parent_file_path )= 0;

	// Returns "false" if it isn't allowed to import file given.
	virtual bool IsImportingFileAllowed( const Path& full_file_path )= 0;

	// Returns "true" if file given is located within one of source directories.
	virtual bool IsFileFromSourcesDirectory( const Path& full_file_path )= 0;
};

using IVfsSharedPtr= std::shared_ptr<IVfs>;

} // namespace U
