#pragma once
#include <unordered_map>
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/syntax_elements.hpp"

namespace U
{

using ParsedUnits= std::unordered_map< std::string, Synt::ProgramElementsList >;
using ParsedUnitsPtr= std::shared_ptr<ParsedUnits>;

struct DepFileOptions
{
	std::string out_file;
	std::string out_dep_file;
};

using DepFileOptionsOpt= std::optional<DepFileOptions>;

class FrontendActionFactory final : public clang::tooling::FrontendActionFactory
{
public:
	FrontendActionFactory( ParsedUnitsPtr out_result, DepFileOptionsOpt dep_file_options );

public:
	virtual std::unique_ptr<clang::FrontendAction> create() override;

private:
	const ParsedUnitsPtr out_result_;
	const DepFileOptionsOpt dep_file_options_;
};

} // namespace U
