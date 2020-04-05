#pragma once

namespace U
{

struct FilePos
{
	unsigned short file_index;
	unsigned short line; // from 1
	unsigned short column;
};

bool operator==( const FilePos& l, const FilePos& r );
bool operator!=( const FilePos& l, const FilePos& r );
bool operator< ( const FilePos& l, const FilePos& r );
bool operator<=( const FilePos& l, const FilePos& r );

} // namespace U
