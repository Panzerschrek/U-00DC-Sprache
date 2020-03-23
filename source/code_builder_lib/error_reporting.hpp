#pragma once
#include "../lex_synt_lib/push_disable_boost_warnings.hpp"
#include <boost/format.hpp>
#include "../lex_synt_lib/pop_boost_warnings.hpp"

#include "../lex_synt_lib/syntax_elements.hpp"
#include "code_builder_errors.hpp"

namespace U
{

// Macro for errors reporting.
#define REPORT_ERROR( error_code, errors_container, ... ) errors_container.push_back( ErrorReportingImpl::ReportError( CodeBuilderErrorCode::error_code, __VA_ARGS__ ) )

namespace ErrorReportingImpl
{

using Formatter= boost::format;

const char* GetErrorMessagePattern( CodeBuilderErrorCode code );

std::string PreprocessArg( const CodeBuilderPrivate::Type& type );
std::string PreprocessArg( const Synt::ComplexName& name );
const std::string& PreprocessArg( const std::string& str );

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
	error.text= f.str();
	return error;
}

} // namespace ErrorReportingImpl

} // namespace U
