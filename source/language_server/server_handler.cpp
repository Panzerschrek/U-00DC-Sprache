#include "server_handler.hpp"

namespace U
{

namespace LangServer
{

namespace
{

enum ErrorCode : int32_t
{
	ParseError = -32700,
	InvalidRequest = -32600,
	MethodNotFound = -32601,
	InvalidParams = -32602,
	InternalError = -32603,
	RequestFailed = -32803,
};

Json::Value SrcLocToPosition( const SrcLoc& src_loc )
{
	Json::Object position;
	position["line"]= src_loc.GetLine() - 1; // LSP uses 0-based line numbers, Ü use 1-based line numbers.
	position["character"]= src_loc.GetColumn();
	return position;
}

Json::Value DocumentPositionToJson( const DocumentPosition& position )
{
	Json::Object out_position;
	out_position["line"]= position.line - 1; // LSP uses 0-based line numbers, Ü use 1-based line numbers.
	out_position["character"]= position.column;
	return out_position;
}

Json::Value DocumentRangeToJson( const DocumentRange& range )
{
	Json::Object out_range;
	out_range["start"]= DocumentPositionToJson( range.start );
	out_range["end"]= DocumentPositionToJson( range.end );
	return out_range;
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

void CreateCodeBuilderErrorsDiagnostics( const CodeBuilderErrorsContainer& errors, Json::Array& out_diagnostics )
{
	for( const CodeBuilderError& error : errors )
	{
		Json::Object diagnostic;
		diagnostic["message"]= error.text;
		diagnostic["severity"]= 1; // Means "error"
		diagnostic["code"]= std::string( CodeBuilderErrorCodeToString( error.code ) );

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

		if( error.template_context != nullptr )
			CreateCodeBuilderErrorsDiagnostics( error.template_context->errors, out_diagnostics );
	}
}

} // namespace

ServerHandler::ServerHandler( std::ostream& log )
	: log_(log)
{
}

ServerResponse ServerHandler::HandleRequest( const std::string_view method, const Json::Value& params )
{
	if( method == "initialize" )
		return ProcessInitialize( params );
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

ServerResponse ServerHandler::ProcessInitialize( const Json::Value& params )
{
	(void)params;

	Json::Object result;

	{
		Json::Object capabilities;
		capabilities["textDocumentSync"]= 1; // Full.
		capabilities["declarationProvider"]= true;
		capabilities["definitionProvider"]= true;
		capabilities["referencesProvider"]= true;
		capabilities["documentHighlightProvider"]= true;
		capabilities["documentSymbolProvider"]= true;
		capabilities["renameProvider"]= true;

		{
			Json::Object link_provider;
			link_provider["resolveProvider"]= true;
			capabilities["documentLinkProvider"]= std::move(link_provider);
		}
		result["capabilities"]= std::move(capabilities);
	}
	return result;
}

ServerResponse ServerHandler::ProcessTextDocumentSymbol( const Json::Value& params )
{
	Json::Array result;

	const auto obj= params.getAsObject();
	if( obj == nullptr )
	{
		log_ << "Not an object!" << std::endl;
		return result;
	}

	const auto text_document= obj->getObject( "textDocument" );
	if( text_document == nullptr )
	{
		log_ << "No textDocument!" << std::endl;
		return result;
	}

	const auto uri= text_document->getString( "uri" );
	if( uri == llvm::None )
	{
		log_ << "No uri!" << std::endl;
		return result;
	}

	const auto it= documents_.find( uri->str() );
	if( it == documents_.end() )
	{
		log_ << "Can't find document " << uri->str() << std::endl;
		return result;
	}

	// Fill dummy.
	// TODO - provide real symbols.
	{
		Json::Object symbol;
		symbol["name"]= "dummy_symbol";
		symbol["range"]= DocumentRangeToJson( { {3, 14}, {4, 25} } );
		result.push_back( std::move(symbol) );
	}

	return result;
}

ServerResponse ServerHandler::ProcessTextDocumentReferences( const Json::Value& params )
{
	Json::Array result;

	const auto obj= params.getAsObject();
	if( obj == nullptr )
	{
		log_ << "Not an object!" << std::endl;
		return result;
	}

	const auto text_document= obj->getObject( "textDocument" );
	if( text_document == nullptr )
	{
		log_ << "No textDocument!" << std::endl;
		return result;
	}

	const auto uri= text_document->getString( "uri" );
	if( uri == llvm::None )
	{
		log_ << "No uri!" << std::endl;
		return result;
	}

	const auto position= obj->getObject( "position" );
	if( position == nullptr )
	{
		log_ << "No position!" << std::endl;
		return result;
	}

	const auto line= position->getInteger( "line" );
	const auto character= position->getInteger( "character" );
	if( line == llvm::None || character == llvm::None )
	{
		log_ << "Invalid position!" << std::endl;
		return result;
	}

	const auto it= documents_.find( uri->str() );
	if( it == documents_.end() )
	{
		log_ << "Can't find document " << uri->str() << std::endl;
		return result;
	}

	for( const DocumentRange& range : it->second.GetAllOccurrences( SrcLoc( 0, uint32_t(*line) + 1, uint32_t(*character) ) ) )
	{
		Json::Object location;
		location["range"]= DocumentRangeToJson( range );
		// TODO - set proper URI for occurences in other files.
		location["uri"]= uri->str();

		result.push_back( std::move(location) );
	}

	return result;
}

ServerResponse ServerHandler::ProcessTextDocumentDefinition( const Json::Value& params )
{
	Json::Object result;

	const auto obj= params.getAsObject();
	if( obj == nullptr )
	{
		log_ << "Not an object!" << std::endl;
		return result;
	}

	const auto text_document= obj->getObject( "textDocument" );
	if( text_document == nullptr )
	{
		log_ << "No textDocument!" << std::endl;
		return result;
	}

	const auto uri= text_document->getString( "uri" );
	if( uri == llvm::None )
	{
		log_ << "No uri!" << std::endl;
		return result;
	}

	const auto position= obj->getObject( "position" );
	if( position == nullptr )
	{
		log_ << "No position!" << std::endl;
		return result;
	}

	const auto line= position->getInteger( "line" );
	const auto character= position->getInteger( "character" );
	if( line == llvm::None || character == llvm::None )
	{
		log_ << "Invalid position!" << std::endl;
		return result;
	}

	const auto it= documents_.find( uri->str() );
	if( it == documents_.end() )
	{
		log_ << "Can't find document " << uri->str() << std::endl;
		return result;
	}

	if( const auto range= it->second.GetDefinitionPoint( SrcLoc( 0, uint32_t(*line) + 1, uint32_t(*character) ) ) )
	{
		result["range"]= DocumentRangeToJson( *range );
		result["uri"]= uri->str();
	}
	return result;
}

ServerResponse ServerHandler::ProcessTextDocumentCompletion( const Json::Value& params )
{
	Json::Object result;

	const auto obj= params.getAsObject();
	if( obj == nullptr )
	{
		log_ << "Not an object!" << std::endl;
		return result;
	}

	const auto text_document= obj->getObject( "textDocument" );
	if( text_document == nullptr )
	{
		log_ << "No textDocument!" << std::endl;
		return result;
	}

	const auto uri= text_document->getString( "uri" );
	if( uri == llvm::None )
	{
		log_ << "No uri!" << std::endl;
		return result;
	}

	const auto position= obj->getObject( "position" );
	if( position == nullptr )
	{
		log_ << "No position!" << std::endl;
		return result;
	}

	// TODO - perform real completion.

	// Fill dummy.
	result["isIncomplete"]= true;

	{
		Json::Array items;

		{
			Json::Object item;

			item["label"]= "CompleteThis";

			items.push_back( std::move(item) );
		}
		result["items"]= std::move(items);
	}

	return result;
}

ServerResponse ServerHandler::ProcessTextDocumentHighlight( const Json::Value& params )
{
	Json::Array result;

	const auto obj= params.getAsObject();
	if( obj == nullptr )
	{
		log_ << "Not an object!" << std::endl;
		return result;
	}

	const auto text_document= obj->getObject( "textDocument" );
	if( text_document == nullptr )
	{
		log_ << "No textDocument!" << std::endl;
		return result;
	}

	const auto uri= text_document->getString( "uri" );
	if( uri == llvm::None )
	{
		log_ << "No uri!" << std::endl;
		return result;
	}

	const auto position= obj->getObject( "position" );
	if( position == nullptr )
	{
		log_ << "No position!" << std::endl;
		return result;
	}

	const auto line= position->getInteger( "line" );
	const auto character= position->getInteger( "character" );
	if( line == llvm::None || character == llvm::None )
	{
		log_ << "Invalid position!" << std::endl;
		return result;
	}

	const auto it= documents_.find( uri->str() );
	if( it == documents_.end() )
	{
		log_ << "Can't find document " << uri->str() << std::endl;
		return result;
	}

	for( const DocumentRange& range : it->second.GetHighlightLocations( SrcLoc( 0, uint32_t(*line) + 1, uint32_t(*character) ) ) )
	{
		Json::Object highlight;
		highlight["range"]= DocumentRangeToJson( range );
		result.push_back( std::move(highlight) );
	}

	return result;
}

ServerResponse ServerHandler::ProcessTextDocumentRename( const Json::Value& params )
{
	Json::Object result;

	const auto obj= params.getAsObject();
	if( obj == nullptr )
	{
		log_ << "Not an object!" << std::endl;
		return result;
	}

	const auto text_document= obj->getObject( "textDocument" );
	if( text_document == nullptr )
	{
		log_ << "No textDocument!" << std::endl;
		return result;
	}

	const auto uri= text_document->getString( "uri" );
	if( uri == llvm::None )
	{
		log_ << "No uri!" << std::endl;
		return result;
	}

	const auto new_name= obj->getString( "newName" );
	if( new_name == llvm::None )
	{
		log_ << "No newName!" << std::endl;
		return result;
	}

	const auto position= obj->getObject( "position" );
	if( position == nullptr )
	{
		log_ << "No position!" << std::endl;
		return result;
	}

	const auto line= position->getInteger( "line" );
	const auto character= position->getInteger( "character" );
	if( line == llvm::None || character == llvm::None )
	{
		log_ << "Invalid position!" << std::endl;
		return result;
	}

	const auto it= documents_.find( uri->str() );
	if( it == documents_.end() )
	{
		log_ << "Can't find document " << uri->str() << std::endl;
		return result;
	}

	const std::string new_name_str= new_name->str();
	if( !IsValidIdentifier( new_name_str ) )
	{
		Json::Object error;
		error["code"]= int32_t(ErrorCode::RequestFailed);
		error["message"]= "Not a valid identifier";
		return ServerResponse( std::move(result), Json::Value(std::move(error)) );
	}

	{
		Json::Object changes;
		{
			Json::Array edits;
			for( const DocumentRange& range : it->second.GetAllOccurrences( SrcLoc( 0, uint32_t(*line) + 1, uint32_t(*character) ) ) )
			{
				Json::Object edit;
				edit["range"]= DocumentRangeToJson( range );
				edit["newText"]= new_name_str;

				edits.push_back( std::move(edit) );
			}

			changes[ uri->str() ]= std::move(edits);
		}

		result["changes"]= std::move(changes);
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

	const auto text_document= obj->getObject( "textDocument" );
	if( text_document == nullptr )
	{
		log_ << "No textDocument!" << std::endl;
		return;
	}

	const auto uri= text_document->getString( "uri" );
	if( uri == llvm::None )
	{
		log_ << "No uri!" << std::endl;
		return;
	}

	const auto text= text_document->getString( "text" );
	if( text == llvm::None )
	{
		log_ << "No text!" << std::endl;
		return;
	}

	log_ << "open a document " << uri->str() << std::endl;

	const auto it_bool_pair= documents_.insert( std::make_pair( uri->str(), Document( log_, text->str() ) ) );

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

	const auto text_document= obj->getObject( "textDocument" );
	if( text_document == nullptr )
	{
		log_ << "No textDocument!" << std::endl;
		return;
	}

	const auto uri= text_document->getString( "uri" );
	if( uri == llvm::None )
	{
		log_ << "No uri!" << std::endl;
		return;
	}

	log_ << "close a document " << uri->str() << std::endl;

	documents_.erase( uri->str() );
}

void ServerHandler::ProcessTextDocumentDidChange( const Json::Value& params )
{
	const auto obj= params.getAsObject();
	if( obj == nullptr )
	{
		log_ << "Not an object!" << std::endl;
		return;
	}

	const auto text_document= obj->getObject("textDocument" );
	if( text_document == nullptr )
	{
		log_ << "No textDocument!" << std::endl;
		return;
	}

	const auto uri= text_document->getString( "uri" );
	if( uri == llvm::None )
	{
		log_ << "No uri!" << std::endl;
		return;
	}

	log_ << "Change document " << uri->str() << std::endl;

	const auto content_changes= obj->getArray("contentChanges" );
	if( content_changes == nullptr )
	{
		log_ << "No contentChanges!" << std::endl;
		return;
	}

	if( content_changes->size() == 0 )
	{
		log_ << "Empty changes!" << std::endl;
		return;
	}

	const Json::Value& change= content_changes->back();

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

	const auto it= documents_.find( uri->str() );
	if( it == documents_.end() )
	{
		log_ << "Can't find document " << uri->str() << std::endl;
		return;
	}

	it->second.SetText( change_text_str->str() );
	GenerateDocumentNotifications( *uri, it->second );
}

void ServerHandler::GenerateDocumentNotifications( const llvm::StringRef uri, const Document& document )
{
	Json::Object result;
	result["uri"]= uri.str();

	{
		Json::Array diagnostics;

		CreateLexSyntErrorsDiagnostics( document.GetLexErrors(), diagnostics );
		CreateLexSyntErrorsDiagnostics( document.GetSyntErrors(), diagnostics );
		CreateCodeBuilderErrorsDiagnostics( document.GetCodeBuilderErrors(), diagnostics );

		result["diagnostics"]= std::move(diagnostics);
	}

	ServerNotification notification{ "textDocument/publishDiagnostics", std::move(result) };
	notifications_queue_.push( std::move(notification) );
}

} // namespace LangServer

} // namespace U
