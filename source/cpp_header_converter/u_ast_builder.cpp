#include <unordered_set>
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <clang/AST/Attr.h>
#include <clang/AST/DeclBase.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/LiteralSupport.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/Support/ConvertUTF.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "../lex_synt_lib/program_string.hpp"

#include "u_ast_builder.hpp"

namespace U
{

namespace
{

const SrcLoc g_dummy_src_loc;

static Synt::Function GetDeletedDefaultConstructor()
{
	// Add deleted default constructor.
	Synt::Function func(g_dummy_src_loc);
	func.name.push_back( Synt::Function::NameComponent{ std::string( Keyword( Keywords::constructor_ ) ), g_dummy_src_loc, false } );
	func.body_kind= Synt::Function::BodyKind::BodyGenerationDisabled;

	return func;
}

// Maintain an internal recursive structure representing rough structure of the given C header.
// It's used almsot 1 to 1 to emit Ü definitions.

struct NamespaceItemNamespace;
struct NamespaceItemRecord;
struct NamespaceItemEnumElement;

using NamespaceItem=
	std::variant<
		NamespaceItemNamespace, // Keep it first here, so that operator[] creates a namespace.
		const clang::FunctionDecl*,
		NamespaceItemRecord,
		const clang::TypedefNameDecl*,
		const clang::EnumDecl*,
		NamespaceItemEnumElement,
		const clang::VarDecl* >;

// Use string map for items, since it's pretty fast.
// WARNING! Don't emit items in order of the map, sort them instead!
using NamespaceItemsMap= llvm::StringMap<NamespaceItem>;

struct NamespaceItemNamespace
{
	NamespaceItemsMap items;
};

struct NamespaceItemRecord
{
	const clang::RecordDecl* record_decl;
	NamespaceItemsMap items;
};

struct NamespaceItemEnumElement
{
	const clang::Type* enum_type;
	const clang::EnumConstantDecl* enum_constant_decl;
};

const auto& g_struct_kind_tag= "struct_";
const auto& g_union_kind_tag= "union_";
const auto& g_enum_kind_tag= "enum_";
const auto& g_scoped_enum_namespace_kind_tag= "scoped_enum_";

const auto& g_anon_record_prefix= "anon_record";
const auto& g_anon_enum_prefix= "anon_enum";
const auto& g_anon_field_prefix= "anon_field_";

// This function should map C names to Ü names without any possibility of collision,
// so that no two C names can get identical Ü name.
std::string TranslateIdentifier( const llvm::StringRef identifier )
{
	U_ASSERT( !identifier.empty() );

	const auto& c_coded_name_postfix= "U__";

	// Add single trailing underscope for Ü keywords.
	if( IsKeyword( identifier ) )
		return ( identifier + "_" ).str();

	size_t num_leading_underscores= 0;
	while( num_leading_underscores < identifier.size() && identifier[ num_leading_underscores ] == '_' )
		++num_leading_underscores;

	if(
		// Code identifiers used for some needs of C++ header converter.
		identifier == g_struct_kind_tag ||
		identifier == g_union_kind_tag ||
		identifier == g_enum_kind_tag ||
		identifier == g_scoped_enum_namespace_kind_tag ||
		identifier == g_anon_record_prefix ||
		identifier == g_anon_enum_prefix ||
		identifier == g_anon_field_prefix ||
		// Code identifiers with leading underscores. Ü doesn't support them.
		num_leading_underscores > 0 ||
		// If an identifier is an Ü keyword with trailing underscore, code it,
		// since it may conflict with coding of identifiers which are Ü keywords.
		( identifier.back() == '_' && IsKeyword( identifier.substr( 0, identifier.size() - 1 ) ) ) ||
		// Code identifiers having coded name prefix.
		identifier.endswith( c_coded_name_postfix ) )
	{
		std::string name_coded;

		if( num_leading_underscores == identifier.size() )
		{
			// For identifiers having noting than underscores add leading "u".
			name_coded+= "u";
			name_coded+= std::to_string( num_leading_underscores );
		}
		else
		{
			const bool first_non_underscore_char_is_digit=
				identifier[ num_leading_underscores ] >= '0' && identifier[ num_leading_underscores ] <= '9';

			if( first_non_underscore_char_is_digit )
				name_coded+= "n"; // Add leading "n" to avoid starting an identifier with digit.

			// Code the number of leading underscores.
			name_coded+= identifier.substr( num_leading_underscores );
			name_coded+= "_";

			if( first_non_underscore_char_is_digit )
				name_coded+= "n"; // Indicate that leading "n" was added.

			name_coded+= std::to_string( num_leading_underscores );

		}

		// Add a postfix for all coded names.
		name_coded+= c_coded_name_postfix;

		return name_coded;
	}

	// Return idenitifier as is. It happens for the majority of cases.
	return identifier.str();
}

// Name including all parent namespaces/structs.
using ItemFullName= std::vector<std::string>;

Synt::ComplexName GetItemNameSyntaxElementImpl( Synt::ComplexName base, const llvm::ArrayRef<std::string> components )
{
	if( components.empty() )
		return base;

	auto names_scope_name_fetch=
		std::make_unique<Synt::NamesScopeNameFetch>(
			Synt::NamesScopeNameFetch{ g_dummy_src_loc, components.front(), std::move(base) } );

	return GetItemNameSyntaxElementImpl( std::move(names_scope_name_fetch), components.slice(1) );
}

Synt::ComplexName GetItemNameSyntaxElement( const ItemFullName& item_name )
{
	Synt::RootNamespaceNameLookup root_namespace_name_lookup( g_dummy_src_loc );
	root_namespace_name_lookup.name= item_name.front();

	return
		GetItemNameSyntaxElementImpl(
			std::move( root_namespace_name_lookup ), llvm::ArrayRef<std::string>( item_name ).slice(1) );
}

Synt::TypeName CreateFundamentalTypeName( const std::string_view name )
{
	// Use simple name lookup (not root namespace name lookup) for simplicity.
	// Redefining fundamental type names isn't possible, so they are always acccessible.

	Synt::NameLookup name_lookup( g_dummy_src_loc );
	name_lookup.name= name;
	return std::move(name_lookup);
}

Synt::IntegerNumericConstant TranslateNumericConstant( const llvm::APInt& n )
{
	Synt::IntegerNumericConstant numeric_constant( g_dummy_src_loc );

	llvm::SmallString<32> s;
	n.toStringUnsigned( s );
	numeric_constant.num= std::string( s );

	return numeric_constant;
}

Synt::FloatingPointNumericConstant TranslateNumericConstant( const llvm::APFloat& n )
{
	Synt::FloatingPointNumericConstant numeric_constant( g_dummy_src_loc );

	llvm::SmallString<32> s;
	n.toString( s, 0 /* FormatPrecision */, 0 /* FormatMaxPadding */, true /* TruncateZero */ );
	numeric_constant.num= std::string( s );

	return numeric_constant;
}

std::optional<std::string> TranslateCallingConventionImpl( const clang::FunctionType& in_type )
{
	// TODO - handle/introduce other calling conventions.
	switch( in_type.getCallConv() )
	{
	case clang::CallingConv::CC_C:
		return "C";
	case clang::CallingConv::CC_X86StdCall:
		return "system";
	case clang::CallingConv::CC_X86FastCall:
		return "fast";
	default: break;
	}

	return std::nullopt;
}

std::unique_ptr<const Synt::Expression> TranslateCallingConvention( const clang::FunctionType& in_type )
{
	if( auto cc= TranslateCallingConventionImpl( in_type ) )
	{
		auto string_literal= std::make_unique<Synt::StringLiteral>( g_dummy_src_loc );
		string_literal->value= std::move(*cc);

		return std::make_unique<Synt::Expression>( std::move(string_literal) );
	}

	return nullptr;
}

clang::SourceLocation GetNamespaceItemSourceLocationImpl( const NamespaceItemNamespace& item )
{
	(void)item;
	return clang::SourceLocation(); // Default constructor produces invalid location.
}


clang::SourceLocation GetNamespaceItemSourceLocationImpl( const NamespaceItemRecord& item )
{
	return item.record_decl->getLocation();
}

clang::SourceLocation GetNamespaceItemSourceLocationImpl( const NamespaceItemEnumElement& item )
{
	return item.enum_constant_decl->getLocation();
}

clang::SourceLocation GetNamespaceItemSourceLocationImpl( const clang::Decl* const item )
{
	return item->getLocation();
}

// Returns invalid location if can't get actual location.
clang::SourceLocation GetNamespaceItemSourceLocation( const NamespaceItem& item )
{
	return std::visit( [&]( const auto& t ) { return GetNamespaceItemSourceLocationImpl( t ); }, item );
}

// A macro directive allowing ignoring some names.
struct CppHeaderConverterIgnoreMacro
{
	enum class Kind : uint8_t
	{
		Define,
		Undefine,
	};

	Kind kind= Kind::Define;
	clang::SourceLocation location;
};

using CppHeaderConverterIgnoreMacros= std::vector<CppHeaderConverterIgnoreMacro>;

using CppHeaderConverterIgnoreMacrosPtr= std::shared_ptr<CppHeaderConverterIgnoreMacros>;

class PreprocessorCallbacks final : public clang::PPCallbacks
{
public:
	PreprocessorCallbacks( CppHeaderConverterIgnoreMacrosPtr ignore_macros )
		: ignore_macros_( std::move( ignore_macros ) )
	{}

public:
	virtual void MacroDefined(
		const clang::Token& macro_name_token, const clang::MacroDirective* const macro_directive ) override
	{
		if( macro_directive == nullptr )
			return;

		const clang::IdentifierInfo* const identifier_info= macro_name_token.getIdentifierInfo();
		if( identifier_info == nullptr )
			return;

		if( identifier_info->getName() == c_ignore_directive_name )
		{
			const clang::SourceLocation location= macro_directive->getLocation();
			if( location.isValid() )
				ignore_macros_->push_back(
					CppHeaderConverterIgnoreMacro{ CppHeaderConverterIgnoreMacro::Kind::Define, location } );
		}
	}

