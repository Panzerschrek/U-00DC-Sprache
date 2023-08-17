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
	else if( method == "textDocument/didChange" )
		ProcessTextDocumentDidChange( params );
}

Json::Value ServerHandler::ProcessInitialize( const Json::Value& params )
{
	Json::Object result;

	{
		Json::Object capabilities;
		capabilities["textDocumentSync"]= 1; // Full.
		result["capabilities"]= std::move(capabilities);
	}
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


void ServerHandler::ProcessTextDocumentDidChange( const Json::Value& params )
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

	log_ << "Change document " << uri_str->str() << std::endl;

	const auto content_changes= obj->get("contentChanges" );
	if( content_changes == nullptr )
	{
		log_ << "No contentChanges!" << std::endl;
		return;
	}
	const auto content_changes_arr= content_changes->getAsArray();
	if( content_changes_arr == nullptr )
	{
		log_ << "contentChanges is not an array!" << std::endl;
		return;
	}

	if( content_changes_arr->size() == 0 )
	{
		log_ << "Empty changes!" << std::endl;
		return;
	}

	const Json::Value& change= content_changes_arr->back();

	const auto change_obj= change.getAsObject();
	if( change_obj == nullptr )
	{
		log_ << "change is not an object!" << std::endl;
		return;
	}

	const auto change_text= change_obj->get("text");
	if( change_text == nullptr )
	{
		log_ << "No change text!" << std::endl;
		return;
	}

	const auto change_text_str= change_text->getAsString();
	if( !change_text_str )
	{
		log_ << "Change text is not a string!" << std::endl;
		return;
	}

	const auto it= documents_.find( uri_str->str() );
	if( it == documents_.end() )
	{
		log_ << "Can't find document " << uri_str->str() << std::endl;
		return;
	}

	it->second.text= change_text_str->str();
}

} // namespace LangServer

} // namespace U
