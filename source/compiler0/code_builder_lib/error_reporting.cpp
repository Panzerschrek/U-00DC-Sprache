#include <sstream>
#include "../../lex_synt_lib_common/assert.hpp"
#include "../lex_synt_lib/program_writer.hpp"
#include "keywords.hpp"
#include "type.hpp"
#include "error_reporting.hpp"

namespace U
{

namespace
{

void RemoveEmptyErrorsContexts_r( CodeBuilderErrorsContainer& errors )
{
	for( size_t i= 0u; i < errors.size(); )
	{
		if( errors[i].template_context != nullptr )
		{
			RemoveEmptyErrorsContexts_r( errors[i].template_context->errors );
			if( errors[i].template_context->errors.empty() )
			{
				if( i + 1u < errors.size() )
					errors[i]= std::move(errors.back());
				errors.pop_back();
				continue;
			}
		}
		++i;
	}
}

void SortErrorsAndRemoveDuplicates_r( CodeBuilderErrorsContainer& errors )
{
	// Soprt by file/line and left only unique error messages.
	std::sort( errors.begin(), errors.end() );
	errors.erase( std::unique( errors.begin(), errors.end() ), errors.end() );

	for( const CodeBuilderError& error : errors )
	{
		if( error.template_context != nullptr )
			SortErrorsAndRemoveDuplicates_r( error.template_context->errors );
	}
}

CodeBuilderErrorsContainer ExpandErrorsInMacros_r(
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

		if( macro_expansion_context.macro_name == Keyword( Keywords::mixin_ ) )
			macro_context_error.text = "in expansion of mixin";
		else
			macro_context_error.text = "in expansion of macro \"" + macro_expansion_context.macro_name + "\"";

		macro_context_error.src_loc= macro_expansion_context.src_loc;
		macro_context_error.code= CodeBuilderErrorCode::MacroExpansionContext;
		macro_context_error.template_context= std::make_shared<TemplateErrorsContext>();
		macro_context_error.template_context->context_name= macro_expansion_context.macro_name;
		macro_context_error.template_context->context_declaration_src_loc= macro_expansion_context.macro_declaration_src_loc;

		macro_contexts_internals.push_back( macro_context_error.template_context );
		macro_contexts_errors.push_back(std::move(macro_context_error));
	}

	CodeBuilderErrorsContainer out_errors;
	out_errors.reserve( errors.size() + macro_expanisoin_contexts.size() );
	for( CodeBuilderError error : errors )
	{
		if( error.template_context != nullptr && !error.template_context->errors.empty() )
			error.template_context->errors= ExpandErrorsInMacros_r( error.template_context->errors, macro_expanisoin_contexts );

		const auto macro_expansion_index= error.src_loc.GetMacroExpansionIndex();
		if( macro_expansion_index < macro_contexts_internals.size() )
			macro_contexts_internals[ macro_expansion_index ]->errors.push_back( std::move(error) );
		else
			out_errors.push_back( std::move(error) );
	}

	for( CodeBuilderError macro_context_error : macro_contexts_errors )
	{
		const auto macro_expansion_index= macro_context_error.src_loc.GetMacroExpansionIndex();
		if( macro_expansion_index < macro_contexts_internals.size() )
			macro_contexts_internals[ macro_expansion_index ]->errors.push_back( std::move( macro_context_error ) );
		else
			out_errors.push_back( std::move( macro_context_error ) );
	}

	return out_errors;
}

} // namespace

CodeBuilderErrorsContainer NormalizeErrors(
	const CodeBuilderErrorsContainer& errors,
	const Synt::MacroExpansionContexts& macro_expanisoin_contexts )
{
	// Remove empty template contexts, than, expand errors in macros.
	// This prevents creation of huge amount of macro contexts.

	CodeBuilderErrorsContainer out_errors= errors;
	RemoveEmptyErrorsContexts_r( out_errors );

	out_errors= ExpandErrorsInMacros_r( out_errors, macro_expanisoin_contexts );

	RemoveEmptyErrorsContexts_r( out_errors );
	SortErrorsAndRemoveDuplicates_r( out_errors );

	return out_errors;
}

namespace ErrorReportingImpl
{

const char* GetErrorMessagePattern( const CodeBuilderErrorCode code )
{
	switch(code)
	{
	#define PROCESS_ERROR(Code, Message) case CodeBuilderErrorCode::Code: return Message;
	#include "../../errors_list.hpp"
	#undef PROCESS_ERROR
	};

	U_ASSERT(false);
	return "";
}

std::string PreprocessArg( const Type& type )
{
	return type.ToString();
}

std::string PreprocessArg( const Synt::ComplexName& name )
{
	std::stringstream ss;

	WriteProgramElement( name, ss );

	return ss.str();
}

std::string PreprocessArg( const Synt::TypeName& name )
{
	std::stringstream ss;

	WriteTypeName( name, ss );

	return ss.str();
}

} // namespace ErrorReportingImpl

} // namespace U
