#pragma once
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/syntax_elements.hpp"

namespace U
{

class CppAstConsumer : public clang::ASTConsumer
{
public:
	CppAstConsumer(
		Synt::ProgramElements& out_elements,
		clang::Preprocessor& preprocessor,
		const clang::LangOptions& lang_options,
		const clang::ASTContext& ast_context );

public:
	virtual bool HandleTopLevelDecl( clang::DeclGroupRef decl_group ) override;
	virtual void HandleTranslationUnit( clang:: ASTContext& ast_context ) override;

private:
	void ProcessDecl( const clang::Decl& decl, Synt::ProgramElements& program_elements, bool externc );
	void ProcessClassDecl( const clang::Decl& decl, Synt::ClassElements& class_elements, bool externc );

	Synt::ClassPtr ProcessRecord( const clang::RecordDecl& record_decl, bool externc );
	Synt::TypeAlias ProcessTypedef( const clang::TypedefNameDecl& typedef_decl );
	Synt::FunctionPtr ProcessFunction( const clang::FunctionDecl& func_decl, bool externc );
	void ProcessEnum( const clang::EnumDecl& enum_decl, Synt::ProgramElements& out_elements );

	Synt::TypeName TranslateType( const clang::Type& in_type );
	std::string TranslateRecordType( const clang::RecordType& in_type );
	std::string GetUFundamentalType( const clang::BuiltinType& in_type );
	Synt::ComplexName TranslateNamedType( const std::string& cpp_type_name );
	Synt::FunctionTypePtr TranslateFunctionType( const clang::FunctionProtoType& in_type );

	std::string TranslateIdentifier( llvm::StringRef identifier );

private:
	Synt::ProgramElements& root_program_elements_;

	clang::Preprocessor& preprocessor_;
	const clang::LangOptions& lang_options_;
	const clang::PrintingPolicy printing_policy_;
	const clang::ASTContext& ast_context_;

	size_t unique_name_index_= 0u;
	std::unordered_map< const clang::RecordType*, std::string > anon_records_names_cache_;
	std::unordered_map< const clang::EnumDecl*, std::string > enum_names_cache_;
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
	virtual std::unique_ptr<clang::FrontendAction> create() override;

private:
	const ParsedUnitsPtr out_result_;
};

} // namespace U
