#pragma once
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/FormatVariadic.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/syntax_analyzer.hpp"
#include "../../code_builder_lib_common/code_builder_errors.hpp"
#include "../../code_builder_lib_common/string_ref.hpp"

namespace U
{

class Type;

// Macro for errors reporting.
#define REPORT_ERROR( error_code, errors_container, ... ) (errors_container).push_back( ErrorReportingImpl::ReportError( CodeBuilderErrorCode::error_code, __VA_ARGS__ ) )

CodeBuilderErrorsContainer NormalizeErrors(
	const CodeBuilderErrorsContainer& errors,
	const Synt::MacroExpansionContexts& macro_expanisoin_contexts );

namespace ErrorReportingImpl
{

const char* GetErrorMessagePattern( CodeBuilderErrorCode code );

std::string PreprocessArg( const Type& type );
std::string PreprocessArg( const Synt::ComplexName& name );
inline llvm::StringRef PreprocessArg( const std::string_view arg ) { return StringViewToStringRef(arg); }
template<class T> const T& PreprocessArg( const T& t ) { return t; }

template<class ... Args>
CodeBuilderError ReportError( const CodeBuilderErrorCode code, const SrcLoc& src_loc, const Args& ... args )
{
	CodeBuilderError error;
	error.code= code;
	error.src_loc= src_loc;
	error.text= llvm::formatv( GetErrorMessagePattern(code), PreprocessArg(args)... ).str();
	return error;
}

} // namespace ErrorReportingImpl

} // namespace U
