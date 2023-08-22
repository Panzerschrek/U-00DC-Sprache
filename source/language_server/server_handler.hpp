#pragma once
#include <ostream>
#include <queue>
#include <string_view>
#include "json.hpp"
#include "document.hpp"

namespace U
{

namespace LangServer
{

struct ServerNotification
{
	std::string method;
	Json::Value params;
};

struct ServerResponce
{
	Json::Value result;
	Json::Value error;

	ServerResponce( Json::Object in_result )
		: result(std::move(in_result)), error( nullptr )
	{}

	ServerResponce( Json::Array in_result )
		: result(std::move(in_result)), error( nullptr )
	{}

	ServerResponce( Json::Value in_result )
		: result(std::move(in_result)), error( nullptr )
	{}

	ServerResponce( Json::Value in_result, Json::Value in_error )
		: result(std::move(in_result)), error(std::move(in_error))
	{}
};

class ServerHandler
{
public:
	explicit ServerHandler( std::ostream& log );

public:
	ServerResponce HandleRequest( std::string_view method, const Json::Value& params );
	void HandleNotification( std::string_view method, const Json::Value& params );

	// Take first notification in queue.
	// Call this, until it returns result.
	std::optional<ServerNotification> TakeNotification();

private:
	// Requests.
	ServerResponce ProcessInitialize( const Json::Value& params );
	ServerResponce ProcessTextDocumentSymbol( const Json::Value& params );
	ServerResponce ProcessTextDocumentReferences( const Json::Value& params );
	ServerResponce ProcessTextDocumentDefinition( const Json::Value& params );
	ServerResponce ProcessTextDocumentCompletion( const Json::Value& params );
	ServerResponce ProcessTextDocumentHighlight( const Json::Value& params );
	ServerResponce ProcessTextDocumentRename( const Json::Value& params );

	// Notofications.
	void ProcessTextDocumentDidOpen( const Json::Value& params );
	void ProcessTextDocumentDidClose( const Json::Value& params );
	void ProcessTextDocumentDidChange( const Json::Value& params );

	// Other stuff.
	void GenerateDocumentNotifications( llvm::StringRef uri, const Document& document );

private:
	std::ostream& log_;
	std::unordered_map<DocumentURI, Document> documents_;

	std::queue<ServerNotification> notifications_queue_;
};

} // namespace LangServer

} // namespace U
