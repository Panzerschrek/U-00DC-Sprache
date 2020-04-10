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

CodeBuilderErrorsContainer ExpandErrorsInMacros(
	const CodeBuilderErrorsContainer& errors,
	const Synt::MacroExpansionContexts& macro_expanisoin_contexts )
{
	std::vector<TemplateErrorsContextPtr> macro_contexts_internals;
	macro_contexts_internals.reserve( macro_expanisoin_contexts.size() );

	CodeBuilderErrorsContainer macro_contexts_errors;
	macro_contexts_errors.reserve( macro_expanisoin_contexts.size() );

	for( const Synt::MacroExpansionContext& macro_expansion_context : macro_expanisoin_contexts )
	{
		CodeBuilderError macro_context_error;
		macro_context_error.text = "in expansion of macro \"" + macro_expansion_context.macro_name + "\"";
		macro_context_error.file_pos= macro_expansion_context.file_pos;
		macro_context_error.code= CodeBuilderErrorCode::MacroExpansionContext;
		macro_context_error.template_context= std::make_shared<TemplateErrorsContext>();
		macro_context_error.template_context->context_name= macro_expansion_context.macro_name;
		macro_context_error.template_context->context_declaration_file_pos=  macro_expansion_context.macro_declaration_file_pos;

		macro_contexts_internals.push_back( macro_context_error.template_context );
		macro_contexts_errors.push_back(std::move(macro_context_error));
	}

	CodeBuilderErrorsContainer out_errors;
	out_errors.reserve( errors.size() + macro_expanisoin_contexts.size() );
	for( const CodeBuilderError& error : errors )
	{
		const auto macro_expansion_index= error.file_pos.GetMacroExpansionIndex();
		if( macro_expansion_index < macro_contexts_internals.size() )
			macro_contexts_internals[ macro_expansion_index ]->errors.push_back(error);
		else
			out_errors.push_back( error );
	}

	for( CodeBuilderError& macro_context_error : macro_contexts_errors )
	{
		const auto macro_expansion_index= macro_context_error.file_pos.GetMacroExpansionIndex();
		if( macro_expansion_index < macro_contexts_internals.size() )
			macro_contexts_internals[ macro_expansion_index ]->errors.push_back( std::move( macro_context_error ) );
	}

	macro_contexts_errors.erase(
		std::remove_if(
			macro_contexts_errors.begin(), macro_contexts_errors.end(),
			[]( const CodeBuilderError& error ) -> bool
			{
				return error.template_context == nullptr || error.template_context->errors.empty();
			} ),
		macro_contexts_errors.end() );

	out_errors.insert( out_errors.end(), macro_contexts_errors.begin(), macro_contexts_errors.end() );

	NormalizeErrors( out_errors );
	return out_errors;
}

} // namespace U
