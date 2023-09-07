#include "../code_builder_lib_common/string_ref.hpp"
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

Json::Value DocumentPositionToJson( const DocumentPosition& position )
{
	Json::Object out_position;
	out_position["line"]= position.line - 1; // LSP uses 0-based line numbers, Ü use 1-based line numbers.
	out_position["character"]= position.character;
	return out_position;
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

Json::Value DocumentRangeToJson( const DocumentRange& range )
{
	Json::Object out_range;
	out_range["start"]= DocumentPositionToJson( range.start );
	out_range["end"]= DocumentPositionToJson( range.end );
	return out_range;
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

Json::Array SymbolsToJson( const std::vector<Symbol>& symbols )
{
	Json::Array result;

	for( const Symbol& symbol : symbols )
	{
		Json::Object out_symbol;
		out_symbol["name"]= symbol.name;
		out_symbol["kind"]= int32_t(symbol.kind);
		out_symbol["range"]= DocumentRangeToJson( symbol.range );
		out_symbol["selectionRange"]= DocumentRangeToJson( symbol.selection_range );

		if( !symbol.children.empty() )
			out_symbol["children"]= SymbolsToJson( symbol.children );

		result.push_back( std::move(out_symbol) );
	}

	return result;
}

} // namespace

ServerHandler::ServerHandler( std::ostream& log )
	: log_(log), document_manager_(log_)
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
	if( method == "textDocument/didClose" )
		return ProcessTextDocumentDidClose( params );
	if( method == "textDocument/didChange" )
		return ProcessTextDocumentDidChange( params );
	if( method == "$/cancelRequest" )
		return ProcessCancelRequest( params );
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
		capabilities["textDocumentSync"]= 2; // Incremental.
		capabilities["positionEncoding"]= "utf-16";
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
		{
			Json::Object completion_options;

			{
				Json::Array trigger_characters;
				trigger_characters.push_back( Json::Value( "." ) );
				// HACK! Use single trigger character ":" instead of "::", since some IDEs (like QtCreator) use longest trigger sequence to trigger completion,
				// so, using "::" breaks completion for ".".
				//trigger_characters.push_back( Json::Value( "::" ) );
				trigger_characters.push_back( Json::Value( ":" ) );

				completion_options["triggerCharacters"]= std::move(trigger_characters);
			}
			capabilities["completionProvider"]= std::move(completion_options);
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

	const auto uri_parsed= Uri::Parse( *uri );
	if( uri_parsed == std::nullopt )
	{
		log_ << "Invalid uri!" << std::endl;
		return result;
	}

	return SymbolsToJson( document_manager_.GetSymbols(*uri_parsed) );
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

	const auto position_in_document= JsonToPositionInDocument( *obj );
	if( position_in_document == std::nullopt )
	{
		log_ << "Failed to get position in document" << std::endl;
		return result;
	}

	for( const RangeInDocument& range_in_document : document_manager_.GetAllOccurrences( *position_in_document ) )
	{
		Json::Object out_location;
		out_location["range"]= DocumentRangeToJson( range_in_document.range );
		out_location["uri"]= range_in_document.uri.ToString();

		result.push_back( std::move(out_location) );
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

	const auto position_in_document= JsonToPositionInDocument( *obj );
	if( position_in_document == std::nullopt )
	{
		log_ << "Failed to get position in document" << std::endl;
		return result;
	}

	if( const std::optional<RangeInDocument> range_in_document= document_manager_.GetDefinitionPoint( *position_in_document ) )
	{
		result["range"]= DocumentRangeToJson( range_in_document->range );
		result["uri"]= range_in_document->uri.ToString();
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

	const auto position_in_document= JsonToPositionInDocument( *obj );
	if( position_in_document == std::nullopt )
	{
		log_ << "Failed to get position in document" << std::endl;
		return result;
	}

	result["isIncomplete"]= false; // Completion provides all possible names.

	{
		Json::Array items;

		for( const CompletionItem& completion_item : document_manager_.Complete( *position_in_document ) )
		{
			Json::Object item;
			item["label"]= completion_item.label;
			if( !completion_item.sort_text.empty() )
				item["sortText"]= completion_item.sort_text;
			if( !completion_item.detail.empty() )
				item["detail"]= completion_item.detail;
			if( completion_item.kind != CompletionItemKind::None )
				item["kind"]= uint32_t(completion_item.kind);

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

	const auto position_in_document= JsonToPositionInDocument( *obj );
	if( position_in_document == std::nullopt )
	{
		log_ << "Failed to get position in document" << std::endl;
		return result;
	}

	for( const DocumentRange& range : document_manager_.GetHighlightLocations( *position_in_document ) )
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


	const auto new_name= obj->getString( "newName" );
	if( new_name == llvm::None )
	{
		log_ << "No newName!" << std::endl;
		return result;
	}

	const auto position_in_document= JsonToPositionInDocument( *obj );
	if( position_in_document == std::nullopt )
	{
		log_ << "Failed to get position in document" << std::endl;
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
		for( const RangeInDocument& range_in_document : document_manager_.GetAllOccurrences( *position_in_document ) )
		{
			Json::Object edit;
			edit["range"]= DocumentRangeToJson( range_in_document.range );
			edit["newText"]= new_name_str;

			std::string change_uri= range_in_document.uri.ToString();
			if( const auto prev_edits= changes.getArray( change_uri ) )
				prev_edits->push_back( std::move(edit) );
			else
			{
				Json::Array edits;
				edits.push_back( std::move(edit) );
				changes.try_emplace( std::move(change_uri), std::move(edits) );
			}
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

	const auto uri_parsed= Uri::Parse( *uri );
	if( uri_parsed == std::nullopt )
	{
		log_ << "Invalid uri!" << std::endl;
		return;
	}

	const auto text= text_document->getString( "text" );
	if( text == llvm::None )
	{
		log_ << "No text!" << std::endl;
		return;
	}

	log_ << "open a document " << uri->str() << std::endl;

	Document* const document= document_manager_.Open( *uri_parsed, text->str() );
	if( document != nullptr )
		GenerateDiagnosticsNotifications( document->GetDiagnostics() );
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

	const auto uri_parsed= Uri::Parse( *uri );
	if( uri_parsed == std::nullopt )
	{
		log_ << "Invalid uri!" << std::endl;
		return;
	}

	log_ << "close a document " << uri->str() << std::endl;

	document_manager_.Close( *uri_parsed );
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

	const auto uri_parsed= Uri::Parse( *uri );
	if( uri_parsed == std::nullopt )
	{
		log_ << "Invalid uri!" << std::endl;
		return;
	}

	Document* const document= document_manager_.GetDocument( *uri_parsed );
	if( document == nullptr )
	{
		log_ << "Can't find document " << uri->str() << std::endl;
		return;
	}

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

	// TODO - check also given document version number.

	for( const Json::Value& change : *content_changes )
	{
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

		if( const auto range_json= change_obj->get( "range" ) )
		{
			const auto range= JsonToDocumentRange( *range_json );
			if( range == std::nullopt )
			{
				log_ << "Failed to parse range" << std::endl;
				return;
			}

			// log_ << "Change document range "
			//	<< range->start.line << ":" << range->start.character << " - "
			//	<< range->end.line << ":" << range->end.character
			//	<< " with new text \"" << StringRefToStringView(*change_text_str) << "\"" << std::endl;

			document->UpdateText( *range, StringRefToStringView( *change_text_str ) );
		}
		else
		{
			log_ << "Change document " << uri->str() << "by replacing whole text" << std::endl;
			document->SetText( change_text_str->str() );
		}
	}

	document->Rebuild(); // TODO - rebuild only if necessary.
	GenerateDiagnosticsNotifications( document->GetDiagnostics() );
}

void ServerHandler::ProcessCancelRequest( const Json::Value& params )
{
	const auto obj= params.getAsObject();
	if( obj == nullptr )
	{
		log_ << "Not an object!" << std::endl;
		return;
	}

	const auto id= obj->get("id");
	if( id == nullptr )
	{
		log_ << "No request id!" << std::endl;
	}
	log_ << "Recieve cancellation for request " << std::endl;
}

void ServerHandler::GenerateDiagnosticsNotifications( const DiagnosticsByDocument& diagnostics )
{
	for( const auto& document_pair : diagnostics )
	{
		Json::Object result;
		result["uri"]= document_pair.first.ToString();

		{
			Json::Array diagnostics;

			for( const DocumentDiagnostic& diagnostic : document_pair.second )
			{
				Json::Object out_diagnostic;
				out_diagnostic["message"]= diagnostic.text;
				out_diagnostic["severity"]= 1; // Means "error"
				out_diagnostic["range"]= DocumentRangeToJson( diagnostic.range );
				if( !diagnostic.code.empty() )
					out_diagnostic["code"]= diagnostic.code;

				diagnostics.push_back( std::move(out_diagnostic) );
			}

			result["diagnostics"]= std::move(diagnostics);
		}

		ServerNotification notification{ "textDocument/publishDiagnostics", std::move(result) };
		notifications_queue_.push( std::move(notification) );
	}
}

} // namespace LangServer

} // namespace U
