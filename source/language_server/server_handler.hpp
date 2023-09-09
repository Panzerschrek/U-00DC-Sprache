#pragma once
#include <string_view>
#include "json.hpp"
#include "logger.hpp"
#include "message_queue.hpp"
#include "transport.hpp"

namespace U
{

namespace LangServer
{

// Class that reads input messages and pushes them into message queue.
class ServerHandler
{
public:
	explicit ServerHandler( Logger& log );

	// Process until input channel is open.
	void Process( IJsonMessageRead& in, MessageQueue& message_queue );

private:
	void HandleMessage( const Json::Value& message, MessageQueue& message_queue );

	// Requests.
	void HandleRequest( RequestId id, std::string_view method, const Json::Value& params, MessageQueue& message_queue );
	RequestParams BuildRequestParams( std::string_view method, const Json::Value& params );

	RequestParams ProcessInitialize( const Json::Value& params );
	RequestParams ProcessShutdown( const Json::Value& params );
	RequestParams ProcessTextDocumentSymbol( const Json::Value& params );
	RequestParams ProcessTextDocumentReferences( const Json::Value& params );
	RequestParams ProcessTextDocumentDefinition( const Json::Value& params );
	RequestParams ProcessTextDocumentCompletion( const Json::Value& params );
	RequestParams ProcessTextDocumentHighlight( const Json::Value& params );
	RequestParams ProcessTextDocumentRename( const Json::Value& params );

	// Notifications.
	void HandleNotification( std::string_view method, const Json::Value& params, MessageQueue& message_queue );
	Notification BuildNorification( std::string_view method, const Json::Value& params );

	Notification ProcessTextDocumentDidOpen( const Json::Value& params );
	Notification ProcessTextDocumentDidClose( const Json::Value& params );
	Notification ProcessTextDocumentDidChange( const Json::Value& params );
	Notification ProcessCancelRequest( const Json::Value& params );

private:
	Logger& log_;
};

} // namespace LangServer

} // namespace U
