#pragma once
#include <memory>
#include <string>
#include <vector>
#include "../lex_synt_lib/syntax_analyzer.hpp"


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

	std::string template_name;
	std::string parameters_description;
};
using TemplateErrorsContextPtr= std::shared_ptr<TemplateErrorsContext>;

struct CodeBuilderError
{
	std::string text;
	TemplateErrorsContextPtr template_context; // For errors of type "TemplateContext"
	CodeBuilderErrorCode code;
	FilePos file_pos;
};

bool operator==( const CodeBuilderError& l, const CodeBuilderError& r );
bool operator!=( const CodeBuilderError& l, const CodeBuilderError& r );
bool operator< ( const CodeBuilderError& l, const CodeBuilderError& r ); // For sorting, using file_pos

const char* CodeBuilderErrorCodeToString( CodeBuilderErrorCode code );

void NormalizeErrors( CodeBuilderErrorsContainer& errors );

// Also normalizes errors.
CodeBuilderErrorsContainer ExpandErrorsInMacros(
	const CodeBuilderErrorsContainer& errors,
	const Synt::MacroExpansionContexts& macro_expanisoin_contexts );

} // namespace U
