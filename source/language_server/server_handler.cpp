#include "server_handler.hpp"

namespace U
{

namespace LangServer
{

Json::Value ServerHandler::HandleRequest( const std::string_view method, const Json::Value& params )
{
	if( method == "initialize" )
	{
		Json::Object result;
		result["capabilities"]= Json::Object();
		return result;
	}
	else
	{
		Json::Object result;
		return result;
	}
}

void ServerHandler::HandleNotification( const std::string_view method, const Json::Value& params )
{
}

} // namespace LangServer

} // namespace U
