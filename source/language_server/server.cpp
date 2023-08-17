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
	while( ProcessStep() )
	{
	}
}

bool Server::ProcessStep()
{
	const std::string message= connection_.Read();
	log_ << "Message: " << message << std::endl;

	llvm::Expected<llvm::json::Value> parse_result= llvm::json::parse( message );
	if( !parse_result )
	{
		log_ << "JSON parse error" << std::endl;
		return false;
	}

	log_ << "JSON parsed successfully" << std::endl;
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
	if( const Json::Value* const method_json= obj->get( "method" ) )
	{
		if( const auto str= method_json->getAsString() )
			method= str->str();
	}

	Json::Value params= Json::Object();
	if( const auto params_json= obj->get( "params" ) )
		params= *params_json;

	if( id.empty() )
	{
		log_ << "Notification " << method << std::endl;

		if( method == "exit" )
			return false;
		else
			handler_.HandleNotification( method, params );
	}
	else
	{
		log_ << "Request " << method << std::endl;
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

			log_ << "Response: " << response_str;
			connection_.Write( response_str );
		}
	}

	return true;
}

} // namespace LangServer

} // namespace U
