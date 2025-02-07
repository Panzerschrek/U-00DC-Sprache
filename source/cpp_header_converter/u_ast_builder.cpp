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
#include "../../code_builder_lib_common/string_ref.hpp"
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
	virtual void HandleTranslationUnit( clang::ASTContext& ast_context ) override;

private:
	using TypeNamesMap= std::unordered_map<const clang::Type*, std::string>;

	// Use ordered map in order to emit elements in stable order.
	template<typename K, typename V>
	using NamesMapContainer= std::map<K, V>;

private:
	void ProcessDecl( const clang::Decl& decl );

	Synt::TypeName TranslateType( const clang::Type& in_type, const TypeNamesMap& type_names_map );
	std::string_view GetUFundamentalType( const clang::BuiltinType& in_type );
	Synt::TypeName StringToTypeName( std::string_view s );
	Synt::FunctionType TranslateFunctionType( const clang::FunctionProtoType& in_type, const TypeNamesMap& type_names_map );
	Synt::FunctionType TranslateFunctionType( const clang::FunctionType& in_type, const TypeNamesMap& type_names_map );
	std::optional<std::string> TranslateCallingConvention( const clang::FunctionType& in_type );

	std::string TranslateIdentifier( llvm::StringRef identifier );

	void CollectSubrecords();

	using NamedFunctionDeclarations= NamesMapContainer<std::string, const clang::FunctionDecl*>;
	NamedFunctionDeclarations GenerateFunctionNames();

	using NamedRecordDeclarations= NamesMapContainer<std::string, const clang::RecordDecl*>;
	NamedRecordDeclarations GenerateRecordNames( const NamedFunctionDeclarations& named_function_declarations );

	using NamedTypedefDeclarations= NamesMapContainer<std::string, const clang::TypedefNameDecl*>;
	NamedTypedefDeclarations GenerateTypedefNames(
		const NamedFunctionDeclarations& named_function_declarations,
		const NamedRecordDeclarations& named_record_declarations );

	using NamedEnumDeclarations= NamesMapContainer<std::string, const clang::EnumDecl*>;
	NamedEnumDeclarations GenerateEnumNames(
		const NamedFunctionDeclarations& named_function_declarations,
		const NamedRecordDeclarations& named_record_declarations,
		const NamedTypedefDeclarations& named_typedef_declarations );

	TypeNamesMap BuildTypeNamesMap(
		const NamedRecordDeclarations& named_record_declarations,
		const NamedTypedefDeclarations& named_typedef_declarations,
		const NamedEnumDeclarations& named_enum_declarations );

	void EmitFunctions( const NamedFunctionDeclarations& function_declarations, const TypeNamesMap& type_names_map );
	void EmitFunction( const std::string& name, const clang::FunctionDecl& function_decl, const TypeNamesMap& type_names_map );

	void EmitRecords(
		const NamedRecordDeclarations& record_declarations,
		const NamedTypedefDeclarations& named_typedef_declarations,
		const NamedEnumDeclarations& named_enum_declarations,
		const TypeNamesMap& type_names_map );

	void EmitRecord(
		const std::string& name,
		const clang::RecordDecl& record_declaration,
		const NamedRecordDeclarations& named_record_declarations,
		const NamedTypedefDeclarations& named_typedef_declarations,
		const NamedEnumDeclarations& named_enum_declarations,
		const TypeNamesMap& type_names_map );

	Synt::ClassElementsList MakeOpaqueRecordElements(
		const clang::RecordDecl& record_declaration,
		std::string_view kind_name );

	void EmitTypedefs( const NamedTypedefDeclarations& typedef_declarations, const TypeNamesMap& type_names_map );
	void EmitTypedef( const std::string& name, const clang::TypedefNameDecl& typedef_declaration, const TypeNamesMap& type_names_map );

	using AnonymousEnumMembersSet= std::unordered_set<std::string>;

	void EmitEnums(
		const NamedFunctionDeclarations& named_function_declarations,
		const NamedRecordDeclarations& named_record_declarations,
		const NamedTypedefDeclarations& named_typedef_declarations,
		const NamedEnumDeclarations& named_enum_declarations,
		const TypeNamesMap& type_names_map,
		AnonymousEnumMembersSet& out_anonymous_enum_members );

	void EmitEnum(
		const std::string& name,
		const clang::EnumDecl& enum_declaration,
		const NamedFunctionDeclarations& named_function_declarations,
		const NamedRecordDeclarations& named_record_declarations,
		const NamedTypedefDeclarations& named_typedef_declarations,
		const NamedEnumDeclarations& named_enum_declarations,
		const TypeNamesMap& type_names_map,
		AnonymousEnumMembersSet& out_anonymous_enum_members );

	void EmitDefinitionsForMacros(
		const NamedFunctionDeclarations& named_function_declarations,
		const NamedRecordDeclarations& named_record_declarations,
		const NamedTypedefDeclarations& named_typedef_declarations,
		const NamedEnumDeclarations& named_enum_declarations,
		const AnonymousEnumMembersSet& anonymous_enum_members );

