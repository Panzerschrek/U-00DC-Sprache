#pragma once
#include "../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include "../code_builder_lib/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/syntax_elements.hpp"

namespace U
{

class CppAstConsumer : public clang::ASTConsumer
{
public:
	CppAstConsumer(
		Synt::ProgramElements& out_elements,
		const clang::SourceManager& source_manager,
		const clang::LangOptions& lang_options );

public:
	virtual bool HandleTopLevelDecl( clang::DeclGroupRef decl_group ) override;

private:
	void ProcessDecl( const clang::Decl& decl, Synt::ProgramElements& program_elements, bool externc );
	Synt::TypeName TranslateType( const clang::Type& in_type ) const;
	std::string TranslateNamedType( const std::string& cpp_type_name ) const;

private:
	Synt::ProgramElements& root_program_elements_;

	const clang::SourceManager& source_manager_;
	const clang::LangOptions& lang_options_;
	const clang::PrintingPolicy printing_policy_;
};

using ParsedUnits= std::map< std::string, Synt::ProgramElements >;
using ParsedUnitsPtr= std::shared_ptr<ParsedUnits>;

class CppAstProcessor : public clang::ASTFrontendAction
{
public:
	explicit CppAstProcessor( ParsedUnitsPtr out_result );

public:
	virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
		clang::CompilerInstance& compiler_intance,
		llvm::StringRef in_file ) override;

private:
	const ParsedUnitsPtr out_result_;
};


class FrontendActionFactory : public clang::tooling::FrontendActionFactory
{
public:
	explicit FrontendActionFactory( ParsedUnitsPtr out_result );

public:
	virtual clang::FrontendAction* create() override;

private:
	const ParsedUnitsPtr out_result_;
};

} // namespace U
