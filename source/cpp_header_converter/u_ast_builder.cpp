#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <clang/AST/Attr.h>
#include <clang/AST/DeclBase.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/LiteralSupport.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/Support/ConvertUTF.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "../../code_builder_lib_common/string_ref.hpp"
#include "keywords.hpp"
#include "../lex_synt_lib/program_string.hpp"

#include "u_ast_builder.hpp"

namespace U
{

namespace
{

class CppAstConsumer : public clang::ASTConsumer
{
public:
	CppAstConsumer(
		Synt::ProgramElementsList::Builder& out_elements,
		const clang::SourceManager& source_manager,
		clang::Preprocessor& preprocessor,
		const clang::TargetInfo& target_info,
		clang::DiagnosticsEngine& diagnostic_engine,
		const clang::LangOptions& lang_options,
		const clang::ASTContext& ast_context,
		bool skip_declarations_from_includes );

public:
	virtual bool HandleTopLevelDecl( clang::DeclGroupRef decl_group ) override;
	virtual void HandleTranslationUnit( clang:: ASTContext& ast_context ) override;

private:
	void ProcessDecl( const clang::Decl& decl, Synt::ProgramElementsList::Builder& program_elements, bool externc );
	void ProcessClassDecl( const clang::Decl& decl, Synt::ClassElementsList::Builder& class_elements, bool externc );

	std::optional<Synt::Class> ProcessRecord( const clang::RecordDecl& record_decl, bool externc );
	Synt::TypeAlias ProcessTypedef( const clang::TypedefNameDecl& typedef_decl );
	Synt::Function ProcessFunction( const clang::FunctionDecl& func_decl, bool externc );
	void ProcessEnum( const clang::EnumDecl& enum_decl, Synt::ProgramElementsList::Builder& out_elements );

	Synt::TypeName TranslateType( const clang::Type& in_type );
	std::string TranslateRecordType( const clang::RecordType& in_type );
	std::string_view GetUFundamentalType( const clang::BuiltinType& in_type );
	Synt::ComplexName TranslateNamedType( llvm::StringRef cpp_type_name );
	Synt::FunctionType TranslateFunctionType( const clang::FunctionProtoType& in_type );
	std::optional<std::string> TranslateCallingConvention( const clang::FunctionType& in_type );

	std::string TranslateIdentifier( llvm::StringRef identifier );

private:
	Synt::ProgramElementsList::Builder& root_program_elements_;

	const clang::SourceManager& source_manager_;
	clang::Preprocessor& preprocessor_;
	const clang::TargetInfo &target_info_;
	clang::DiagnosticsEngine& diagnostic_engine_;
	const clang::LangOptions& lang_options_;
	const clang::PrintingPolicy printing_policy_;
	const clang::ASTContext& ast_context_;
	const bool skip_declarations_from_includes_;

	size_t unique_name_index_= 0u;
	std::unordered_map< const clang::RecordType*, std::string > anon_records_names_cache_;
	std::unordered_map< const clang::EnumDecl*, std::string > enum_names_cache_;

	std::unordered_set< std::string > globals_names_;
};

class CppAstProcessor : public clang::ASTFrontendAction
{
public:
	CppAstProcessor( ParsedUnitsPtr out_result, bool skip_declarations_from_includes );

public:
	virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
		clang::CompilerInstance& compiler_intance,
		llvm::StringRef in_file ) override;

private:
	const ParsedUnitsPtr out_result_;
	const bool skip_declarations_from_includes_;
};

const SrcLoc g_dummy_src_loc;

CppAstConsumer::CppAstConsumer(
	Synt::ProgramElementsList::Builder& out_elements,
	const clang::SourceManager& source_manager,
	clang::Preprocessor& preprocessor,
	const clang::TargetInfo& target_info,
	clang::DiagnosticsEngine& diagnostic_engine,
	const clang::LangOptions& lang_options,
	const clang::ASTContext& ast_context,
	const bool skip_declarations_from_includes )
	: root_program_elements_(out_elements)
	, source_manager_(source_manager)
	, preprocessor_(preprocessor)
	, target_info_(target_info)
	, diagnostic_engine_(diagnostic_engine)
	, lang_options_(lang_options)
	, printing_policy_(lang_options_)
	, ast_context_(ast_context)
	, skip_declarations_from_includes_(skip_declarations_from_includes)
{
}

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

		std::string name= ident_info->getName().str();
		if( name.empty() )
			continue;
		if( preprocessor_.getPredefines().find( "#define " + name ) != std::string::npos )
			continue;

		const clang::MacroDirective* const macro_directive= macro_pair.second.getLatest();
		if( macro_directive->getKind() != clang::MacroDirective::MD_Define )
			continue;

		if( skip_declarations_from_includes_ &&
			source_manager_.getFileID( macro_directive->getLocation() ) != source_manager_.getMainFileID() )
			return;

		const clang::MacroInfo* const macro_info= macro_directive->getMacroInfo();
		if( macro_info->isBuiltinMacro() )
			continue;