private:
	Synt::ProgramElementsList::Builder& root_program_elements_;

	const clang::SourceManager& source_manager_;
	clang::Preprocessor& preprocessor_;
	const clang::TargetInfo &target_info_;
	clang::DiagnosticsEngine& diagnostic_engine_;
	const clang::LangOptions& lang_options_;
	const clang::ASTContext& ast_context_;
	const bool skip_declarations_from_includes_;

	// Declaration of symbols to translate.
	// Use vector, because we need stable order in order to perform (if necessary) consistent renaming.
	std::vector< const clang::FunctionDecl* > function_declarations_;
	std::vector< const clang::RecordDecl* > record_declarations_;
	std::vector< const clang::TypedefNameDecl* > typedef_declarations_;
	std::vector< const clang::EnumDecl* > enum_declarations_;

	size_t unique_name_index_= 0u;
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
	, ast_context_(ast_context)
	, skip_declarations_from_includes_(skip_declarations_from_includes)
{
}

bool CppAstConsumer::HandleTopLevelDecl( const clang::DeclGroupRef decl_group )
{
	for( const clang::Decl* const decl : decl_group )
		ProcessDecl( *decl );
	return true;
}

void CppAstConsumer::HandleTranslationUnit( clang::ASTContext& ast_context )
{
	(void)ast_context;

	// Collect subrecords.
	// Do it via a separate step in order to process all subrecords late and avoid renaming of main records in case of naming conflicts.
	CollectSubrecords();

	// First, generate names for functions.
	// This step is first, since we need to try to preserve original names.
	// It's fine to rename types later, if they conflict with function names.
	const NamedFunctionDeclarations function_names= GenerateFunctionNames();

	// Second, generate names for types.
	// We can rename them in case of conflicts.
	// It's fine to rename, since type names in C aren't used for symbols mangling.
	const NamedRecordDeclarations record_names= GenerateRecordNames( function_names );

	// Third, generate names for typedefs.
	// Rename in case of conflicts, if necessary.
	const NamedTypedefDeclarations typedef_names= GenerateTypedefNames( function_names, record_names );

	// Lastly, generate names for enums.
	// Skip anonymous enums, rename, if necessary.
	const NamedEnumDeclarations enum_names= GenerateEnumNames( function_names, record_names, typedef_names );

	// Build types map to referr to types by name (possible renamed).
	const TypeNamesMap type_names_map= BuildTypeNamesMap( record_names, typedef_names, enum_names );

	//
	// Emit symbols.
	//

	EmitFunctions( function_names, type_names_map );

	EmitRecords( record_names, typedef_names, enum_names, type_names_map );

	EmitTypedefs( typedef_names, type_names_map );

	// While emitting enums we may emit also variables for members of anonymous enums.
	// Use this variables list later to avoid naming conflicts.
	AnonymousEnumMembersSet anonymous_enum_members;
	EmitEnums( function_names, record_names, typedef_names, enum_names, type_names_map, anonymous_enum_members );

	EmitDefinitionsForMacros( function_names, record_names, typedef_names, enum_names, anonymous_enum_members );
}

void CppAstConsumer::ProcessDecl( const clang::Decl& decl )
{
	if( skip_declarations_from_includes_ &&
		source_manager_.getFileID( decl.getLocation() ) != source_manager_.getMainFileID() )
		return;

	if( const auto record_decl= llvm::dyn_cast<clang::RecordDecl>(&decl) )
	{
		if( record_decl->isCompleteDefinition() && !record_decl->isTemplated() )
			record_declarations_.push_back( record_decl );
	}
	else if( const auto type_alias_decl= llvm::dyn_cast<clang::TypedefNameDecl>(&decl) )
		typedef_declarations_.push_back( type_alias_decl );
	else if( const auto func_decl= llvm::dyn_cast<clang::FunctionDecl>(&decl) )
	{
		if( func_decl->isFirstDecl() || func_decl->getBuiltinID() != 0 )
			function_declarations_.push_back( func_decl );
	}
	else if( const auto enum_decl= llvm::dyn_cast<clang::EnumDecl>(&decl) )
		enum_declarations_.push_back( enum_decl );
	else if( const auto decl_context= llvm::dyn_cast<clang::DeclContext>(&decl) )
	{
		for( const clang::Decl* const sub_decl : decl_context->decls() )
			ProcessDecl( *sub_decl );
	}
}

