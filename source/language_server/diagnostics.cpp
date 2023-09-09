#include "diagnostics.hpp"

namespace U
{

namespace LangServer
{

bool operator==( const DocumentDiagnostic& l, const DocumentDiagnostic& r )
{
	return l.range == r.range && l.text == r.text && l.code == r.code;
}

bool operator <( const DocumentDiagnostic& l, const DocumentDiagnostic& r )
{
	if( !(l.range == r.range) )
		return l.range < r.range;
	if( l.text != r.text )
		return l.text < r.text;
	return l.code < r.code;
}

} // namespace LangServer

} // namespace U
