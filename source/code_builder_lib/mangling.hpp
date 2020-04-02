#pragma once
#include "class.hpp"
#include "names_scope.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

// Mangling with Itanium ABI rules.

std::string MangleFunction(
	const NamesScope& parent_scope,
	const std::string& function_name,
	const Function& function_type,
	const std::vector<TemplateParameter>* template_parameters= nullptr);

std::string MangleGlobalVariable(
	const NamesScope& parent_scope,
	const std::string& variable_name );

std::string MangleType( const Type& type );

} // namespace CodeBuilderPrivate

} // namespace U
