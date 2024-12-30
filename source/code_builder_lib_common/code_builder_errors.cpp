#include "../lex_synt_lib_common/assert.hpp"
#include "code_builder_errors.hpp"

namespace U
{

bool operator==( const CodeBuilderError& l, const CodeBuilderError& r )
{
	return l.code == r.code && l.src_loc == r.src_loc && l.text == r.text && l.template_context == r.template_context;
}

bool operator< ( const CodeBuilderError& l, const CodeBuilderError& r )
{
	// Sort by position in file, then, by code, then, by text.
	if( l.src_loc != r.src_loc )
		return l.src_loc < r.src_loc;
	if( l.code != r.code )
		return l.code < r.code;
	if( l.text != r.text )
		return l.text < r.text;
	return l.template_context < r.template_context;
}

std::string_view CodeBuilderErrorCodeToString( const CodeBuilderErrorCode code )
{
	switch(code)
	{
	#define PROCESS_ERROR(Code, Message) case CodeBuilderErrorCode::Code: return #Code;
	#include "../compiler1/errors_list.hpp"
	#undef PROCESS_ERROR
	};

	U_ASSERT(false);
	return "";
}

} // namespace U