		if( macro_info->getNumParams() != 0u || macro_info->isFunctionLike() )
			continue;
		if( macro_info->getNumTokens() != 1u )
			continue;

		name= TranslateIdentifier(name);
		if( IsKeyword( name ) )
			name+= "_";

		if( globals_names_.count( name ) != 0 )
			continue; // Avoid redefining something.

		const clang::Token& token= macro_info->tokens().front();
		if( token.getKind() == clang::tok::numeric_constant )
		{
			const std::string numeric_literal_str( token.getLiteralData(), token.getLength() );
			clang::NumericLiteralParser numeric_literal_parser(
				numeric_literal_str,
				token.getLocation(),
				source_manager_,
				lang_options_,
				target_info_,
				diagnostic_engine_ );

			Synt::AutoVariableDeclaration auto_variable_declaration( g_dummy_src_loc );
			auto_variable_declaration.mutability_modifier= Synt::MutabilityModifier::Constexpr;
			auto_variable_declaration.name= name;

			Synt::NumericConstant numeric_constant( g_dummy_src_loc );

			llvm::APInt int_val( 64u, 0u );
			numeric_literal_parser.GetIntegerValue( int_val );
			numeric_constant.value_int= int_val.getLimitedValue();

			if( numeric_literal_parser.getRadix() == 10 )
			{
				llvm::APFloat float_val(0.0);
				numeric_literal_parser.GetFloatValue( float_val );

				// "HACK! fix infinity.
				if( float_val.isInfinity() )
					float_val= llvm::APFloat::getLargest( float_val.getSemantics(), float_val.isNegative() );
				numeric_constant.value_double= float_val.convertToDouble();
			}
			else
				numeric_constant.value_double= static_cast<double>(numeric_constant.value_int);

			if( numeric_literal_parser.isFloat )
				numeric_constant.type_suffix[0]= 'f';
			else if( numeric_literal_parser.isUnsigned )
			{
				if( numeric_literal_parser.isLongLong )
				{
					numeric_constant.type_suffix[0]= 'i';
					numeric_constant.type_suffix[1]= '6';
					numeric_constant.type_suffix[2]= '4';
				}
				else
					numeric_constant.type_suffix[0]= 'u';
			}
			else
			{
				if( numeric_literal_parser.isLongLong )
				{
					numeric_constant.type_suffix[0]= 'u';
					numeric_constant.type_suffix[1]= '6';
					numeric_constant.type_suffix[2]= '4';
				}
			}

			numeric_constant.has_fractional_point= numeric_literal_parser.isFloatingLiteral();

			auto_variable_declaration.initializer_expression= std::move(numeric_constant);
			root_program_elements_.Append( std::move( auto_variable_declaration ) );

			globals_names_.insert(name);
		}
		else if( clang::tok::isStringLiteral( token.getKind() ) )
		{
			clang::StringLiteralParser string_literal_parser( { token }, preprocessor_ );

			Synt::AutoVariableDeclaration auto_variable_declaration( g_dummy_src_loc );
			auto_variable_declaration.reference_modifier= Synt::ReferenceModifier::Reference;
			auto_variable_declaration.mutability_modifier= Synt::MutabilityModifier::Constexpr;
			auto_variable_declaration.name= name;

			auto string_constant= std::make_unique<Synt::StringLiteral>( g_dummy_src_loc );

			if( string_literal_parser.isOrdinary() || string_literal_parser.isUTF8() )
			{
				string_constant->value= string_literal_parser.GetString().str();
				string_constant->value.push_back( '\0' ); // C/C++ have null-terminated strings, instead of Ü.

				auto_variable_declaration.initializer_expression= std::move(string_constant);
				root_program_elements_.Append( std::move( auto_variable_declaration ) );
			}
			else if( string_literal_parser.isUTF16() ||
				( string_literal_parser.isWide() && ast_context_.getTypeSize(ast_context_.getWCharType()) == 16 ) )
			{
				llvm::convertUTF16ToUTF8String(
					llvm::ArrayRef<llvm::UTF16>(
						reinterpret_cast<const llvm::UTF16*>(string_literal_parser.GetString().data()),
						string_literal_parser.GetNumStringChars() ),
					string_constant->value );
				string_constant->value.push_back( '\0' ); // C/C++ have null-terminated strings, instead of Ü.

				string_constant->type_suffix= "u16";

				auto_variable_declaration.initializer_expression= std::move(string_constant);
				root_program_elements_.Append( std::move( auto_variable_declaration ) );

				globals_names_.insert(name);
			}
			else if( string_literal_parser.isUTF32() ||
				( string_literal_parser.isWide() && ast_context_.getTypeSize(ast_context_.getWCharType()) == 32 ) )
			{
				const auto string_ref = string_literal_parser.GetString();
				for( size_t i= 0u; i < string_literal_parser.GetNumStringChars(); ++i )
					PushCharToUTF8String( reinterpret_cast<const sprache_char*>(string_ref.data())[i], string_constant->value );

				string_constant->value.push_back( '\0' ); // C/C++ have null-terminated strings, instead of Ü.

				string_constant->type_suffix= "u32";

				auto_variable_declaration.initializer_expression= std::move(string_constant);
				root_program_elements_.Append( std::move( auto_variable_declaration ) );

				globals_names_.insert(name);
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

			Synt::AutoVariableDeclaration auto_variable_declaration( g_dummy_src_loc );
			auto_variable_declaration.mutability_modifier= Synt::MutabilityModifier::Constexpr;
			auto_variable_declaration.name= name;

			auto string_constant= std::make_unique<Synt::StringLiteral>( g_dummy_src_loc );
			string_constant->value.push_back( char(char_literal_parser.getValue()) );
			string_constant->type_suffix= "c8";

			auto_variable_declaration.initializer_expression= std::move(string_constant);
			root_program_elements_.Append( std::move( auto_variable_declaration ) );

			globals_names_.insert(name);
		}
	} // for defines

