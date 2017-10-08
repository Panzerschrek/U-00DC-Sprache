#pragma once
#include "code_builder_types.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

// Mangling with Itanium ABI rules.

std::string MangleFunction(
	const NamesScope& parent_scope,
	const ProgramString& function_name,
	const Function& function_type,
	bool is_this_call_method );

std::string MangleGlobalVariable(
	const NamesScope& parent_scope,
	const ProgramString& variable_name );

std::string MangleType( const Type& type );

} // namespace CodeBuilderPrivate

} // namespace U
