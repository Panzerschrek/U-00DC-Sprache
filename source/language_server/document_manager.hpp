#pragma once
#include "document.hpp"
#include "uri.hpp"

namespace U
{

namespace LangServer
{

class DocumentManager
{
public:
	explicit DocumentManager( std::ostream& log );

	Document* Open( const Uri& uri, std::string text );
	Document* GetDocument( const Uri& uri );
	void Close( const Uri& uri );

private:
	std::ostream& log_;
	// TODO - use unordered map.
	std::map<Uri, Document> documents_;
};

} // namespace LangServer

} // namespace U