	// Create dummy definition for opaque structs.
	for( const auto type : ast_context.getTypes() )
	{
		if( const auto record_type= llvm::dyn_cast<clang::RecordType>( type ) )
		{
			// Process only incomplete records.
			// ignore implicitely-defined records, like "_GUID".
			if( record_type->isIncompleteType() && ! record_type->getDecl()->isImplicit() )
			{
				Synt::Class class_(g_dummy_src_loc);
				class_.name= TranslateRecordType( *record_type );
				class_.keep_fields_order= true; // C/C++ structs/classes have fixed fields order.

				// TODO - add deleted default constructor?

				root_program_elements_.Append( std::move(class_) );
			}
		}
	}

	// Add implicit "size_t", if it wasn't defined explicitely.
	if( globals_names_.count( "size_t" ) == 0 )
	{
		// HACK! Add type alias for "size_t".
		// We can't use "size_type" from Ü, because "size_t" in C is just an alias for uint32_t or uint64_t.

		Synt::TypeAlias type_alias( g_dummy_src_loc );
		type_alias.name= "size_t";
		type_alias.value= TranslateType( *ast_context_.getSizeType().getTypePtr() );

		root_program_elements_.Append( std::move(type_alias) );
	}

	// "__builtin_va_list" is also sometimes implicitely defined. Create something for it.
	std::string va_list_name= TranslateIdentifier( "__builtin_va_list" );
	if( globals_names_.count( va_list_name ) == 0 )
	{
		Synt::TypeAlias type_alias( g_dummy_src_loc );
		type_alias.name= std::move(va_list_name);

		Synt::NameLookup void_name(g_dummy_src_loc);
		void_name.name=  Keyword( Keywords::void_ );

		Synt::RawPointerType pointer_type(g_dummy_src_loc);
		pointer_type.element_type= std::move(void_name);

		type_alias.value= std::make_unique<Synt::RawPointerType>( std::move(pointer_type) );

		root_program_elements_.Append( std::move(type_alias) );
	}
}

void CppAstConsumer::ProcessDecl( const clang::Decl& decl, Synt::ProgramElementsList::Builder& program_elements, const bool externc )
{
	if( skip_declarations_from_includes_ &&
		source_manager_.getFileID( decl.getLocation() ) != source_manager_.getMainFileID() )
		return;

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
		if( auto record= ProcessRecord( *record_decl, current_externc ) )
			program_elements.Append( std::move(*record) );
	}
	else if( const auto type_alias_decl= llvm::dyn_cast<clang::TypedefNameDecl>(&decl) )
	{
		if( type_alias_decl->isFirstDecl() )
		{
			Synt::TypeAlias type_alias= ProcessTypedef(*type_alias_decl);

			bool is_same_name= false;
			if( const auto name_lookup= std::get_if<Synt::NameLookup>( &type_alias.value ) )
				is_same_name= name_lookup->name == type_alias.name;

			if( !is_same_name )
			{
				globals_names_.insert( type_alias.name );
				program_elements.Append( std::move(type_alias) );
			}
		}
	}
	else if( const auto func_decl= llvm::dyn_cast<clang::FunctionDecl>(&decl) )
	{
		if( func_decl->isFirstDecl() )
			program_elements.Append( ProcessFunction( *func_decl, current_externc ) );
	}
	else if( const auto enum_decl= llvm::dyn_cast<clang::EnumDecl>(&decl) )
		ProcessEnum( *enum_decl, program_elements );
	else if( const auto namespace_decl= llvm::dyn_cast<clang::NamespaceDecl>(&decl) )
	{
		Synt::Namespace namespace_( g_dummy_src_loc );
		namespace_.name= TranslateIdentifier( namespace_decl->getName() );

		Synt::ProgramElementsList::Builder namespace_elements;
		for( const clang::Decl* const sub_decl : namespace_decl->decls() )
			ProcessDecl( *sub_decl, namespace_elements, current_externc );
		namespace_.elements= namespace_elements.Build();

		program_elements.Append(std::move(namespace_));
	}
	else if( const auto decl_context= llvm::dyn_cast<clang::DeclContext>(&decl) )
	{
		for( const clang::Decl* const sub_decl : decl_context->decls() )
			ProcessDecl( *sub_decl, program_elements, current_externc );
	}
}

