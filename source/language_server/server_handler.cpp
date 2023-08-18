#include <ostream>
#include "server_handler.hpp"

namespace U
{

namespace LangServer
{

namespace
{

Json::Value SrcLocToPosition( const SrcLoc& src_loc )
{
	Json::Object position;
	position["line"]= src_loc.GetLine() - 1; // LSP uses 0-based line numbers, Ü use 1-based line numbers.
	position["character"]= src_loc.GetColumn();
	return position;
}

void CreateLexSyntErrorsDiagnostics( const LexSyntErrors& errors, Json::Array& out_diagnostics )
{
	for( const LexSyntError& error : errors)
	{
		Json::Object diagnostic;
		diagnostic["message"]= error.text;
		diagnostic["severity"]= 1; // Means "error"

		{
			Json::Object range;

			range["start"]= SrcLocToPosition( error.src_loc );
			{
				// TODO - extract length of the lexem.
				const SrcLoc src_loc( error.src_loc.GetFileIndex(), error.src_loc.GetLine(), error.src_loc.GetColumn() + 1 );
				range["end"]= SrcLocToPosition( src_loc );
			}

			diagnostic["range"]= std::move(range);
		}

		out_diagnostics.push_back( std::move(diagnostic) );
	}
}

} // namespace

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
		return ProcessTextDocumentDidOpen( params );
	else if( method == "textDocument/didClose" )
		return ProcessTextDocumentDidClose( params );
	else if( method == "textDocument/didChange" )
		return ProcessTextDocumentDidChange( params );
}

std::optional<ServerNotification> ServerHandler::TakeNotification()
{
	if( notifications_queue_.empty() )
		return std::nullopt;

	std::optional<ServerNotification> result( notifications_queue_.front() );
	notifications_queue_.pop();
	return result;
}

Json::Value ServerHandler::ProcessInitialize( const Json::Value& params )
{
	(void)params;

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

	const auto it_bool_pair= documents_.insert( std::make_pair( uri_str->str(), Document( text_str->str() ) ) );

	GenerateDocumentNotifications( *uri, it_bool_pair.first->second );
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

	it->second.SetText( change_text_str->str() );
	GenerateDocumentNotifications( *uri, it->second );
}

void ServerHandler::GenerateDocumentNotifications( const Json::Value& uri, const Document& document )
{
	Json::Object result;
	result["uri"]= uri;

	{
		Json::Array diagnostics;

		CreateLexSyntErrorsDiagnostics( document.GetLexErrors(), diagnostics );
		CreateLexSyntErrorsDiagnostics( document.GetSyntErrors(), diagnostics );

		result["diagnostics"]= std::move(diagnostics);
	}

	ServerNotification notification{ "textDocument/publishDiagnostics", std::move(result) };
	notifications_queue_.push( std::move(notification) );
}

} // namespace LangServer

} // namespace U
