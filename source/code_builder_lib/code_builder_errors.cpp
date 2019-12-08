#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"
#include "type.hpp"

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

void NormalizeErrors( CodeBuilderErrorsContainer& errors )
{
	// Soprt by file/line and left only unique error messages.
	std::sort( errors.begin(), errors.end() );
	errors.erase( std::unique( errors.begin(), errors.end() ), errors.end() );

	for( const CodeBuilderError& error : errors )
	{
		if( error.template_context != nullptr )
			NormalizeErrors( error.template_context->errors );
	}

	errors.erase(
		std::remove_if(
			errors.begin(), errors.end(),
			[]( const CodeBuilderError& error ) -> bool
			{
				return error.template_context != nullptr && error.template_context->errors.empty();
			} ),
		errors.end() );
}

namespace ErrorReportingImpl
{

const char* GetErrorMessagePattern( CodeBuilderErrorCode code )
{
	switch(code)
	{
	#define PROCESS_ERROR(Code, Message) case CodeBuilderErrorCode::Code: return Message;
	#include "errors_list.hpp"
	#undef PROCESS_ERROR
	};

	U_ASSERT(false);
	return "";
}

std::string PreprocessArg( const ProgramString& str )
{
	return ToUTF8(str);
}

std::string PreprocessArg( const CodeBuilderPrivate::Type& type )
{
	return ToUTF8(type.ToString());
}

std::string PreprocessArg( const Synt::ComplexName& name )
{
	ProgramString str;
	for( const Synt::ComplexName::Component& component : name.components )
	{
		str+= component.name;
		if( &component != &name.components.back() )
			str+= "::"_SpC;
	}
	return ToUTF8(str);
}

} // namespace ErrorReportingImpl

} // namespace U