void CppAstConsumer::ProcessClassDecl( const clang::Decl& decl, Synt::ClassElementsList::Builder& class_elements, bool externc )
{
	if( decl.isImplicit() )
		return;

	if( const auto field_decl= llvm::dyn_cast<clang::FieldDecl>(&decl) )
	{
		Synt::ClassField field( g_dummy_src_loc );

		const clang::Type* field_type= field_decl->getType().getTypePtr();

		if( field_type->isReferenceType() )
		{
			// Ü has some restrictions for references in structs. So, replace all references with raw pointers.
			const clang::QualType type_qual= field_type->getPointeeType();
			field_type= type_qual.getTypePtr();

			auto raw_pointer_type= std::make_unique<Synt::RawPointerType>( g_dummy_src_loc );
			raw_pointer_type->element_type= TranslateType( *field_type );

			field.type= std::move(raw_pointer_type);
		}
		else
			field.type= TranslateType( *field_type );

		field.name= TranslateIdentifier( field_decl->getName() );
		if( IsKeyword( field.name ) )
			field.name+= "_";

		while( globals_names_.count( field.name ) != 0 )
		{
			// HACK!
			// For cases where field name is the same as some global name, add name suffix to avoid collsiions.
			// C allows such collisions, but Ü doesn't.
			field.name+= "_";
		}

		class_elements.Append( std::move(field) );
	}
	else if( const auto record_decl= llvm::dyn_cast<clang::RecordDecl>(&decl) )
	{
		if( auto record = ProcessRecord( *record_decl, externc ) )
			class_elements.Append( std::move(*record) );
	}
	else if( const auto func_decl= llvm::dyn_cast<clang::FunctionDecl>(&decl) )
	{
		if( func_decl->isFirstDecl() )
			class_elements.Append( ProcessFunction(* func_decl, false ) );
	}
	else if( const auto type_alias_decl= llvm::dyn_cast<clang::TypedefNameDecl>(&decl) )
	{
		if( type_alias_decl->isFirstDecl() )
			class_elements.Append( ProcessTypedef(*type_alias_decl) );
	}
}

std::optional<Synt::Class> CppAstConsumer::ProcessRecord( const clang::RecordDecl& record_decl, const bool externc )
{
	if( !record_decl.isCompleteDefinition() )
	{
		// Opaque struct/union.
		return std::nullopt;
	}

	if( record_decl.isTemplated() )
		return std::nullopt; // Ignore templates.

	if( record_decl.isStruct() || record_decl.isClass() )
	{
		Synt::Class class_(g_dummy_src_loc);
		class_.name= TranslateRecordType( *llvm::dyn_cast<clang::RecordType>( record_decl.getTypeForDecl() ) );

		// HACK! C allows to declare a struct and a function with the same name.
		// This doesn't create name conflict, as soon as struct is accessed via "struct StructName".
		// But in Ü this isn't possible, so, correct class name.
		while( globals_names_.count( class_.name ) != 0 )
			class_.name+= "_";

		globals_names_.insert( class_.name );

		class_.keep_fields_order= true; // C/C++ structs/classes have fixed fields order.

		Synt::ClassElementsList::Builder class_elements;
		for( const clang::Decl* const sub_decl : record_decl.decls() )
			ProcessClassDecl( *sub_decl, class_elements, externc );

		class_.elements= class_elements.Build();

		return std::move(class_);
	}
	else if( record_decl.isUnion() )
	{
		// Emulate union, using array of bytes with required alignment.

		Synt::Class class_(g_dummy_src_loc);
		class_.name= TranslateRecordType( *llvm::dyn_cast<clang::RecordType>( record_decl.getTypeForDecl() ) );
		class_.keep_fields_order= true; // C/C++ structs/classes have fixed fields order.

		// HACK! C allows to declare a struct and a function with the same name.
		// This doesn't create name conflict, as soon as struct is accessed via "struct StructName".
		// But in Ü this isn't possible, so, correct class name.
		while( globals_names_.count( class_.name ) != 0 )
			class_.name+= "_";

		globals_names_.insert( class_.name );

		const auto size= ast_context_.getTypeSize( record_decl.getTypeForDecl() ) / 8u;
		const auto byte_size= ast_context_.getTypeAlign( record_decl.getTypeForDecl() ) / 8u;
		const auto num= ( size + byte_size - 1u ) / byte_size;

		std::string byte_name;
		switch(byte_size)
		{
		case  1: byte_name= Keyword( Keywords::  byte8_ ); break;
		case  2: byte_name= Keyword( Keywords:: byte16_ ); break;
		case  4: byte_name= Keyword( Keywords:: byte32_ ); break;
		case  8: byte_name= Keyword( Keywords:: byte64_ ); break;
		case 16: byte_name= Keyword( Keywords::byte128_ ); break;
		default: U_ASSERT(false); break;
		};

		auto array_type= std::make_unique<Synt::ArrayTypeName>( g_dummy_src_loc );
		array_type->element_type= Synt::ComplexNameToTypeName( TranslateNamedType( byte_name ) );

		Synt::NumericConstant numeric_constant( g_dummy_src_loc );
		numeric_constant.value_int= num;
		numeric_constant.value_double= double(numeric_constant.value_int);
		array_type->size= std::move(numeric_constant);

		Synt::ClassField field( g_dummy_src_loc );
		field.name= "union_content";
		field.type= std::move(array_type);

		Synt::ClassElementsList::Builder class_elements;
		class_elements.Append( std::move(field) );
		class_.elements= class_elements.Build();

		return std::move(class_);
	}

	return std::nullopt;
}

