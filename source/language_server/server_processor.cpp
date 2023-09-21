#include "../lex_synt_lib_common/assert.hpp"
#include "../code_builder_lib_common/string_ref.hpp"
#include "options.hpp"
#include "server_processor.hpp"

namespace U
{

namespace LangServer
{

namespace
{

enum class ErrorCode : int32_t
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

Json::Array SymbolsToJson( const Symbols& symbols )
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

llvm::ThreadPoolStrategy CreateThreadPoolStrategy( Logger& log )
{
	if( Options::num_threads == 0 )
	{
		log() << "Auto-select number of threads in the thread pool." << std::endl;
		return llvm::hardware_concurrency();
	}

	log() << "Create " << Options::num_threads << " threads in the thread pool." << std::endl;
	return llvm::hardware_concurrency( Options::num_threads );
}

} // namespace

ServerProcessor::ServerProcessor( Logger& log, IJsonMessageWrite& out )
	: log_(log)
	, out_(out)
	, thread_pool_(CreateThreadPoolStrategy(log_))
	, document_manager_(log_)
{
	log_() << "Created thead pool with " << thread_pool_.getThreadCount() << " threads." << std::endl;
}

void ServerProcessor::Process( MessageQueue& message_queue )
{
	while(!message_queue.IsClosed())
	{
		const DocumentClock::duration wait_duration= document_manager_.PerfromDelayedRebuild( thread_pool_ );
		const auto wait_ms= std::chrono::duration_cast<std::chrono::milliseconds>(wait_duration);

		UpdateDiagnostics();

		// Wait for new messages, but only until next document needs to be updated.
		if( const std::optional<Message> message= message_queue.TryPop( wait_ms ) )
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
	if( shutdown_received_ )
	{
		log_() << "Request after shutdown!" << std::endl;

		Json::Object error;
		error["code"]= int32_t(ErrorCode::InvalidRequest);
		error["message"]= "Should not receive more requests after shutdown.";
		return ServerResponse( Json::Object(), Json::Value(std::move(error)) );
	}

	return std::visit( [&]( const auto& r ) { return HandleRequestImpl(r); }, request.params );
}

void ServerProcessor::HandleNotification( const Notification& notification )
{
	return std::visit( [&]( const auto& n ) { return HandleNotificationImpl(n); }, notification );
}

ServerProcessor::ServerResponse ServerProcessor::HandleRequestImpl( const InvalidParams& invalid_params )
{
	log_() << "Invalid params: " << invalid_params.message << std::endl;

	Json::Object error;
	error["code"]= int32_t(ErrorCode::InvalidParams);
	error["message"]= invalid_params.message;
	return ServerResponse( Json::Object(), Json::Value(std::move(error)) );
}

ServerProcessor::ServerResponse ServerProcessor::HandleRequestImpl( const MethodNotFound& method_not_fund )
{
	log_() << "Method " << method_not_fund.method_name << " not found" << std::endl;

	Json::Object error;
	error["code"]= int32_t(ErrorCode::MethodNotFound);
	error["message"]= "No method " + method_not_fund.method_name;
	return ServerResponse( Json::Object(), Json::Value(std::move(error)) );
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
		{
			Json::Object signature_help_options;

			{
				Json::Array trigger_characters;
				trigger_characters.push_back( Json::Value( "(" ) );
				trigger_characters.push_back( Json::Value( "," ) );

				signature_help_options["triggerCharacters"]= std::move(trigger_characters);
			}

			capabilities["signatureHelpProvider"]= std::move(signature_help_options);
		}

		result["capabilities"]= std::move(capabilities);
	}
	return result;
}

ServerProcessor::ServerResponse ServerProcessor::HandleRequestImpl( const Requests::Shutdown& shutdown )
{
	(void)shutdown;
	shutdown_received_= true;
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
	if( const std::optional<RangeInDocument> range_in_document= document_manager_.GetDefinitionPoint( definition.position ) )
	{
		Json::Object result;
		result["range"]= DocumentRangeToJson( range_in_document->range );
		result["uri"]= range_in_document->uri.ToString();
		return result;
	}

	return Json::Value(nullptr);
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

ServerProcessor::ServerResponse ServerProcessor::HandleRequestImpl( const Requests::SignatureHelp& signature_help )
{
	Json::Object result;

	Json::Array signatures;
	{
		for( const CodeBuilder::SignatureHelpItem& item : document_manager_.GetSignatureHelp( signature_help.position ) )
		{
			Json::Object signature;
			signature["label"]= item.label;
			{
				Json::Array parameters;
				signature["parameters"]= std::move(parameters);
			}
			signatures.push_back( std::move(signature) );

		}
		result["signatures"]= std::move(signatures);
	}
	result["activeSignature"]= Json::Value( int64_t(0) );
	result["activeParameter"]= Json::Value( int64_t(0) );

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
		log_() << "Invalid identifier for rename: " << rename.new_name << std::endl;

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

void ServerProcessor::HandleNotificationImpl( const InvalidParams& invalid_params )
{
	log_() << "Invalid params: " << invalid_params.message << std::endl;
}

void ServerProcessor::HandleNotificationImpl( const MethodNotFound& method_not_fund )
{
	log_() << "Method " << method_not_fund.method_name << " not found" << std::endl;
}

void ServerProcessor::HandleNotificationImpl( const Notifications::Initialized& initialized )
{
	// Nothing to do here.
	(void)initialized;
}

void ServerProcessor::HandleNotificationImpl( const Notifications::TextDocumentDidOpen& text_document_did_open )
{
	log_() << "open a document " << text_document_did_open.uri.ToString() << std::endl;

	Document* const document= document_manager_.Open( text_document_did_open.uri, text_document_did_open.text );
	if( document != nullptr )
		GenerateDiagnosticsNotifications( document->GetDiagnostics() );
}

void ServerProcessor::HandleNotificationImpl( const Notifications::TextDocumentDidClose& text_document_did_close )
{
	log_() << "close a document " << text_document_did_close.uri.ToString() << std::endl;

	document_manager_.Close( text_document_did_close.uri );
}

void ServerProcessor::HandleNotificationImpl( const Notifications::TextDocumentDidChange& text_document_did_change )
{
	Document* const document= document_manager_.GetDocument( text_document_did_change.uri );
	if( document == nullptr )
	{
		log_() << "Can't find document " << text_document_did_change.uri.ToString() << std::endl;
		return;
	}

	for( const Notifications::TextDocumentChange& change : text_document_did_change.changes )
	{
		// TODO - somehow invalidate document in case of synchronization errors.
		if( const auto incremental_change= std::get_if<Notifications::TextDocumentIncrementalChange>( &change ) )
			document->UpdateText( incremental_change->range, incremental_change->new_text );
		else if( const auto full_change= std::get_if<std::string>( &change ) )
			document->SetText( *full_change );
		else U_ASSERT(false); // Unhandled variant.
	}
}

void ServerProcessor::HandleNotificationImpl( const Notifications::TextDocumentWillSave& text_document_will_save )
{
	// There is no reason for now to handle this notification somehow.
	log_() << "Will save document " << text_document_will_save.uri.ToString() << std::endl;
}

void ServerProcessor::HandleNotificationImpl( const Notifications::TextDocumentDidSave& text_document_did_save )
{
	// There is no reason for now to handle this notification somehow.
	log_() << "Did save document " << text_document_did_save.uri.ToString() << std::endl;
}

void ServerProcessor::HandleNotificationImpl( const Notifications::CancelRequest& cancel_request )
{
	// Assume that cancellation is performing before this handler - in messages queue itself.
	(void)cancel_request;
}

void ServerProcessor::UpdateDiagnostics()
{
	if( !document_manager_.DiagnosticsWereUpdated() )
		return; // No need to update.

	// Flatten diagnostics maps - collect errors in common imported files caused by compilation of different documents.
	DiagnosticsByDocument diagnostcs_flat;
	for( const auto& document_diagnostics : document_manager_.GetDiagnostics() )
	{
		// Force create entry for owned document in order to clear previous diagnostics if no new diagnostics were generated.
		// This effectevely allows to clear errors in fixed file(s).
		diagnostcs_flat[ document_diagnostics.first ];

		for( const auto& diagnostic_pair : document_diagnostics.second )
			diagnostcs_flat[ diagnostic_pair.first ]= diagnostic_pair.second;
	}

	// Make sure diagnostics are unique.
	for( auto& diagnostic_pair : diagnostcs_flat )
	{
		DocumentDiagnostics& document_diagnostics= diagnostic_pair.second;
		std::sort( document_diagnostics.begin(), document_diagnostics.end() );
		document_diagnostics.erase(
			std::unique( document_diagnostics.begin(), document_diagnostics.end() ),
			document_diagnostics.end() );
	}

	GenerateDiagnosticsNotifications( diagnostcs_flat );

	document_manager_.ResetDiagnosticsUpdatedFlag();
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
