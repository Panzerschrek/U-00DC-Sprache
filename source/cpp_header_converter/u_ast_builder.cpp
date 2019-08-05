#include <iostream>

#include "../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <clang/AST/Attr.h>
#include <clang/AST/DeclBase.h>
#include <clang/Frontend/CompilerInstance.h>
#include "../code_builder_lib/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/assert.hpp"

#include "u_ast_builder.hpp"

namespace U
{

static const FilePos g_dummy_file_pos{ 0u, 0u, 0u };

CppAstConsumer::CppAstConsumer(
	Synt::ProgramElements& out_elements,
	const clang::SourceManager& source_manager,
	const clang::LangOptions& lang_options )
	: root_program_elements_(out_elements)
	, source_manager_(source_manager)
	, lang_options_(lang_options)
	, printing_policy_(lang_options_)
{}

bool CppAstConsumer::HandleTopLevelDecl( const clang::DeclGroupRef decl_group )
{
	for( const clang::Decl* const decl : decl_group )
		ProcessDecl( *decl, root_program_elements_, false );
	return true;
}

void CppAstConsumer::ProcessDecl( const clang::Decl& decl, Synt::ProgramElements& program_elements, const bool externc )
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
		{
			Synt::ClassPtr class_( new Synt::Class(g_dummy_file_pos) );
			class_->name_= ToProgramString( record_decl->getName().data() );
			class_->keep_fields_order_= true; // C/C++ structs/classes have fixed fields order.

			for( const clang::Decl* const sub_decl : record_decl->decls() )
				ProcessClassDecl( *sub_decl, class_->elements_, current_externc );

			program_elements.push_back( std::move(class_) );
		}
	}
	else if( const clang::FunctionDecl* const func_decl= llvm::dyn_cast<clang::FunctionDecl>(&decl) )
	{
		Synt::FunctionPtr func( new Synt::Function(g_dummy_file_pos) );

		func->name_.components.emplace_back();
		func->name_.components.back().name= ToProgramString( func_decl->getName().str() );
		func->no_mangle_= current_externc;
		func->type_.unsafe_= true; // All C/C++ functions is unsafe.

		func->type_.arguments_.reserve( func_decl->param_size() );
		size_t i= 0u;
		for( const clang::ParmVarDecl* const param : func_decl->parameters() )
		{
			Synt::FunctionArgument arg( g_dummy_file_pos );
			arg.name_= ToProgramString( param->getName().str() );
			if( arg.name_.empty() )
				arg.name_= ToProgramString( "arg" + std::to_string(i) );


			const clang::Type* arg_type= param->getType().getTypePtr();
			if( arg_type->isReferenceType() )
			{
				arg.reference_modifier_= Synt::ReferenceModifier::Reference;
				arg_type= arg_type->getPointeeType().getTypePtr();

				if( param->getType().isConstQualified() )
					arg.mutability_modifier_= Synt::MutabilityModifier::Immutable;
				else
					arg.mutability_modifier_= Synt::MutabilityModifier::Mutable;
			}
			else if( arg_type->isPointerType() )
			{
				arg.reference_modifier_= Synt::ReferenceModifier::Reference;
				const clang::QualType type_qual= arg_type->getPointeeType();
				arg_type= type_qual.getTypePtr();

				if( type_qual.isConstQualified() )
					arg.mutability_modifier_= Synt::MutabilityModifier::Immutable;
				else
					arg.mutability_modifier_= Synt::MutabilityModifier::Mutable;
			}

			arg.type_= TranslateType( *arg_type );
			func->type_.arguments_.push_back(std::move(arg));
			++i;
		}

		const clang::Type* return_type= func_decl->getReturnType().getTypePtr();
		if( return_type->isReferenceType() )
		{
			func->type_.return_value_reference_modifier_= Synt::ReferenceModifier::Reference;
			return_type= return_type->getPointeeType().getTypePtr();

			if( func_decl->getReturnType().isConstQualified() )
				func->type_.return_value_mutability_modifier_= Synt::MutabilityModifier::Immutable;
			else
				func->type_.return_value_mutability_modifier_= Synt::MutabilityModifier::Mutable;
		}
		else if( return_type->isPointerType() )
		{
			func->type_.return_value_reference_modifier_= Synt::ReferenceModifier::Reference;
			const clang::QualType type_qual= return_type->getPointeeType();
			return_type= type_qual.getTypePtr();

			if( type_qual.isConstQualified() )
				func->type_.return_value_mutability_modifier_= Synt::MutabilityModifier::Immutable;
			else
				func->type_.return_value_mutability_modifier_= Synt::MutabilityModifier::Mutable;
		}
		func->type_.return_type_.reset( new Synt::TypeName( TranslateType( *return_type ) ) );

		program_elements.push_back(std::move(func));
	}
	else if( const clang::NamespaceDecl* const namespace_decl= llvm::dyn_cast<clang::NamespaceDecl>(&decl) )
	{
		Synt::NamespacePtr namespace_( new Synt::Namespace( g_dummy_file_pos ) );
		namespace_->name_= ToProgramString( namespace_decl->getName() );
		for( const clang::Decl* const sub_decl : namespace_decl->decls() )
			ProcessDecl( *sub_decl, namespace_->elements_, current_externc );

		program_elements.push_back(std::move(namespace_));
	}
	else if( const clang::DeclContext* const decl_context= llvm::dyn_cast<clang::DeclContext>(&decl) )
	{
		for( const clang::Decl* const sub_decl : decl_context->decls() )
			ProcessDecl( *sub_decl, program_elements, current_externc );
	}
}