Synt::TypeAlias CppAstConsumer::ProcessTypedef( const clang::TypedefNameDecl& typedef_decl )
{
	Synt::TypeAlias type_alias( g_dummy_src_loc );
	type_alias.name= TranslateIdentifier( typedef_decl.getName() );
	type_alias.value= TranslateType( *typedef_decl.getUnderlyingType().getTypePtr() );

	return type_alias;
}

Synt::Function CppAstConsumer::ProcessFunction( const clang::FunctionDecl& func_decl, const bool externc )
{
	Synt::Function func(g_dummy_src_loc);

	func.name.push_back( Synt::Function::NameComponent{ TranslateIdentifier( func_decl.getName() ), g_dummy_src_loc } );

	// Fix collisions with keywords.
	// TODO - find a better way to do this.
	// Sometimes it may be necessary to call such functions using exactly the same name.
	if( IsKeyword( func.name.back().name ) )
		func.name.back().name+= "_";

	// HACK! C allows to declare a struct and a function with the same name.
	// This doesn't create name conflict, as soon as struct is accessed via "struct StructName".
	// But in Ü this isn't possible, so, correct function name.
	while( globals_names_.count( func.name.back().name ) != 0 )
		func.name.back().name+= "_";

	globals_names_.insert( func.name.back().name );

	func.no_mangle= externc;
	func.type.unsafe= true; // All C/C++ functions are unsafe.

	func.type.params.reserve( func_decl.param_size() );
	size_t i= 0u;
	for( const clang::ParmVarDecl* const param : func_decl.parameters() )
	{
		Synt::FunctionParam arg( g_dummy_src_loc );
		arg.name= TranslateIdentifier( param->getName() );
		if( arg.name.empty() )
			arg.name= "arg" + std::to_string(i);
		if( IsKeyword( arg.name ) )
			arg.name+= "_";

		const clang::Type* arg_type= param->getType().getTypePtr();
		if( arg_type->isReferenceType() )
		{
			arg.reference_modifier= Synt::ReferenceModifier::Reference;
			const clang::QualType type_qual= arg_type->getPointeeType();
			arg_type= type_qual.getTypePtr();

			if( type_qual.isConstQualified() )
				arg.mutability_modifier= Synt::MutabilityModifier::Immutable;
			else
				arg.mutability_modifier= Synt::MutabilityModifier::Mutable;
		}

		arg.type= TranslateType( *arg_type );
		func.type.params.push_back(std::move(arg));
		++i;
	}

	const clang::Type* return_type= func_decl.getReturnType().getTypePtr();
	if( return_type->isReferenceType() )
	{
		func.type.return_value_reference_modifier= Synt::ReferenceModifier::Reference;
		const clang::QualType type_qual= return_type->getPointeeType();
		return_type= type_qual.getTypePtr();

		if( type_qual.isConstQualified() )
			func.type.return_value_mutability_modifier= Synt::MutabilityModifier::Immutable;
		else
			func.type.return_value_mutability_modifier= Synt::MutabilityModifier::Mutable;
	}
	func.type.return_type= std::make_unique<Synt::TypeName>( TranslateType( *return_type ) );

	const clang::Type* function_type= func_decl.getType().getTypePtr();

	while(true)
	{
		if( const auto paren_type= llvm::dyn_cast<clang::ParenType>( function_type ) )
			function_type= paren_type->getInnerType().getTypePtr();
		else if( const auto elaborated_type= llvm::dyn_cast<clang::ElaboratedType>( function_type ) )
			function_type= elaborated_type->desugar().getTypePtr();
		else if( const auto attributed_type= llvm::dyn_cast<clang::AttributedType>( function_type ) )
			function_type= attributed_type->desugar().getTypePtr(); // TODO - maybe collect such attributes?
		else
			break;
	}

	if( const auto ft= llvm::dyn_cast<clang::FunctionType>( function_type ) )
		func.type.calling_convention= TranslateCallingConvention( *ft );

	return func;
}

