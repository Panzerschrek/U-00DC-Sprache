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

class ServerHandler
{
public:
	explicit ServerHandler( std::ostream& log );

public:
	Json::Value HandleRequest( std::string_view method, const Json::Value& params );
	void HandleNotification( std::string_view method, const Json::Value& params );

	// Take first notification in queue.
	// Call this, until it returns result.
	std::optional<ServerNotification> TakeNotification();

private:
	// Requests.
	Json::Value ProcessInitialize( const Json::Value& params );
	Json::Value ProcessTextDocumentSymbol( const Json::Value& params );
	Json::Value ProcessTextDocumentReferences( const Json::Value& params );
	Json::Value ProcessTextDocumentDefinition( const Json::Value& params );
	Json::Value ProcessTextDocumentCompletion( const Json::Value& params );
	Json::Value ProcessTextDocumentHighlight( const Json::Value& params );
	Json::Value ProcessTextDocumentRename( const Json::Value& params );

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
