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
		const clang::LangOptions& lang_options,
		const clang::ASTContext& ast_context );

public:
	virtual bool HandleTopLevelDecl( clang::DeclGroupRef decl_group ) override;

private:
	void ProcessDecl( const clang::Decl& decl, Synt::ProgramElements& program_elements, bool externc );
	void ProcessClassDecl( const clang::Decl& decl, Synt::ClassElements& class_elements, bool externc );
	Synt::TypeName TranslateType( const clang::Type& in_type );
	ProgramString GetUFundamentalType( const clang::BuiltinType& in_type );
	Synt::NamedTypeName TranslateNamedType( const std::string& cpp_type_name );
	Synt::FunctionTypePtr TranslateFunctionType( const clang::FunctionProtoType& in_type );
	ProgramString TranslateIdentifier( const std::string& identifier );

private:
	Synt::ProgramElements& root_program_elements_;

	const clang::SourceManager& source_manager_;
	const clang::LangOptions& lang_options_;
	const clang::PrintingPolicy printing_policy_;
	const clang::ASTContext& ast_context_;
	size_t unique_name_index_= 0u;
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
