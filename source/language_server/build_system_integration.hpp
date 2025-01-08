#pragma once
#include <optional>
#include <string>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Twine.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "logger.hpp"

namespace U
{

namespace LangServer
{

std::optional<std::string> TryLoadWorkspaceInfoFileFromBuildDirectory( Logger& logger, llvm::Twine build_directory );

struct WorkspaceDirectoriesGroup
{
	std::vector<std::string> directories;
	std::vector<std::string> includes;
};

using WorkspaceDirectoriesGroups= std::vector<WorkspaceDirectoriesGroup>;

std::optional<WorkspaceDirectoriesGroups> ParseWorkspaceInfoFile( Logger& log, llvm::StringRef file_contents );

} // namespace LangServer

} // namespace U
