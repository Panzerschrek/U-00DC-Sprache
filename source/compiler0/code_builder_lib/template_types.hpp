#pragma once

#include "../lex_synt_lib/syntax_elements.hpp"
#include "names_scope.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

using TemplateArg= std::variant< Variable, Type >;
using TemplateArgs= std::vector<TemplateArg>;

struct TemplateBase
{
	virtual ~TemplateBase()= default;

	struct TemplateParameter
	{
		std::string name;
		const Synt::ComplexName* type_name= nullptr; // Exists for value parameters.
	};

	std::vector< TemplateParameter > template_params;
	std::vector< std::optional<DeducedTemplateParameter> > params_types;

	std::vector<DeducedTemplateParameter> signature_params; // Function params for function templates.

	NamesScope* parent_namespace= nullptr; // NamesScope, where defined. NOT changed after import.

	FilePos file_pos;
};

struct TypeTemplate final : TemplateBase
{
	std::vector< const Synt::Expression* > default_signature_params;
	size_t first_optional_signature_param= ~0u;

	enum class Kind
	{
		Class,
		Typedef,
	};

	Kind kind= Kind::Class;
	// Store syntax tree element for instantiation.
	// Syntax tree must live longer, than this struct.
	const Synt::TypeTemplateBase* syntax_element= nullptr;
};

struct FunctionTemplate final : public TemplateBase
{
	// Store syntax tree element for instantiation.
	// Syntax tree must live longer, than this struct.
	const Synt::FunctionTemplate* syntax_element= nullptr;

	ClassProxyPtr base_class;

	// In case of manual parameters specifying, like foo</A, B, C/> we create new template and store known arguments and reference to base template.
	TemplateArgs known_template_args;
	FunctionTemplatePtr parent;
};

} //namespace CodeBuilderPrivate

} // namespace U
