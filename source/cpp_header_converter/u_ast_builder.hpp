#pragma once
#include <unordered_map>
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/syntax_elements.hpp"

namespace U
{

using ParsedUnits= std::unordered_map< std::string, Synt::ProgramElementsList::Builder >;
using ParsedUnitsPtr= std::shared_ptr<ParsedUnits>;

class FrontendActionFactory : public clang::tooling::FrontendActionFactory
{
public:
	FrontendActionFactory( ParsedUnitsPtr out_result, bool skip_declarations_from_includes );

public:
	virtual std::unique_ptr<clang::FrontendAction> create() override;

private:
	const ParsedUnitsPtr out_result_;
	const bool skip_declarations_from_includes_;
};

} // namespace U
