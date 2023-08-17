#pragma once
#include <string_view>
#include "json.hpp"

namespace U
{

namespace LangServer
{

class ServerHandler
{
public:
	Json::Value HandleRequest( std::string_view method, const Json::Value& params );
	void HandleNotification( std::string_view method, const Json::Value& params  );

private:
};

} // namespace LangServer

} // namespace U
