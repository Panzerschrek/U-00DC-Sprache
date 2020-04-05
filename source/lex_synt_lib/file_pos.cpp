#include "file_pos.hpp"

namespace U
{

bool operator==( const FilePos& l, const FilePos& r )
{
	return l.file_index == r.file_index && l.line == r.line && r.column == l.column;
}

bool operator!=( const FilePos& l, const FilePos& r )
{
	return !( l == r );
}

bool operator< ( const FilePos& l, const FilePos& r )
{
	if( l.file_index != r.file_index )
		return l.file_index < r.file_index;
	if( l.line != r.line )
		return l.line < r.line;
	return l.column < r.column;
}

bool operator<=( const FilePos& l, const FilePos& r )
{
	return l < r || l == r;
}

} // namespace U
