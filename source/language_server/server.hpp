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
	void ProcessStep();

private:
	Connection connection_;
	ServerHandler& handler_;
	std::ostream& log_;
};

} // namespace LangServer

} // namespace U
