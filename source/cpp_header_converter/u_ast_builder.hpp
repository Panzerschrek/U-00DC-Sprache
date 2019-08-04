#pragma once
#include "../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <clang/Frontend/FrontendActions.h>
#include "../code_builder_lib/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/syntax_elements.hpp"

namespace U
{

class CppAstConsumer : public clang::ASTConsumer
{
public:
	virtual bool HandleTopLevelDecl( clang::DeclGroupRef decl_group ) override;

private:
	void ProcessDecl( const clang::Decl& decl, bool externc );
	std::string TranslateNamedType( const std::string& cpp_type_name ) const;

private:
	Synt::ProgramElements program_elements_;
};

class CppAstProcessor : public clang::ASTFrontendAction
{
public:
	virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
		clang::CompilerInstance& compiler_intance,
		llvm::StringRef in_file ) override;
};

} // namespace U
