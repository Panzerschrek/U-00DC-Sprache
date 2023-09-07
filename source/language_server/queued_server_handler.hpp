#pragma once
#include <ostream>
#include <queue>
#include <string_view>
#include "json.hpp"
#include "message_queue.hpp"
#include "transport.hpp"

namespace U
{

namespace LangServer
{

class QueuedServerHandler
{
public:
	explicit QueuedServerHandler( std::ostream& log );

	// Process until input channel is open.
	void Process( IJsonMessageRead& in, MessageQueue& message_queue );

private:
	void HandleMessage( const Json::Value& message, MessageQueue& message_queue );

	// Requests.
	void HandleRequest( RequestId id, std::string_view method, const Json::Value& params, MessageQueue& message_queue );
	std::optional<RequestParams> BuildRequestParams( std::string_view method, const Json::Value& params );

	std::optional<RequestParams> ProcessInitialize( const Json::Value& params );
	std::optional<RequestParams> ProcessTextDocumentSymbol( const Json::Value& params );
	std::optional<RequestParams> ProcessTextDocumentReferences( const Json::Value& params );
	std::optional<RequestParams> ProcessTextDocumentDefinition( const Json::Value& params );
	std::optional<RequestParams> ProcessTextDocumentCompletion( const Json::Value& params );
	std::optional<RequestParams> ProcessTextDocumentHighlight( const Json::Value& params );
	std::optional<RequestParams> ProcessTextDocumentRename( const Json::Value& params );

	// Notifications.
	void HandleNotification( std::string_view method, const Json::Value& params, MessageQueue& message_queue );
	std::optional<Notification> BuildNorification( std::string_view method, const Json::Value& params );

	std::optional<Notification> ProcessTextDocumentDidOpen( const Json::Value& params );
	std::optional<Notification> ProcessTextDocumentDidClose( const Json::Value& params );
	std::optional<Notification> ProcessTextDocumentDidChange( const Json::Value& params );
	std::optional<Notification> ProcessCancelRequest( const Json::Value& params );

private:
	std::ostream& log_;
};

} // namespace LangServer

} // namespace U
