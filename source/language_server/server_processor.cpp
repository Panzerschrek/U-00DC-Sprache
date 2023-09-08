#include "../lex_synt_lib_common/assert.hpp"
#include "../code_builder_lib_common/string_ref.hpp"
#include "server_processor.hpp"

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

Json::Value DocumentRangeToJson( const DocumentRange& range )
{
	Json::Object out_range;
	out_range["start"]= DocumentPositionToJson( range.start );
	out_range["end"]= DocumentPositionToJson( range.end );
	return out_range;
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

Json::Value RequestIdToJson( const RequestId& id )
{
	return std::visit( []( const auto& el ) { return Json::Value(el); }, id );
}

} // namespace

ServerProcessor::ServerProcessor( Logger& log, IJsonMessageWrite& out )
	: log_(log), out_(out), document_manager_(log_)
{
}

void ServerProcessor::Process( MessageQueue& message_queue )
{
	while(!message_queue.IsClosed())
	{
		document_manager_.PerfromDelayedRebuild();

		// TODO - make wait time dependent on something like document rebuild timers.
		if( const std::optional<Message> message= message_queue.TryPop( std::chrono::milliseconds(250) ) )
			HandleMessage( *message );
	}
}

void ServerProcessor::HandleMessage( const Message& message )
{
	return std::visit( [&]( const auto& n ) { return HandleMessageImpl(n); }, message );
}

void ServerProcessor::HandleMessageImpl( const Request& request )
{
	ServerResponse response= HandleRequest( request );

	Json::Object response_obj;
	response_obj["id"]= RequestIdToJson(request.id);
	response_obj["result"]= std::move(response.result);
	if( response.error.kind() != Json::Value::Null )
		response_obj["error"]= std::move(response.error);

	out_.Write( Json::Value( std::move(response_obj) ) );
}

void ServerProcessor::HandleMessageImpl( const Notification& notification )
{
	HandleNotification( notification );
}

ServerProcessor::ServerResponse ServerProcessor::HandleRequest( const Request& request )
{
	return std::visit( [&]( const auto& r ) { return HandleRequestImpl(r); }, request.params );
}

void ServerProcessor::HandleNotification( const Notification& notification )
{
	return std::visit( [&]( const auto& n ) { return HandleNotificationImpl(n); }, notification );
}

ServerProcessor::ServerResponse ServerProcessor::HandleRequestImpl( const Requests::Initialize& initiailize )
{
	(void)initiailize;

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

ServerProcessor::ServerResponse ServerProcessor::HandleRequestImpl( const Requests::Shutdown& shutdown )
{
	(void)shutdown;
	return Json::Value(nullptr);
}

ServerProcessor::ServerResponse ServerProcessor::HandleRequestImpl( const Requests::Symbols& symbols )
{
	return SymbolsToJson( document_manager_.GetSymbols(symbols.uri) );
}

ServerProcessor::ServerResponse ServerProcessor::HandleRequestImpl( const Requests::References& references )
{
	Json::Array result;

	for( const RangeInDocument& range_in_document : document_manager_.GetAllOccurrences( references.position ) )
	{
		Json::Object out_location;
		out_location["range"]= DocumentRangeToJson( range_in_document.range );
		out_location["uri"]= range_in_document.uri.ToString();

		result.push_back( std::move(out_location) );
	}

	return result;
}

ServerProcessor::ServerResponse ServerProcessor::HandleRequestImpl( const Requests::Definition& definition )
{
	Json::Object result;

	if( const std::optional<RangeInDocument> range_in_document= document_manager_.GetDefinitionPoint( definition.position ) )
	{
		result["range"]= DocumentRangeToJson( range_in_document->range );
		result["uri"]= range_in_document->uri.ToString();
	}

	return result;
}

ServerProcessor::ServerResponse ServerProcessor::HandleRequestImpl( const Requests::Complete& complete )
{
	Json::Object result;

	result["isIncomplete"]= false; // Completion provides all possible names.

	{
		Json::Array items;

		for( const CompletionItem& completion_item : document_manager_.Complete( complete.position ) )
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

ServerProcessor::ServerResponse ServerProcessor::HandleRequestImpl( const Requests::Highlight& highlight )
{
	Json::Array result;

	for( const DocumentRange& range : document_manager_.GetHighlightLocations( highlight.position ) )
	{
		Json::Object highlight;
		highlight["range"]= DocumentRangeToJson( range );
		result.push_back( std::move(highlight) );
	}

	return result;
}

ServerProcessor::ServerResponse ServerProcessor::HandleRequestImpl( const Requests::Rename& rename )
{
	Json::Object result;

	if( !IsValidIdentifier( rename.new_name ) )
	{
		Json::Object error;
		error["code"]= int32_t(ErrorCode::RequestFailed);
		error["message"]= "Not a valid identifier";
		return ServerResponse( std::move(result), Json::Value(std::move(error)) );
	}

	{
		Json::Object changes;
		for( const RangeInDocument& range_in_document : document_manager_.GetAllOccurrences( rename.position ) )
		{
			Json::Object edit;
			edit["range"]= DocumentRangeToJson( range_in_document.range );
			edit["newText"]= rename.new_name;

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

void ServerProcessor::HandleNotificationImpl( const Notifications::TextDocumentDidOpen& text_document_did_open )
{
	log_ << "open a document " << text_document_did_open.uri.ToString() << endl;

	Document* const document= document_manager_.Open( text_document_did_open.uri, text_document_did_open.text );
	if( document != nullptr )
		GenerateDiagnosticsNotifications( document->GetDiagnostics() );
}

void ServerProcessor::HandleNotificationImpl( const Notifications::TextDocumentDidClose& text_document_did_close )
{
	log_ << "close a document " << text_document_did_close.uri.ToString() << endl;

	document_manager_.Close( text_document_did_close.uri );
}

void ServerProcessor::HandleNotificationImpl( const Notifications::TextDocumentDidChange& text_document_did_change )
{
	Document* const document= document_manager_.GetDocument( text_document_did_change.uri );
	if( document == nullptr )
	{
		log_ << "Can't find document " << text_document_did_change.uri.ToString() << endl;
		return;
	}

	for( const Notifications::TextDocumentChange& change : text_document_did_change.changes )
	{
		if( const auto incremental_change= std::get_if<Notifications::TextDocumentIncrementalChange>( &change ) )
			document->UpdateText( incremental_change->range, incremental_change->new_text );
		else if( const auto full_change= std::get_if<std::string>( &change ) )
			document->SetText( *full_change );
		else U_ASSERT(false); // Unhandled variant.
	}
}

void ServerProcessor::HandleNotificationImpl( const Notifications::CancelRequest& cancel_request )
{
	// Assume that cancellation is performing before this handler - in messages queue itself.
	(void)cancel_request;
}

void ServerProcessor::GenerateDiagnosticsNotifications( const DiagnosticsByDocument& diagnostics )
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

		PublishNotification( "textDocument/publishDiagnostics", std::move(result) );
	}
}

void ServerProcessor::PublishNotification( const std::string_view method, Json::Value params )
{
	llvm::json::Object notification_obj;
	notification_obj["method"]= StringViewToStringRef(method);
	notification_obj["params"]= std::move(params);

	out_.Write( Json::Value( std::move(notification_obj) ) );
}

} // namespace LangServer

} // namespace U
