#include "parser.hpp"

namespace U
{

namespace LangServer
{

namespace Json= llvm::json;

std::optional<RequestMessage> ParseRequestMessage( const llvm::json::Value& value )
{
	RequestMessage result;

	const Json::Object* const obj= value.getAsObject();
	if( obj == nullptr )
		return std::nullopt;

	if( const Json::Value* const id= obj->get( "id" ) )
	{
		if( const auto str= id->getAsString() )
			result.id= str->str();
		else if( const auto num= id->getAsInteger() )
			result.id= std::to_string( *num );
	}

	if( const Json::Value* const method= obj->get( "method" ) )
	{
		if( const auto str= method->getAsString() )
			result.method= str->str();
	}

	return result;
}

} // namespace LangServer

} // namespace U
