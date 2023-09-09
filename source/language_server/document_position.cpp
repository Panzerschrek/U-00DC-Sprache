#include "document_position.hpp"

namespace U
{

namespace LangServer
{

bool operator==( const DocumentPosition& l, const DocumentPosition& r )
{
	return l.line == r.line && l.character == r.character;
}

bool operator <( const DocumentPosition& l, const DocumentPosition& r )
{
	if( l.line != r.line )
		return l.line < r.line;
	return l.character < r.character;
}

bool operator==( const DocumentRange& l, const DocumentRange& r )
{
	return l.start == r.start && l.end == r.end;
}

bool operator <( const DocumentRange& l, const DocumentRange& r )
{
	if( !( l.start == r.start ) )
		return l.start < r.start;
	return l.end < r.end;
}

} // namespace LangServer

} // namespace U
