#include <thread>
#include "logger.hpp"
#include "server_handler.hpp"
#include "server_processor.hpp"
#include "async_server.hpp"

namespace U
{

namespace LangServer
{

void RunAsyncServer( Logger& log, std::string installation_directory )
{
	const auto transport= OpenJSONStdioTransport( log );
	MessageQueue message_queue;

	// Process messages using additional thread.
	std::thread processor_tread(
		[&]
		{
			ServerProcessor processor( log, *transport.second, std::move(installation_directory) );
			log() << "Start async server processor" << std::endl;
			processor.Process( message_queue );
			log() << "End async server processor" << std::endl;
		} );

	// Read messages from input using current thread.
	ProcessMessages( *transport.first, message_queue, log );

	processor_tread.join();
}

} // namespace LangServer

} // namespace U