Synt::TypeName CppAstConsumer::TranslateType( const clang::Type& in_type, const TypeNamesMap& type_names_map )
{
	// Records, typedefs, enums should have names in this map.
	if( const auto named_type_it= type_names_map.find( &in_type ); named_type_it != type_names_map.end() )
		return StringToTypeName( named_type_it->second );

	if( const auto built_in_type= llvm::dyn_cast<clang::BuiltinType>(&in_type) )
		return StringToTypeName( GetUFundamentalType( *built_in_type ) );
	else if( const auto typedef_type= llvm::dyn_cast<clang::TypedefType>(&in_type) )
	{
		// Normally we should create entries for typedefs in types map.
		// But if this doesn't work, use underlying type instead.
		return TranslateType( *typedef_type->desugar().getTypePtr(), type_names_map );
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

		Synt::NumericConstant numeric_constant( g_dummy_src_loc );
		numeric_constant.num.value_int= 2;
		numeric_constant.num.value_double= 2.0;
		array_type->size= std::move(numeric_constant);

		return std::move(array_type);
	}
	else if( const auto constant_array_type= llvm::dyn_cast<clang::ConstantArrayType>(&in_type) )
	{
		// For arrays with constant size use normal Ü array.
		auto array_type= std::make_unique<Synt::ArrayTypeName>(g_dummy_src_loc);
		array_type->element_type= TranslateType( *constant_array_type->getElementType().getTypePtr(), type_names_map );

		Synt::NumericConstant numeric_constant( g_dummy_src_loc );
		numeric_constant.num.value_int= constant_array_type->getSize().getLimitedValue();
		numeric_constant.num.value_double= static_cast<double>(numeric_constant.num.value_int);
		numeric_constant.num.type_suffix[0]= 'u';
		if( numeric_constant.num.value_int >= 0x7FFFFFFFFu )
		{
			numeric_constant.num.type_suffix[1]= '6';
			numeric_constant.num.type_suffix[2]= '4';
		}
		array_type->size= std::move(numeric_constant);

		return std::move(array_type);
	}
	else if( const auto array_type= llvm::dyn_cast<clang::ArrayType>(&in_type) )
	{
		// For other variants of array types use zero size.
		auto out_array_type= std::make_unique<Synt::ArrayTypeName>(g_dummy_src_loc);
		out_array_type->element_type= TranslateType( *array_type->getElementType().getTypePtr(), type_names_map );

		Synt::NumericConstant numeric_constant( g_dummy_src_loc );
		numeric_constant.num.value_int= 0;
		numeric_constant.num.value_double= 0.0;
		numeric_constant.num.type_suffix[0]= 'u';
		out_array_type->size= std::move(numeric_constant);

		return std::move(out_array_type);
	}
	else if( const auto incomplete_array_type= llvm::dyn_cast<clang::IncompleteArrayType>(&in_type) )
	{
		// Translate incomplete array types as raw pointers.
		auto raw_pointer_type= std::make_unique<Synt::RawPointerType>( g_dummy_src_loc );
		raw_pointer_type->element_type= TranslateType( *incomplete_array_type->getPointeeType().getTypePtr(), type_names_map );

		return std::move(raw_pointer_type);
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
		return StringToTypeName( Keyword( Keywords::void_ ) );
	}
	else if( const auto pointer_type= llvm::dyn_cast<clang::PointerType>(&in_type) )
	{
		auto raw_pointer_type= std::make_unique<Synt::RawPointerType>( g_dummy_src_loc );
		raw_pointer_type->element_type= TranslateType( *pointer_type->getPointeeType().getTypePtr(), type_names_map );

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

	// Fallback for some unlikely case.
	return StringToTypeName( Keyword( Keywords::void_ ) );
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

Synt::TypeName CppAstConsumer::StringToTypeName( const std::string_view s )
{
	Synt::NameLookup name_lookup( g_dummy_src_loc );
	name_lookup.name= s;
	return std::move(name_lookup);
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
	U_ASSERT( !identifier.empty() );

	// In Ü identifier can not start with "_", shadow it. "_" in C++ used for impl identiferes, so, it may not needed.
	if( identifier[0] == '_' )
		return ( "ü" + identifier ).str();

	// Avoid using keywords as names.
	if( IsKeyword( StringRefToStringView( identifier ) ) )
		return (identifier + "_").str();

	return identifier.str();
}

void CppAstConsumer::CollectSubrecords()
{
	// Use index-based for loop, since container may be modified.
	for( size_t i= 0; i < record_declarations_.size(); ++i )
	{
		const auto record_declaration= record_declarations_[i];
		for( const clang::Decl* const sub_decl : record_declaration->decls() )
		{
			// Collect sub-structs into flat result container.
			// Doing so we force move nested records into the root namespace.
			// This may cause renaming due to name conflicts.
			// But it's not so bad.
			if( const auto subrecord= llvm::dyn_cast<clang::RecordDecl>( sub_decl ) )
				record_declarations_.push_back( subrecord );
		}
	}
}

CppAstConsumer::NamedFunctionDeclarations CppAstConsumer::GenerateFunctionNames()
{
	NamedFunctionDeclarations named_declarations;
	for( const auto function_declaration : function_declarations_ )
	{
		if( function_declaration->getIdentifier() == nullptr )
			continue;

		std::string name= function_declaration->getName().str();
		if( IsKeyword( name ) || ( !name.empty() && name[0] == '_' ) )
		{
			// It's for now impossible to use a function with name, which isn't a valid Ü identifier.
			continue;
		}

		if( named_declarations.count( name ) != 0 )
			continue; // Aleady has this declaration.

		named_declarations.emplace( std::move(name), function_declaration );
	}

	return named_declarations;
}

CppAstConsumer::NamedRecordDeclarations CppAstConsumer::GenerateRecordNames( const NamedFunctionDeclarations& named_function_declarations )
{
	NamedRecordDeclarations named_declarations;

	for( const auto record_declaration : record_declarations_ )
	{
		const auto src_name= record_declaration->getName();

		std::string name;
		if( src_name.empty() )
			name= "ü_anon_record_" + std::to_string( ++unique_name_index_ );
		else
			name= TranslateIdentifier( src_name );

		// Rename record until we have no name conflict.
		while(
			named_function_declarations.count( name ) != 0 ||
			named_declarations.count( name ) != 0 )
			name+= "_";

		named_declarations.emplace( std::move(name), record_declaration );
	}

	// Collect declarations of incomplete records.
	std::vector< const clang::RecordDecl* > incomplete_records;

	for( const auto type : ast_context_.getTypes() )
	{
		if( const auto record_type= llvm::dyn_cast<clang::RecordType>( type ) )
		{
			if( const auto record_declaration= record_type->getDecl() )
			{
				// Process only incomplete records.
				// ignore implicitely-defined records, like "_GUID".
				if( record_type->isIncompleteType() && ! record_declaration->isImplicit() )
					incomplete_records.push_back( record_declaration );
			}
		}
	}

	// Sort incomplete records in order to get stable order and thus consistent renaming.
	std::sort(
		incomplete_records.begin(),
		incomplete_records.end(),
		[]( const clang::RecordDecl* const l, const clang::RecordDecl* const r )
		{
			return l->getSourceRange().getBegin() < r->getSourceRange().getBegin();
		} );

	// Add names of incomplete records.
	for( const clang::RecordDecl* const incomplete_record : incomplete_records )
	{
		const auto src_name= incomplete_record->getName();

		std::string name;
		if( src_name.empty() )
			name= "ü_anon_record_" + std::to_string( ++unique_name_index_ );
		else
			name= TranslateIdentifier( src_name );

		// Rename record until we have no name conflict.
		while(
			named_function_declarations.count( name ) != 0 ||
			named_declarations.count( name ) != 0 )
			name+= "_";

		named_declarations.emplace( std::move(name), incomplete_record );
	}

	return named_declarations;
}

CppAstConsumer::NamedTypedefDeclarations CppAstConsumer::GenerateTypedefNames(
	const NamedFunctionDeclarations& named_function_declarations,
	const NamedRecordDeclarations& named_record_declarations )
{
	NamedTypedefDeclarations named_declarations;

	for( const auto typedef_decl : typedef_declarations_ )
	{
		const auto src_name= typedef_decl->getName();
		if( src_name.empty() )
			continue; // Is it possible?

		std::string name= TranslateIdentifier( src_name );

		// Rename typedef until we have no name conflict.
		while(
			named_function_declarations.count( name ) != 0 ||
			named_record_declarations.count( name ) != 0 ||
			named_declarations.count( name ) != 0 )
			name+= "_";

		named_declarations.emplace( std::move(name), typedef_decl );
	}

	// Add some implicit typedefs.

	const std::string size_t_name= "size_t";
	const llvm::StringRef builtin_va_list_name= "__builtin_va_list";

	for( const auto type : ast_context_.getTypes() )
	{
		if( const auto typedef_type= llvm::dyn_cast<clang::TypedefType>(type) )
		{
			if( const auto decl= typedef_type->getDecl() )
			{
				// Add implicit size_t.
				if( decl->getName() == size_t_name &&
					named_function_declarations.count( size_t_name ) == 0 &&
					named_record_declarations.count( size_t_name ) == 0 &&
					named_declarations.count( size_t_name ) == 0 )
				{
					named_declarations.emplace( size_t_name, decl );
				}

				// Add implicit "__builtin_va_list".
				if( decl->getName() == builtin_va_list_name )
				{
					const auto builtin_va_list_name_translated= TranslateIdentifier( builtin_va_list_name );

					if( named_function_declarations.count( builtin_va_list_name_translated ) == 0 &&
						named_record_declarations.count( builtin_va_list_name_translated ) == 0 &&
						named_declarations.count( builtin_va_list_name_translated ) == 0 )
					{
						named_declarations.emplace( builtin_va_list_name_translated, decl );
					}
				}
			}
		}

	}

	return named_declarations;
}

CppAstConsumer::NamedEnumDeclarations CppAstConsumer::GenerateEnumNames(
	const NamedFunctionDeclarations& named_function_declarations,
	const NamedRecordDeclarations& named_record_declarations,
	const NamedTypedefDeclarations& named_typedef_declarations )
{
	NamedEnumDeclarations named_declarations;

	for( const auto enum_decl : enum_declarations_ )
	{
		const auto src_name= enum_decl->getName();

		std::string name;
		if( src_name.empty() )
			name= "ü_anon_enum_" + std::to_string( ++unique_name_index_ );
		else
			name= TranslateIdentifier( enum_decl->getName() );

		// Rename enum until we have no name conflict.
		while(
			named_function_declarations.count( name ) != 0 ||
			named_record_declarations.count( name ) != 0 ||
			named_typedef_declarations.count( name ) != 0 ||
			named_declarations.count( name ) != 0 )
			name+= "_";

		named_declarations.emplace( std::move(name), enum_decl );
	}

	return named_declarations;
}

CppAstConsumer::TypeNamesMap CppAstConsumer::BuildTypeNamesMap(
	const NamedRecordDeclarations& named_record_declarations,
	const NamedTypedefDeclarations& named_typedef_declarations,
	const NamedEnumDeclarations& named_enum_declarations )
{
	TypeNamesMap res;

	for( const auto& pair : named_record_declarations )
		res[ pair.second->getTypeForDecl() ] = pair.first;

	for( const auto& pair : named_typedef_declarations )
		res[ pair.second->getTypeForDecl() ] = pair.first;

	for( const auto& pair : named_enum_declarations )
		res[ pair.second->getTypeForDecl() ] = pair.first;

	return res;
}

void CppAstConsumer::EmitFunctions( const NamedFunctionDeclarations& function_declarations, const TypeNamesMap& type_names_map )
{
	for( const auto& pair : function_declarations )
		EmitFunction( pair.first, *pair.second, type_names_map );
}

void CppAstConsumer::EmitFunction( const std::string& name, const clang::FunctionDecl& function_decl, const TypeNamesMap& type_names_map )
{
	Synt::Function func(g_dummy_src_loc);

	func.name.push_back( Synt::Function::NameComponent{ name, g_dummy_src_loc } );

	func.no_mangle= true; // For now import only C functions witohut mangling.

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

	root_program_elements_.Append( std::move(func) );
}

void CppAstConsumer::EmitRecords(
	const NamedRecordDeclarations& record_declarations,
	const NamedTypedefDeclarations& named_typedef_declarations,
	const NamedEnumDeclarations& named_enum_declarations,
	const TypeNamesMap& type_names_map )
{
	for( const auto& pair : record_declarations )
		EmitRecord( pair.first, *pair.second, record_declarations, named_typedef_declarations, named_enum_declarations, type_names_map );
}

void CppAstConsumer::EmitRecord(
	const std::string& name,
	const clang::RecordDecl& record_declaration,
	const NamedRecordDeclarations& named_record_declarations,
	const NamedTypedefDeclarations& named_typedef_declarations,
	const NamedEnumDeclarations& named_enum_declarations,
	const TypeNamesMap& type_names_map )
{
	if( record_declaration.isStruct() || record_declaration.isClass() )
	{
		Synt::Class class_(g_dummy_src_loc);
		class_.name= name;

		class_.keep_fields_order= true; // C/C++ structs/classes have fixed fields order.

		if( record_declaration.hasAttr<clang::WarnUnusedResultAttr>() )
			class_.no_discard= true;

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

			if( has_bitfields )
			{
				// Ü has no bitfields support. And generally we can't replace C bitfields with something else.
				// So, for now just create stub struct.
				class_.elements= MakeOpaqueRecordElements( record_declaration, "struct_with_bitfields" );
			}
			else
			{
				Synt::ClassElementsList::Builder class_elements;

				for( const clang::FieldDecl* const field_declaration : record_declaration.fields() )
				{
					Synt::ClassField field( g_dummy_src_loc );

					field.type= TranslateType( *field_declaration->getType().getTypePtr(), type_names_map );

					const auto src_name= field_declaration->getName();
					if( src_name.empty() )
						field.name= "ü_anon_field_" + std::to_string( ++unique_name_index_ );
					else
						field.name= TranslateIdentifier( src_name );

					// HACK!
					// For cases where field name is the same as some global name, add name suffix to avoid collisions.
					// C allows such collisions, but Ü doesn't.
					while(
						named_record_declarations.count( field.name ) != 0 ||
						named_typedef_declarations.count( field.name ) != 0 ||
						named_enum_declarations.count( field.name ) != 0 )
						field.name+= "_";

					class_elements.Append( std::move(field) );
				}

				// Assume we have at least one field (which is necessary for all structs in C).

				class_.elements= class_elements.Build();
			}
		}
		else
		{
			// Add deleted default constructor.
			Synt::ClassElementsList::Builder class_elements;
			class_elements.Append( GetDeletedDefaultConstructor() );
			class_.elements= class_elements.Build();
		}

		root_program_elements_.Append( std::move(class_ ) );
	}
	else if( record_declaration.isUnion() )
	{
		// Emulate union, using array of bytes with required alignment.
		Synt::Class class_(g_dummy_src_loc);
		class_.name= name;
		class_.keep_fields_order= true; // C/C++ structs/classes have fixed fields order.

		if( record_declaration.hasAttr<clang::WarnUnusedResultAttr>() )
			class_.no_discard= true;

		if( record_declaration.isCompleteDefinition() )
			class_.elements= MakeOpaqueRecordElements( record_declaration, "union" );
		else
		{
			// Add deleted default constructor.
			Synt::ClassElementsList::Builder class_elements;
			class_elements.Append( GetDeletedDefaultConstructor() );
			class_.elements= class_elements.Build();
		}

		root_program_elements_.Append( std::move(class_ ) );
	}
}

Synt::ClassElementsList CppAstConsumer::MakeOpaqueRecordElements(
	const clang::RecordDecl& record_declaration,
	const std::string_view kind_name )
{
	const auto size= ast_context_.getTypeSize( record_declaration.getTypeForDecl() ) / 8u;
	const auto byte_size= ast_context_.getTypeAlign( record_declaration.getTypeForDecl() ) / 8u;
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
	array_type->element_type= StringToTypeName( byte_name );

	Synt::NumericConstant numeric_constant( g_dummy_src_loc );
	numeric_constant.num.value_int= num_elements;
	numeric_constant.num.value_double= double(numeric_constant.num.value_int);
	array_type->size= std::move(numeric_constant);

	Synt::ClassField field( g_dummy_src_loc );
	field.name+= kind_name;
	field.name+= "_contents";
	field.type= std::move(array_type);

	Synt::ClassElementsList::Builder class_elements;
	class_elements.Append( std::move(field) );
	return class_elements.Build();
}

void CppAstConsumer::EmitTypedefs( const NamedTypedefDeclarations& typedef_declarations, const TypeNamesMap& type_names_map )
{
	for( const auto& pair : typedef_declarations )
		EmitTypedef( pair.first, *pair.second, type_names_map );
}

void CppAstConsumer::EmitTypedef( const std::string& name, const clang::TypedefNameDecl& typedef_declaration, const TypeNamesMap& type_names_map )
{
	Synt::TypeAlias type_alias( g_dummy_src_loc );
	type_alias.name= name;
	type_alias.value= TranslateType( *typedef_declaration.getUnderlyingType().getTypePtr(), type_names_map );

	root_program_elements_.Append( std::move(type_alias ) );
}

void CppAstConsumer::EmitEnums(
	const NamedFunctionDeclarations& named_function_declarations,
	const NamedRecordDeclarations& named_record_declarations,
	const NamedTypedefDeclarations& named_typedef_declarations,
	const NamedEnumDeclarations& named_enum_declarations,
	const TypeNamesMap& type_names_map,
	AnonymousEnumMembersSet& out_anonymous_enum_members )
{
	for( const auto& pair: named_enum_declarations )
		EmitEnum(
			pair.first,
			*pair.second,
			named_function_declarations,
			named_record_declarations,
			named_typedef_declarations,
			named_enum_declarations,
			type_names_map,
			out_anonymous_enum_members );
}

void CppAstConsumer::EmitEnum(
	const std::string& name,
	const clang::EnumDecl& enum_declaration,
	const NamedFunctionDeclarations& named_function_declarations,
	const NamedRecordDeclarations& named_record_declarations,
	const NamedTypedefDeclarations& named_typedef_declarations,
	const NamedEnumDeclarations& named_enum_declarations,
	const TypeNamesMap& type_names_map,
	AnonymousEnumMembersSet& out_anonymous_enum_members )
{
	if( !enum_declaration.isComplete() )
	{
		// Create dummy class with deleted default constructor for incomplete enums.

		Synt::Class enum_class_( g_dummy_src_loc );
		enum_class_.name= name;

		Synt::ClassElementsList::Builder class_elements;
		class_elements.Append( GetDeletedDefaultConstructor() );
		enum_class_.elements= class_elements.Build();

		root_program_elements_.Append( std::move(enum_class_ ) );

		return;
	}

	const auto enumerators_range= enum_declaration.enumerators();

	if( enum_declaration.getName().empty() )
	{
		// Anonymous enum. Just create a bunch of constants for it in space, where this enum is located.

		// Create type alias for this enum.
		{
			Synt::TypeAlias type_alias( g_dummy_src_loc );
			type_alias.name= name;

			std::string_view underlying_type_name;
			if( const auto built_in_type= llvm::dyn_cast<clang::BuiltinType>( enum_declaration.getIntegerType().getTypePtr() ) )
				underlying_type_name= GetUFundamentalType( *built_in_type );

			type_alias.value= StringToTypeName( underlying_type_name );

			root_program_elements_.Append( std::move(type_alias) );
		}

		Synt::VariablesDeclaration variables_declaration( g_dummy_src_loc );
		variables_declaration.type= StringToTypeName( name );

		for( const clang::EnumConstantDecl* const enumerator : enumerators_range )
		{
			Synt::NumericConstant initializer_number( g_dummy_src_loc );
			const llvm::APSInt val= enumerator->getInitVal();
			initializer_number.num.value_int= val.isNegative() ? uint64_t(val.getExtValue()) : val.getLimitedValue();
			initializer_number.num.value_double= static_cast<double>(initializer_number.num.value_int);
			initializer_number.num.type_suffix[0]= 'u';
			if( initializer_number.num.value_int >= 0x7FFFFFFFFu )
			{
				initializer_number.num.type_suffix[1]= '6';
				initializer_number.num.type_suffix[2]= '4';
			}

			Synt::ConstructorInitializer constructor_initializer( g_dummy_src_loc );
			constructor_initializer.arguments.push_back( std::move(initializer_number) );

			Synt::VariablesDeclaration::VariableEntry var;
			var.src_loc= g_dummy_src_loc;

			var.name= TranslateIdentifier( enumerator->getName() );

			// Avoid name conflicts.
			while(
				named_function_declarations.count( var.name ) != 0 ||
				named_record_declarations.count( var.name ) != 0 ||
				named_typedef_declarations.count( var.name ) != 0 ||
				named_enum_declarations.count( var.name ) != 0 )
				var.name+= "_";

			var.mutability_modifier= Synt::MutabilityModifier::Constexpr;
			var.initializer= std::make_unique<Synt::Initializer>( std::move(constructor_initializer) );

			out_anonymous_enum_members.insert( var.name );

			variables_declaration.variables.push_back( std::move(var) );
		}

		root_program_elements_.Append( std::move(variables_declaration) );

		return;
	}

	// C++ enum can be Ü enum, if it`s members form sequence 0-N with step 1.
	bool can_be_u_enum= true;
	if( !enumerators_range.empty() )
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
	else
		can_be_u_enum= false;

	end_check:
	if( can_be_u_enum )
	{
		Synt::Enum enum_( g_dummy_src_loc );
		enum_.name= name;

		if( enum_declaration.hasAttr<clang::WarnUnusedResultAttr>() )
			enum_.no_discard= true;

		Synt::TypeName type_name= TranslateType( *enum_declaration.getIntegerType().getTypePtr(), type_names_map );
		if( const auto named_type_name= std::get_if<Synt::NameLookup>( &type_name ) )
			enum_.underlying_type_name= std::move(*named_type_name);

		for( const clang::EnumConstantDecl* const enumerator : enumerators_range )
		{
			enum_.members.emplace_back();
			enum_.members.back().src_loc= g_dummy_src_loc;
			enum_.members.back().name= TranslateIdentifier( enumerator->getName() );
		}

		root_program_elements_.Append( std::move(enum_) );
	}
	else
	{
		// Can't use Ü enum. So, create struct type and a bunch of constants inside.
		// Since such struct contains singe scalar inside, it is passed via this scalar.

		Synt::Class enum_class_( g_dummy_src_loc );
		enum_class_.name= name;

		if( enum_declaration.hasAttr<clang::WarnUnusedResultAttr>() )
			enum_class_.no_discard= true;

		Synt::ClassElementsList::Builder class_elements;

		const std::string field_name= "ü_underlying_value";
		{
			Synt::ClassField field( g_dummy_src_loc );
			field.name= field_name;
			field.type= TranslateType( *enum_declaration.getIntegerType().getTypePtr(), type_names_map );
			class_elements.Append( std::move(field) );
		}

		Synt::NameLookup enum_class_name( g_dummy_src_loc );
		enum_class_name.name= enum_class_.name;

		for( const clang::EnumConstantDecl* const enumerator : enumerators_range )
		{
			Synt::NumericConstant initializer_number( g_dummy_src_loc );
			const llvm::APSInt val= enumerator->getInitVal();
			initializer_number.num.value_int= val.isNegative() ? uint64_t(val.getExtValue()) : val.getLimitedValue();
			initializer_number.num.value_double= static_cast<double>(initializer_number.num.value_int);
			initializer_number.num.type_suffix[0]= 'u';
			if( initializer_number.num.value_int >= 0x7FFFFFFFFu )
			{
				initializer_number.num.type_suffix[1]= '6';
				initializer_number.num.type_suffix[2]= '4';
			}

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

		root_program_elements_.Append( std::move(enum_class_) );
	}
}

void CppAstConsumer::EmitDefinitionsForMacros(
	const NamedFunctionDeclarations& named_function_declarations,
	const NamedRecordDeclarations& named_record_declarations,
	const NamedTypedefDeclarations& named_typedef_declarations,
	const NamedEnumDeclarations& named_enum_declarations,
	const AnonymousEnumMembersSet& anonymous_enum_members )
{
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

		// Rename to avoid name conflicts.
		while(
			named_function_declarations.count( name ) != 0 ||
			named_record_declarations.count( name ) != 0 ||
			named_typedef_declarations.count( name ) != 0 ||
			named_enum_declarations.count( name ) != 0 ||
			anonymous_enum_members.count( name ) != 0 )
			name+= "_";

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
			numeric_constant.num.value_int= int_val.getLimitedValue();

			if( numeric_literal_parser.getRadix() == 10 )
			{
				llvm::APFloat float_val(0.0);
				numeric_literal_parser.GetFloatValue( float_val );

				// "HACK! fix infinity.
				if( float_val.isInfinity() )
					float_val= llvm::APFloat::getLargest( float_val.getSemantics(), float_val.isNegative() );
				numeric_constant.num.value_double= float_val.convertToDouble();
			}
			else
				numeric_constant.num.value_double= static_cast<double>(numeric_constant.num.value_int);

			if( numeric_literal_parser.isFloat )
				numeric_constant.num.type_suffix[0]= 'f';
			else if( numeric_literal_parser.isUnsigned )
			{
				numeric_constant.num.type_suffix[0]= 'u';
				if( numeric_literal_parser.isLongLong )
				{
					numeric_constant.num.type_suffix[1]= '6';
					numeric_constant.num.type_suffix[2]= '4';
				}
			}
			else
			{
				if( numeric_literal_parser.isLongLong )
				{
					numeric_constant.num.type_suffix[0]= 'i';
					numeric_constant.num.type_suffix[1]= '6';
					numeric_constant.num.type_suffix[2]= '4';
				}
			}

			numeric_constant.num.has_fractional_point= numeric_literal_parser.isFloatingLiteral();

			auto_variable_declaration.initializer_expression= std::move(numeric_constant);
			root_program_elements_.Append( std::move( auto_variable_declaration ) );
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
				auto_variable_declaration.name= name;

				Synt::CharLiteral char_literal( g_dummy_src_loc );
				char_literal.code_point= uint32_t( char_literal_parser.getValue() );

				auto_variable_declaration.initializer_expression= std::move(char_literal);
				root_program_elements_.Append( std::move( auto_variable_declaration ) );
			}
		}
	} // for defines
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
