#pragma once

#include "../lex_synt_lib/syntax_elements.hpp"
#include "names_scope.hpp"
#include "template_signature_param.hpp"

namespace U
{


using TemplateArg= std::variant< Variable, Type >;
using TemplateArgs= std::vector<TemplateArg>;

struct TemplateBase
{
	virtual ~TemplateBase()= default;

	struct TemplateParameter
	{
		std::string name;
		std::optional<TemplateSignatureParam> type; // For variable params.
	};

	std::vector<TemplateParameter> template_params;
	std::vector<TemplateSignatureParam> signature_params; // Function params for function templates.

	// NamesScope, where defined. NOT changed after import or inheritance.
	NamesScope* parent_namespace= nullptr;

	SrcLoc src_loc;
};

struct TypeTemplate final : TemplateBase
{
	size_t first_optional_signature_param= ~0u;

	// Store syntax tree element for instantiation.
	// Syntax tree must live longer, than this struct.
	const Synt::TypeTemplate* syntax_element= nullptr;
};

struct FunctionTemplate final : public TemplateBase
{
	// Store syntax tree element for instantiation.
	// Syntax tree must live longer, than this struct.
	const Synt::FunctionTemplate* syntax_element= nullptr;

	ClassPtr base_class= nullptr;

	// In case of manual parameters specifying, like foo</A, B, C/> we create new template and store known arguments and reference to base template.
	TemplateArgs known_template_args;
	FunctionTemplatePtr parent;
};

} // namespace U
