#pragma once

namespace U
{

namespace LangServer
{

// Log must be thread-safe!
void RunAsyncServer( std::ostream& log );

} // namespace LangServer

} // namespace U
