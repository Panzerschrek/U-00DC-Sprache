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

namespace CodeBuilderPrivate
{
	class Type;
} // namespace CodeBuilderPrivate

enum class CodeBuilderErrorCode : uint16_t
{
	#define PROCESS_ERROR( Code, Message ) Code,
	#include "errors_list.hpp"
	#undef PROCESS_ERROR
};

struct CodeBuilderError;
using CodeBuilderErrorsContainer= std::vector<CodeBuilderError>;

struct TemplateErrorsContext
{
	CodeBuilderErrorsContainer errors;
	FilePos template_declaration_file_pos;
};
using TemplateErrorsContextPtr= std::shared_ptr<TemplateErrorsContext>;

struct CodeBuilderError
{
	ProgramString text;
	TemplateErrorsContextPtr template_context; // For errors of type "TemplateContext"
	CodeBuilderErrorCode code;
	FilePos file_pos;
};

bool operator==( const CodeBuilderError& l, const CodeBuilderError& r );
bool operator!=( const CodeBuilderError& l, const CodeBuilderError& r );
bool operator< ( const CodeBuilderError& l, const CodeBuilderError& r ); // For sorting, using file_pos

const char* CodeBuilderErrorCodeToString( CodeBuilderErrorCode code );
void NormalizeErrors( CodeBuilderErrorsContainer& errors );

// Macro for errors reporting.
#define REPORT_ERROR( error_code, errors_container, ... ) errors_container.push_back( ErrorReportingImpl::ReportError( CodeBuilderErrorCode::error_code, __VA_ARGS__ ) )

namespace ErrorReportingImpl
{

// Using formatter for UTF-8, because formatter for "sprache_char" works incorrectly.
using Formatter= boost::format;

const char* GetErrorMessagePattern( CodeBuilderErrorCode code );

std::string PreprocessArg( const CodeBuilderPrivate::Type& type );
std::string PreprocessArg( const Synt::ComplexName& name );
std::string PreprocessArg( const ProgramString& str );

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
