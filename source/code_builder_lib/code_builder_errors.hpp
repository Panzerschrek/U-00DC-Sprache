#pragma once
#include <memory>
#include <string>
#include <vector>
#include "../lex_synt_lib/file_pos.hpp"


namespace U
{

enum class CodeBuilderErrorCode : uint16_t
{
	// WARNING! Values of this enum must be equal to same values in Ü compiler1.
	#define PROCESS_ERROR( Code, Message ) Code,
	#include "../errors_list.hpp"
	#undef PROCESS_ERROR
};

struct CodeBuilderError;
using CodeBuilderErrorsContainer= std::vector<CodeBuilderError>;

// Context for macros expansion and templates instantiation.
struct TemplateErrorsContext
{
	CodeBuilderErrorsContainer errors;
	FilePos context_declaration_file_pos; // Declaration position of context, macro.

	std::string context_name; // Name of template, macro.
	std::string parameters_description;
};
using TemplateErrorsContextPtr= std::shared_ptr<TemplateErrorsContext>;

struct CodeBuilderError
{
	std::string text;
	TemplateErrorsContextPtr template_context; // For errors of type "TemplateContext" or "MacroExpansionContext"
	CodeBuilderErrorCode code;
	FilePos file_pos;
};

bool operator==( const CodeBuilderError& l, const CodeBuilderError& r );
bool operator!=( const CodeBuilderError& l, const CodeBuilderError& r );
bool operator< ( const CodeBuilderError& l, const CodeBuilderError& r ); // For sorting, using file_pos

const char* CodeBuilderErrorCodeToString( CodeBuilderErrorCode code );

} // namespace U
