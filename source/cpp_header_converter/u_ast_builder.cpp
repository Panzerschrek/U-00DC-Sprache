#include <iostream>

#include "../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <clang/AST/Attr.h>
#include <clang/AST/DeclBase.h>
#include "../code_builder_lib/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/assert.hpp"

#include "u_ast_builder.hpp"

namespace U
{

bool CppAstConsumer::HandleTopLevelDecl( const clang::DeclGroupRef decl_group )
{
	for( const clang::Decl* const decl : decl_group )
		ProcessDecl( *decl, false );
	return true;
}

void CppAstConsumer::ProcessDecl( const clang::Decl& decl, const bool externc )
{
	bool current_externc= externc;
	if( const auto decl_context= llvm::dyn_cast<clang::DeclContext>(&decl) )
	{
		if( decl_context->isExternCContext() )
			current_externc= true;
		if( decl_context->isExternCXXContext() )
			current_externc= false;
	}

	if( const clang::RecordDecl* const record_decl= llvm::dyn_cast<clang::RecordDecl>(&decl) )
	{
		if( record_decl->isStruct() || record_decl->isClass() )
			std::cout << "struct " << record_decl->getName().str() << "{};" << std::endl;
	}
	else if( const clang::FunctionDecl* const func_decl= llvm::dyn_cast<clang::FunctionDecl>(&decl) )
	{
		std::cout << "fn ";
		if( current_externc )
			std::cout << " nomangle ";
		std::cout << func_decl->getName().str();

		std::cout << "( ";
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

		std::cout << " unsafe : " << TranslateNamedType(func_decl->getReturnType().getAsString());
		std::cout << ";" << std::endl;
	}
	else if( const clang::NamespaceDecl* const namespace_decl= llvm::dyn_cast<clang::NamespaceDecl>(&decl) )
	{
		std::cout << "namespace " << namespace_decl->getName().str() << "\n{\n" << std::endl;
		for( const clang::Decl* const sub_decl : namespace_decl->decls() )
			ProcessDecl( *sub_decl, current_externc );
		std::cout << "\n}" << std::endl;
	}
	else if( const clang::DeclContext* const decl_context= llvm::dyn_cast<clang::DeclContext>(&decl) )
	{
		for( const clang::Decl* const sub_decl : decl_context->decls() )
			ProcessDecl( *sub_decl, current_externc );
	}
}

std::string CppAstConsumer::TranslateNamedType( const std::string& cpp_type_name ) const
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

std::unique_ptr<clang::ASTConsumer> CppAstProcessor::CreateASTConsumer(
	clang::CompilerInstance& compiler_intance,
	const llvm::StringRef in_file )
{
	U_UNUSED( compiler_intance );
	U_UNUSED( in_file );
	return std::unique_ptr<clang::ASTConsumer>( new CppAstConsumer() );
}

} // namespace U
