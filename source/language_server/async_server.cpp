#include <thread>
#include "server_handler.hpp"
#include "server_processor.hpp"
#include "async_server.hpp"


namespace U
{

namespace LangServer
{

void RunAsyncServer( std::ostream& log )
{
	const auto transport= OpenJSONStdioTransport();
	MessageQueue message_queue;

	// Process messages using additional thread.
	std::thread processor_tread(
		[&]
		{
			ServerProcessor processor( log, *transport.second );
			processor.Process( message_queue );
		} );

	// Read messages from input using current thread.
	ServerHandler handler( log );
	handler.Process( *transport.first, message_queue );

	processor_tread.join();
}

} // namespace LangServer

} // namespace U
