#pragma once

#include "../lex_synt_lib/syntax_elements.hpp"
#include "names_scope.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

struct TemplateBase
{
	virtual ~TemplateBase()= default;

	struct TemplateParameter
	{
		std::string name;
		const Synt::ComplexName* type_name= nullptr; // Exists for value parameters.
	};

	std::vector< TemplateParameter > template_parameters;

	NamesScope* parent_namespace= nullptr; // NamesScope, where defined. NOT changed after import.

	FilePos file_pos;
};

struct TypeTemplate final : TemplateBase
{
	std::vector< const Synt::Expression* > signature_arguments;
	std::vector< const Synt::Expression* > default_signature_arguments;
	size_t first_optional_signature_argument= ~0u;

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

	std::vector< std::pair< std::string, Value > > known_template_parameters;
};

using DeducibleTemplateParameter= std::variant< int, Type, Variable >; // int means not deduced
using DeducibleTemplateParameters= std::vector<DeducibleTemplateParameter>;

} //namespace CodeBuilderPrivate

} // namespace U
