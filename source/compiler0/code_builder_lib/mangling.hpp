#pragma once
#include "class.hpp"
#include "names_scope.hpp"

namespace U
{

class IMangler
{
public:
	virtual ~IMangler()= default;

	virtual std::string MangleFunction(
		const NamesScope& parent_scope,
		const std::string& function_name,
		const FunctionType& function_type,
		const TemplateArgs* template_args= nullptr ) = 0;
	virtual std::string MangleGlobalVariable( const NamesScope& parent_scope, const std::string& variable_name, const Type& type, bool is_constant ) = 0;
	virtual std::string MangleType( const Type& type ) = 0;
	virtual std::string MangleTemplateArgs( const TemplateArgs& template_args ) = 0;
	virtual std::string MangleVirtualTable( const Type& type ) = 0;
};

std::unique_ptr<IMangler> CreateManglerItaniumABI();
std::unique_ptr<IMangler> CreateManglerMSVC(bool is_32_bit);

} // namespace U