void CppAstConsumer::ProcessClassDecl( const clang::Decl& decl, Synt::ClassElements& class_elements, bool externc )
{
	U_UNUSED(externc);
	if( const clang::FieldDecl* const field_decl= llvm::dyn_cast<clang::FieldDecl>(&decl) )
	{
		Synt::ClassField field( g_dummy_file_pos );

		field.type= TranslateType( *field_decl->getType().getTypePtr() );
		field.name= ToProgramString( field_decl->getName().str() );
		class_elements.push_back( std::move(field) );
	}
}

Synt::TypeName CppAstConsumer::TranslateType( const clang::Type& in_type ) const
{
	if( const clang::BuiltinType* const build_in_type= llvm::dyn_cast<clang::BuiltinType>(&in_type) )
		return TranslateNamedType( build_in_type->getNameAsCString( printing_policy_ ) );
	else if( const clang::RecordType* const record_type= llvm::dyn_cast<clang::RecordType>(&in_type) )
		return TranslateNamedType( record_type->getDecl()->getName().str() );
	else if( const clang::TypedefType* const typedef_type= llvm::dyn_cast<clang::TypedefType>(&in_type) )
		return TranslateNamedType( typedef_type->getDecl()->getName().str() );
	else if( const clang::ConstantArrayType* const constna_array_type= llvm::dyn_cast<clang::ConstantArrayType>(&in_type) )
	{
		// For arrays with constant size use normal Ü array.
		Synt::ArrayTypeName array_type(g_dummy_file_pos);
		array_type.element_type.reset( new Synt::TypeName( TranslateType( *constna_array_type->getElementType().getTypePtr() ) ) );

		Synt::NumericConstant numeric_constant( g_dummy_file_pos );
		numeric_constant.value_= static_cast<Synt::NumericConstant::LongFloat>( constna_array_type->getSize().getLimitedValue() );
		numeric_constant.type_suffix_[0]= 'u';
		array_type.size.reset( new Synt::Expression( std::move(numeric_constant) ) );

		return std::move(array_type);
	}
	else if( const clang::ArrayType* const array_type= llvm::dyn_cast<clang::ArrayType>(&in_type) )
	{
		// For other variants of array types use zero size.
		Synt::ArrayTypeName out_array_type(g_dummy_file_pos);
		out_array_type.element_type.reset( new Synt::TypeName( TranslateType( *array_type->getElementType().getTypePtr() ) ) );

		Synt::NumericConstant numeric_constant( g_dummy_file_pos );
		numeric_constant.value_= 0;
		numeric_constant.type_suffix_[0]= 'u';
		out_array_type.size.reset( new Synt::Expression( std::move(numeric_constant) ) );

		return std::move(out_array_type);
	}
	else if( in_type.isFunctionPointerType() )
	{
		const clang::Type* function_type= in_type.getPointeeType().getTypePtr();
		while( const clang::ParenType* const paren_type= llvm::dyn_cast<clang::ParenType>( function_type ) )
			function_type= paren_type->getInnerType().getTypePtr();

		if( const clang::FunctionProtoType* const function_proto_type= llvm::dyn_cast<clang::FunctionProtoType>( function_type ) )
			return TranslateFunctionType( *function_proto_type );
	}
	else if( const clang::ParenType* const paren_type= llvm::dyn_cast<clang::ParenType>( &in_type ) )
		return TranslateType( *paren_type->getInnerType().getTypePtr() );

	return TranslateNamedType( "void" );
}

