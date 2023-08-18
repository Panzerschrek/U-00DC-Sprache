#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/JSON.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "server.hpp"

namespace U
{

namespace LangServer
{

Server::Server( Connection connection, ServerHandler& handler, std::ostream& log )
	: connection_( std::move(connection) ), handler_(handler), log_(log)
{}

void Server::Run()
{
	// TODO - fix this.
	// ReadAndProcessInputMessage is blocking.
	// Make it nonblocking in order to have possibility to send notifications without waiting for another input message.
	while( ReadAndProcessInputMessage() )
	{
		PushNotifications();
	}
}

bool Server::ReadAndProcessInputMessage()
{
	const std::string message= connection_.Read();
	// log_ << "Message: " << message << std::endl;

	llvm::Expected<llvm::json::Value> parse_result= llvm::json::parse( message );
	if( !parse_result )
	{
		log_ << "JSON parse error" << std::endl;
		return false;
	}

	const llvm::json::Value& value= parse_result.get();

	const Json::Object* const obj= value.getAsObject();
	if( obj == nullptr )
	{
		log_ << "JSON is not an object" << std::endl;
		return false;
	}

	std::string id;
	if( const Json::Value* const id_json= obj->get( "id" ) )
	{
		if( const auto str= id_json->getAsString() )
			id= str->str();
		else if( const auto num= id_json->getAsInteger() )
			id= std::to_string( *num );
	}

	std::string method;
	if( const auto method_json= obj->getString( "method" ) )
		method= method_json->str();

	Json::Value params= Json::Object();
	if( const auto params_json= obj->get( "params" ) )
		params= *params_json;

	if( id.empty() )
	{
		// log_ << "Notification " << method << std::endl;

		if( method == "exit" )
			return false;
		else
			handler_.HandleNotification( method, params );
	}
	else
	{
		// log_ << "Request " << method << std::endl;
		if( method == "shutdown" )
		{
			llvm::json::Object response_obj;
			response_obj["id"]= id;

			std::string response_str;
			llvm::raw_string_ostream stream(response_str);
			stream << llvm::json::Object( std::move(response_obj) );
			stream.flush();

			connection_.Write( response_str );
		}
		else
		{
			Json::Value result= handler_.HandleRequest( method, params );

			llvm::json::Object response_obj;
			response_obj["id"]= id;
			response_obj["result"]= std::move(result);

			std::string response_str;
			llvm::raw_string_ostream stream(response_str);
			stream << llvm::json::Object( std::move(response_obj) );
			stream.flush();

			// log_ << "Response: " << response_str;
			connection_.Write( response_str );
		}
	}

	return true;
}

void Server::PushNotifications()
{
	while(true)
	{
		auto notification= handler_.TakeNotification();
		if( notification == std::nullopt )
			break;

		llvm::json::Object notification_obj;
		notification_obj["method"]= std::move(notification->method);
		notification_obj["params"]= std::move(notification->params);

		std::string response_str;
		llvm::raw_string_ostream stream(response_str);
		stream << llvm::json::Object( std::move(notification_obj) );
		stream.flush();

		// log_ << "Notification: " << response_str;
		connection_.Write( response_str );
	}
}

} // namespace LangServer

} // namespace U
