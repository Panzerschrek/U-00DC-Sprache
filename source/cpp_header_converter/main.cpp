#include <iostream>

#include "../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <clang/AST/Attr.h>
#include <clang/AST/DeclBase.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include "../code_builder_lib/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/assert.hpp"

namespace U
{

class CppAstConsumer : public clang::ASTConsumer
{
public:
	virtual bool HandleTopLevelDecl( const clang::DeclGroupRef decl_group ) override
	{
		for( const clang::Decl* const decl : decl_group )
			ProcessDecl( decl );
		return true;
	}

private:
	void ProcessDecl( const clang::Decl* const decl )
	{
		if( const clang::RecordDecl* const record_decl= llvm::dyn_cast<clang::RecordDecl>(decl) )
		{
			if( record_decl->isStruct() || record_decl->isClass() )
				std::cout << "struct " << record_decl->getName().str() << "{};" << std::endl;
		}
		if( const clang::FunctionDecl* const func_decl= llvm::dyn_cast<clang::FunctionDecl>(decl) )
		{
			std::cout << "fn " << func_decl->getName().str() << "( ";
			size_t i= 0u;

			for( const clang::ParmVarDecl* const param : func_decl->parameters() )
			{
				const clang::QualType& type= param->getType();
				std::cout << TranslateNamedType(type.getAsString()) << " " << param->getName().str();
				++i;
				if( i != func_decl->param_size() )
					std::cout << ", ";
			}
			std::cout << " )";

			std::cout << " : " << TranslateNamedType(func_decl->getReturnType().getAsString());
			std::cout << ";" << std::endl;
		}
		if( const clang::NamespaceDecl* const namespace_decl= llvm::dyn_cast<clang::NamespaceDecl>(decl) )
		{
			std::cout << "namespace " << namespace_decl->getName().str() << "\n{\n" << std::endl;
			for( const clang::Decl* const sub_decl : namespace_decl->decls() )
				ProcessDecl( sub_decl );
			std::cout << "\n}" << std::endl;
		}
	}

	std::string TranslateNamedType( const std::string& cpp_type_name )
	{
		static const std::map< std::string, std::string > c_map
		{
			{ "int", "i32" },
			{ "unsigned int", "u32" },
			{ "long int", "i64" },
			{ "long unsigned int", "u64" },
			{ "char", "char8" },
			{ "signed char", "i8" },
			{ "unsigned char", "u8" },
			{ "float", "f32" },
			{ "double", "f64" },
		};

		const auto it= c_map.find(cpp_type_name);
		if( it != c_map.end() )
			return it->second;
		return cpp_type_name;
	}
};

class CppAstProcessor : public clang::ASTFrontendAction
{
public:
	virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
		clang::CompilerInstance &compiler_intance,
		StringRef in_file ) override
	{
		U_UNUSED( compiler_intance );
		U_UNUSED( in_file );
		return std::unique_ptr<clang::ASTConsumer>( new CppAstConsumer() );
	}
};

} // namespace U
int main( int argc, const char* argv[] )
{
	llvm::cl::OptionCategory tool_category("my-tool options");

	clang::tooling::CommonOptionsParser options_parser( argc, argv, tool_category );
	clang::tooling::ClangTool tool( options_parser.getCompilations(), options_parser.getSourcePathList() );
	tool.run(clang::tooling::newFrontendActionFactory<U::CppAstProcessor>().get());;
	return 0;
}
