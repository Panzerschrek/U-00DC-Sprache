#include <ostream>
#include "server_handler.hpp"

namespace U
{

namespace LangServer
{

ServerHandler::ServerHandler( std::ostream& log )
	: log_(log)
{
}

Json::Value ServerHandler::HandleRequest( const std::string_view method, const Json::Value& params )
{
	if( method == "initialize" )
		return ProcessInitialize( params );

	Json::Object result;
	return result;
}

void ServerHandler::HandleNotification( const std::string_view method, const Json::Value& params )
{
	if( method == "textDocument/didOpen" )
		ProcessTextDocumentDidOpen( params );
	else if( method == "textDocument/didClose" )
		ProcessTextDocumentDidClose( params );
}

Json::Value ServerHandler::ProcessInitialize( const Json::Value& params )
{
	Json::Object result;
	result["capabilities"]= Json::Object();
	return result;
}

void ServerHandler::ProcessTextDocumentDidOpen( const Json::Value& params )
{
	const auto obj= params.getAsObject();
	if( obj == nullptr )
	{
		log_ << "Not an object!" << std::endl;
		return;
	}

	const auto text_document= obj->get("textDocument" );
	if( text_document == nullptr )
	{
		log_ << "No textDocument!" << std::endl;
		return;
	}
	const auto text_document_obj= text_document->getAsObject();
	if( text_document_obj == nullptr )
	{
		log_ << "Text document is not an object!" << std::endl;
		return;
	}

	const auto uri= text_document_obj->get( "uri" );
	if( uri == nullptr )
	{
		log_ << "No uri!" << std::endl;
		return;
	}
	const auto uri_str= uri->getAsString();
	if( !uri_str )
	{
		log_ << "URI is not a string!" << std::endl;
		return;
	}

	const auto text= text_document_obj->get( "text" );
	if( text == nullptr )
	{
		log_ << "No text!" << std::endl;
		return;
	}
	const auto text_str= uri->getAsString();
	if( !text_str )
	{
		log_ << "Text is not a string!" << std::endl;
		return;
	}

	log_ << "open a document " << uri_str->str() << std::endl;

	Document document;
	document.text= text_str->str();

	documents_[ uri_str->str() ]= std::move(document);
}


void ServerHandler::ProcessTextDocumentDidClose( const Json::Value& params )
{
	const auto obj= params.getAsObject();
	if( obj == nullptr )
	{
		log_ << "Not an object!" << std::endl;
		return;
	}

	const auto text_document= obj->get("textDocument" );
	if( text_document == nullptr )
	{
		log_ << "No textDocument!" << std::endl;
		return;
	}
	const auto text_document_obj= text_document->getAsObject();
	if( text_document_obj == nullptr )
	{
		log_ << "Text document is not an object!" << std::endl;
		return;
	}

	const auto uri= text_document_obj->get( "uri" );
	if( uri == nullptr )
	{
		log_ << "No uri!" << std::endl;
		return;
	}
	const auto uri_str= uri->getAsString();
	if( !uri_str )
	{
		log_ << "URI is not a string!" << std::endl;
		return;
	}

	log_ << "close a document " << uri_str->str() << std::endl;

	documents_.erase( uri_str->str() );
}

} // namespace LangServer

} // namespace U
