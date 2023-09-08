#include <thread>
#include "logger.hpp"
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

	Logger logger(log);

	// Process messages using additional thread.
	std::thread processor_tread(
		[&]
		{
			ServerProcessor processor( logger, *transport.second );
			logger << "Start async server processor" << endl;
			processor.Process( message_queue );
			logger << "End async server processor" << endl;
		} );

	// Read messages from input using current thread.
	ServerHandler handler( logger );
	handler.Process( *transport.first, message_queue );

	processor_tread.join();
}

} // namespace LangServer

} // namespace U
