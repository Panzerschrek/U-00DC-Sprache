#pragma once
#include <optional>
#include "json.hpp"

namespace U
{

namespace LangServer
{

class IJsonMessageRead
{
public:
	virtual ~IJsonMessageRead()= default;

	// Extract new message. This call is blocking. Returns nullopt if channel is closed or in case of error.
	virtual std::optional<Json::Value> Read()= 0;
};

class IJsonMessageWrite
{
public:
	virtual ~IJsonMessageWrite()= default;

	virtual void Write( const Json::Value& value )= 0;
};

using IJsonMessageReadPtr= std::unique_ptr<IJsonMessageRead>;
using IJsonMessageWritePtr= std::unique_ptr<IJsonMessageWrite>;

// Returns a pair of read/write channels.
// It is safe to use result channels in separate threads, but not to share each channel accross threads.
std::pair<IJsonMessageReadPtr, IJsonMessageWritePtr> OpenJSONStdioTransport();

} // namespace LangServer

} // namespace U
