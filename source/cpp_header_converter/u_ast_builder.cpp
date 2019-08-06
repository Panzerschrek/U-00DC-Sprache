#include <iostream>

#include "../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <clang/AST/Attr.h>
#include <clang/AST/DeclBase.h>
#include <clang/Frontend/CompilerInstance.h>
#include "../code_builder_lib/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"

#include "u_ast_builder.hpp"

namespace U
{

static const FilePos g_dummy_file_pos{ 0u, 0u, 0u };

CppAstConsumer::CppAstConsumer(
	Synt::ProgramElements& out_elements,
	const clang::SourceManager& source_manager,
	const clang::LangOptions& lang_options,
	const clang::ASTContext& ast_context )
	: root_program_elements_(out_elements)
	, source_manager_(source_manager)
	, lang_options_(lang_options)
	, printing_policy_(lang_options_)
	, ast_context_(ast_context)
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
		Synt::ClassPtr record= ProcessRecord( *record_decl, current_externc );
		if( record != nullptr )
			program_elements.push_back( std::move(record) );
	}
	else if( const clang::TypedefNameDecl* const type_alias_decl= llvm::dyn_cast<clang::TypedefNameDecl>(&decl) )
	{
		Synt::Typedef typedef_( g_dummy_file_pos );
		typedef_.name= TranslateIdentifier( type_alias_decl->getName().str() );
		typedef_.value= TranslateType( *type_alias_decl->getUnderlyingType().getTypePtr() );

		program_elements.push_back( std::move(typedef_) );
	}
	else if( const clang::FunctionDecl* const func_decl= llvm::dyn_cast<clang::FunctionDecl>(&decl) )
	{
		Synt::FunctionPtr func( new Synt::Function(g_dummy_file_pos) );

		func->name_.components.emplace_back();
		func->name_.components.back().name= TranslateIdentifier( func_decl->getName().str() );
		func->no_mangle_= current_externc;
		func->type_.unsafe_= true; // All C/C++ functions is unsafe.

		func->type_.arguments_.reserve( func_decl->param_size() );
		size_t i= 0u;
		for( const clang::ParmVarDecl* const param : func_decl->parameters() )
		{
			Synt::FunctionArgument arg( g_dummy_file_pos );
			arg.name_= TranslateIdentifier( param->getName().str() );
			if( arg.name_.empty() )
				arg.name_= ToProgramString( "arg" + std::to_string(i) );

			const clang::Type* arg_type= param->getType().getTypePtr();
			if( arg_type->isPointerType() || arg_type->isReferenceType() )
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
		if( return_type->isPointerType() || return_type->isReferenceType() )
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
		namespace_->name_= TranslateIdentifier( namespace_decl->getName() );
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
	if( decl.isImplicit() )
		return;

	if( const clang::FieldDecl* const field_decl= llvm::dyn_cast<clang::FieldDecl>(&decl) )
	{
		Synt::ClassField field( g_dummy_file_pos );

		const clang::Type* field_type= field_decl->getType().getTypePtr();

		if( field_type->isPointerType() || field_type->isReferenceType() )
		{
			field.reference_modifier= Synt::ReferenceModifier::Reference;
			const clang::QualType type_qual= field_type->getPointeeType();
			field_type= type_qual.getTypePtr();

			if( type_qual.isConstQualified() )
				field.mutability_modifier= Synt::MutabilityModifier::Immutable;
			else
				field.mutability_modifier= Synt::MutabilityModifier::Mutable;
		}

		field.type= TranslateType( *field_type );
		field.name= TranslateIdentifier( field_decl->getName().str() );

		class_elements.push_back( std::move(field) );
	}
	else if( const clang::RecordDecl* const record_decl= llvm::dyn_cast<clang::RecordDecl>(&decl) )
	{
		Synt::ClassPtr record= ProcessRecord( *record_decl, externc );
		if( record != nullptr )
			class_elements.push_back( std::move(record) );
	}
}

Synt::ClassPtr CppAstConsumer::ProcessRecord( const clang::RecordDecl& record_decl, const bool externc )
{
	if( record_decl.isStruct() || record_decl.isClass() )
	{
		Synt::ClassPtr class_( new Synt::Class(g_dummy_file_pos) );
		class_->name_= TranslateRecordType( *llvm::dyn_cast<clang::RecordType>( record_decl.getTypeForDecl() ) );
		class_->keep_fields_order_= true; // C/C++ structs/classes have fixed fields order.

		for( const clang::Decl* const sub_decl : record_decl.decls() )
			ProcessClassDecl( *sub_decl, class_->elements_, externc );

		return std::move(class_);
	}
	else if( record_decl.isUnion() )
	{
		// Emulate union, using array if ints with maximum alignment.

		Synt::ClassPtr class_( new Synt::Class(g_dummy_file_pos) );
		class_->name_= TranslateRecordType( *llvm::dyn_cast<clang::RecordType>( record_decl.getTypeForDecl() ) );
		class_->keep_fields_order_= true; // C/C++ structs/classes have fixed fields order.

		const auto size= ast_context_.getTypeSize( record_decl.getTypeForDecl() ) / 8u;
		const auto int_size= 8u;
		const auto num= ( size + int_size - 1u ) / int_size;

		Synt::ClassField field( g_dummy_file_pos );
		field.name= "union_content"_SpC;

		Synt::ArrayTypeName array_type( g_dummy_file_pos );
		array_type.element_type.reset( new Synt::TypeName( TranslateNamedType( KeywordAscii( Keywords::u64_ ) ) ) );

		Synt::NumericConstant numeric_constant( g_dummy_file_pos );
		numeric_constant.value_= num;
		array_type.size.reset( new Synt::Expression( std::move(numeric_constant) ) );

		field.type= std::move(array_type);
		class_->elements_.push_back( std::move(field) );

		return std::move(class_);
	}

	return nullptr;
}

Synt::TypeName CppAstConsumer::TranslateType( const clang::Type& in_type )
{
	if( const clang::BuiltinType* const built_in_type= llvm::dyn_cast<clang::BuiltinType>(&in_type) )
	{
		Synt::NamedTypeName named_type(g_dummy_file_pos);
		named_type.name.components.emplace_back();
		named_type.name.components.back().name= GetUFundamentalType( *built_in_type );
		return std::move(named_type);
	}
	else if( const clang::RecordType* const record_type= llvm::dyn_cast<clang::RecordType>(&in_type) )
	{
		Synt::NamedTypeName named_type(g_dummy_file_pos);
		named_type.name.components.emplace_back();
		named_type.name.components.back().name= TranslateRecordType( *record_type );
		return std::move(named_type);
	}
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
	else if( in_type.isPointerType() )
	{
		// Ü does not spports pointers. Use int with size of pointer.
		return TranslateNamedType( KeywordAscii( Keywords::size_type_ ) );
	}
	else if( const clang::DecltypeType* decltype_type= llvm::dyn_cast<clang::DecltypeType>( &in_type ) )
		return TranslateType( *decltype_type->desugar().getTypePtr() );
	else if( const clang::ParenType* const paren_type= llvm::dyn_cast<clang::ParenType>( &in_type ) )
		return TranslateType( *paren_type->getInnerType().getTypePtr() );
	else if( const clang::ElaboratedType* const elaborated_type= llvm::dyn_cast<clang::ElaboratedType>( &in_type ) )
		return TranslateType( *elaborated_type->desugar().getTypePtr() );

	return TranslateNamedType( "void" );
}

ProgramString CppAstConsumer::TranslateRecordType( const clang::RecordType& in_type )
{
	const std::string name= in_type.getDecl()->getName().str();
	if( name.empty() )
	{
		const auto it= anon_records_names_cache_.find( &in_type );
		if( it != anon_records_names_cache_.end() )
			return it->second;
		else
		{
			const ProgramString& anon_name= DecodeUTF8( "anon_record_" ) + ToProgramString( std::to_string( ++unique_name_index_ ) );
			anon_records_names_cache_[ &in_type ]= anon_name;
			return anon_name;
		}
	}
	else
		return TranslateIdentifier( name );
}

ProgramString CppAstConsumer::GetUFundamentalType( const clang::BuiltinType& in_type )
{
	switch( in_type.getKind() )
	{
	case clang::BuiltinType::Void: return Keyword( Keywords::void_ );
	case clang::BuiltinType::Bool: return Keyword( Keywords::bool_ );

	//case clang::BuiltinType::Char  : return Keyword( Keywords::char8_ );
	case clang::BuiltinType::Char_S: return Keyword( Keywords::char8_ );
	case clang::BuiltinType::Char_U: return Keyword( Keywords::char8_ );
	case clang::BuiltinType::Char16: return Keyword( Keywords::char16_ );
	case clang::BuiltinType::Char32: return Keyword( Keywords::char32_ );

	case clang::BuiltinType::NullPtr: return Keyword( Keywords::size_type_ );

	default:
		const auto size= ast_context_.getTypeSize( &in_type );
		if( in_type.isFloatingPoint() )
		{
			if( size == 32 )
				return Keyword( Keywords::f32_ );
			else
				return Keyword( Keywords::f64_ );
		}
		if( in_type.isSignedInteger() )
		{
			if( size ==  8 ) return Keyword( Keywords:: i8_ );
			if( size == 16 ) return Keyword( Keywords::i16_ );
			if( size == 32 ) return Keyword( Keywords::i32_ );
			if( size == 64 ) return Keyword( Keywords::i64_ );
			return Keyword( Keywords::i64_ );
		}
		if( in_type.isUnsignedInteger() )
		{
			if( size ==  8 ) return Keyword( Keywords:: u8_ );
			if( size == 16 ) return Keyword( Keywords::u16_ );
			if( size == 32 ) return Keyword( Keywords::u32_ );
			if( size == 64 ) return Keyword( Keywords::u64_ );
			return Keyword( Keywords::u64_ );
		}
		std::cout << "is hz " << std::endl;
		return Keyword( Keywords::void_ );
	};
}

Synt::NamedTypeName CppAstConsumer::TranslateNamedType( const std::string& cpp_type_name )
{
	Synt::NamedTypeName named_type(g_dummy_file_pos);
	named_type.name.components.emplace_back();
	named_type.name.components.back().name= TranslateIdentifier( cpp_type_name );

	return std::move(named_type);
}

Synt::FunctionTypePtr CppAstConsumer::TranslateFunctionType( const clang::FunctionProtoType& in_type )
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
		if( arg_type->isPointerType() || arg_type->isReferenceType() )
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
	if( return_type->isPointerType() || return_type->isReferenceType() )
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

ProgramString CppAstConsumer::TranslateIdentifier( const std::string& identifier )
{
	// For case of errors or something anonimous, generate unqiue identifier.
	if( identifier.empty() )
		return ToProgramString( "ident" + std::to_string( ++unique_name_index_ ) );
	// In Ü identifier can not start with "_", shadow it. "_" in C++ used for impl identiferes, so, it may not needed.
	else if( identifier[0] == '_' )
		return DecodeUTF8("ü") + ToProgramString( identifier );
	return ToProgramString( identifier );
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
				compiler_intance.getLangOpts(),
				compiler_intance.getASTContext() ) );
}

FrontendActionFactory::FrontendActionFactory( ParsedUnitsPtr out_result )
	: out_result_(std::move(out_result))
{}

clang::FrontendAction* FrontendActionFactory::create()
{
	return new CppAstProcessor(out_result_);
}

} // namespace U
