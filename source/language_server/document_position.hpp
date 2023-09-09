#pragma once
#include "../lex_synt_lib_common/src_loc.hpp"
#include "uri.hpp"

namespace U
{

namespace LangServer
{

// Position within document, without specifying document instance (specific document depends on context).
struct DocumentPosition
{
	uint32_t line= 0; // From 1, as in SrcLoc.
	uint32_t character= 0; // From 0, in UTF-16 words.
};

bool operator==( const DocumentPosition& l, const DocumentPosition& r );
bool operator <( const DocumentPosition& l, const DocumentPosition& r );

struct DocumentRange
{
	DocumentPosition start;
	DocumentPosition end;
};

bool operator==( const DocumentRange& l, const DocumentRange& r );
bool operator <( const DocumentRange& l, const DocumentRange& r );

// Position within specific document.
struct PositionInDocument
{
	DocumentPosition position;
	Uri uri;
};

struct SrcLocInDocument
{
	SrcLoc src_loc;
	Uri uri;
};

// Range within specific document.
struct RangeInDocument
{
	DocumentRange range;
	Uri uri;
};

} // namespace LangServer

} // namespace U
