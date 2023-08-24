#pragma once
#include <ostream>
#include <queue>
#include <string_view>
#include "json.hpp"
#include "document_manager.hpp"

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

	ServerResponse( Json::Object in_result )
		: result(std::move(in_result)), error( nullptr )
	{}

	ServerResponse( Json::Array in_result )
		: result(std::move(in_result)), error( nullptr )
	{}

	ServerResponse( Json::Value in_result )
		: result(std::move(in_result)), error( nullptr )
	{}

	ServerResponse( Json::Value in_result, Json::Value in_error )
		: result(std::move(in_result)), error(std::move(in_error))
	{}
};

class ServerHandler
{
public:
	explicit ServerHandler( std::ostream& log );

public:
	ServerResponse HandleRequest( std::string_view method, const Json::Value& params );
	void HandleNotification( std::string_view method, const Json::Value& params );

	// Take first notification in queue.
	// Call this, until it returns result.
	std::optional<ServerNotification> TakeNotification();

private:
	// Requests.
	ServerResponse ProcessInitialize( const Json::Value& params );
	ServerResponse ProcessTextDocumentSymbol( const Json::Value& params );
	ServerResponse ProcessTextDocumentReferences( const Json::Value& params );
	ServerResponse ProcessTextDocumentDefinition( const Json::Value& params );
	ServerResponse ProcessTextDocumentCompletion( const Json::Value& params );
	ServerResponse ProcessTextDocumentHighlight( const Json::Value& params );
	ServerResponse ProcessTextDocumentRename( const Json::Value& params );

	// Notifications.
	void ProcessTextDocumentDidOpen( const Json::Value& params );
	void ProcessTextDocumentDidClose( const Json::Value& params );
	void ProcessTextDocumentDidChange( const Json::Value& params );

	// Other stuff.
	void GenerateDocumentNotifications( llvm::StringRef uri, const Document& document );

private:
	std::ostream& log_;
	DocumentManager document_manager_;

	std::queue<ServerNotification> notifications_queue_;
};

} // namespace LangServer

} // namespace U
