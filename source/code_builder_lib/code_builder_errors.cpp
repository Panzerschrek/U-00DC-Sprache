#include <algorithm>
#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"

#include "code_builder_errors.hpp"

namespace U
{

bool operator==( const CodeBuilderError& l, const CodeBuilderError& r )
{
	return l.code == r.code && l.file_pos == r.file_pos && l.text == r.text;
}

bool operator!=( const CodeBuilderError& l, const CodeBuilderError& r )
{
	return !(l == r);
}

bool operator< ( const CodeBuilderError& l, const CodeBuilderError& r )
{
	// Sort by position in file, then, by code, then, by text.
	if( l.file_pos != r.file_pos )
		return l.file_pos < r.file_pos;
	if( l.code != r.code )
		return l.code < r.code;
	return l.text < r.text;
}

const char* CodeBuilderErrorCodeToString( const CodeBuilderErrorCode code )
{
	switch(code)
	{
	#define PROCESS_ERROR(Code, Message) case CodeBuilderErrorCode::Code: return #Code;
	#include "errors_list.hpp"
	#undef PROCESS_ERROR
	};

	U_ASSERT(false);
	return "";
}

} // namespace U
