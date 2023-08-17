#pragma once
#include <string_view>
#include "json.hpp"
#include "document.hpp"

namespace U
{

namespace LangServer
{

class ServerHandler
{
public:
	explicit ServerHandler( std::ostream& log );

public:
	Json::Value HandleRequest( std::string_view method, const Json::Value& params );
	void HandleNotification( std::string_view method, const Json::Value& params );

private:
	// Requests.
	Json::Value ProcessInitialize( const Json::Value& params );

	// Notofications.
	void ProcessTextDocumentDidOpen( const Json::Value& params );
	void ProcessTextDocumentDidClose( const Json::Value& params );
	void ProcessTextDocumentDidChange( const Json::Value& params );

private:
	std::ostream& log_;
	std::unordered_map<DocumentURI, Document> documents_;
};

} // namespace LangServer

} // namespace U
