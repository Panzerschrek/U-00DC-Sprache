#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "../lex_synt_lib_common/src_loc.hpp"


namespace U
{

enum class CodeBuilderErrorCode : uint8_t // Make uint16_t if 255 values will be not enough.
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
	SrcLoc context_declaration_src_loc; // Declaration position of context, macro.

	std::string context_name; // Name of template, macro.
	std::string parameters_description;
};
using TemplateErrorsContextPtr= std::shared_ptr<TemplateErrorsContext>;

struct CodeBuilderError
{
	std::string text;
	TemplateErrorsContextPtr template_context; // For errors of type "TemplateContext" or "MacroExpansionContext"
	CodeBuilderErrorCode code;
	SrcLoc src_loc;
};

bool operator==( const CodeBuilderError& l, const CodeBuilderError& r );
inline bool operator!=( const CodeBuilderError& l, const CodeBuilderError& r ) { return !(l == r); }
bool operator< ( const CodeBuilderError& l, const CodeBuilderError& r ); // For sorting, using src_loc

std::string_view CodeBuilderErrorCodeToString( CodeBuilderErrorCode code );

} // namespace U
