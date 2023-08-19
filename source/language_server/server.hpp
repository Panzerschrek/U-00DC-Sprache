#pragma once
#include "connection.hpp"
#include "server_handler.hpp"

namespace U
{

namespace LangServer
{

class Server
{
public:
	Server( Connection connection, ServerHandler& handler, std::ostream& log );

	void Run();

private:
	// Returns true, if should continue.
	bool ReadAndProcessInputMessage();
	void PushNotifications();

private:
	Connection connection_;
	ServerHandler& handler_;
	std::ostream& log_;
};

} // namespace LangServer

} // namespace U
