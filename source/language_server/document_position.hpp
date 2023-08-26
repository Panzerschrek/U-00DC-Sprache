#pragma once
#include "uri.hpp"

namespace U
{

namespace LangServer
{

// Position within document, without specifying document instance (specific document depends on context).
struct DocumentPosition
{
	uint32_t line= 0; // From 1, as in SrcLoc.
	uint32_t column= 0; // From 0, as in SrcLoc.
};

struct DocumentRange
{
	DocumentPosition start;
	DocumentPosition end;
};

// Position within specific document.
struct PositionInDocument
{
	DocumentPosition position;
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
