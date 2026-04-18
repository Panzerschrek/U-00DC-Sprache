#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace U
{

class IVfs
{
public:
	using Path= std::string;
	using FileContent= std::string;

	struct PathCompletionItem
	{
		Path completed_path;
		Path absolute_path;
		std::string sort_text;
	};

	virtual ~IVfs()= default;

	virtual std::optional<FileContent> LoadFileContent( const Path& full_file_path )= 0;

	// Empty "full_parent_file_path" means root file.
	virtual Path GetFullFilePath( const Path& file_path, const Path& full_parent_file_path )= 0;

	// If it's supported returns completed paths for given prefix (available for importing).
	virtual std::vector<PathCompletionItem> CompletePath(
		const Path& file_path_prefix, const Path& full_parent_file_path )= 0;

	// Returns "false" if it isn't allowed to import file given.
	virtual bool IsImportingFileAllowed( const Path& full_file_path )= 0;

	// Returns "true" if file given is located within one of source directories.
	virtual bool IsFileFromSourcesDirectory( const Path& full_file_path )= 0;
};

using IVfsSharedPtr= std::shared_ptr<IVfs>;

} // namespace U