void CppAstConsumer::ProcessEnum( const clang::EnumDecl& enum_decl, Synt::ProgramElementsList::Builder& out_elements )
{
	if( !enum_decl.isComplete() )
		return;

	const std::string enum_name= TranslateIdentifier( enum_decl.getName() );
	const auto enumerators_range= enum_decl.enumerators();

	if( enum_decl.getName().empty() )
	{
		// Anonimous enum. Just create a bunch of constants for it in space, where this enum is located.
		Synt::VariablesDeclaration variables_declaration( g_dummy_src_loc );

		std::string_view underlying_type_name;
		if( const auto built_in_type= llvm::dyn_cast<clang::BuiltinType>( enum_decl.getIntegerType().getTypePtr() ) )
			underlying_type_name= GetUFundamentalType( *built_in_type );

		{
			Synt::NameLookup name_lookup(g_dummy_src_loc);
			name_lookup.name= underlying_type_name;
			variables_declaration.type= std::move(name_lookup);
		}

		variables_declaration.type= TranslateType( *enum_decl.getIntegerType().getTypePtr() );

		for( const clang::EnumConstantDecl* const enumerator : enumerators_range )
		{
			Synt::NumericConstant initializer_number( g_dummy_src_loc );
			const llvm::APSInt val= enumerator->getInitVal();
			if( val.isNegative() )
				initializer_number.value_int= uint64_t(val.getExtValue());
			else
				initializer_number.value_int= val.getLimitedValue();
			initializer_number.value_double= static_cast<double>(initializer_number.value_int);

			Synt::ConstructorInitializer constructor_initializer( g_dummy_src_loc );
			constructor_initializer.arguments.push_back( std::move(initializer_number) );

			Synt::VariablesDeclaration::VariableEntry var;
			var.src_loc= g_dummy_src_loc;
			var.name= TranslateIdentifier( enumerator->getName() );
			var.mutability_modifier= Synt::MutabilityModifier::Constexpr;
			var.initializer= std::make_unique<Synt::Initializer>( std::move(constructor_initializer) );

			globals_names_.insert( var.name );

			variables_declaration.variables.push_back( std::move(var) );
		}

		out_elements.Append( std::move(variables_declaration) );

		enum_names_cache_[ &enum_decl ]= underlying_type_name;

		return;
	}

	enum_names_cache_[ &enum_decl ]= enum_name;
	globals_names_.insert( enum_name );

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
		Synt::Enum enum_( g_dummy_src_loc );
		enum_.name= enum_name;

		Synt::TypeName type_name= TranslateType( *enum_decl.getIntegerType().getTypePtr() );
		if( const auto named_type_name= std::get_if<Synt::NameLookup>( &type_name ) )
			enum_.underlying_type_name= std::move(*named_type_name);

		for( const clang::EnumConstantDecl* const enumerator : enumerators_range )
		{
			enum_.members.emplace_back();
			enum_.members.back().src_loc= g_dummy_src_loc;
			enum_.members.back().name= TranslateIdentifier( enumerator->getName() );
		}

		out_elements.Append( std::move(enum_) );
	}
	else
	{
		// Can't use Ü enum. So, create struct type and a bunch of constants inside.
		// Since such struct contains singe scalar inside, it is passed via this scalar.

		Synt::Class enum_class_( g_dummy_src_loc );
		enum_class_.name= TranslateIdentifier( enum_name );

		Synt::ClassElementsList::Builder class_elements;

		const std::string field_name= "ü_underlying_value";
		{
			Synt::ClassField field( g_dummy_src_loc );
			field.name= field_name;
			field.type= TranslateType( *enum_decl.getIntegerType().getTypePtr() );
			class_elements.Append( std::move(field) );
		}

		Synt::NameLookup enum_class_name( g_dummy_src_loc );
		enum_class_name.name= enum_class_.name;

		for( const clang::EnumConstantDecl* const enumerator : enumerators_range )
		{
			Synt::NumericConstant initializer_number( g_dummy_src_loc );
			const llvm::APSInt val= enumerator->getInitVal();
			if( val.isNegative() )
				initializer_number.value_int= uint64_t(val.getExtValue());
			else
				initializer_number.value_int= val.getLimitedValue();
			initializer_number.value_double= static_cast<double>(initializer_number.value_int);

			Synt::ConstructorInitializer constructor_initializer( g_dummy_src_loc );
			constructor_initializer.arguments.push_back( std::move(initializer_number) );

			Synt::StructNamedInitializer::MemberInitializer member_initializer;
			member_initializer.initializer= std::move(constructor_initializer);
			member_initializer.name= field_name;

			Synt::StructNamedInitializer initializer( g_dummy_src_loc );
			initializer.members_initializers.push_back( std::move(member_initializer) );

			Synt::VariablesDeclaration::VariableEntry var;
			var.src_loc= g_dummy_src_loc;
			var.name= TranslateIdentifier( enumerator->getName() );
			var.mutability_modifier= Synt::MutabilityModifier::Constexpr;
			var.initializer= std::make_unique<Synt::Initializer>( std::move(initializer) );

			Synt::VariablesDeclaration variables_declaration( g_dummy_src_loc );
			variables_declaration.type= enum_class_name;
			variables_declaration.variables.push_back( std::move(var) );

			class_elements.Append( std::move(variables_declaration) );
		}

		enum_class_.elements= class_elements.Build();

		out_elements.Append( std::move(enum_class_) );
	}
}

