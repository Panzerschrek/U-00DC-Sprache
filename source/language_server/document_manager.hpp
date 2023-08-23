#pragma once
#include "document.hpp"
namespace U
{

namespace LangServer
{

class DocumentManager
{
public:
	explicit DocumentManager( std::ostream& log );

	Document* Open( const DocumentURI& uri, std::string text );
	Document* GetDocument(  const DocumentURI& uri );
	void Close( const DocumentURI& uri );

private:
	std::ostream& log_;
	std::unordered_map<DocumentURI, Document> documents_;
};

} // namespace LangServer

} // namespace U
