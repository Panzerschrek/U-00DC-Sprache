#pragma once
#include <queue>
#include "document_manager.hpp"
#include "json.hpp"
#include "messages.hpp"

namespace U
{


namespace LangServer
{

struct ServerNotification
{
	std::string method;
	Json::Value params;
};

struct ServerResponse
{
	Json::Value result;
	Json::Value error;

	ServerResponse( Json::Object in_result ) noexcept
		: result(std::move(in_result)), error( nullptr )
	{}

	ServerResponse( Json::Array in_result ) noexcept
		: result(std::move(in_result)), error( nullptr )
	{}

	ServerResponse( Json::Value in_result ) noexcept
		: result(std::move(in_result)), error( nullptr )
	{}

	ServerResponse( Json::Value in_result, Json::Value in_error ) noexcept
		: result(std::move(in_result)), error(std::move(in_error))
	{}

	ServerResponse( const ServerResponse& )= default;
	ServerResponse( ServerResponse&& )= default;

	ServerResponse& operator=( const ServerResponse& )= default;
	ServerResponse& operator=( ServerResponse&& )= default;
};

class ServerProcessor
{
public:
	explicit ServerProcessor( std::ostream& log );

	ServerResponse HandleRequest( const Request& request );

	void HandleNotification( const Notification& notification );

private:
	// Requests.
	ServerResponse HandleRequestImpl( const Requests::Initialize& initiailize );
	ServerResponse HandleRequestImpl( const Requests::Symbols& symbols );
	ServerResponse HandleRequestImpl( const Requests::References& references );
	ServerResponse HandleRequestImpl( const Requests::Definition& definition );
	ServerResponse HandleRequestImpl( const Requests::Complete& complete );
	ServerResponse HandleRequestImpl( const Requests::Highlight& highlight );
	ServerResponse HandleRequestImpl( const Requests::Rename& rename );

	// Notifications.
	void HandleNotificationImpl( const Notifications::TextDocumentDidOpen& text_document_did_open );
	void HandleNotificationImpl( const Notifications::TextDocumentDidClose& text_document_did_close );
	void HandleNotificationImpl( const Notifications::TextDocumentDidChange& text_document_did_change );
	void HandleNotificationImpl( const Notifications::CancelRequest& cancel_request );

	// Other stuff.
	void GenerateDiagnosticsNotifications( const DiagnosticsByDocument& diagnostics );

private:
	std::ostream& log_;
	DocumentManager document_manager_;

	std::queue<ServerNotification> notifications_queue_;
};

} // namespace LangServer

} // namespace U