Synt::TypeName CppAstConsumer::TranslateType( const clang::Type& in_type )
{
	if( const auto built_in_type= llvm::dyn_cast<clang::BuiltinType>(&in_type) )
		return Synt::ComplexNameToTypeName( TranslateNamedType( StringViewToStringRef( GetUFundamentalType( *built_in_type ) ) ) );
	else if( const auto record_type= llvm::dyn_cast<clang::RecordType>(&in_type) )
		return Synt::ComplexNameToTypeName( TranslateNamedType( TranslateRecordType( *record_type ) ) );
	else if( const auto enum_type= llvm::dyn_cast<clang::EnumType>(&in_type) )
	{
		if( const auto it= enum_names_cache_.find( enum_type->getDecl() ); it != enum_names_cache_.end() )
			return Synt::ComplexNameToTypeName( TranslateNamedType( it->second ) );
	}
	else if( const auto typedef_type= llvm::dyn_cast<clang::TypedefType>(&in_type) )
		return Synt::ComplexNameToTypeName( TranslateNamedType( typedef_type->getDecl()->getName() ) );
	else if( const auto constna_array_type= llvm::dyn_cast<clang::ConstantArrayType>(&in_type) )
	{
		// For arrays with constant size use normal Ü array.
		auto array_type= std::make_unique<Synt::ArrayTypeName>(g_dummy_src_loc);
		array_type->element_type= TranslateType( *constna_array_type->getElementType().getTypePtr() );

		Synt::NumericConstant numeric_constant( g_dummy_src_loc );
		numeric_constant.value_int= constna_array_type->getSize().getLimitedValue();
		numeric_constant.value_double= static_cast<double>(numeric_constant.value_int);
		numeric_constant.type_suffix[0]= 'u';
		array_type->size= std::move(numeric_constant);

		return std::move(array_type);
	}
	else if( const auto array_type= llvm::dyn_cast<clang::ArrayType>(&in_type) )
	{
		// For other variants of array types use zero size.
		auto out_array_type= std::make_unique<Synt::ArrayTypeName>(g_dummy_src_loc);
		out_array_type->element_type= TranslateType( *array_type->getElementType().getTypePtr() );

		Synt::NumericConstant numeric_constant( g_dummy_src_loc );
		numeric_constant.value_int= 0;
		numeric_constant.value_double= 0.0;
		numeric_constant.type_suffix[0]= 'u';
		out_array_type->size= std::move(numeric_constant);

		return std::move(out_array_type);
	}
	else if( in_type.isFunctionPointerType() )
	{
		const clang::Type* function_type= in_type.getPointeeType().getTypePtr();

		while(true)
		{
			if( const auto paren_type= llvm::dyn_cast<clang::ParenType>( function_type ) )
				function_type= paren_type->getInnerType().getTypePtr();
			else if( const auto elaborated_type= llvm::dyn_cast<clang::ElaboratedType>( function_type ) )
				function_type= elaborated_type->desugar().getTypePtr();
			else if( const auto attributed_type= llvm::dyn_cast<clang::AttributedType>( function_type ) )
				function_type= attributed_type->desugar().getTypePtr(); // TODO - maybe collect such attributes?
			else
				break;
		}

		if( const auto function_proto_type= llvm::dyn_cast<clang::FunctionProtoType>( function_type ) )
			return std::make_unique<Synt::FunctionType>( TranslateFunctionType( *function_proto_type ) );
	}
	else if( const auto pointer_type= llvm::dyn_cast<clang::PointerType>(&in_type) )
	{
		auto raw_pointer_type= std::make_unique<Synt::RawPointerType>( g_dummy_src_loc );
		raw_pointer_type->element_type= TranslateType( *pointer_type->getPointeeType().getTypePtr() );

		return std::move(raw_pointer_type);
	}
	else if( const auto decltype_type= llvm::dyn_cast<clang::DecltypeType>( &in_type ) )
		return TranslateType( *decltype_type->desugar().getTypePtr() );
	else if( const auto paren_type= llvm::dyn_cast<clang::ParenType>( &in_type ) )
		return TranslateType( *paren_type->getInnerType().getTypePtr() );
	else if( const auto elaborated_type= llvm::dyn_cast<clang::ElaboratedType>( &in_type ) )
		return TranslateType( *elaborated_type->desugar().getTypePtr() );
	else if( const auto attributed_type= llvm::dyn_cast<clang::AttributedType>( &in_type ) )
		return TranslateType( *attributed_type->desugar().getTypePtr() ); // TODO - maybe process attributes?

	return Synt::ComplexNameToTypeName( TranslateNamedType( "void" ) );
}

