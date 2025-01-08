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

} // namespace LangServer

} // namespace U
