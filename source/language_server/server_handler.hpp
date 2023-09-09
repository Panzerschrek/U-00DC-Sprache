#pragma once
#include "logger.hpp"
#include "message_queue.hpp"
#include "transport.hpp"

namespace U
{

namespace LangServer
{

// Process messages and populate message queue until input channel is open.
// Can also close message queue at exit message.
void ProcessMessages( IJsonMessageRead& in, MessageQueue& message_queue, Logger& log );

} // namespace LangServer

} // namespace U