std::string CppAstConsumer::TranslateRecordType( const clang::RecordType& in_type )
{
	const llvm::StringRef name= in_type.getDecl()->getName();
	if( name.empty() )
	{
		const auto it= anon_records_names_cache_.find( &in_type );
		if( it != anon_records_names_cache_.end() )
			return it->second;
		else
		{
			const std::string anon_name= "ü_anon_record" + std::to_string( ++unique_name_index_ );
			anon_records_names_cache_[ &in_type ]= anon_name;
			return anon_name;
		}
	}
	else
		return TranslateIdentifier( name );
}

std::string_view CppAstConsumer::GetUFundamentalType( const clang::BuiltinType& in_type )
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

Synt::ComplexName CppAstConsumer::TranslateNamedType( const llvm::StringRef cpp_type_name )
{
	Synt::NameLookup named_type(g_dummy_src_loc);
	named_type.name= TranslateIdentifier( cpp_type_name );
	return Synt::ComplexName( std::move(named_type) );
}

Synt::FunctionType CppAstConsumer::TranslateFunctionType( const clang::FunctionProtoType& in_type )
{
	Synt::FunctionType function_type( g_dummy_src_loc );

	function_type.unsafe= true; // All C/C++ functions are unsafe.

	function_type.params.reserve( in_type.getNumParams() );
	size_t i= 0u;
	for( const clang::QualType& param_qual : in_type.getParamTypes() )
	{
		Synt::FunctionParam arg( g_dummy_src_loc );
		arg.name= "arg" + std::to_string(i);

		const clang::Type* arg_type= param_qual.getTypePtr();
		if( arg_type->isReferenceType() )
		{
			arg.reference_modifier= Synt::ReferenceModifier::Reference;
			const clang::QualType type_qual= arg_type->getPointeeType();
			arg_type= type_qual.getTypePtr();

			if( type_qual.isConstQualified() )
				arg.mutability_modifier= Synt::MutabilityModifier::Immutable;
			else
				arg.mutability_modifier= Synt::MutabilityModifier::Mutable;
		}

		arg.type= TranslateType( *arg_type );
		function_type.params.push_back(std::move(arg));
		++i;
	}

	const clang::Type* return_type= in_type.getReturnType().getTypePtr();
	if( return_type->isReferenceType() )
	{
		function_type.return_value_reference_modifier= Synt::ReferenceModifier::Reference;
		const clang::QualType type_qual= return_type->getPointeeType();
		return_type= type_qual.getTypePtr();

		if( type_qual.isConstQualified() )
			function_type.return_value_mutability_modifier= Synt::MutabilityModifier::Immutable;
		else
			function_type.return_value_mutability_modifier= Synt::MutabilityModifier::Mutable;
	}
	function_type.return_type= std::make_unique<Synt::TypeName>( TranslateType( *return_type ) );

	function_type.calling_convention= TranslateCallingConvention( in_type );

	return function_type;
}

std::optional<std::string> CppAstConsumer::TranslateCallingConvention( const clang::FunctionType& in_type )
{
	// TODO - handle/introduce other calling conventions.
	switch( in_type.getCallConv() )
	{
	case clang::CallingConv::CC_C:
		return std::nullopt;
	case clang::CallingConv::CC_X86StdCall:
		return "system";
	case clang::CallingConv::CC_X86FastCall:
		return "fast";
	default: break;
	}

	return std::nullopt;
}

std::string CppAstConsumer::TranslateIdentifier( const llvm::StringRef identifier )
{
	// For case of errors or something anonimous, generate unqiue identifier.
	if( identifier.empty() )
		return "ident" + std::to_string( ++unique_name_index_ );
	// In Ü identifier can not start with "_", shadow it. "_" in C++ used for impl identiferes, so, it may not needed.
	else if( identifier[0] == '_' )
		return ( "ü" + identifier ).str();

	return identifier.str();
}

CppAstProcessor::CppAstProcessor( ParsedUnitsPtr out_result, const bool skip_declarations_from_includes )
	: out_result_(std::move(out_result)), skip_declarations_from_includes_(skip_declarations_from_includes)
{}

std::unique_ptr<clang::ASTConsumer> CppAstProcessor::CreateASTConsumer(
	clang::CompilerInstance& compiler_intance,
	const llvm::StringRef in_file )
{
	return
		std::make_unique<CppAstConsumer>(
			(*out_result_)[in_file.str()],
			compiler_intance.getSourceManager(),
			compiler_intance.getPreprocessor(),
			compiler_intance.getTarget(),
			compiler_intance.getDiagnostics(),
			compiler_intance.getLangOpts(),
			compiler_intance.getASTContext(),
			skip_declarations_from_includes_ );
}

} // namespace

FrontendActionFactory::FrontendActionFactory( ParsedUnitsPtr out_result, const bool skip_declarations_from_includes )
	: out_result_(std::move(out_result)), skip_declarations_from_includes_(skip_declarations_from_includes)
{}

std::unique_ptr<clang::FrontendAction> FrontendActionFactory::create()
{
	return std::make_unique<CppAstProcessor>(out_result_, skip_declarations_from_includes_);
}

} // namespace U