Synt::NamedTypeName CppAstConsumer::TranslateNamedType( const std::string& cpp_type_name ) const
{
	static const std::map< std::string, std::string > c_map
	{
		{ "signed char"      ,  "i8" },
		{ "int8_t"           ,  "i8" },
		{ "unsigned char"    ,  "u8" },
		{ "uint8_t"          ,  "i8" },
		{ "sort"             , "i16" },
		{ "int16_t"          , "i16" },
		{ "unsigned short"   , "u16" },
		{ "uint16_t"         , "u16" },
		{ "int"              , "i32" },
		{ "int32_t"          , "i32" },
		{ "unsigned int"     , "u32" },
		{ "uint32_t"         , "u32" },
		{ "long int"         , "i64" },
		{ "int64_t"          , "i64" },
		{ "long unsigned int", "u64" },
		{ "uint64_t"         , "u64" },
		{ "char"  , "char8" },
		{ "float" ,  "f32" },
		{ "double",  "f64" },
	};

	Synt::NamedTypeName named_type(g_dummy_file_pos);
	named_type.name.components.emplace_back();

	const auto it= c_map.find(cpp_type_name);
	if( it != c_map.end() )
		named_type.name.components.back().name= ToProgramString( it->second );
	else
		named_type.name.components.back().name= ToProgramString( cpp_type_name );

	return std::move(named_type);
}

Synt::FunctionTypePtr CppAstConsumer::TranslateFunctionType( const clang::FunctionProtoType& in_type ) const
{
	Synt::FunctionTypePtr function_type( new Synt::FunctionType( g_dummy_file_pos ) );

	function_type->unsafe_= true; // All C/C++ functions is unsafe.

	function_type->arguments_.reserve( in_type.getNumParams() );
	size_t i= 0u;
	for( const clang::QualType& param_qual : in_type.getParamTypes() )
	{
		Synt::FunctionArgument arg( g_dummy_file_pos );
		arg.name_= ToProgramString( "arg" + std::to_string(i) );

		const clang::Type* arg_type= param_qual.getTypePtr();
		if( arg_type->isReferenceType() )
		{
			arg.reference_modifier_= Synt::ReferenceModifier::Reference;
			arg_type= arg_type->getPointeeType().getTypePtr();

			if( param_qual.isConstQualified() )
				arg.mutability_modifier_= Synt::MutabilityModifier::Immutable;
			else
				arg.mutability_modifier_= Synt::MutabilityModifier::Mutable;
		}
		else if( arg_type->isPointerType() )
		{
			arg.reference_modifier_= Synt::ReferenceModifier::Reference;
			const clang::QualType type_qual= arg_type->getPointeeType();
			arg_type= type_qual.getTypePtr();

			if( type_qual.isConstQualified() )
				arg.mutability_modifier_= Synt::MutabilityModifier::Immutable;
			else
				arg.mutability_modifier_= Synt::MutabilityModifier::Mutable;
		}

		arg.type_= TranslateType( *arg_type );
		function_type->arguments_.push_back(std::move(arg));
		++i;
	}

	const clang::Type* return_type= in_type.getReturnType().getTypePtr();
	if( return_type->isReferenceType() )
	{
		function_type->return_value_reference_modifier_= Synt::ReferenceModifier::Reference;
		return_type= return_type->getPointeeType().getTypePtr();

		if( in_type.getReturnType().isConstQualified() )
			function_type->return_value_mutability_modifier_= Synt::MutabilityModifier::Immutable;
		else
			function_type->return_value_mutability_modifier_= Synt::MutabilityModifier::Mutable;
	}
	else if( return_type->isPointerType() )
	{
		function_type->return_value_reference_modifier_= Synt::ReferenceModifier::Reference;
		const clang::QualType type_qual= return_type->getPointeeType();
		return_type= type_qual.getTypePtr();

		if( type_qual.isConstQualified() )
			function_type->return_value_mutability_modifier_= Synt::MutabilityModifier::Immutable;
		else
			function_type->return_value_mutability_modifier_= Synt::MutabilityModifier::Mutable;
	}
	function_type->return_type_.reset( new Synt::TypeName( TranslateType( *return_type ) ) );

	return std::move(function_type);
}

CppAstProcessor::CppAstProcessor( ParsedUnitsPtr out_result )
	: out_result_(std::move(out_result))
{}

std::unique_ptr<clang::ASTConsumer> CppAstProcessor::CreateASTConsumer(
	clang::CompilerInstance& compiler_intance,
	const llvm::StringRef in_file )
{
	return
		std::unique_ptr<clang::ASTConsumer>(
			new CppAstConsumer(
				(*out_result_)[in_file.str()],
				compiler_intance.getSourceManager(),
				compiler_intance.getLangOpts()) );
}

FrontendActionFactory::FrontendActionFactory( ParsedUnitsPtr out_result )
	: out_result_(std::move(out_result))
{}

clang::FrontendAction* FrontendActionFactory::create()
{
	return new CppAstProcessor(out_result_);
}

} // namespace U