	virtual void MacroUndefined(
		const clang::Token& macro_name_token,
		const clang::MacroDefinition& macro_definition,
		const clang::MacroDirective* const macro_directive ) override
	{
		(void)macro_definition;
		(void)macro_directive;

		const clang::IdentifierInfo* const identifier_info= macro_name_token.getIdentifierInfo();
		if( identifier_info == nullptr )
			return;

		if( identifier_info->getName() == c_ignore_directive_name )
		{
			const clang::SourceLocation location= macro_directive->getLocation();
			if( location.isValid() )
				ignore_macros_->push_back(
					CppHeaderConverterIgnoreMacro{ CppHeaderConverterIgnoreMacro::Kind::Undefine, location } );
		}
	}

private:
	static constexpr auto& c_ignore_directive_name= "U_CPP_HEADER_CONVERTER_IGNORE";

private:
	const CppHeaderConverterIgnoreMacrosPtr ignore_macros_;
};

class CppAstConsumer final : public clang::ASTConsumer
{
public:
	CppAstConsumer(
		Synt::ProgramElementsList& out_elements,
		const clang::SourceManager& source_manager,
		clang::Preprocessor& preprocessor,
		const clang::TargetInfo& target_info,
		clang::DiagnosticsEngine& diagnostic_engine,
		const clang::LangOptions& lang_options,
		const clang::ASTContext& ast_context );

public:
	virtual bool HandleTopLevelDecl( clang::DeclGroupRef decl_group ) override;
	virtual void HandleTranslationUnit( clang::ASTContext& ast_context ) override;

private:
	using TypeNamesMap= std::unordered_map< const clang::Type*, ItemFullName >;

private:
	void ProcessDecl( const clang::Decl& decl );

	std::string GetAnonymousItemUniqueName( std::string_view prefix, clang::SourceLocation location );

	Synt::TypeName TranslateType( const clang::Type& in_type, const TypeNamesMap& type_names_map );
	std::string_view GetUFundamentalType( const clang::BuiltinType& in_type );
	Synt::FunctionType TranslateFunctionType( const clang::FunctionProtoType& in_type, const TypeNamesMap& type_names_map );
	Synt::FunctionType TranslateFunctionType( const clang::FunctionType& in_type, const TypeNamesMap& type_names_map );

	void PrepareIgnoreDirectives();
	bool ShouldSkipEmittingItem( const clang::SourceLocation& location );

	void BuildTypeNamesMap( TypeNamesMap& map, ItemFullName& prefix, const NamespaceItem& item );
	void BuildTypeNamesMapImpl( TypeNamesMap& map, ItemFullName& prefix, const NamespaceItemNamespace& item );
	void BuildTypeNamesMapImpl( TypeNamesMap& map, ItemFullName& prefix, const clang::FunctionDecl* item );
	void BuildTypeNamesMapImpl( TypeNamesMap& map, ItemFullName& prefix, const NamespaceItemRecord& item );
	void BuildTypeNamesMapImpl( TypeNamesMap& map, ItemFullName& prefix, const clang::TypedefNameDecl* item );
	void BuildTypeNamesMapImpl( TypeNamesMap& map, ItemFullName& prefix, const clang::EnumDecl* item );
	void BuildTypeNamesMapImpl( TypeNamesMap& map, ItemFullName& prefix, const NamespaceItemEnumElement& item );
	void BuildTypeNamesMapImpl( TypeNamesMap& map, ItemFullName& prefix, const clang::VarDecl* item );

	void CollectSubrecords( NamespaceItem& item );

	template<typename ListBuilder>
	void EmitItemsSorted( ListBuilder& out_items, const TypeNamesMap& type_names_map, const NamespaceItemsMap& items );

	Synt::Namespace EmitNamespaceItem(
		const TypeNamesMap& type_names_map, std::string_view name, const NamespaceItemNamespace& item );

	// Emit classes for namespaces, since actual namespaces can't be placed in classes, but we need this sometimes.
	Synt::Class EmitItemImpl(
		const TypeNamesMap& type_names_map, std::string_view name, const NamespaceItemNamespace& item );

	Synt::Function EmitItemImpl(
		const TypeNamesMap& type_names_map, std::string_view name, const clang::FunctionDecl* item );

	Synt::Class EmitItemImpl(
		const TypeNamesMap& type_names_map, std::string_view name, const NamespaceItemRecord& item );

	Synt::TypeAlias EmitItemImpl(
		const TypeNamesMap& type_names_map, std::string_view name, const clang::TypedefNameDecl* item );

	Synt::TypeAlias EmitItemImpl(
		const TypeNamesMap& type_names_map, std::string_view name, const clang::EnumDecl* item );

	Synt::VariablesDeclaration EmitItemImpl(
		const TypeNamesMap& type_names_map, std::string_view name, const NamespaceItemEnumElement& item );

	Synt::VariablesDeclaration EmitItemImpl(
		const TypeNamesMap& type_names_map, std::string_view name, const clang::VarDecl* item );

	Synt::ClassElementsList MakeOpaqueRecordElements(
		const TypeNamesMap& type_names_map,
		const clang::RecordDecl& record_declaration,
		std::string_view kind_name,
		const NamespaceItemsMap& subitems );

	Synt::Initializer TranslateVariableInitializer_r( const clang::Type& variable_type, const clang::APValue& value );

	void EmitDefinitionsForMacros( Synt::ProgramElementsList::Builder& out_items );

	Synt::Expression TranslateNumericLiteral( const clang::Token& token );

private:
	Synt::ProgramElementsList& out_program_elements_;

	const clang::SourceManager& source_manager_;
	clang::Preprocessor& preprocessor_;
	const clang::TargetInfo &target_info_;
	clang::DiagnosticsEngine& diagnostic_engine_;
	const clang::LangOptions& lang_options_;
	const clang::ASTContext& ast_context_;

	const CppHeaderConverterIgnoreMacrosPtr ignore_macros_;

	NamespaceItemNamespace root_namespace_;
};

class CppAstProcessor final : public clang::ASTFrontendAction
{
public:
	CppAstProcessor( ParsedUnitsPtr out_result, DepFileOptionsOpt dep_file_options );

public:
	virtual bool PrepareToExecuteAction( clang::CompilerInstance& compiler_intance ) override;

	virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
		clang::CompilerInstance& compiler_intance, llvm::StringRef in_file ) override;

private:
	const ParsedUnitsPtr out_result_;
	const DepFileOptionsOpt dep_file_options_;
};

CppAstConsumer::CppAstConsumer(
	Synt::ProgramElementsList& out_elements,
	const clang::SourceManager& source_manager,
	clang::Preprocessor& preprocessor,
	const clang::TargetInfo& target_info,
	clang::DiagnosticsEngine& diagnostic_engine,
	const clang::LangOptions& lang_options,
	const clang::ASTContext& ast_context )
	: out_program_elements_(out_elements)
	, source_manager_(source_manager)
	, preprocessor_(preprocessor)
	, target_info_(target_info)
	, diagnostic_engine_(diagnostic_engine)
	, lang_options_(lang_options)
	, ast_context_(ast_context)
	, ignore_macros_( std::make_shared<CppHeaderConverterIgnoreMacros>() )
{
	// Install preprocessor callbacks to be able to extract all locations of ignore macros usage.
	preprocessor_.addPPCallbacks( std::make_unique<PreprocessorCallbacks>( ignore_macros_ ) );
}

bool CppAstConsumer::HandleTopLevelDecl( const clang::DeclGroupRef decl_group )
{
	for( const clang::Decl* const decl : decl_group )
		ProcessDecl( *decl );
	return true;
}

void CppAstConsumer::HandleTranslationUnit( clang::ASTContext& ast_context )
{
	PrepareIgnoreDirectives();

	(void)ast_context;

	for( auto& pair : root_namespace_.items )
		CollectSubrecords( pair.second );

	TypeNamesMap type_names_map;

	{
		ItemFullName prefix;
		BuildTypeNamesMapImpl( type_names_map, prefix, root_namespace_ );
	}

	Synt::ProgramElementsList::Builder root_program_elements;

	EmitItemsSorted( root_program_elements, type_names_map, root_namespace_.items );

	EmitDefinitionsForMacros( root_program_elements );

	out_program_elements_= root_program_elements.Build();
}

void CppAstConsumer::ProcessDecl( const clang::Decl& decl )
{
	if( const auto record_decl= llvm::dyn_cast<clang::RecordDecl>(&decl) )
	{
		if( !record_decl->isTemplated() )
		{
			const auto src_name= record_decl->getName();

			const std::string name_translated=
				src_name.empty()
					? GetAnonymousItemUniqueName( g_anon_record_prefix, record_decl->getLocation() )
					: TranslateIdentifier( src_name );

			// Tagged names (structs, unions, enums) in C build a separae namespace.
			// So, put tags into separate Ü namespace.

			NamespaceItemsMap& items_map=
				std::get< NamespaceItemNamespace>( root_namespace_.items[ record_decl->isUnion() ? g_union_kind_tag : g_struct_kind_tag ] ).items;

			// Insert if has no entry, replace if is complete definition.
			if( items_map.count( name_translated ) == 0 || record_decl->isCompleteDefinition() )
				items_map.insert_or_assign( name_translated, NamespaceItemRecord{ record_decl, NamespaceItemsMap() } );
		}
	}
	else if( const auto type_alias_decl= llvm::dyn_cast<clang::TypedefNameDecl>(&decl) )
		root_namespace_.items.insert_or_assign( TranslateIdentifier( type_alias_decl->getName() ), type_alias_decl );
	else if( const auto func_decl= llvm::dyn_cast<clang::FunctionDecl>(&decl) )
	{
		if( func_decl->isFirstDecl() || func_decl->getBuiltinID() != 0 )
		{
			if( func_decl->getIdentifier() != nullptr && !( func_decl->hasBody() && func_decl->isInlineSpecified() ) )
			{
				const llvm::StringRef original_name= func_decl->getName();

				if( TranslateIdentifier( original_name ) == original_name )
					root_namespace_.items.insert_or_assign( original_name, func_decl );
				else
				{
					// If it's impossible to use the original name for a function - ignore it.
					// Emitting it as renaming is useless, since calling such function isn't possible anyway.
				}
			}
		}
	}
	else if( const auto enum_decl= llvm::dyn_cast<clang::EnumDecl>(&decl) )
	{
		const auto src_name= enum_decl->getName();

		const std::string name_translated=
			src_name.empty()
				? GetAnonymousItemUniqueName( g_anon_enum_prefix, enum_decl->getLocation() )
				: TranslateIdentifier( src_name );

		// Tagged names (structs, unions, enums) in C build a separae namespace.
		// So, put tags into separate Ü namespace.

		NamespaceItemsMap& items_map= std::get< NamespaceItemNamespace>( root_namespace_.items[ g_enum_kind_tag ] ).items;

		// Insert if has no entry, replace if is complete definition.
		if( items_map.count( name_translated ) == 0 || enum_decl->isComplete() )
			items_map.insert_or_assign( name_translated, enum_decl );

		if( enum_decl->isComplete() )
		{
			if( enum_decl->isScoped() )
			{
				// Create a namespace for elements of a scoped enum within "scoped_enum_" namespace.
				// Doing so we avoid possible name conflicts and preserve enum elements in single place.

				NamespaceItemsMap& scoped_enum_items_map=
					std::get< NamespaceItemNamespace>( root_namespace_.items[ g_scoped_enum_namespace_kind_tag ] ).items;

				NamespaceItemsMap& scoped_enum_members_map=
					std::get< NamespaceItemNamespace>( scoped_enum_items_map[ name_translated ] ).items;

				for( const clang::EnumConstantDecl* const enumerator : enum_decl->enumerators() )
					scoped_enum_members_map.insert_or_assign(
						TranslateIdentifier( enumerator->getName() ),
						NamespaceItemEnumElement{ enum_decl->getTypeForDecl(), enumerator } );
			}
			else
			{
				for( const clang::EnumConstantDecl* const enumerator : enum_decl->enumerators() )
					root_namespace_.items.insert_or_assign(
						TranslateIdentifier( enumerator->getName() ),
						NamespaceItemEnumElement{ enum_decl->getTypeForDecl(), enumerator } );
			}
		}
	}
	else if( const auto decl_context= llvm::dyn_cast<clang::DeclContext>(&decl) )
	{
		for( const clang::Decl* const sub_decl : decl_context->decls() )
			ProcessDecl( *sub_decl );
	}
	else if( const auto var_decl= llvm::dyn_cast<clang::VarDecl>(&decl) )
	{
		if( var_decl->hasInit() &&
			var_decl->hasConstantInitialization() &&
			var_decl->getType().isConstant( ast_context_ ) )
			root_namespace_.items.insert_or_assign( TranslateIdentifier( var_decl->getName() ), var_decl );
	}
}

std::string CppAstConsumer::GetAnonymousItemUniqueName( std::string_view prefix, clang::SourceLocation location )
{
	// Use prefix_device_file_line_column format.
	// This provides stable and deterministic names (at least on single machine).

	std::string name= std::string( prefix );

	// Use a loop to include locations of each macro expansion in the result name.
	while( true )
	{
		if( !location.isValid() )
			break;

		const auto decomposed_loc=
			source_manager_.getDecomposedLoc( source_manager_.getImmediateSpellingLoc( location ) );
		const clang::FileID file_id= decomposed_loc.first;
		if( file_id.isValid() )
		{
			if( const clang::FileEntry* const entry= source_manager_.getFileEntryForID( file_id ) )
			{
				const llvm::sys::fs::UniqueID unique_id= entry->getUniqueID();
				name+= "_d";
				name+= std::to_string( unique_id.getDevice() );
				name+= "_f";
				name+= std::to_string( unique_id.getFile() );
			}
		}
		name+= "_l";
		name+= std::to_string( source_manager_.getLineNumber( file_id, decomposed_loc.second ) );
		name+= "_c";
		name+= std::to_string( source_manager_.getColumnNumber( file_id, decomposed_loc.second ) );

		if( location.isMacroID() )
			location= source_manager_.getImmediateExpansionRange( location ).getBegin();
		else
			break;
	}

	return name;
}

Synt::TypeName CppAstConsumer::TranslateType( const clang::Type& in_type, const TypeNamesMap& type_names_map )
{
	// Records, typedefs, enums should have names in this map.
	if( const auto named_type_it= type_names_map.find( &in_type ); named_type_it != type_names_map.end() )
		return Synt::ComplexNameToTypeName( GetItemNameSyntaxElement( named_type_it->second ) );

	if( const auto built_in_type= llvm::dyn_cast<clang::BuiltinType>(&in_type) )
		return CreateFundamentalTypeName( GetUFundamentalType( *built_in_type ) );
	else if( const auto typedef_type= llvm::dyn_cast<clang::TypedefType>(&in_type) )
	{
		// Normally we should create entries for typedefs in types map.
		// But if this doesn't work, use underlying type instead.
		return TranslateType( *typedef_type->desugar().getTypePtr(), type_names_map );
	}
	else if( const clang::RecordType* record_type= llvm::dyn_cast<clang::RecordType>(&in_type) )
	{
		// Normally we should create entries for records in types map.
		// Here we process some special types.

		if( const auto decl= record_type->getDecl() )
			if( decl->getIdentifier() != nullptr && decl->getName() == "__va_list_tag" )
				return CreateFundamentalTypeName( Keyword( Keywords::byte8_ ) );
	}
	else if( const auto atomic_type= llvm::dyn_cast<clang::AtomicType>(&in_type) )
	{
		// For now translate atomic types as underlying types.
		return TranslateType( *atomic_type->getValueType().getTypePtr(), type_names_map );
	}
	else if( const auto complex_type= llvm::dyn_cast<clang::ComplexType>(&in_type) )
	{
		// For now translate complex types as arrays of two values of underlying types.
		auto array_type= std::make_unique<Synt::ArrayTypeName>(g_dummy_src_loc);
		array_type->element_type= TranslateType( *complex_type->getElementType().getTypePtr(), type_names_map );

		Synt::IntegerNumericConstant numeric_constant( g_dummy_src_loc );
		numeric_constant.num= "2";
		array_type->size= std::make_unique< Synt::IntegerNumericConstant >( std::move(numeric_constant) );

		return std::move(array_type);
	}
	else if( const auto constant_array_type= llvm::dyn_cast<clang::ConstantArrayType>(&in_type) )
	{
		// For arrays with constant size use normal Ü array.
		auto array_type= std::make_unique<Synt::ArrayTypeName>(g_dummy_src_loc);
		array_type->element_type= TranslateType( *constant_array_type->getElementType().getTypePtr(), type_names_map );

		array_type->size=
			std::make_unique< Synt::IntegerNumericConstant >(
				TranslateNumericConstant( constant_array_type->getSize() ) );

		return std::move(array_type);
	}
	else if( const auto incomplete_array_type= llvm::dyn_cast<clang::IncompleteArrayType>(&in_type) )
	{
		// Translate incomplete array types as raw pointers.
		auto raw_pointer_type= std::make_unique<Synt::RawPointerType>( g_dummy_src_loc );
		raw_pointer_type->element_type= TranslateType( *incomplete_array_type->getArrayElementTypeNoTypeQual(), type_names_map );
		return std::move(raw_pointer_type);
	}
	else if( const auto array_type= llvm::dyn_cast<clang::ArrayType>(&in_type) )
	{
		// For other kinds of array types use zero size.
		auto out_array_type= std::make_unique<Synt::ArrayTypeName>(g_dummy_src_loc);
		out_array_type->element_type= TranslateType( *array_type->getElementType().getTypePtr(), type_names_map );

		Synt::IntegerNumericConstant numeric_constant( g_dummy_src_loc );
		numeric_constant.num= "0";
		out_array_type->size= std::make_unique< Synt::IntegerNumericConstant >( std::move(numeric_constant) );

		return std::move(out_array_type);
	}
	else if( const auto vector_type= llvm::dyn_cast<clang::VectorType>( &in_type ) )
	{
		// For now translate vector types as arrays.
		// It's not fully correct, since vector types may use custom alignment larger than element aligment.
		// But at least result size matches, which is important if such type is used for a struct field.

		auto out_array_type= std::make_unique<Synt::ArrayTypeName>(g_dummy_src_loc);
		out_array_type->element_type= TranslateType( *vector_type->getElementType().getTypePtr(), type_names_map );

		Synt::IntegerNumericConstant numeric_constant( g_dummy_src_loc );
		numeric_constant.num= std::to_string( vector_type->getNumElements() );
		out_array_type->size= std::make_unique< Synt::IntegerNumericConstant >( std::move(numeric_constant) );

		return std::move(out_array_type);
	}
	else if( const auto decayed_type= llvm::dyn_cast<clang::DecayedType>(&in_type) )
	{
		// Decayed type - implicit array to pointer conversion.
		auto raw_pointer_type= std::make_unique<Synt::RawPointerType>( g_dummy_src_loc );
		raw_pointer_type->element_type= TranslateType( *decayed_type->getPointeeType().getTypePtr(), type_names_map );

		return std::move(raw_pointer_type);
	}
	else if( in_type.isFunctionPointerType() )
	{
		const clang::Type* function_type= in_type.getPointeeType().getTypePtr();

		while(true)
		{
			if( const auto pointer_type= llvm::dyn_cast<clang::PointerType>( function_type ) )
				function_type= pointer_type->getPointeeType().getTypePtr();
			else if( const auto paren_type= llvm::dyn_cast<clang::ParenType>( function_type ) )
				function_type= paren_type->getInnerType().getTypePtr();
			else if( const auto elaborated_type= llvm::dyn_cast<clang::ElaboratedType>( function_type ) )
				function_type= elaborated_type->desugar().getTypePtr();
			else if( const auto attributed_type= llvm::dyn_cast<clang::AttributedType>( function_type ) )
				function_type= attributed_type->desugar().getTypePtr(); // TODO - maybe collect such attributes?
			else if( const auto typedef_type= llvm::dyn_cast<clang::TypedefType>( function_type ) )
			{
				const auto aliased_type= typedef_type->desugar().getTypePtr();
				if( aliased_type == nullptr )
					break;
				function_type= aliased_type;
			}
			else
				break;
		}

		if( const auto function_proto_type= llvm::dyn_cast<clang::FunctionProtoType>( function_type ) )
			return std::make_unique<Synt::FunctionType>( TranslateFunctionType( *function_proto_type, type_names_map ) );
		else if( const auto function_no_proto_type= llvm::dyn_cast<clang::FunctionNoProtoType>( function_type ) )
			return std::make_unique<Synt::FunctionType>( TranslateFunctionType( *function_no_proto_type, type_names_map ) );
	}
	else if( llvm::isa<clang::FunctionProtoType>( &in_type ) )
	{
		// This is function type and not function pointer type.
		// This is typical in typedefs.
		// We can't transalte such types, so, use void stub.
		return CreateFundamentalTypeName( Keyword( Keywords::void_ ) );
	}
	else if( const auto pointer_type= llvm::dyn_cast<clang::PointerType>(&in_type) )
	{
		auto raw_pointer_type= std::make_unique<Synt::RawPointerType>( g_dummy_src_loc );
		raw_pointer_type->element_type= TranslateType( *pointer_type->getPointeeType().getTypePtr(), type_names_map );

		return std::move(raw_pointer_type);
	}
	else if( const auto reference_type= llvm::dyn_cast<clang::ReferenceType>(&in_type) )
	{
		// Translate C++ references as raw pointers, since they are not so limited as Ü references.
		auto raw_pointer_type= std::make_unique<Synt::RawPointerType>( g_dummy_src_loc );
		raw_pointer_type->element_type= TranslateType( *reference_type->getPointeeType().getTypePtr(), type_names_map );

		return std::move(raw_pointer_type);
	}
	else if( const auto decltype_type= llvm::dyn_cast<clang::DecltypeType>( &in_type ) )
		return TranslateType( *decltype_type->desugar().getTypePtr(), type_names_map );
	else if( const auto paren_type= llvm::dyn_cast<clang::ParenType>( &in_type ) )
		return TranslateType( *paren_type->getInnerType().getTypePtr(), type_names_map );
	else if( const auto elaborated_type= llvm::dyn_cast<clang::ElaboratedType>( &in_type ) )
		return TranslateType( *elaborated_type->desugar().getTypePtr(), type_names_map );
	else if( const auto attributed_type= llvm::dyn_cast<clang::AttributedType>( &in_type ) )
		return TranslateType( *attributed_type->desugar().getTypePtr(), type_names_map ); // TODO - maybe process attributes?
	else if( const auto auto_type= llvm::dyn_cast<clang::AutoType>( &in_type ) )
	{
		const clang::QualType deduced_type= auto_type->getDeducedType();
		if( !deduced_type.isNull() )
			return TranslateType( *deduced_type.getTypePtr(), type_names_map );
	}

	// Fallback for some unlikely case.
	return CreateFundamentalTypeName( Keyword( Keywords::void_ ) );
}

std::string_view CppAstConsumer::GetUFundamentalType( const clang::BuiltinType& in_type )
{
	switch( in_type.getKind() )
	{
	case clang::BuiltinType::Void:
		// "void" in C used in two roles - as pointer to untyped bytes and as return value for functions returning nothing.
		// Handle the first case here and process functions return-void specially later.
		return Keyword( Keywords::byte8_ );

	case clang::BuiltinType::Bool: return Keyword( Keywords::bool_ );

	case clang::BuiltinType::Char_S:
	case clang::BuiltinType::Char_U:
	case clang::BuiltinType::Char8:
		return Keyword( Keywords::char8_ );

	case clang::BuiltinType::Char16: return Keyword( Keywords::char16_ );
	case clang::BuiltinType::Char32: return Keyword( Keywords::char32_ );

	// nullptr_t is a pointer type.
	case clang::BuiltinType::NullPtr: return "$(byte8)";

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

Synt::FunctionType CppAstConsumer::TranslateFunctionType( const clang::FunctionProtoType& in_type, const TypeNamesMap& type_names_map )
{
	// Translate info other than params.
	Synt::FunctionType function_type= TranslateFunctionType( static_cast<const clang::FunctionType&>(in_type), type_names_map );

	// Translate params.

	function_type.params.reserve( in_type.getNumParams() );
	size_t i= 0u;
	for( const clang::QualType& param_qual : in_type.getParamTypes() )
	{
		Synt::FunctionParam param( g_dummy_src_loc );
		param.name= "arg" + std::to_string(i);

		param.type= TranslateType( *param_qual.getTypePtr(), type_names_map );
		function_type.params.push_back(std::move(param));
		++i;
	}

	return function_type;
}

Synt::FunctionType CppAstConsumer::TranslateFunctionType( const clang::FunctionType& in_type, const TypeNamesMap& type_names_map )
{
	// Translate function without information about params.
	// This is somewhat limited.

	Synt::FunctionType function_type( g_dummy_src_loc );

	function_type.unsafe= true; // All C/C++ functions are unsafe.

	const clang::Type* const return_type= in_type.getReturnType().getTypePtr();
	function_type.return_type= std::make_unique<Synt::TypeName>( TranslateType( *return_type, type_names_map ) );

	const clang::Type* return_type_desugared= return_type;
	while(true)
	{
		if( const auto typedef_type= llvm::dyn_cast<clang::TypedefType>(return_type_desugared) )
			return_type_desugared= typedef_type->desugar().getTypePtr();
		else if( const auto elaborated_type= llvm::dyn_cast<clang::ElaboratedType>(return_type_desugared) )
			return_type_desugared= elaborated_type->desugar().getTypePtr();
		else
			break;
	}

	if( const auto built_in_type= llvm::dyn_cast<clang::BuiltinType>(return_type_desugared) )
	{
		if( built_in_type->getKind() == clang::BuiltinType::Void )
		{
			// Process specially functions returning "void".
			// Use "void" from Ü only for them.
			// Remove return type sepcifying, since in Ü this means default-void.
			function_type.return_type= nullptr;
		}
	}

	function_type.calling_convention= TranslateCallingConvention( in_type );

	return function_type;
}

void CppAstConsumer::PrepareIgnoreDirectives()
{
	// We are not sure that ignore macros are emitted in their natural order.
	// So, sort them to be able to use binary search later.
	std::sort(
		ignore_macros_->begin(),
		ignore_macros_->end(),
		[&]( const CppHeaderConverterIgnoreMacro& l, const CppHeaderConverterIgnoreMacro& r )
		{
			return source_manager_.isBeforeInTranslationUnit( l.location, r.location );
		} );
}

bool CppAstConsumer::ShouldSkipEmittingItem( const clang::SourceLocation& location )
{
	if( location.isInvalid() )
		return false;

	// Use binary search in case if we have a lot of "ignore" directives.

	const CppHeaderConverterIgnoreMacros& ignore_macros= *ignore_macros_;

	const auto it=
		std::lower_bound(
			ignore_macros.begin(),
			ignore_macros.end(),
			location,
			[&]( const CppHeaderConverterIgnoreMacro& l, const clang::SourceLocation& r )
			{
				return source_manager_.isBeforeInTranslationUnit( l.location, r );
			} );

	if( it == ignore_macros.begin() )
		return false; // The given location is before the first "ignore" directive.

	const CppHeaderConverterIgnoreMacro& closest_ignore_macro_before= *std::prev( it );

	// If it's an ignore definition - skip the given item, else (if it's undefinition) - preserve it.
	switch( closest_ignore_macro_before.kind )
	{
	case CppHeaderConverterIgnoreMacro::Kind::Define: return true;
	case CppHeaderConverterIgnoreMacro::Kind::Undefine: return false;
	};
	U_ASSERT(false);
	return false;
}

void CppAstConsumer::BuildTypeNamesMap( TypeNamesMap& map, ItemFullName& prefix, const NamespaceItem& item )
{
	std::visit( [&]( const auto& t ) { BuildTypeNamesMapImpl( map, prefix, t ); }, item );
}

void CppAstConsumer::BuildTypeNamesMapImpl( TypeNamesMap& map, ItemFullName& prefix, const NamespaceItemNamespace& item )
{
	for( const auto& pair : item.items )
	{
		prefix.push_back( pair.getKey().str() );
		BuildTypeNamesMap( map, prefix, pair.getValue() );
		prefix.pop_back();
	}
}

void CppAstConsumer::BuildTypeNamesMapImpl( TypeNamesMap& map, ItemFullName& prefix, const clang::FunctionDecl* const item )
{
	(void)map;
	(void)prefix;
	(void)item;
}

void CppAstConsumer::BuildTypeNamesMapImpl( TypeNamesMap& map, ItemFullName& prefix, const NamespaceItemRecord& item )
{
	map.emplace( item.record_decl->getTypeForDecl(), prefix );

	for( const auto& pair : item.items )
	{
		prefix.push_back( pair.getKey().str() );
		BuildTypeNamesMap( map, prefix, pair.getValue() );
		prefix.pop_back();
	}
}

void CppAstConsumer::BuildTypeNamesMapImpl( TypeNamesMap& map, ItemFullName& prefix, const clang::TypedefNameDecl* const item )
{
	map.emplace( ast_context_.getTypedefType( item ).getTypePtr(), prefix );
}

void CppAstConsumer::BuildTypeNamesMapImpl( TypeNamesMap& map, ItemFullName& prefix, const clang::EnumDecl* const item )
{
	map.emplace( item->getTypeForDecl(), prefix );
}

void CppAstConsumer::BuildTypeNamesMapImpl( TypeNamesMap& map, ItemFullName& prefix, const NamespaceItemEnumElement& item )
{
	(void)map;
	(void)prefix;
	(void)item;
}

void CppAstConsumer::BuildTypeNamesMapImpl( TypeNamesMap& map, ItemFullName& prefix, const clang::VarDecl* const item )
{
	(void)map;
	(void)prefix;
	(void)item;
}

void CppAstConsumer::CollectSubrecords( NamespaceItem& item )
{
	if( const auto namespace_= std::get_if< NamespaceItemNamespace >( &item ) )
	{
		for( auto& pair : namespace_->items )
			CollectSubrecords( pair.second );
	}
	else if( const auto record_item	= std::get_if< NamespaceItemRecord >( &item ) )
	{
		for( const clang::Decl* const sub_decl : record_item->record_decl->decls() )
		{
			if( const auto subrecord= llvm::dyn_cast<clang::RecordDecl>( sub_decl ) )
			{
				if( !subrecord->isTemplated() )
				{
					const auto src_name= subrecord->getName();

					const std::string name_translated=
						src_name.empty()
							? GetAnonymousItemUniqueName( g_anon_record_prefix, subrecord->getLocation() )
							: TranslateIdentifier( src_name );

					NamespaceItemsMap& items_map=
						std::get< NamespaceItemNamespace >( record_item->items[ subrecord->isUnion() ? g_union_kind_tag : g_struct_kind_tag ] ).items;

					// Insert if has no entry, replace if is complete definition.
					if( items_map.count( name_translated ) == 0 || subrecord->isCompleteDefinition() )
					{
						NamespaceItem& subrecord_item=
							items_map.insert_or_assign( name_translated, NamespaceItemRecord{ subrecord, NamespaceItemsMap() } ).first->second;
						if( subrecord->isCompleteDefinition() )
							CollectSubrecords( subrecord_item );
					}
				}
			}
		}
	}
}

template<typename ListBuilder>
void CppAstConsumer::EmitItemsSorted( ListBuilder& out_items, const TypeNamesMap& type_names_map, const NamespaceItemsMap& items )
{
	// Emit namespace elements in their definition order.
	// If can't do this - sort by name.
	// Using definition order increases readability.
	// Sorting names with no location is important to produce deterministic result and avoid emitting items in hash-map order.

	struct ItemToSort
	{
		clang::SourceLocation location;
		llvm::StringRef name; // Points to the entry in the items map given to this function.
	};

	std::vector<ItemToSort> items_to_sort_by_location;
	std::vector<llvm::StringRef> items_to_sort_by_name;

	for( const auto& pair : items )
	{
		const clang::SourceLocation location= GetNamespaceItemSourceLocation( pair.second );

		if( location.isValid() )
		{
			if( !ShouldSkipEmittingItem( location ) )
				items_to_sort_by_location.push_back( ItemToSort{ location, pair.getKey() } );
		}
		else
			items_to_sort_by_name.push_back( pair.getKey() );
	}

	std::sort(
		items_to_sort_by_location.begin(),
		items_to_sort_by_location.end(),
		[&]( const ItemToSort& l, const ItemToSort& r )
		{
			return source_manager_.isBeforeInTranslationUnit( l.location, r.location );
		} );

	for( const ItemToSort& item_to_sort : items_to_sort_by_location )
	{
		const NamespaceItem& item= items.find( item_to_sort.name )->second;

		if( const auto namespace_= std::get_if< NamespaceItemNamespace >( &item ) )
		{
			// HACK! Emit a namespace if can, otherwise emit a class.
			if constexpr( std::is_same_v< ListBuilder, Synt::ProgramElementsList::Builder > )
				out_items.Append( EmitNamespaceItem( type_names_map, item_to_sort.name, *namespace_ ) );
			else
				out_items.Append( EmitItemImpl( type_names_map, item_to_sort.name, *namespace_ ) );
		}
		else
			std::visit(
				[&]( const auto& t ) { out_items.Append( EmitItemImpl( type_names_map, item_to_sort.name, t ) ); },
				item );
	}

	std::sort( items_to_sort_by_name.begin(), items_to_sort_by_name.end() );

	for( const llvm::StringRef item_name : items_to_sort_by_name )
	{
		const NamespaceItem& item= items.find( item_name )->second;

		if( const auto namespace_= std::get_if< NamespaceItemNamespace >( &item ) )
		{
			// HACK! Emit a namespace if can, otherwise emit a class.
			if constexpr( std::is_same_v< ListBuilder, Synt::ProgramElementsList::Builder > )
				out_items.Append( EmitNamespaceItem( type_names_map, item_name, *namespace_ ) );
			else
				out_items.Append( EmitItemImpl( type_names_map, item_name, *namespace_ ) );
		}
		else
			std::visit(
				[&]( const auto& t ) { out_items.Append( EmitItemImpl( type_names_map, item_name, t ) ); },
				item );
	}
}

Synt::Namespace CppAstConsumer::EmitNamespaceItem(
	const TypeNamesMap& type_names_map, std::string_view name, const NamespaceItemNamespace& item )
{
	Synt::ProgramElementsList::Builder items_builder;

	EmitItemsSorted( items_builder, type_names_map, item.items );

	Synt::Namespace namespace_( g_dummy_src_loc );
	namespace_.name= name;
	namespace_.elements= items_builder.Build();

	return namespace_;
}

Synt::Class CppAstConsumer::EmitItemImpl(
	const TypeNamesMap& type_names_map, const std::string_view name, const NamespaceItemNamespace& item )
{
	Synt::ClassElementsList::Builder class_items_builder;

	EmitItemsSorted( class_items_builder, type_names_map, item.items );

	Synt::Class class_( g_dummy_src_loc );
	class_.name= name;
	class_.elements= class_items_builder.Build();

	return class_;
}

Synt::Function CppAstConsumer::EmitItemImpl(
	const TypeNamesMap& type_names_map, const std::string_view name, const clang::FunctionDecl* const item )
{
	const clang::FunctionDecl& function_decl= *item;

	Synt::Function func(g_dummy_src_loc);

	func.name.push_back( Synt::Function::NameComponent{ std::string( name ), g_dummy_src_loc } );

	func.no_mangle= true; // For now import only C functions without mangling.

	if( function_decl.hasAttr<clang::WarnUnusedResultAttr>() )
		func.no_discard= true;

	func.type= TranslateFunctionType( *function_decl.getFunctionType(), type_names_map );

	func.type.params.reserve( function_decl.param_size() );
	size_t i= 0u;
	for( const clang::ParmVarDecl* const in_param : function_decl.parameters() )
	{
		Synt::FunctionParam out_param( g_dummy_src_loc );

		const auto src_name= in_param->getName();
		if( src_name.empty() )
			out_param.name= "param" + std::to_string(i);
		else
			out_param.name= TranslateIdentifier( src_name );

		out_param.type= TranslateType( *in_param->getType().getTypePtr(), type_names_map );
		func.type.params.push_back(std::move(out_param));
		++i;
	}

	return func;
}

Synt::Class CppAstConsumer::EmitItemImpl(
	const TypeNamesMap& type_names_map, const std::string_view name, const NamespaceItemRecord& item )
{
	const clang::RecordDecl& record_declaration= *item.record_decl;

	Synt::Class class_(g_dummy_src_loc);
	class_.name= name;
	class_.keep_fields_order= true; // C/C++ structs/classes have fixed fields order.

	if( record_declaration.hasAttr<clang::WarnUnusedResultAttr>() )
		class_.no_discard= true;

	if( record_declaration.isStruct() || record_declaration.isClass() )
	{
		if( record_declaration.isCompleteDefinition() )
		{
			bool has_bitfields= false;
			for( const clang::FieldDecl* const field_declaration : record_declaration.fields() )
			{
				if( field_declaration->isBitField() )
				{
					has_bitfields= true;
					break;
				}
			}

			bool is_empty= record_declaration.fields().empty();

			Synt::ClassElementsList::Builder class_elements;

			if( const auto cxx_record= llvm::dyn_cast<clang::CXXRecordDecl>( &record_declaration ) )
			{
				size_t num_bases= 0;
				bool has_polymorphic_base= false;
				for( const clang::CXXBaseSpecifier& base : cxx_record->bases() )
				{
					Synt::ClassField field( g_dummy_src_loc );
					field.type= TranslateType( *base.getType().getTypePtr(), type_names_map );
					field.name= "ü_base" + std::to_string( num_bases );

					class_elements.Append( std::move(field) );

					++num_bases;

					const clang::Type* base_type= base.getType().getTypePtr();
					while(true)
					{
						if( const auto paren_type= llvm::dyn_cast<clang::ParenType>( base_type ) )
							base_type= paren_type->getInnerType().getTypePtr();
						else if( const auto elaborated_type= llvm::dyn_cast<clang::ElaboratedType>( base_type ) )
							base_type= elaborated_type->desugar().getTypePtr();
						else if( const auto attributed_type= llvm::dyn_cast<clang::AttributedType>( base_type ) )
							base_type= attributed_type->desugar().getTypePtr(); // TODO - maybe collect such attributes?
						else if( const auto typedef_type= llvm::dyn_cast<clang::TypedefType>( base_type ) )
						{
							const auto aliased_type= typedef_type->desugar().getTypePtr();
							if( aliased_type == nullptr )
								break;
							base_type= aliased_type;
						}
						else
							break;
					}

					if( const clang::RecordType* const base_record_type= llvm::dyn_cast<clang::RecordType>( base_type ) )
					{
						if( const auto cxx_base_record= llvm::dyn_cast<clang::CXXRecordDecl>( base_record_type->getDecl() ) )
							has_polymorphic_base|= cxx_base_record->isPolymorphic();
					}
				}


				if( cxx_record->isPolymorphic() && !has_polymorphic_base )
				{
					is_empty= false;

					Synt::ClassField field( g_dummy_src_loc );

					Synt::NameLookup byte8_type( g_dummy_src_loc );
					byte8_type.name= Keyword( Keywords::byte8_ );

					Synt::RawPointerType raw_pointer_type( g_dummy_src_loc );
					raw_pointer_type.element_type= std::move(byte8_type);

					field.type= std::make_unique<Synt::RawPointerType>( std::move( raw_pointer_type ) );

					field.name= "ü_vptr";

					class_elements.Append( std::move(field) );
				}
			}

			if( has_bitfields )
			{
				// Ü has no bitfields support. And generally we can't replace C bitfields with something else.
				// So, for now just create stub struct.
				class_.elements= MakeOpaqueRecordElements( type_names_map, record_declaration, "struct_with_bitfields", item.items );
			}
			else if( is_empty )
			{
				// If struct has no fields we may still need to create some fields for it.
				// It may be necessary in C++ mode, where empty structs have size 1.
				class_.elements= MakeOpaqueRecordElements( type_names_map, record_declaration, "empty_struct", item.items );
			}
			else
			{
				uint32_t anonymous_field_index= 0u;

				for( const clang::FieldDecl* const field_declaration : record_declaration.fields() )
				{
					Synt::ClassField field( g_dummy_src_loc );

					const clang::Type& field_type= *field_declaration->getType().getTypePtr();

					if( record_declaration.hasFlexibleArrayMember() && field_type.isIncompleteArrayType() )
					{
						// Create a zero-sized array for a flexible array member.
						auto array_type= std::make_unique<Synt::ArrayTypeName>(g_dummy_src_loc);
						array_type->element_type= TranslateType( *field_type.getArrayElementTypeNoTypeQual(), type_names_map );

						Synt::IntegerNumericConstant numeric_constant( g_dummy_src_loc );
						numeric_constant.num= "0";
						array_type->size= std::make_unique< Synt::IntegerNumericConstant>( std::move(numeric_constant) );

						field.type= std::move( array_type );
					}
					else
						field.type= TranslateType( field_type, type_names_map );

					const auto src_name= field_declaration->getName();

					if( src_name.empty() )
					{
						// Name anonymous fields sequentially by using a simple counter.
						// It's fine, since these names should be unique only within this struct.
						field.name= g_anon_field_prefix + std::to_string( anonymous_field_index );
						++anonymous_field_index;
					}
					else
						field.name= TranslateIdentifier( src_name );

					class_elements.Append( std::move(field) );
				}

				// Assume we have at least one field (which is necessary for all structs in C).

				EmitItemsSorted( class_elements, type_names_map, item.items );

				class_.elements= class_elements.Build();
			}
		}
		else
		{
			// Mark is at "class" to make it non copy-constructible.
			class_.kind_attribute= Synt::ClassKindAttribute::Class;

			// Add deleted default constructor.
			Synt::ClassElementsList::Builder class_elements;
			class_elements.Append( GetDeletedDefaultConstructor() );
			class_.elements= class_elements.Build();
		}
	}
	else if( record_declaration.isUnion() )
	{
		// Emulate union, using array of bytes with required alignment.

		if( record_declaration.isCompleteDefinition() )
			class_.elements= MakeOpaqueRecordElements( type_names_map, record_declaration, "union", item.items );
		else
		{
			// Mark is at "class" to make it non copy-constructible.
			class_.kind_attribute= Synt::ClassKindAttribute::Class;

			// Add deleted default constructor.
			Synt::ClassElementsList::Builder class_elements;
			class_elements.Append( GetDeletedDefaultConstructor() );

			EmitItemsSorted( class_elements, type_names_map, item.items );

			class_.elements= class_elements.Build();
		}
	}

	return class_;
}

Synt::TypeAlias CppAstConsumer::EmitItemImpl(
	const TypeNamesMap& type_names_map, const std::string_view name, const clang::TypedefNameDecl* const item )
{
	Synt::TypeAlias type_alias( g_dummy_src_loc );
	type_alias.name= name;
	type_alias.value= TranslateType( *item->getUnderlyingType().getTypePtr(), type_names_map );

	return type_alias;
}

Synt::TypeAlias CppAstConsumer::EmitItemImpl(
	const TypeNamesMap& type_names_map, const std::string_view name, const clang::EnumDecl* const item )
{
	(void)type_names_map;

	const clang::EnumDecl& enum_declaration= *item;

	// Always create a type alias and a bunch of enumerator constants for C and C++ enums.
	// We can't use Ü enums, since C enums may be unsequentional and it's not guaranteed, that a varible of enum type has one of the listed values.
	// We can't use a wrapper struct, since on some ABIs structs are passed differently from enums.
	// For C++ scoped enums we create a namespace with "_" postfix, where all enumerators are stored.
	// A type alias is created even for an anonymous enum, since such enum may be used inside "typedef" and we need some name for typedef source type (even if it's generated).

	Synt::TypeAlias type_alias( g_dummy_src_loc );
	type_alias.name= name;

	if( enum_declaration.isComplete() )
	{
		std::string_view underlying_type_name;
		if( const auto built_in_type= llvm::dyn_cast<clang::BuiltinType>( enum_declaration.getIntegerType().getTypePtr() ) )
			underlying_type_name= GetUFundamentalType( *built_in_type );
		else if( const auto built_in_type= llvm::dyn_cast<clang::BuiltinType>( enum_declaration.getPromotionType().getTypePtr() ) )
			underlying_type_name= GetUFundamentalType( *built_in_type );
		else
		{
			// Some very strange enum. Assume it's int.
			// This strange code from Darwin header "vm_types.h" produces such enum.
			/*
				__enum_decl(mach_vm_range_flavor_t, uint32_t, {
					MACH_VM_RANGE_FLAVOR_INVALID,
					MACH_VM_RANGE_FLAVOR_V1,
				});
			*/
			if( const auto built_in_type= llvm::dyn_cast<clang::BuiltinType>( ast_context_.IntTy.getTypePtr() ) )
				underlying_type_name= GetUFundamentalType(* built_in_type );
			else
				underlying_type_name= Keyword( Keywords::i32_ );
		}

		type_alias.value= CreateFundamentalTypeName( underlying_type_name );
	}
	else
	{
		// use "void" as alis for incomplete enums.
		type_alias.value= CreateFundamentalTypeName( Keyword( Keywords::void_ ) );
	}

	return type_alias;
}

Synt::VariablesDeclaration CppAstConsumer::EmitItemImpl(
	const TypeNamesMap& type_names_map, const std::string_view name, const NamespaceItemEnumElement& item )
{
	Synt::VariablesDeclaration variables_declaration( g_dummy_src_loc );
	variables_declaration.type= TranslateType( *item.enum_type, type_names_map );

	{
		Synt::VariablesDeclaration::VariableEntry var;
		var.src_loc= g_dummy_src_loc;
		var.name= std::move(name);

		var.mutability_modifier= Synt::MutabilityModifier::Constexpr;

		{
			Synt::ConstructorInitializer constructor_initializer( g_dummy_src_loc );

			const llvm::APSInt val= item.enum_constant_decl->getInitVal();

			if( val.isNegative() )
			{
				Synt::UnaryMinus unary_minus( g_dummy_src_loc );
				unary_minus.expression= std::make_unique< Synt::IntegerNumericConstant>( TranslateNumericConstant( -val ) );

				constructor_initializer.arguments.push_back(
					std::make_unique<const Synt::UnaryMinus>( std::move( unary_minus ) ) );
			}
			else
				constructor_initializer.arguments.push_back(
					std::make_unique< Synt::IntegerNumericConstant>( TranslateNumericConstant( val ) ) );

			var.initializer= std::make_unique<Synt::Initializer>( std::move(constructor_initializer) );
		}

		variables_declaration.variables.push_back( std::move( var ) );
	}

	return variables_declaration;
}

Synt::VariablesDeclaration CppAstConsumer::EmitItemImpl(
	const TypeNamesMap& type_names_map, const std::string_view name, const clang::VarDecl* const item )
{
	const clang::VarDecl& variable= *item;

	Synt::VariablesDeclaration variables_declaration( g_dummy_src_loc );
	variables_declaration.type= TranslateType( *variable.getType().getTypePtr(), type_names_map );

	const clang::APValue* const init_val= variable.evaluateValue();
	if( init_val == nullptr )
		return variables_declaration;

	const clang::Type* variable_type= variable.getType().getTypePtr();
	while(true)
	{
		if( const auto paren_type= llvm::dyn_cast<clang::ParenType>( variable_type ) )
			variable_type= paren_type->getInnerType().getTypePtr();
		else if( const auto elaborated_type= llvm::dyn_cast<clang::ElaboratedType>( variable_type ) )
			variable_type= elaborated_type->desugar().getTypePtr();
		else if( const auto attributed_type= llvm::dyn_cast<clang::AttributedType>( variable_type ) )
			variable_type= attributed_type->desugar().getTypePtr(); // TODO - maybe collect such attributes?
		else if( const auto typedef_type= llvm::dyn_cast<clang::TypedefType>( variable_type ) )
		{
			const auto aliased_type= typedef_type->desugar().getTypePtr();
			if( aliased_type == nullptr )
				break;
			variable_type= aliased_type;
		}
		else if( const auto auto_type= llvm::dyn_cast<clang::AutoType>( variable_type ) )
		{
			const clang::QualType deduced_type= auto_type->getDeducedType();
			if( !deduced_type.isNull() )
				variable_type= deduced_type.getTypePtr();
			else
				break;
		}
		else
			break;
	}

	Synt::Initializer initializer= TranslateVariableInitializer_r( *variable_type, *init_val );
	if( std::holds_alternative<Synt::EmptyVariant>( initializer ) )
		return variables_declaration;

	Synt::VariablesDeclaration::VariableEntry entry;
	entry.src_loc= g_dummy_src_loc;
	entry.name= name;
	entry.initializer= std::make_unique<Synt::Initializer>( std::move(initializer ) );
	entry.mutability_modifier= Synt::MutabilityModifier::Constexpr;

	variables_declaration.variables.push_back( std::move(entry) );

	return variables_declaration;
}

Synt::ClassElementsList CppAstConsumer::MakeOpaqueRecordElements(
	const TypeNamesMap& type_names_map,
	const clang::RecordDecl& record_declaration,
	const std::string_view kind_name,
	const NamespaceItemsMap& subitems )
{
	const auto size= ( ast_context_.getTypeSize( record_declaration.getTypeForDecl() ) + 7u ) / 8u;
	const auto byte_size= ( ast_context_.getTypeAlign( record_declaration.getTypeForDecl() ) + 7u ) / 8u;
	const auto num_elements= ( size + byte_size - 1u ) / byte_size;

	std::string_view byte_name;
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
	array_type->element_type= CreateFundamentalTypeName( byte_name );

	Synt::IntegerNumericConstant numeric_constant( g_dummy_src_loc );
	numeric_constant.num= std::to_string( num_elements );
	array_type->size= std::make_unique< Synt::IntegerNumericConstant>( std::move(numeric_constant) );

	Synt::ClassField field( g_dummy_src_loc );
	field.name+= kind_name;
	field.name+= "_contents";
	field.type= std::move(array_type);

	Synt::ClassElementsList::Builder class_elements;
	class_elements.Append( std::move(field) );

	EmitItemsSorted( class_elements, type_names_map, subitems );

	return class_elements.Build();
}

Synt::Initializer CppAstConsumer::TranslateVariableInitializer_r( const clang::Type& variable_type, const clang::APValue& value )
{
	if( value.isInt() )
	{
		const llvm::APInt init_val_int= value.getInt();

		Synt::ConstructorInitializer initializer( g_dummy_src_loc );

		if( variable_type.isSignedIntegerType() && init_val_int.isNegative() )
		{
			Synt::UnaryMinus unary_minus( g_dummy_src_loc );
			unary_minus.expression= std::make_unique< Synt::IntegerNumericConstant >( TranslateNumericConstant( -init_val_int ) );

			initializer.arguments.push_back( std::make_unique<const Synt::UnaryMinus>( std::move( unary_minus ) ) );
		}
		else
			initializer.arguments.push_back(
				std::make_unique< Synt::IntegerNumericConstant >( TranslateNumericConstant( init_val_int ) ) );

		return std::move(initializer);
	}
	else if( value.isFloat() )
	{
		const llvm::APFloat init_val_float= value.getFloat();

		Synt::ConstructorInitializer initializer( g_dummy_src_loc );

		if( init_val_float.isNegative() )
		{
			Synt::UnaryMinus unary_minus( g_dummy_src_loc );
			unary_minus.expression=
				std::make_unique< Synt::FloatingPointNumericConstant >( TranslateNumericConstant( -init_val_float ) );

			initializer.arguments.push_back( std::make_unique<const Synt::UnaryMinus>( std::move( unary_minus ) ) );
		}
		else
			initializer.arguments.push_back(
				std::make_unique< Synt::FloatingPointNumericConstant >( TranslateNumericConstant( init_val_float ) ) );

		return std::move(initializer);
	}
	else if( value.isArray() )
	{
		if( const auto array_type= llvm::dyn_cast<clang::ConstantArrayType>( &variable_type ) )
		{
			if( array_type->getSize() == value.getArrayInitializedElts() )
			{
				const clang::Type* element_type= array_type->getElementType().getTypePtr();
				while(true)
				{
					if( const auto paren_type= llvm::dyn_cast<clang::ParenType>( element_type ) )
						element_type= paren_type->getInnerType().getTypePtr();
					else if( const auto elaborated_type= llvm::dyn_cast<clang::ElaboratedType>( element_type ) )
						element_type= elaborated_type->desugar().getTypePtr();
					else if( const auto attributed_type= llvm::dyn_cast<clang::AttributedType>( element_type ) )
						element_type= attributed_type->desugar().getTypePtr(); // TODO - maybe collect such attributes?
					else if( const auto typedef_type= llvm::dyn_cast<clang::TypedefType>( element_type ) )
					{
						const auto aliased_type= typedef_type->desugar().getTypePtr();
						if( aliased_type == nullptr )
							break;
						element_type= aliased_type;
					}
					else
						break;
				}

				std::vector<Synt::Initializer> element_initializers;
				element_initializers.reserve( value.getArraySize() );
				for( uint32_t i= 0; i < value.getArraySize(); ++i )
				{
					Synt::Initializer element_initializer= TranslateVariableInitializer_r( *element_type, value.getArrayInitializedElt( i ) );
					if( std::holds_alternative<Synt::EmptyVariant>( element_initializer ) )
						return Synt::EmptyVariant();

					element_initializers.push_back( std::move(element_initializer) );
				}

				Synt::SequenceInitializer initializer( g_dummy_src_loc );
				initializer.initializers= std::move(element_initializers);

				return std::move(initializer);
			}
		}
	}

	// For now unsupported kind.
	return Synt::EmptyVariant();
}

void CppAstConsumer::EmitDefinitionsForMacros( Synt::ProgramElementsList::Builder& out_items )
{
	llvm::StringSet translated_function_names, translated_type_names, translated_variable_names;
	for( const auto& pair : root_namespace_.items )
	{
		if( std::holds_alternative< const clang::FunctionDecl* >( pair.getValue() ) )
			translated_function_names.insert( pair.getKey() );
		else if(
			std::holds_alternative< NamespaceItemRecord >( pair.getValue() ) ||
			std::holds_alternative< const clang::TypedefNameDecl* >( pair.getValue() ) ||
			std::holds_alternative< const clang::EnumDecl* >( pair.getValue() ) )
			translated_type_names.insert( pair.getKey() );
		else if(
			std::holds_alternative< NamespaceItemEnumElement >( pair.getValue() ) ||
			std::holds_alternative< const clang::VarDecl* >( pair.getValue() ) )
			translated_variable_names.insert( pair.getKey() );
	}

	// Extract all macros and sort them by their location.
	// We need natural order here (from includes to the main file, line-by-line in each file).

	using MacroPair= std::pair< std::string, const clang::MacroDirective* >;

	std::vector< MacroPair > macro_directives;
	for( const clang::Preprocessor::macro_iterator::value_type& macro_pair : preprocessor_.macros() )
	{
		const clang::IdentifierInfo* ident_info= macro_pair.first;

		std::string name= ident_info->getName().str();
		if( name.empty() )
			continue; // Can't work with empty macros.

		if( preprocessor_.getPredefines().find( "#define " + name ) != std::string::npos )
			continue;

		const clang::MacroDirective* const macro_directive= macro_pair.second.getLatest();

		if( macro_directive->getKind() != clang::MacroDirective::MD_Define )
			continue; // Process only "define" but not "undef".

		if( !macro_directive->getLocation().isValid() )
			continue; // Ignore macros with no location. They all seems to be compiler built-in macros.

		macro_directives.push_back( std::make_pair( std::move( name ), macro_directive ) );
	}

	std::sort(
		macro_directives.begin(),
		macro_directives.end(),
		[&]( const MacroPair& l, const MacroPair& r )
		{
			return source_manager_.isBeforeInTranslationUnit( l.second->getLocation(), r.second->getLocation() );
		} );

	for( const MacroPair& macro_pair : macro_directives )
	{
		const clang::MacroDirective* const macro_directive= macro_pair.second;
		const clang::MacroInfo* const macro_info= macro_directive->getMacroInfo();
		if( macro_info->isBuiltinMacro() )
			continue;

		if( macro_info->getNumParams() != 0u || macro_info->isFunctionLike() )
			continue;

		const std::string macro_translated_name= TranslateIdentifier( macro_pair.first );

		if( root_namespace_.items.count( macro_translated_name ) != 0 ||
			translated_function_names.count( macro_translated_name ) != 0 ||
			translated_type_names.count( macro_translated_name ) != 0 ||
			translated_variable_names.count( macro_translated_name ) != 0 )
			continue;

		const auto append_item_if_should=
			[&]( auto item )
			{
				if( !ShouldSkipEmittingItem( macro_directive->getLocation() ) )
					out_items.Append( std::move( item ) );
			};

		clang::MacroInfo::const_tokens_iterator tokens_begin= macro_info->tokens_begin();
		clang::MacroInfo::const_tokens_iterator tokens_end= macro_info->tokens_end();

		// Strip ().
		while( tokens_begin < tokens_end &&
			tokens_begin->getKind() == clang::tok::l_paren && std::prev(tokens_end)->getKind() == clang::tok::r_paren )
		{
			++tokens_begin;
			--tokens_end;
		}

		if( tokens_begin == tokens_end )
		{
			// There is no reason to translate macros defining nothing.
		}
		else if( tokens_end - tokens_begin == 1 )
		{
			const clang::Token& token= *tokens_begin;
			if( token.getKind() == clang::tok::numeric_constant )
			{
				Synt::AutoVariableDeclaration auto_variable_declaration( g_dummy_src_loc );
				auto_variable_declaration.mutability_modifier= Synt::MutabilityModifier::Constexpr;
				auto_variable_declaration.name= macro_translated_name;
				auto_variable_declaration.initializer_expression= TranslateNumericLiteral( token );

				append_item_if_should( std::move( auto_variable_declaration ) );
				translated_variable_names.insert( macro_translated_name );
			}
			else if( clang::tok::isStringLiteral( token.getKind() ) )
			{
				clang::StringLiteralParser string_literal_parser( { token }, preprocessor_ );

				Synt::AutoVariableDeclaration auto_variable_declaration( g_dummy_src_loc );
				auto_variable_declaration.reference_modifier= Synt::ReferenceModifier::Reference;
				auto_variable_declaration.mutability_modifier= Synt::MutabilityModifier::Constexpr;
				auto_variable_declaration.name= macro_translated_name;

				auto string_constant= std::make_unique<Synt::StringLiteral>( g_dummy_src_loc );

				if( string_literal_parser.isOrdinary() || string_literal_parser.isUTF8() )
				{
					string_constant->value= string_literal_parser.GetString().str();
					string_constant->value.push_back( '\0' ); // C/C++ have null-terminated strings, instead of Ü.

					auto_variable_declaration.initializer_expression= std::move(string_constant);

					append_item_if_should( std::move( auto_variable_declaration ) );
					translated_variable_names.insert( macro_translated_name );
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

					append_item_if_should( std::move( auto_variable_declaration ) );
					translated_variable_names.insert( macro_translated_name );
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

					append_item_if_should( std::move( auto_variable_declaration ) );
					translated_variable_names.insert( macro_translated_name );
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

				if( !char_literal_parser.isMultiChar() )
				{
					Synt::AutoVariableDeclaration auto_variable_declaration( g_dummy_src_loc );
					auto_variable_declaration.mutability_modifier= Synt::MutabilityModifier::Constexpr;
					auto_variable_declaration.name= macro_translated_name;

					Synt::CharLiteral char_literal( g_dummy_src_loc );
					char_literal.code_point= uint32_t( char_literal_parser.getValue() );

					auto_variable_declaration.initializer_expression= std::move(char_literal);

					append_item_if_should( std::move( auto_variable_declaration ) );
					translated_variable_names.insert( macro_translated_name );
				}
			}
			else if( token.getKind() == clang::tok::identifier )
			{
				// Something like #define X Y.

				if( const auto identifier_info= token.getIdentifierInfo() )
				{
					const std::string idenfier_name= TranslateIdentifier( identifier_info->getName().str() );

					if( translated_function_names.count( idenfier_name ) != 0 )
					{
						// For functions just create "auto x= y;".
						// This creates a function-pointer variable.

						Synt::AutoVariableDeclaration auto_variable_declaration( g_dummy_src_loc );
						auto_variable_declaration.mutability_modifier= Synt::MutabilityModifier::Constexpr;
						auto_variable_declaration.name= macro_translated_name;

						Synt::NameLookup name_lookup( g_dummy_src_loc );
						name_lookup.name= idenfier_name;

						auto_variable_declaration.initializer_expression= std::move( name_lookup );

						append_item_if_should( std::move( auto_variable_declaration ) );
						translated_variable_names.insert( macro_translated_name );
					}
					else if( translated_variable_names.count( idenfier_name ) != 0 )
					{
						// For variables just create "auto& x= y;"
						Synt::AutoVariableDeclaration auto_variable_declaration( g_dummy_src_loc );
						auto_variable_declaration.mutability_modifier= Synt::MutabilityModifier::Constexpr;
						auto_variable_declaration.name= macro_translated_name;
						auto_variable_declaration.reference_modifier= Synt::ReferenceModifier::Reference;

						Synt::NameLookup name_lookup( g_dummy_src_loc );
						name_lookup.name= idenfier_name;

						auto_variable_declaration.initializer_expression= std::move( name_lookup );

						append_item_if_should( std::move( auto_variable_declaration ) );
						translated_variable_names.insert( macro_translated_name );
					}
					else if( translated_type_names.count( idenfier_name ) != 0 )
					{
						// For types create a type alias.

						Synt::TypeAlias type_alias( g_dummy_src_loc );
						type_alias.name= macro_translated_name;

						Synt::NameLookup name_lookup( g_dummy_src_loc );
						name_lookup.name= idenfier_name;

						type_alias.value= std::move( name_lookup );

						append_item_if_should( std::move( type_alias ) );
						translated_type_names.insert( macro_translated_name );
					}
				}
			}
		}
		else if( tokens_end - tokens_begin == 2 )
		{
			const clang::Token& token0= tokens_begin[0];
			const clang::Token& token1= tokens_begin[1];

			if( token0.getKind() == clang::tok::minus && token1.getKind() == clang::tok::numeric_constant )
			{
				// Negative numeric literals.

				Synt::AutoVariableDeclaration auto_variable_declaration( g_dummy_src_loc );
				auto_variable_declaration.mutability_modifier= Synt::MutabilityModifier::Constexpr;
				auto_variable_declaration.name= macro_translated_name;

				auto minus= std::make_unique<Synt::UnaryMinus>( g_dummy_src_loc );
				minus->expression= TranslateNumericLiteral( token1 );
				auto_variable_declaration.initializer_expression= std::move( minus );

				append_item_if_should( std::move( auto_variable_declaration ) );
				translated_variable_names.insert( macro_translated_name );
			}
		}
	} // for defines
}

Synt::Expression CppAstConsumer::TranslateNumericLiteral( const clang::Token& token )
{
	const std::string numeric_literal_str( token.getLiteralData(), token.getLength() );
	clang::NumericLiteralParser numeric_literal_parser(
		numeric_literal_str,
		token.getLocation(),
		source_manager_,
		lang_options_,
		target_info_,
		diagnostic_engine_ );

	if( numeric_literal_parser.isIntegerLiteral() || numeric_literal_parser.getRadix() != 10 )
	{
		// Parse and stringify. Don't use source string in order to ignore C++-specific number format.
		llvm::APInt int_val( 64u, 0u );
		numeric_literal_parser.GetIntegerValue( int_val );
		Synt::IntegerNumericConstant numeric_constant= TranslateNumericConstant( int_val );

		if( numeric_literal_parser.isUnsigned )
			numeric_constant.type_suffix= numeric_literal_parser.isLongLong ? "u64" : "u";
		else
			numeric_constant.type_suffix= numeric_literal_parser.isLongLong ? "i64" : "" ;

		return std::make_unique< Synt::IntegerNumericConstant >( numeric_constant );
	}
	else
	{
		// Parse and stringify. Don't use source string in order to ignore C++-specific number format.
		llvm::APFloat float_val(0.0);
		numeric_literal_parser.GetFloatValue( float_val );

		// HACK fix infinity.
		if( float_val.isInfinity() )
			float_val= llvm::APFloat::getLargest( float_val.getSemantics(), float_val.isNegative() );

		Synt::FloatingPointNumericConstant numeric_constant= TranslateNumericConstant( float_val );

		if( numeric_literal_parser.isFloat )
			numeric_constant.type_suffix= "f";

		return std::make_unique< Synt::FloatingPointNumericConstant >( numeric_constant );
	}
}

CppAstProcessor::CppAstProcessor( ParsedUnitsPtr out_result, DepFileOptionsOpt dep_file_options )
	: out_result_(std::move(out_result)), dep_file_options_( std::move(dep_file_options) )
{}

bool CppAstProcessor::PrepareToExecuteAction( clang::CompilerInstance& compiler_intance )
{
	if( dep_file_options_ != std::nullopt )
	{
		clang::DependencyOutputOptions opts;
		opts.Targets.push_back( dep_file_options_->out_file );
		opts.OutputFile= dep_file_options_->out_dep_file;
		opts.IncludeSystemHeaders= 1;

		compiler_intance.addDependencyCollector( std::make_shared<clang::DependencyFileGenerator>(opts) );
	}

	return true;
}

std::unique_ptr<clang::ASTConsumer> CppAstProcessor::CreateASTConsumer(
	clang::CompilerInstance& compiler_intance, const llvm::StringRef in_file )
{
	return
		std::make_unique<CppAstConsumer>(
			(*out_result_)[ in_file.str() ],
			compiler_intance.getSourceManager(),
			compiler_intance.getPreprocessor(),
			compiler_intance.getTarget(),
			compiler_intance.getDiagnostics(),
			compiler_intance.getLangOpts(),
			compiler_intance.getASTContext() );
}

} // namespace

FrontendActionFactory::FrontendActionFactory( ParsedUnitsPtr out_result, DepFileOptionsOpt dep_file_options )
	: out_result_(std::move(out_result)), dep_file_options_( std::move(dep_file_options) )
{}

std::unique_ptr<clang::FrontendAction> FrontendActionFactory::create()
{
	return std::make_unique<CppAstProcessor>( out_result_, dep_file_options_ );
}

} // namespace U
