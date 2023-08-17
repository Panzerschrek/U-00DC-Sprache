#pragma once
#include <optional>
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/JSON.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "protocol.hpp"

namespace U
{

namespace LangServer
{

std::optional<RequestMessage> ParseRequestMessage( const llvm::json::Value& value );

} // namespace LangServer

} // namespace U
