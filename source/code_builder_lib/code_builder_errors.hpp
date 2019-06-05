#pragma once
#include <string>
#include "../lex_synt_lib/push_disable_boost_warnings.hpp"
#include <boost/format.hpp>
#include "../lex_synt_lib/pop_boost_warnings.hpp"

#include "../lex_synt_lib/lang_types.hpp"
#include "../lex_synt_lib/lexical_analyzer.hpp"
#include "../lex_synt_lib/program_string.hpp"
#include "../lex_synt_lib/syntax_elements.hpp"

namespace U
{

enum class CodeBuilderErrorCode : uint16_t
{
	#define PROCESS_ERROR( Code, Message ) Code,
	#include "errors_list.hpp"
	#undef PROCESS_ERROR
};

struct CodeBuilderError
{
	ProgramString text;
	CodeBuilderErrorCode code;
	FilePos file_pos;
};

bool operator==( const CodeBuilderError& l, const CodeBuilderError& r );
bool operator!=( const CodeBuilderError& l, const CodeBuilderError& r );
bool operator< ( const CodeBuilderError& l, const CodeBuilderError& r ); // For sorting, using file_pos

const char* CodeBuilderErrorCodeToString( CodeBuilderErrorCode code );
const char* GetErrorMessagePattern( CodeBuilderErrorCode code );

// Macro for errors reporting.
#define REPORT_ERROR( error_code, errors_container, ... ) errors_container.push_back( ErrorReportingImpl::ReportError( CodeBuilderErrorCode::error_code, __VA_ARGS__ ) )

namespace ErrorReportingImpl
{

// Using formatter for UTF-8, because formatter for "sprache_char" works incorrectly.
using Formatter= boost::format;

inline std::string PreprocessArg( const ProgramString& str )
{
	return ToUTF8(str);
}

inline std::string PreprocessArg( const Synt::ComplexName& name )
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

template<class T>
const T& PreprocessArg( const T& t )
{
	return t;
}

inline void FeedArgs( Formatter& ){}

template<class T, class ... Args>
void FeedArgs( Formatter& format, const T& arg0, const Args& ... args )
{
	format % PreprocessArg(arg0);
	FeedArgs( format, args... );
}

template<class ... Args>
CodeBuilderError ReportError( const CodeBuilderErrorCode code, const FilePos& file_pos, const Args& ... args )
{
	CodeBuilderError error;
	error.code= code;
	error.file_pos= file_pos;

	Formatter f( GetErrorMessagePattern(code) );
	FeedArgs( f, args... );
	error.text= DecodeUTF8( f.str() );
	return error;
}

} // namespace ErrorReportingImpl

} // namespace U
