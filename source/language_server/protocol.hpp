#pragma once
#include <string>

namespace U
{

namespace LangServer
{

struct RequestMessage
{
	std::string id;
	std::string method;
};

struct NotifyMessage
{
};

} // namespace LangServer

} // namespace U
