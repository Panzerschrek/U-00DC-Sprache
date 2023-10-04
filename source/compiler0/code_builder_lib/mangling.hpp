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
		std::string_view function_name,
		const FunctionType& function_type,
		std::optional<llvm::ArrayRef<TemplateArg>> template_args= std::nullopt ) = 0;
	virtual std::string MangleGlobalVariable( const NamesScope& parent_scope, std::string_view variable_name, const Type& type, bool is_constant ) = 0;
	virtual std::string MangleType( const Type& type ) = 0;
	virtual std::string MangleVirtualTable( const Type& type ) = 0;
};

std::unique_ptr<IMangler> CreateManglerItaniumABI();
std::unique_ptr<IMangler> CreateManglerMSVC(bool is_32_bit);

} // namespace U
