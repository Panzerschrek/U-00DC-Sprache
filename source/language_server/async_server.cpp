#include <thread>
#include "logger.hpp"
#include "server_handler.hpp"
#include "server_processor.hpp"
#include "async_server.hpp"

namespace U
{

namespace LangServer
{

void RunAsyncServer( Logger& log )
{
	const auto transport= OpenJSONStdioTransport( log );
	MessageQueue message_queue;

	// Process messages using additional thread.
	std::thread processor_tread(
		[&]
		{
			ServerProcessor processor( log, *transport.second );
			log << "Start async server processor" << std::endl;
			processor.Process( message_queue );
			log << "End async server processor" << std::endl;
		} );

	// Read messages from input using current thread.
	ServerHandler handler( log );
	handler.Process( *transport.first, message_queue );

	processor_tread.join();
}

} // namespace LangServer

} // namespace U
