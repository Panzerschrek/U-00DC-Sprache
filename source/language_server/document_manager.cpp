#include "document_manager.hpp"

namespace U
{

namespace LangServer
{

DocumentManager::DocumentManager( std::ostream& log )
	: log_(log)
{}

Document* DocumentManager::Open( const Uri& uri, std::string text )
{
	const auto it_bool_pair= documents_.insert( std::make_pair( uri, Document( log_, std::move(text) ) ) );
	return &it_bool_pair.first->second;
}

Document* DocumentManager::GetDocument( const Uri& uri )
{
	const auto it= documents_.find( uri );
	if( it == documents_.end() )
		return nullptr;
	return &it->second;
}

void DocumentManager::Close( const Uri& uri )
{
	documents_.erase( uri );
}

} // namespace LangServer

} // namespace U
