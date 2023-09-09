#include "../code_builder_lib_common/string_ref.hpp"
#include "server_handler.hpp"

namespace U
{

namespace LangServer
{

namespace
{

std::optional<RequestId> ParseRequestId( const Json::Value& value )
{
	if( const auto s= value.getAsString() )
		return s->str();

	if( const auto i= value.getAsInteger() )
		return *i;

	return std::nullopt;
}

std::optional<DocumentPosition> JsonToDocumentPosition( const Json::Value& value )
{
	if( const auto obj= value.getAsObject() )
	{
		const auto line= obj->getInteger( "line" );
		const auto column= obj->getInteger( "character" );
		if( line != llvm::None && column != llvm::None )
			return DocumentPosition{ uint32_t(*line) + 1, uint32_t(*column) };
	}
	return std::nullopt;
}

std::optional<DocumentRange> JsonToDocumentRange( const Json::Value& value )
{
	if( const auto obj= value.getAsObject() )
	{
		const auto start= obj->get( "start" );
		const auto end= obj->get( "end" );
		if( start != nullptr && end != nullptr )
		{
			const auto start_parsed= JsonToDocumentPosition( *start );
			const auto end_parsed= JsonToDocumentPosition( *end );
			if( start_parsed != std::nullopt && end_parsed != std::nullopt )
				return DocumentRange{ *start_parsed, *end_parsed };
		}
	}

	return std::nullopt;
}

std::optional<PositionInDocument> JsonToPositionInDocument( const Json::Object& value )
{
	const auto text_document= value.getObject( "textDocument" );
	if( text_document == nullptr )
		return std::nullopt;

	const auto uri= text_document->getString( "uri" );
	if( uri == llvm::None )
		return std::nullopt;

	std::optional<Uri> uri_parsed= Uri::Parse( *uri );
	if( uri_parsed == std::nullopt )
		return std::nullopt;

	const auto position_json= value.get( "position" );
	if( position_json == nullptr )
		return std::nullopt;

	std::optional<DocumentPosition> position= JsonToDocumentPosition( *position_json );
	if( position == std::nullopt )
		return std::nullopt;

	return PositionInDocument{ std::move(*position), std::move(*uri_parsed) };
}

} // namespace

ServerHandler::ServerHandler( Logger& log )
	: log_(log)
{
}

void ServerHandler::Process( IJsonMessageRead& in, MessageQueue& message_queue )
{
	while(!message_queue.IsClosed())
	{
		const std::optional<Json::Value> message= in.Read();
		if( message == std::nullopt )
			return;

		HandleMessage( *message, message_queue );
	}
}

void ServerHandler::HandleMessage( const Json::Value& message, MessageQueue& message_queue )
{
	const Json::Object* const obj= message.getAsObject();
	if( obj == nullptr )
	{
		log_() << "JSON is not an object" << std::endl;
		return;
	}

	std::string method;
	if( const auto method_json= obj->getString( "method" ) )
		method= method_json->str();

	Json::Value params= Json::Object();
	if( const auto params_json= obj->get( "params" ) )
		params= *params_json;

	if( const Json::Value* const id_json= obj->get( "id" ) )
	{
		auto id= ParseRequestId( *id_json );
		if( id == std::nullopt )
		{
			log_() << "Failed to pase id!" << std::endl;
			return;
		}
		HandleRequest( std::move(*id), method, params, message_queue );
	}
	else
		HandleNotification( method, params, message_queue );
}

void ServerHandler::HandleRequest( RequestId id, const std::string_view method, const Json::Value& params, MessageQueue& message_queue )
{
	message_queue.Push( Request{ std::move(id), BuildRequestParams( method, params ) } );
}

RequestParams ServerHandler::BuildRequestParams( const std::string_view method, const Json::Value& params )
{
	if( method == "initialize" )
		return ProcessInitialize( params );
	if( method == "shutdown" )
		return ProcessShutdown( params );
	if( method == "textDocument/documentSymbol" )
		return ProcessTextDocumentSymbol( params );
	if( method == "textDocument/references" )
		return ProcessTextDocumentReferences( params );
	if( method == "textDocument/definition" )
		return ProcessTextDocumentDefinition( params );
	if( method == "textDocument/completion" )
		return ProcessTextDocumentCompletion( params );
	if( method == "textDocument/documentHighlight" )
		return ProcessTextDocumentHighlight( params );
	if( method == "textDocument/rename" )
		return ProcessTextDocumentRename( params );

	return MethodNotFound{ std::string(method) };
}

RequestParams ServerHandler::ProcessInitialize( const Json::Value& params )
{
	(void)params;
	return Requests::Initialize{};
}

RequestParams ServerHandler::ProcessShutdown( const Json::Value& params )
{
	(void)params;
	return Requests::Shutdown{};
}

RequestParams ServerHandler::ProcessTextDocumentSymbol( const Json::Value& params )
{
	const auto obj= params.getAsObject();
	if( obj == nullptr )
		return InvalidParams{ "Not an object!" };

	const auto text_document= obj->getObject( "textDocument" );
	if( text_document == nullptr )
		return InvalidParams{ "No textDocument!" };

	const auto uri= text_document->getString( "uri" );
	if( uri == llvm::None )
		return InvalidParams{ "No uri!" };

	auto uri_parsed= Uri::Parse( *uri );
	if( uri_parsed == std::nullopt )
		return InvalidParams{ "Invalid uri!" };

	return Requests::Symbols{ std::move(*uri_parsed) };
}

RequestParams ServerHandler::ProcessTextDocumentReferences( const Json::Value& params )
{
	const auto obj= params.getAsObject();
	if( obj == nullptr )
		return InvalidParams{ "Not an object!" };

	auto position_in_document= JsonToPositionInDocument( *obj );
	if( position_in_document == std::nullopt )
		return InvalidParams{ "Failed to get position in document!" };

	return Requests::References{ std::move( *position_in_document ) };
}

RequestParams ServerHandler::ProcessTextDocumentDefinition( const Json::Value& params )
{
	const auto obj= params.getAsObject();
	if( obj == nullptr )
		return InvalidParams{ "Not an object!" };

	auto position_in_document= JsonToPositionInDocument( *obj );
	if( position_in_document == std::nullopt )
		return InvalidParams{ "Failed to get position in document!" };

	return Requests::Definition{ std::move( *position_in_document ) };
}

RequestParams ServerHandler::ProcessTextDocumentCompletion( const Json::Value& params )
{
	const auto obj= params.getAsObject();
	if( obj == nullptr )
		return InvalidParams{ "Not an object!" };

	auto position_in_document= JsonToPositionInDocument( *obj );
	if( position_in_document == std::nullopt )
		return InvalidParams{ "Failed to get position in document!" };

	return Requests::Complete{ std::move( *position_in_document ) };
}

RequestParams ServerHandler::ProcessTextDocumentHighlight( const Json::Value& params )
{
	const auto obj= params.getAsObject();
	if( obj == nullptr )
		return InvalidParams{ "Not an object!" };

	auto position_in_document= JsonToPositionInDocument( *obj );
	if( position_in_document == std::nullopt )
		return InvalidParams{ "Failed to get position in document!" };

	return Requests::Highlight{ std::move( *position_in_document ) };
}

RequestParams ServerHandler::ProcessTextDocumentRename( const Json::Value& params )
{
	const auto obj= params.getAsObject();
	if( obj == nullptr )
		return InvalidParams{ "Not an object!" };

	const auto new_name= obj->getString( "newName" );
	if( new_name == llvm::None )
		return InvalidParams{ "No newName" };

	auto position_in_document= JsonToPositionInDocument( *obj );
	if( position_in_document == std::nullopt )
		return InvalidParams{ "Failed to get position in document!" };

	return Requests::Rename{ std::move( *position_in_document ), new_name->str() };
}

void ServerHandler::HandleNotification( const std::string_view method, const Json::Value& params, MessageQueue& message_queue )
{
	if( method == "exit" )
	{
		// Close message queue - this causes all users of message queue to end processing.
		message_queue.Close();
		return;
	}

	message_queue.Push( BuildNorification( method, params ) );
}

Notification ServerHandler::BuildNorification( const std::string_view method, const Json::Value& params )
{
	if( method == "textDocument/didOpen" )
		return ProcessTextDocumentDidOpen( params );
	if( method == "textDocument/didClose" )
		return ProcessTextDocumentDidClose( params );
	if( method == "textDocument/didChange" )
		return ProcessTextDocumentDidChange( params );
	if( method == "$/cancelRequest" )
		return ProcessCancelRequest( params );

	return MethodNotFound{ std::string(method) };
}

Notification ServerHandler::ProcessTextDocumentDidOpen( const Json::Value& params )
{
	const auto obj= params.getAsObject();
	if( obj == nullptr )
		return InvalidParams{ "Not an object!" };

	const auto text_document= obj->getObject( "textDocument" );
	if( text_document == nullptr )
		return InvalidParams{ "No textDocument!" };

	const auto uri= text_document->getString( "uri" );
	if( uri == llvm::None )
		return InvalidParams{ "No uri!" };

	const auto uri_parsed= Uri::Parse( *uri );
	if( uri_parsed == std::nullopt )
		return InvalidParams{ "Invalid uri!" };

	const auto text= text_document->getString( "text" );
	if( text == llvm::None )
		return InvalidParams{ "No text!" };

	return Notifications::TextDocumentDidOpen{ std::move( *uri_parsed ), text->str() };
}

Notification ServerHandler::ProcessTextDocumentDidClose( const Json::Value& params )
{
	const auto obj= params.getAsObject();
	if( obj == nullptr )
		return InvalidParams{ "Not an object!" };

	const auto text_document= obj->getObject( "textDocument" );
	if( text_document == nullptr )
		return InvalidParams{ "No textDocument!" };

	const auto uri= text_document->getString( "uri" );
	if( uri == llvm::None )
		return InvalidParams{ "No uri!" };

	const auto uri_parsed= Uri::Parse( *uri );
	if( uri_parsed == std::nullopt )
		return InvalidParams{ "Invalid uri!" };

	return Notifications::TextDocumentDidClose{ std::move( *uri_parsed ) };
}

Notification ServerHandler::ProcessTextDocumentDidChange( const Json::Value& params )
{
	const auto obj= params.getAsObject();
	if( obj == nullptr )
		return InvalidParams{ "Not an object!" };

	const auto text_document= obj->getObject("textDocument" );
	if( text_document == nullptr )
		return InvalidParams{ "No textDocument!" };

	const auto uri= text_document->getString( "uri" );
	if( uri == llvm::None )
		return InvalidParams{ "No uri!" };

	const auto uri_parsed= Uri::Parse( *uri );
	if( uri_parsed == std::nullopt )
		return InvalidParams{ "Invalid uri!" };

	const auto content_changes= obj->getArray("contentChanges" );
	if( content_changes == nullptr )
		return InvalidParams{ "No contentChanges!" };

	// TODO - check also given document version number.

	// TODO - somehow invalidate document in case of synchronization errors.

	Notifications::TextDocumentDidChange change_notification;
	change_notification.uri= std::move( *uri_parsed );

	for( const Json::Value& change : *content_changes )
	{
		const auto change_obj= change.getAsObject();
		if( change_obj == nullptr )
			return InvalidParams{ "Change is not an object!" };


		const auto change_text= change_obj->get("text");
		if( change_text == nullptr )
			return InvalidParams{ "No change text!" };

		const auto change_text_str= change_text->getAsString();
		if( !change_text_str )
			return InvalidParams{ "Change text is not a string!" };

		if( const auto range_json= change_obj->get( "range" ) )
		{
			const auto range= JsonToDocumentRange( *range_json );
			if( range == std::nullopt )
				return InvalidParams{ "Failed to parse range!" };

			change_notification.changes.push_back( Notifications::TextDocumentIncrementalChange{ std::move(*range), change_text_str->str() } );
		}
		else
			change_notification.changes.push_back( change_text_str->str() );
	}

	return std::move(change_notification);
}

Notification ServerHandler::ProcessCancelRequest( const Json::Value& params )
{
	const auto obj= params.getAsObject();
	if( obj == nullptr )
		return InvalidParams{ "Not an object!" };

	const auto id= obj->get("id");
	if( id == nullptr )
		return InvalidParams{ "No request id!" };

	auto id_parsed= ParseRequestId(*id);
	if( id_parsed == std::nullopt )
		return InvalidParams{ "Failed to parse request id!" };

	return Notifications::CancelRequest{ std::move(*id_parsed) };
}

} // namespace LangServer

} // namespace U
