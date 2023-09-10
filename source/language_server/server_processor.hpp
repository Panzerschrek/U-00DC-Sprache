#pragma once
#include <queue>
#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/ThreadPool.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "document_manager.hpp"
#include "json.hpp"
#include "message_queue.hpp"
#include "transport.hpp"

namespace U
{


namespace LangServer
{

// Class that performs actual messages processing.
class ServerProcessor
{
public:
	explicit ServerProcessor( Logger& log, IJsonMessageWrite& out );

	// Process messages from queue until it is not closed.
	void Process( MessageQueue& message_queue );

private:
	struct ServerResponse
	{
		Json::Value result;
		Json::Value error;

		ServerResponse( Json::Object in_result ) noexcept
			: result(std::move(in_result)), error( nullptr )
		{}

		ServerResponse( Json::Array in_result ) noexcept
			: result(std::move(in_result)), error( nullptr )
		{}

		ServerResponse( Json::Value in_result ) noexcept
			: result(std::move(in_result)), error( nullptr )
		{}

		ServerResponse( Json::Value in_result, Json::Value in_error ) noexcept
			: result(std::move(in_result)), error(std::move(in_error))
		{}

		ServerResponse( const ServerResponse& )= default;
		ServerResponse( ServerResponse&& )= default;

		ServerResponse& operator=( const ServerResponse& )= default;
		ServerResponse& operator=( ServerResponse&& )= default;
	};

private:
	void HandleMessage( const Message& message );
	void HandleMessageImpl( const Request& request );
	void HandleMessageImpl( const Notification& notification );

	ServerResponse HandleRequest( const Request& request );

	void HandleNotification( const Notification& notification );

	// Requests.
	ServerResponse HandleRequestImpl( const InvalidParams& invalid_params );
	ServerResponse HandleRequestImpl( const MethodNotFound& method_not_fund );
	ServerResponse HandleRequestImpl( const Requests::Initialize& initiailize );
	ServerResponse HandleRequestImpl( const Requests::Shutdown& shutdown );
	ServerResponse HandleRequestImpl( const Requests::Symbols& symbols );
	ServerResponse HandleRequestImpl( const Requests::References& references );
	ServerResponse HandleRequestImpl( const Requests::Definition& definition );
	ServerResponse HandleRequestImpl( const Requests::Complete& complete );
	ServerResponse HandleRequestImpl( const Requests::Highlight& highlight );
	ServerResponse HandleRequestImpl( const Requests::Rename& rename );

	// Notifications.
	void HandleNotificationImpl( const InvalidParams& invalid_params );
	void HandleNotificationImpl( const MethodNotFound& method_not_fund );
	void HandleNotificationImpl( const Notifications::Initialized& initialized );
	void HandleNotificationImpl( const Notifications::TextDocumentDidOpen& text_document_did_open );
	void HandleNotificationImpl( const Notifications::TextDocumentDidClose& text_document_did_close );
	void HandleNotificationImpl( const Notifications::TextDocumentDidChange& text_document_did_change );
	void HandleNotificationImpl( const Notifications::CancelRequest& cancel_request );

	// Other stuff.
	void UpdateDiagnostics();
	void GenerateDiagnosticsNotifications( const DiagnosticsByDocument& diagnostics );

	void PublishNotification( std::string_view method, Json::Value params );

private:
	Logger& log_;
	IJsonMessageWrite& out_;
	llvm::ThreadPool thread_pool_;
	DocumentManager document_manager_;
	bool shutdown_received_= false;
};

} // namespace LangServer

} // namespace U
