#include "../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <clang/AST/Attr.h>
#include <clang/AST/DeclBase.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/LiteralSupport.h>
#include <clang/Lex/Preprocessor.h>
#include "../code_builder_lib/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"

#include "u_ast_builder.hpp"

namespace U
{

static const FilePos g_dummy_file_pos{ 0u, 0u, 0u };

CppAstConsumer::CppAstConsumer(
	Synt::ProgramElements& out_elements,
	clang::Preprocessor& preprocessor,
	const clang::LangOptions& lang_options,
	const clang::ASTContext& ast_context )
	: root_program_elements_(out_elements)
	, preprocessor_(preprocessor)
	, lang_options_(lang_options)
	, printing_policy_(lang_options_)
	, ast_context_(ast_context)
{}

bool CppAstConsumer::HandleTopLevelDecl( const clang::DeclGroupRef decl_group )
{
	const bool externc= !lang_options_.CPlusPlus;
	for( const clang::Decl* const decl : decl_group )
		ProcessDecl( *decl, root_program_elements_, externc );
	return true;
}

void CppAstConsumer::HandleTranslationUnit( clang::ASTContext& ast_context )
{
	U_UNUSED(ast_context);

	// Dump definitions of simple constants, using "define".
	for( const clang::Preprocessor::macro_iterator::value_type& macro_pair : preprocessor_.macros() )
	{
		const clang::IdentifierInfo* ident_info= macro_pair.first;

		const std::string name= ident_info->getName().str();
		if( name.empty() )
			continue;
		if( preprocessor_.getPredefines().find( "#define " + name ) != std::string::npos )
			continue;

		const clang::MacroDirective* const macro_directive= macro_pair.second.getLatest();
		if( macro_directive->getKind() != clang::MacroDirective::MD_Define )
			continue;

		const clang::MacroInfo* const macro_info= macro_directive->getMacroInfo();
		if( macro_info->isBuiltinMacro() )
			continue;

		if( macro_info->getNumParams() != 0u || macro_info->isFunctionLike() )
			continue;
		if( macro_info->getNumTokens() != 1u )
			continue;

		const clang::Token& token= macro_info->tokens().front();
		if( token.getKind() == clang::tok::numeric_constant )
		{
			const std::string numeric_literal_str( token.getLiteralData(), token.getLength() );
			clang::NumericLiteralParser numeric_literal_parser(
				numeric_literal_str,
				token.getLocation(),
				preprocessor_ );

			Synt::AutoVariableDeclaration auto_variable_declaration( g_dummy_file_pos );
			auto_variable_declaration.mutability_modifier= Synt::MutabilityModifier::Constexpr;
			auto_variable_declaration.name= TranslateIdentifier( name );

			Synt::NumericConstant numeric_constant( g_dummy_file_pos );

			llvm::APInt int_val( 64u, 0u );
			numeric_literal_parser.GetIntegerValue( int_val );
			numeric_constant.value_int_= int_val.getLimitedValue();

			if( numeric_literal_parser.getRadix() == 10 )
			{
				llvm::APFloat float_val(0.0);
				numeric_literal_parser.GetFloatValue( float_val );

				// "HACK! fix infinity.
				if( float_val.isInfinity() )
					float_val= llvm::APFloat::getLargest( float_val.getSemantics(), float_val.isNegative() );
				numeric_constant.value_double_= float_val.convertToDouble();
			}
			else
				numeric_constant.value_double_= static_cast<double>(numeric_constant.value_int_);

			if( numeric_literal_parser.isFloat )
				numeric_constant.type_suffix_[0]= 'f';
			else if( numeric_literal_parser.isUnsigned )
			{
				if( numeric_literal_parser.isLongLong )
				{
					numeric_constant.type_suffix_[0]= 'i';
					numeric_constant.type_suffix_[1]= '6';
					numeric_constant.type_suffix_[2]= '4';
				}
				else
					numeric_constant.type_suffix_[0]= 'u';
			}
			else
			{
				if( numeric_literal_parser.isLongLong )
				{
					numeric_constant.type_suffix_[0]= 'u';
					numeric_constant.type_suffix_[1]= '6';
					numeric_constant.type_suffix_[2]= '4';
				}
			}

			numeric_constant.has_fractional_point_= numeric_literal_parser.isFloatingLiteral();

			auto_variable_declaration.initializer_expression= std::move(numeric_constant);
			root_program_elements_.push_back( std::move( auto_variable_declaration ) );
		}
		else if( clang::tok::isStringLiteral( token.getKind() ) )
		{
			clang::StringLiteralParser string_literal_parser( { token }, preprocessor_ );

			if( string_literal_parser.isAscii() || string_literal_parser.isUTF8() )
			{
				Synt::AutoVariableDeclaration auto_variable_declaration( g_dummy_file_pos );
				auto_variable_declaration.reference_modifier= Synt::ReferenceModifier::Reference;
				auto_variable_declaration.mutability_modifier= Synt::MutabilityModifier::Constexpr;
				auto_variable_declaration.name= TranslateIdentifier( name );

				Synt::StringLiteral string_constant( g_dummy_file_pos );
				string_constant.value_= DecodeUTF8( string_literal_parser.GetString().str() );
				string_constant.value_.push_back( '\0' ); // C/C++ have null-terminated strings, instead of Ü.

				auto_variable_declaration.initializer_expression= std::move(string_constant);
				root_program_elements_.push_back( std::move( auto_variable_declaration ) );
			}
		}
		else if( token.getKind() == clang::tok::char_constant || token.getKind() == clang::tok::utf8_char_constant )
		{
			clang::CharLiteralParser char_literal_parser(
					token.getLiteralData(),
					token.getLiteralData() + token.getLength(),
					token.getLocation(),
					preprocessor_,
					token.getKind() );

			Synt::AutoVariableDeclaration auto_variable_declaration( g_dummy_file_pos );
			auto_variable_declaration.mutability_modifier= Synt::MutabilityModifier::Constexpr;
			auto_variable_declaration.name= TranslateIdentifier( name );

			Synt::StringLiteral string_constant( g_dummy_file_pos );
			string_constant.value_.push_back( char_literal_parser.getValue() );
			string_constant.type_suffix_[0]= 'c';
			string_constant.type_suffix_[1]= '8';

			auto_variable_declaration.initializer_expression= std::move(string_constant);
			root_program_elements_.push_back( std::move( auto_variable_declaration ) );
		}
	} // for defines
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

	if( const auto record_decl= llvm::dyn_cast<clang::RecordDecl>(&decl) )
	{
		Synt::ClassPtr record= ProcessRecord( *record_decl, current_externc );
		if( record != nullptr )
			program_elements.push_back( std::move(record) );
	}
	else if( const auto type_alias_decl= llvm::dyn_cast<clang::TypedefNameDecl>(&decl) )
	{
		if( type_alias_decl->isFirstDecl() )
			program_elements.push_back( ProcessTypedef(*type_alias_decl) );
	}
	else if( const auto func_decl= llvm::dyn_cast<clang::FunctionDecl>(&decl) )
	{
		if( func_decl->isFirstDecl() )
			program_elements.push_back( ProcessFunction( *func_decl, current_externc ) );
	}
	else if( const auto enum_decl= llvm::dyn_cast<clang::EnumDecl>(&decl) )
		ProcessEnum( *enum_decl, program_elements );
	else if( const auto namespace_decl= llvm::dyn_cast<clang::NamespaceDecl>(&decl) )
	{
		auto namespace_= std::make_unique<Synt::Namespace>( g_dummy_file_pos );
		namespace_->name_= TranslateIdentifier( namespace_decl->getName() );
		for( const clang::Decl* const sub_decl : namespace_decl->decls() )
			ProcessDecl( *sub_decl, namespace_->elements_, current_externc );

		program_elements.push_back(std::move(namespace_));
	}
	else if( const auto decl_context= llvm::dyn_cast<clang::DeclContext>(&decl) )
	{
		for( const clang::Decl* const sub_decl : decl_context->decls() )
			ProcessDecl( *sub_decl, program_elements, current_externc );
	}
}

void CppAstConsumer::ProcessClassDecl( const clang::Decl& decl, Synt::ClassElements& class_elements, bool externc )
{
	if( decl.isImplicit() )
		return;

	if( const auto field_decl= llvm::dyn_cast<clang::FieldDecl>(&decl) )
	{
		Synt::ClassField field( g_dummy_file_pos );

		const clang::Type* field_type= field_decl->getType().getTypePtr();

		if( ( field_type->isPointerType() || field_type->isReferenceType() ) && !field_type->isFunctionPointerType() )
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
		if( IsKeyword( field.name ) )
			field.name+= "_"_SpC;

		class_elements.push_back( std::move(field) );
	}
	else if( const auto record_decl= llvm::dyn_cast<clang::RecordDecl>(&decl) )
	{
		Synt::ClassPtr record= ProcessRecord( *record_decl, externc );
		if( record != nullptr )
			class_elements.push_back( std::move(record) );
	}
	else if( const auto func_decl= llvm::dyn_cast<clang::FunctionDecl>(&decl) )
	{
		if( func_decl->isFirstDecl() )
			class_elements.push_back( ProcessFunction(* func_decl, false ) );
	}
	else if( const auto type_alias_decl= llvm::dyn_cast<clang::TypedefNameDecl>(&decl) )
	{
		if( type_alias_decl->isFirstDecl() )
			class_elements.push_back( ProcessTypedef(*type_alias_decl) );
	}
}

Synt::ClassPtr CppAstConsumer::ProcessRecord( const clang::RecordDecl& record_decl, const bool externc )
{
	if( record_decl.isStruct() || record_decl.isClass() )
	{
		auto class_= std::make_unique<Synt::Class>(g_dummy_file_pos);
		class_->name_= TranslateRecordType( *llvm::dyn_cast<clang::RecordType>( record_decl.getTypeForDecl() ) );
		class_->keep_fields_order_= true; // C/C++ structs/classes have fixed fields order.

		if( record_decl.isCompleteDefinition() )
		{
			for( const clang::Decl* const sub_decl : record_decl.decls() )
				ProcessClassDecl( *sub_decl, class_->elements_, externc );
		}
		else
			class_->is_forward_declaration_= true;

		return std::move(class_);
	}
	else if( record_decl.isUnion() )
	{
		// Emulate union, using array of ints with required alignment.

		auto class_= std::make_unique<Synt::Class>(g_dummy_file_pos);
		class_->name_= TranslateRecordType( *llvm::dyn_cast<clang::RecordType>( record_decl.getTypeForDecl() ) );
		class_->keep_fields_order_= true; // C/C++ structs/classes have fixed fields order.

		if( record_decl.isCompleteDefinition() )
		{
			const auto size= ast_context_.getTypeSize( record_decl.getTypeForDecl() ) / 8u;
			const auto int_size= ast_context_.getTypeAlign( record_decl.getTypeForDecl() ) / 8u;
			const auto num= ( size + int_size - 1u ) / int_size;

			ProgramString int_name;
			switch(int_size)
			{
			case  1: int_name= Keyword( Keywords::  u8_ ); break;
			case  2: int_name= Keyword( Keywords:: u16_ ); break;
			case  4: int_name= Keyword( Keywords:: u32_ ); break;
			case  8: int_name= Keyword( Keywords:: u64_ ); break;
			case 16: int_name= Keyword( Keywords::u128_ ); break;
			default: U_ASSERT(false); break;
			};

			Synt::NamedTypeName named_type_name(g_dummy_file_pos);
			named_type_name.name.components.emplace_back();
			named_type_name.name.components.back().name= std::move(int_name);

			Synt::ArrayTypeName array_type( g_dummy_file_pos );
			array_type.element_type= std::make_unique<Synt::TypeName>( std::move(named_type_name) );

			Synt::NumericConstant numeric_constant( g_dummy_file_pos );
			numeric_constant.value_int_= num;
			numeric_constant.value_double_= static_cast<double>(numeric_constant.value_int_);
			array_type.size= std::make_unique<Synt::Expression>( std::move(numeric_constant) );

			Synt::ClassField field( g_dummy_file_pos );
			field.name= "union_content"_SpC;
			field.type= std::move(array_type);
			class_->elements_.push_back( std::move(field) );
		}
		else
			class_->is_forward_declaration_= true;

		return std::move(class_);
	}

	return nullptr;
}

Synt::Typedef CppAstConsumer::ProcessTypedef( const clang::TypedefNameDecl& typedef_decl )
{
	Synt::Typedef typedef_( g_dummy_file_pos );
	typedef_.name= TranslateIdentifier( typedef_decl.getName().str() );
	typedef_.value= TranslateType( *typedef_decl.getUnderlyingType().getTypePtr() );
	return typedef_;
}

Synt::FunctionPtr CppAstConsumer::ProcessFunction( const clang::FunctionDecl& func_decl, bool externc )
{
	auto func= std::make_unique<Synt::Function>(g_dummy_file_pos);

	func->name_.components.emplace_back();
	func->name_.components.back().name= TranslateIdentifier( func_decl.getName().str() );
	func->no_mangle_= externc;
	func->type_.unsafe_= true; // All C/C++ functions is unsafe.

	func->type_.arguments_.reserve( func_decl.param_size() );
	size_t i= 0u;
	for( const clang::ParmVarDecl* const param : func_decl.parameters() )
	{
		Synt::FunctionArgument arg( g_dummy_file_pos );
		arg.name_= TranslateIdentifier( param->getName().str() );
		if( arg.name_.empty() )
			arg.name_= ToProgramString( "arg" + std::to_string(i) );
		if( IsKeyword( arg.name_ ) )
			arg.name_+= "_"_SpC;

		const clang::Type* arg_type= param->getType().getTypePtr();
		if( ( arg_type->isPointerType() || arg_type->isReferenceType() ) && ! arg_type->isFunctionPointerType() )
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

	const clang::Type* return_type= func_decl.getReturnType().getTypePtr();
	if( ( return_type->isPointerType() || return_type->isReferenceType() ) && ! return_type->isFunctionPointerType() )
	{
		func->type_.return_value_reference_modifier_= Synt::ReferenceModifier::Reference;
		const clang::QualType type_qual= return_type->getPointeeType();
		return_type= type_qual.getTypePtr();

		if( type_qual.isConstQualified() )
			func->type_.return_value_mutability_modifier_= Synt::MutabilityModifier::Immutable;
		else
			func->type_.return_value_mutability_modifier_= Synt::MutabilityModifier::Mutable;
	}
	func->type_.return_type_= std::make_unique<Synt::TypeName>( TranslateType( *return_type ) );

	return func;
}

void CppAstConsumer::ProcessEnum( const clang::EnumDecl& enum_decl, Synt::ProgramElements& out_elements )
{
	if( !enum_decl.isComplete() )
		return;

	const ProgramString enum_name= TranslateIdentifier( enum_decl.getName().str() );
	const auto enumerators_range= enum_decl.enumerators();

	// C++ enum can be Ü enum, if it`s members form sequence 0-N with step 1.
	bool can_be_u_enum= true;
	{
		auto it= enumerators_range.begin();
		llvm::APSInt prev_val= it->getInitVal();
		if( prev_val.getLimitedValue() != 0 )
		{
			can_be_u_enum= false;
			goto end_check;
		}

		++it;
		for(; it != enumerators_range.end(); ++it )
		{
			const llvm::APSInt cur_val= it->getInitVal();
			if( (cur_val - prev_val).getLimitedValue() != 1u )
			{
				can_be_u_enum= false;
				goto end_check;
			}
			prev_val= cur_val;
		}
	}

	end_check:
	if( can_be_u_enum )
	{
		Synt::Enum enum_( g_dummy_file_pos );
		enum_.name= enum_name;

		Synt::TypeName type_name= TranslateType( *enum_decl.getIntegerType().getTypePtr() );
		if( Synt::NamedTypeName* const named_type_name= std::get_if<Synt::NamedTypeName>( &type_name ) )
			enum_.underlaying_type_name= std::move(named_type_name->name);

		for( const clang::EnumConstantDecl* const enumerator : enum_decl.enumerators() )
		{
			enum_.members.emplace_back();
			enum_.members.back().file_pos= g_dummy_file_pos;
			enum_.members.back().name= TranslateIdentifier( enumerator->getName().str() );
		}

		out_elements.push_back( std::move(enum_) );
	}
	else
	{
		auto enum_namespace_= std::make_unique<Synt::Namespace>( g_dummy_file_pos );
		enum_namespace_->name_= enum_name + "_namespace"_SpC;

		Synt::VariablesDeclaration variables_declaration( g_dummy_file_pos );
		variables_declaration.type= TranslateType( *enum_decl.getIntegerType().getTypePtr() );

		for( const clang::EnumConstantDecl* const enumerator : enum_decl.enumerators() )
		{
			Synt::VariablesDeclaration::VariableEntry var;
			var.file_pos= g_dummy_file_pos;
			var.name= TranslateIdentifier( enumerator->getName().str() );
			var.mutability_modifier= Synt::MutabilityModifier::Constexpr;

			Synt::ConstructorInitializer initializer( g_dummy_file_pos );
			Synt::NumericConstant initializer_number( g_dummy_file_pos );

			const llvm::APSInt val= enumerator->getInitVal();
			if( val.isNegative() )
				initializer_number.value_int_= val.getExtValue();
			else
				initializer_number.value_int_= val.getLimitedValue();
			initializer_number.value_double_= static_cast<double>(initializer_number.value_int_);

			initializer.call_operator.arguments_.push_back( std::move(initializer_number) );

			var.initializer= std::make_unique<Synt::Initializer>( std::move(initializer) );
			variables_declaration.variables.push_back( std::move(var) );
		}

		enum_namespace_->elements_.push_back( std::move(variables_declaration) );
		out_elements.push_back( std::move(enum_namespace_) );

		Synt::Typedef typedef_( g_dummy_file_pos );
		typedef_.name= enum_name;
		typedef_.value= TranslateType( *enum_decl.getIntegerType().getTypePtr() );
		out_elements.push_back( std::move( typedef_ ) );
	}
}

Synt::TypeName CppAstConsumer::TranslateType( const clang::Type& in_type )
{
	if( const auto built_in_type= llvm::dyn_cast<clang::BuiltinType>(&in_type) )
	{
		Synt::NamedTypeName named_type(g_dummy_file_pos);
		named_type.name.components.emplace_back();
		named_type.name.components.back().name= GetUFundamentalType( *built_in_type );
		return std::move(named_type);
	}
	else if( const auto record_type= llvm::dyn_cast<clang::RecordType>(&in_type) )
	{
		Synt::NamedTypeName named_type(g_dummy_file_pos);
		named_type.name.components.emplace_back();
		named_type.name.components.back().name= TranslateRecordType( *record_type );
		return std::move(named_type);
	}
	else if( const auto typedef_type= llvm::dyn_cast<clang::TypedefType>(&in_type) )
		return TranslateNamedType( typedef_type->getDecl()->getName().str() );
	else if( const auto constna_array_type= llvm::dyn_cast<clang::ConstantArrayType>(&in_type) )
	{
		// For arrays with constant size use normal Ü array.
		Synt::ArrayTypeName array_type(g_dummy_file_pos);
		array_type.element_type= std::make_unique<Synt::TypeName>( TranslateType( *constna_array_type->getElementType().getTypePtr() ) );

		Synt::NumericConstant numeric_constant( g_dummy_file_pos );
		numeric_constant.value_int_= constna_array_type->getSize().getLimitedValue();
		numeric_constant.value_double_= static_cast<double>(numeric_constant.value_int_);
		numeric_constant.type_suffix_[0]= 'u';
		array_type.size= std::make_unique<Synt::Expression>( std::move(numeric_constant) );

		return std::move(array_type);
	}
	else if( const auto array_type= llvm::dyn_cast<clang::ArrayType>(&in_type) )
	{
		// For other variants of array types use zero size.
		Synt::ArrayTypeName out_array_type(g_dummy_file_pos);
		out_array_type.element_type= std::make_unique<Synt::TypeName>( TranslateType( *array_type->getElementType().getTypePtr() ) );

		Synt::NumericConstant numeric_constant( g_dummy_file_pos );
		numeric_constant.value_int_= 0;
		numeric_constant.value_double_= 0.0;
		numeric_constant.type_suffix_[0]= 'u';
		out_array_type.size= std::make_unique<Synt::Expression>( std::move(numeric_constant) );

		return std::move(out_array_type);
	}
	else if( in_type.isFunctionPointerType() )
	{
		const clang::Type* function_type= in_type.getPointeeType().getTypePtr();
		while( const auto paren_type= llvm::dyn_cast<clang::ParenType>( function_type ) )
			function_type= paren_type->getInnerType().getTypePtr();

		if( const auto function_proto_type= llvm::dyn_cast<clang::FunctionProtoType>( function_type ) )
			return TranslateFunctionType( *function_proto_type );
	}
	else if( in_type.isPointerType() )
	{
		// Ü does not spports pointers. Use int with size of pointer.
		return TranslateNamedType( KeywordAscii( Keywords::size_type_ ) );
	}
	else if( const auto decltype_type= llvm::dyn_cast<clang::DecltypeType>( &in_type ) )
		return TranslateType( *decltype_type->desugar().getTypePtr() );
	else if( const auto paren_type= llvm::dyn_cast<clang::ParenType>( &in_type ) )
		return TranslateType( *paren_type->getInnerType().getTypePtr() );
	else if( const auto elaborated_type= llvm::dyn_cast<clang::ElaboratedType>( &in_type ) )
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
			const ProgramString& anon_name= DecodeUTF8("ü_anon_record") + ToProgramString( std::to_string( ++unique_name_index_ ) );
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
			if( size ==   8 ) return Keyword( Keywords::  i8_ );
			if( size ==  16 ) return Keyword( Keywords:: i16_ );
			if( size ==  32 ) return Keyword( Keywords:: i32_ );
			if( size ==  64 ) return Keyword( Keywords:: i64_ );
			if( size == 128 ) return Keyword( Keywords::i128_ );
			return Keyword( Keywords::i32_ );
		}
		if( in_type.isUnsignedInteger() )
		{
			if( size ==   8 ) return Keyword( Keywords::  u8_ );
			if( size ==  16 ) return Keyword( Keywords:: u16_ );
			if( size ==  32 ) return Keyword( Keywords:: u32_ );
			if( size ==  64 ) return Keyword( Keywords:: u64_ );
			if( size == 128 ) return Keyword( Keywords::u128_ );
			return Keyword( Keywords::u32_ );
		}
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
	auto function_type= std::make_unique<Synt::FunctionType>( g_dummy_file_pos );

	function_type->unsafe_= true; // All C/C++ functions is unsafe.

	function_type->arguments_.reserve( in_type.getNumParams() );
	size_t i= 0u;
	for( const clang::QualType& param_qual : in_type.getParamTypes() )
	{
		Synt::FunctionArgument arg( g_dummy_file_pos );
		arg.name_= ToProgramString( "arg" + std::to_string(i) );

		const clang::Type* arg_type= param_qual.getTypePtr();
		if( ( arg_type->isPointerType() || arg_type->isReferenceType() ) && !arg_type->isFunctionPointerType() )
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
	if( ( return_type->isPointerType() || return_type->isReferenceType() ) && !return_type->isFunctionPointerType() )
	{
		function_type->return_value_reference_modifier_= Synt::ReferenceModifier::Reference;
		const clang::QualType type_qual= return_type->getPointeeType();
		return_type= type_qual.getTypePtr();

		if( type_qual.isConstQualified() )
			function_type->return_value_mutability_modifier_= Synt::MutabilityModifier::Immutable;
		else
			function_type->return_value_mutability_modifier_= Synt::MutabilityModifier::Mutable;
	}
	function_type->return_type_= std::make_unique<Synt::TypeName>( TranslateType( *return_type ) );

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

	return ToProgramString(identifier);
}

CppAstProcessor::CppAstProcessor( ParsedUnitsPtr out_result )
	: out_result_(std::move(out_result))
{}

std::unique_ptr<clang::ASTConsumer> CppAstProcessor::CreateASTConsumer(
	clang::CompilerInstance& compiler_intance,
	const llvm::StringRef in_file )
{
	return
		std::make_unique<CppAstConsumer>(
			(*out_result_)[in_file.str()],
			compiler_intance.getPreprocessor(),
			compiler_intance.getLangOpts(),
			compiler_intance.getASTContext() );
}

FrontendActionFactory::FrontendActionFactory( ParsedUnitsPtr out_result )
	: out_result_(std::move(out_result))
{}

clang::FrontendAction* FrontendActionFactory::create()
{
	return new CppAstProcessor(out_result_);
}

} // namespace U
