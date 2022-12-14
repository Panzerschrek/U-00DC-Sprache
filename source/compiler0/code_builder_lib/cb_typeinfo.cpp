#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

namespace
{

const std::string g_name_field_name= "name";
const std::string g_type_field_name= "type";
const SrcLoc g_dummy_src_loc;

// Use reserved by language names, started with "_";
const std::string g_typeinfo_root_class_name= "_TI";
const std::string g_typeinfo_enum_elements_list_node_class_name= "_TIEL_";
const std::string g_typeinfo_class_fields_list_node_class_name= "_TICFiL_";
const std::string g_typeinfo_class_types_list_node_class_name= "_TICTL_";
const std::string g_typeinfo_class_functions_list_node_class_name= "_TICFuL_";
const std::string g_typeinfo_class_parents_list_node_class_name= "_TICPL_";
const std::string g_typeinfo_function_arguments_list_node_class_name= "_TIAL_";
const std::string g_typeinfo_tuple_elements_list_node_class_name= "_TITL_";

std::string GetTypeinfoVariableName( const ClassPtr& typeinfo_class )
{
	return "_val_of_" + std::string(typeinfo_class->llvm_type->getName());
}

struct TypeinfoListElement
{
	std::string name_for_ordering;
	llvm::Constant* initializer= nullptr;
	ClassPtr type= nullptr;
};

Variable FinalizeTypeinfoList( llvm::LLVMContext& llvm_context, std::vector<TypeinfoListElement>& list )
{
	std::sort(
		list.begin(), list.end(),
		[]( const TypeinfoListElement& l, const TypeinfoListElement& r )
		{
			return l.name_for_ordering < r.name_for_ordering;
		} );

	TupleType list_type;
	std::vector< llvm::Type* > list_elements_llvm_types;
	std::vector< llvm::Constant* > list_elements_initializers;
	list_type.elements.reserve( list.size() );
	list_elements_llvm_types.reserve( list.size() );
	list_elements_initializers.reserve( list.size() );

	for( const TypeinfoListElement& list_element : list )
	{
		list_type.elements.emplace_back( list_element.type );
		list_elements_llvm_types.push_back( list_element.type->llvm_type );
		list_elements_initializers.push_back( list_element.initializer );
	}

	list_type.llvm_type= llvm::StructType::get( llvm_context, list_elements_llvm_types );
	llvm::Constant* const initializer= llvm::ConstantStruct::get( list_type.llvm_type, list_elements_initializers );

	return
		Variable(
			std::move(list_type),
			Variable::Location::LLVMRegister,
			ValueType::Value,
			initializer,
			initializer );
}

} // namespace

Variable CodeBuilder::BuildTypeInfo( const Type& type, NamesScope& root_namespace )
{
	if( const auto it= typeinfo_cache_.find( type ); it != typeinfo_cache_.end() )
		return it->second;

	typeinfo_cache_.emplace( type, BuildTypeinfoPrototype( type, root_namespace ) );
	Variable& var= typeinfo_cache_[type];
	return var;
}

ClassPtr CodeBuilder::CreateTypeinfoClass( NamesScope& root_namespace, const Type& src_type, std::string name )
{
	// Currently, give "random" names for typeinfo classes.
	llvm::StructType* const llvm_type= llvm::StructType::create( llvm_context_ );

	const auto typeinfo_class_ptr= std::make_shared<Class>( std::move(name), &root_namespace );
	typeinfo_class_table_.push_back(typeinfo_class_ptr);
	ClassPtr typeinfo_class= typeinfo_class_ptr.get();

	typeinfo_class->llvm_type= llvm_type;
	typeinfo_class->typeinfo_type= src_type;

	llvm_type->setName( mangler_->MangleType( typeinfo_class ) );

	typeinfo_class_ptr->inner_reference_type= InnerReferenceType::Imut; // Almost all typeinfo have references to another typeinfo.

	return typeinfo_class_ptr.get();
}

Variable CodeBuilder::BuildTypeinfoPrototype( const Type& type, NamesScope& root_namespace )
{
	const ClassPtr typeinfo_class= CreateTypeinfoClass( root_namespace, type, g_typeinfo_root_class_name );
	Variable result( typeinfo_class, Variable::Location::Pointer, ValueType::ReferenceImut );

	result.constexpr_value= llvm::UndefValue::get( typeinfo_class->llvm_type ); // Currently uninitialized.
	result.llvm_value=
		CreateGlobalConstantVariable(
			result.type,
			GetTypeinfoVariableName( typeinfo_class ),
			result.constexpr_value );

	Type src_type= type;

	// Replace function type alias with function pointer type alias to forbid usage of function type in user code.
	if( const auto function_type= type.GetFunctionType() )
	{
		FunctionPointerType function_pointer_type;
		function_pointer_type.function= *function_type;
		function_pointer_type.llvm_type= function_type->llvm_type->getPointerTo();
		src_type= std::move(function_pointer_type);
	}

	// This allows to get typename itself, using typeinfo variable and use such type as normal.
	typeinfo_class->members->AddName( "src_type", Value( src_type, g_dummy_src_loc ) );

	return result;
}

void CodeBuilder::BuildFullTypeinfo( const Type& type, Variable& typeinfo_variable, NamesScope& root_namespace )
{
	if( !( EnsureTypeComplete( type ) || type.GetFunctionType() != nullptr ) )
	{
		// Just ignore here incomplete types, report about error while building "typeinfo" operator.
		return;
	}

	U_ASSERT( typeinfo_variable.type.GetClassType() != nullptr );

	const ClassPtr typeinfo_class= typeinfo_variable.type.GetClassType();
	if( typeinfo_class->is_complete )
		return;

	ClassFieldsVector<llvm::Type*> fields_llvm_types;
	ClassFieldsVector<llvm::Constant*> fields_initializers;

	const auto add_bool_field=
	[&]( const std::string& name, const bool value )
	{
		typeinfo_class->members->AddName(
			name,
			Value(ClassField( typeinfo_class, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
		fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, uint64_t(value) ) ) );
	};

	const auto add_size_field=
	[&]( const std::string& name, const uint64_t value )
	{
		typeinfo_class->members->AddName(
			name,
			Value( ClassField( typeinfo_class, size_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
		fields_llvm_types.push_back( size_type_.GetLLVMType() );
		fields_initializers.push_back(
			llvm::Constant::getIntegerValue(
				size_type_.GetLLVMType(),
				llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), value ) ) );
	};

	const auto add_typeinfo_field=
	[&]( const std::string& name, const Type& dependent_type )
	{
		const Variable dependent_type_typeinfo= BuildTypeInfo( dependent_type, root_namespace );

		typeinfo_class->members->AddName(
			name,
			Value( ClassField( typeinfo_class, dependent_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), g_dummy_src_loc ) );
		fields_llvm_types.push_back( dependent_type_typeinfo.type.GetLLVMType()->getPointerTo() );
		fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo.llvm_value ) );
	};

	const auto add_list_field=
	[&]( const std::string& name, const Variable& variable )
	{
		typeinfo_class->members->AddName(
			name,
			Value( ClassField( typeinfo_class, variable.type, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
		fields_llvm_types.push_back( variable.type.GetLLVMType() );
		fields_initializers.push_back( variable.constexpr_value );
	};

	// Fields sorted by alignment - first, "size_type" types and reference types, then, bool types.

	if( type.GetFunctionType() == nullptr )
	{
		llvm::Type* const llvm_type= type.GetLLVMType();
		// see llvm/lib/IR/DataLayout.cpp:40
		add_size_field( "size_of" , data_layout_.getTypeAllocSize   ( llvm_type ) );
		add_size_field( "align_of", data_layout_.getABITypeAlignment( llvm_type ) );
	}

	add_size_field( "references_tags_count", type.ReferencesTagsCount() );
	add_bool_field( "contains_mutable_references", type.GetInnerReferenceType() == InnerReferenceType::Mut );

	add_bool_field( "is_fundamental"     , type.GetFundamentalType()     != nullptr );
	add_bool_field( "is_enum"            , type.GetEnumType()            != nullptr );
	add_bool_field( "is_array"           , type.GetArrayType()           != nullptr );
	add_bool_field( "is_tuple"           , type.GetTupleType()           != nullptr );
	add_bool_field( "is_class"           , type.GetClassType()           != nullptr );
	add_bool_field( "is_raw_pointer"     , type.GetRawPointerType()      != nullptr );
	add_bool_field( "is_function_pointer", type.GetFunctionPointerType() != nullptr );
	add_bool_field( "is_function"        , type.GetFunctionType()        != nullptr );

	add_bool_field( "is_default_constructible", type.IsDefaultConstructible() );
	add_bool_field( "is_copy_constructible"   , type.IsCopyConstructible()    );
	add_bool_field( "is_copy_assignable"      , type.IsCopyAssignable()       );
	add_bool_field( "is_equality_comparable"  , type.IsEqualityComparable()   );

	if( const FundamentalType* const fundamental_type= type.GetFundamentalType() )
	{
		add_bool_field( "is_integer"         , IsInteger        ( fundamental_type->fundamental_type ) );
		add_bool_field( "is_numeric"         , IsNumericType    ( fundamental_type->fundamental_type ) );
		add_bool_field( "is_signed_integer"  , IsSignedInteger  ( fundamental_type->fundamental_type ) );
		add_bool_field( "is_unsigned_integer", IsUnsignedInteger( fundamental_type->fundamental_type ) );
		add_bool_field( "is_float"           , IsFloatingPoint  ( fundamental_type->fundamental_type ) );
		add_bool_field( "is_char"            , IsChar           ( fundamental_type->fundamental_type ) );
		add_bool_field( "is_bool"            , fundamental_type->fundamental_type == U_FundamentalType::Bool );
		add_bool_field( "is_void"            , fundamental_type->fundamental_type == U_FundamentalType::Void );
	}
	else if( const Enum* const enum_type= type.GetEnumType() )
	{
		add_size_field( "element_count", enum_type->element_count );
		add_typeinfo_field( "underlaying_type", enum_type->underlaying_type );
		add_list_field( "elements_list", BuildTypeinfoEnumElementsList( type.GetEnumType(), root_namespace ) );
	}
	else if( const ArrayType* const array_type= type.GetArrayType() )
	{
		add_size_field( "element_count", array_type->size );
		add_typeinfo_field( "element_type", array_type->type );
	}
	else if( const TupleType* const tuple_type= type.GetTupleType() )
	{
		add_size_field( "element_count", tuple_type->elements.size() );
		add_list_field( "elements_list", BuildTypeinfoTupleElements( *tuple_type, root_namespace ) );
	}
	else if( const ClassPtr class_type= type.GetClassType() )
	{
		U_ASSERT( class_type->is_complete );

		const bool is_polymorph=
			class_type->kind == Class::Kind::Interface ||
			class_type->kind == Class::Kind::Abstract ||
			class_type->kind == Class::Kind::PolymorphNonFinal ||
			class_type->kind == Class::Kind::PolymorphFinal;

		add_size_field( "field_count", class_type->field_count );
		add_size_field( "parent_count", class_type->parents.size() );

		add_list_field( "fields_list"   , BuildTypeinfoClassFieldsList(    class_type, root_namespace ) );
		add_list_field( "types_list"    , BuildTypeinfoClassTypesList(     class_type, root_namespace ) );
		add_list_field( "functions_list", BuildTypeinfoClassFunctionsList( class_type, root_namespace ) );
		add_list_field( "parents_list"  , BuildTypeinfoClassParentsList(  class_type, root_namespace ) );

		if( is_polymorph )
		{
			U_ASSERT( class_type->polymorph_type_id_table != nullptr );
			typeinfo_class->members->AddName(
				"type_id",
				Value( ClassField( typeinfo_class, size_type_, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( fundamental_llvm_types_.int_ptr->getPointerTo() );

			// Take address of fist member of first element of typeinfo table, which is offset of type "int_ptr".
			llvm::Value* const gep_indices[]{ GetZeroGEPIndex(), GetZeroGEPIndex(), GetZeroGEPIndex() };
			const auto address=
				llvm::ConstantExpr::getGetElementPtr(
					class_type->polymorph_type_id_table->getType()->getPointerElementType(),
					class_type->polymorph_type_id_table,
					gep_indices );
			fields_initializers.push_back( address );
		}

		add_bool_field( "is_struct", class_type->kind == Class::Kind::Struct );
		add_bool_field( "is_polymorph", is_polymorph );
		add_bool_field( "is_final",
			class_type->kind == Class::Kind::Struct ||
			class_type->kind == Class::Kind::NonPolymorph ||
			class_type->kind == Class::Kind::PolymorphFinal );
		add_bool_field( "is_abstract",
			class_type->kind == Class::Kind::Abstract ||
			class_type->kind == Class::Kind::Interface );

		add_bool_field( "is_interface", class_type->kind == Class::Kind::Interface );

		add_bool_field( "is_typeinfo", class_type->typeinfo_type != std::nullopt );

		// TODO - remove this typeinfo field.
		add_bool_field( "shared", false );
	}
	else if( const RawPointerType* const raw_pointer_type= type.GetRawPointerType() )
	{
		add_typeinfo_field( "element_type", raw_pointer_type->type );
	}
	else if( const FunctionPointerType* const function_pointer_type= type.GetFunctionPointerType() )
	{
		add_typeinfo_field( "element_type", function_pointer_type->function );
	}
	else if( const FunctionType* const function_type= type.GetFunctionType() )
	{
		add_typeinfo_field( "return_type", function_type->return_type );
		add_list_field( "arguments_list"      , BuildTypeinfoFunctionArguments( *function_type, root_namespace ) );
		add_bool_field( "return_value_is_reference", function_type->return_value_type != ValueType::Value );
		add_bool_field( "return_value_is_mutable"  , function_type->return_value_type == ValueType::ReferenceMut );
		add_bool_field( "unsafe"                   , function_type->unsafe );
		// SPRACHE_TODO - add also reference pollution.
	}
	else U_ASSERT(false);

	FinishTypeinfoClass( typeinfo_class, fields_llvm_types );

	// Prepare result value
	typeinfo_variable.constexpr_value= llvm::ConstantStruct::get( typeinfo_class->llvm_type, fields_initializers );
	llvm::dyn_cast<llvm::GlobalVariable>(typeinfo_variable.llvm_value)->setInitializer( typeinfo_variable.constexpr_value );
}

void CodeBuilder::FinishTypeinfoClass( const ClassPtr& class_type, const ClassFieldsVector<llvm::Type*>& fields_llvm_types )
{
	Class& class_= *class_type;
	class_.llvm_type->setBody( fields_llvm_types );
	class_.kind= Class::Kind::Struct;
	class_.is_complete= true;
	class_.can_be_constexpr= true;

	// Generate only destructor, because almost all structs and classes must have it.
	// Other methods - constructors, assignment operators does not needs for typeinfo classes.
	TryGenerateDestructor( class_type );

	const FunctionVariable& destructor= class_.members->GetThisScopeValue( Keyword( Keywords::destructor_ ) )->GetFunctionsSet()->functions.front();
	destructor.llvm_function->setName( mangler_->MangleFunction( *class_.members, Keyword( Keywords::destructor_ ), *destructor.type.GetFunctionType() ) );
}

Variable CodeBuilder::BuildTypeinfoEnumElementsList( const EnumPtr& enum_type, NamesScope& root_namespace )
{
	std::vector<TypeinfoListElement> list_elements;
	list_elements.reserve( enum_type->element_count );

	enum_type->members.ForEachInThisScope(
		[&]( const std::string& name, const Value& enum_member )
		{
			llvm::Constant* const enum_member_value= enum_member.GetVariable()->constexpr_value;

			const ClassPtr node_type=
				CreateTypeinfoClass(
					root_namespace,
					enum_type,
					g_typeinfo_enum_elements_list_node_class_name + std::to_string( enum_member_value->getUniqueInteger().getLimitedValue() ) );
			Class& node_type_class= *node_type;

			ClassFieldsVector<llvm::Type*> fields_llvm_types;
			ClassFieldsVector<llvm::Constant*> fields_initializers;

			node_type_class.members->AddName(
				"value",
				Value( ClassField( node_type, enum_type->underlaying_type, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( enum_type->underlaying_type.llvm_type );
			fields_initializers.push_back( enum_member_value );

			{
				ArrayType name_type;
				name_type.type= FundamentalType( U_FundamentalType::char8, fundamental_llvm_types_.char8 );
				name_type.size= name.size();
				name_type.llvm_type= llvm::ArrayType::get( name_type.type.GetLLVMType(), name_type.size );

				ClassField field( node_type, name_type, static_cast<unsigned int>(fields_llvm_types.size()), true, false );

				node_type_class.members->AddName( g_name_field_name, Value( std::move(field), g_dummy_src_loc ) );
				fields_llvm_types.push_back( name_type.llvm_type );
				fields_initializers.push_back( llvm::ConstantDataArray::getString( llvm_context_, name, false /* not null terminated */ ) );
			}

			FinishTypeinfoClass( node_type, fields_llvm_types );

			list_elements.push_back(
				TypeinfoListElement{
					name,
					llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ),
					node_type } );
		} );

	return FinalizeTypeinfoList( llvm_context_, list_elements );
}

void CodeBuilder::CreateTypeinfoClassMembersListNodeCommonFields(
	const Class& class_, const ClassPtr& node_class_type,
	const std::string& member_name,
	ClassFieldsVector<llvm::Type*>& fields_llvm_types, ClassFieldsVector<llvm::Constant*>& fields_initializers )
{
	Class& node_class= *node_class_type;

	{
		ArrayType name_type;
		name_type.type= FundamentalType( U_FundamentalType::char8, fundamental_llvm_types_.char8 );
		name_type.size= member_name.size();
		name_type.llvm_type= llvm::ArrayType::get( name_type.type.GetLLVMType(), name_type.size );

		node_class.members->AddName(
			g_name_field_name,
			Value( ClassField( node_class_type, name_type, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
		fields_llvm_types.push_back( name_type.llvm_type );
		fields_initializers.push_back( llvm::ConstantDataArray::getString( llvm_context_, member_name, false /* not null terminated */ ) );
	}

	const ClassMemberVisibility member_visibility= class_.GetMemberVisibility( member_name );

	node_class.members->AddName(
		"is_public",
		Value( ClassField( node_class_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
	fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
	fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, member_visibility == ClassMemberVisibility::Public    ) ) );

	node_class.members->AddName(
		"is_protected",
		Value( ClassField( node_class_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
	fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
	fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, member_visibility == ClassMemberVisibility::Protected ) ) );

	node_class.members->AddName(
		"is_private",
		Value( ClassField( node_class_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
	fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
	fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, member_visibility == ClassMemberVisibility::Private   ) ) );
}

Variable CodeBuilder::BuildTypeinfoClassFieldsList( const ClassPtr& class_type, NamesScope& root_namespace )
{
	std::vector<TypeinfoListElement> list_elements;

	const auto process_class_member=
		[&]( const std::string& member_name, const Value& class_member )
		{
			const ClassField* const class_field= class_member.GetClassField();
			if( class_field == nullptr )
				return;

			const ClassPtr node_type= CreateTypeinfoClass( root_namespace, class_type, g_typeinfo_class_fields_list_node_class_name + member_name );
			Class& node_type_class= *node_type;

			ClassFieldsVector<llvm::Type*> fields_llvm_types;
			ClassFieldsVector<llvm::Constant*> fields_initializers;

			{
				const Variable field_type_typeinfo= BuildTypeInfo( class_field->type, root_namespace );
				ClassField field( node_type, field_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

				node_type_class.members->AddName( g_type_field_name, Value( std::move(field), g_dummy_src_loc ) );
				fields_llvm_types.push_back( field_type_typeinfo.type.GetLLVMType()->getPointerTo() );
				fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( field_type_typeinfo.llvm_value ) );
			}
			{
				const Variable fields_class_type_typeinfo= BuildTypeInfo( class_field->class_, root_namespace );
				ClassField field( node_type, fields_class_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

				node_type_class.members->AddName( "class_type", Value( std::move(field), g_dummy_src_loc ) );
				fields_llvm_types.push_back( fields_class_type_typeinfo.type.GetLLVMType()->getPointerTo() );
				fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( fields_class_type_typeinfo.llvm_value ) );
			}
			{
				// For ancestor fields accumulate offset.
				uint64_t offset= 0u;
				ClassPtr class_for_field_search= class_type;
				while(true)
				{
					if( class_for_field_search == class_field->class_ )
					{
						offset+= data_layout_.getStructLayout( class_for_field_search->llvm_type )->getElementOffset( class_field->index );
						break;
					}
					else
					{
						U_ASSERT( class_for_field_search->base_class != nullptr );
						offset+= data_layout_.getStructLayout( class_for_field_search->llvm_type )->getElementOffset( 0u /*base class is allways first field */ );
						class_for_field_search= class_for_field_search->base_class;
					}
				}

				node_type_class.members->AddName(
					"offset",
					Value( ClassField( node_type, size_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
				fields_llvm_types.push_back( size_type_.GetLLVMType() );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( size_type_.GetLLVMType(), llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), offset ) ) );
			}

			node_type_class.members->AddName(
				"is_reference",
				Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
			fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, class_field->is_reference ) ) );

			node_type_class.members->AddName(
				"is_mutable",
				Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
			fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, class_field->is_mutable ) ) );

			CreateTypeinfoClassMembersListNodeCommonFields( *class_type, node_type, member_name, fields_llvm_types, fields_initializers );

			FinishTypeinfoClass( node_type, fields_llvm_types );

			list_elements.push_back(
				TypeinfoListElement{
					member_name,
					llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ),
					node_type } );
		};

	class_type->members->ForEachInThisScope( process_class_member );

	return FinalizeTypeinfoList( llvm_context_, list_elements );
}

Variable CodeBuilder::BuildTypeinfoClassTypesList( const ClassPtr& class_type, NamesScope& root_namespace )
{
	std::vector<TypeinfoListElement> list_elements;

	const auto process_class_member=
		[&]( const std::string& name, Value& class_member )
		{
			if( class_member.GetTypedef() != nullptr ) // Event in complete class typedefs may be not yet complete. Complete it now.
				GlobalThingBuildTypedef( *class_type->members, class_member );

			const Type* const class_inner_type= class_member.GetTypeName();
			if( class_inner_type == nullptr )
				return;

			const ClassPtr node_type= CreateTypeinfoClass( root_namespace, class_type, g_typeinfo_class_types_list_node_class_name + name );
			Class& node_type_class= *node_type;

			ClassFieldsVector<llvm::Type*> fields_llvm_types;
			ClassFieldsVector<llvm::Constant*> fields_initializers;

			{
				const Variable dependent_type_typeinfo= BuildTypeInfo( *class_inner_type, root_namespace );
				ClassField field( node_type, dependent_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

				node_type_class.members->AddName( g_type_field_name, Value( std::move(field), g_dummy_src_loc ) );
				fields_llvm_types.push_back( dependent_type_typeinfo.type.GetLLVMType()->getPointerTo() );
				fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo.llvm_value ) );
			}

			CreateTypeinfoClassMembersListNodeCommonFields( *class_type, node_type, name, fields_llvm_types, fields_initializers );

			FinishTypeinfoClass( node_type, fields_llvm_types );

			list_elements.push_back(
				TypeinfoListElement{
					name,
					llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ),
					node_type } );
		};

	class_type->members->ForEachInThisScope( process_class_member );

	return FinalizeTypeinfoList( llvm_context_, list_elements );
}

Variable CodeBuilder::BuildTypeinfoClassFunctionsList( const ClassPtr& class_type, NamesScope& root_namespace )
{
	std::vector<TypeinfoListElement> list_elements;

	const auto process_class_member=
		[&]( const std::string& name, const Value& class_member )
		{
			const OverloadedFunctionsSet* const functions_set= class_member.GetFunctionsSet();
			if( functions_set == nullptr )
				return;

			for( const FunctionVariable& function : functions_set->functions )
			{
				const ClassPtr node_type=
					CreateTypeinfoClass(
						root_namespace,
						class_type,
						g_typeinfo_class_functions_list_node_class_name + std::string(function.llvm_function->getName()) ); // Use mangled name for type name.
				Class& node_type_class= *node_type;

				ClassFieldsVector<llvm::Type*> fields_llvm_types;
				ClassFieldsVector<llvm::Constant*> fields_initializers;

				{
					const Variable dependent_type_typeinfo= BuildTypeInfo( function.type, root_namespace );
					ClassField field( node_type, dependent_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

					node_type_class.members->AddName( g_type_field_name, Value( std::move(field), g_dummy_src_loc ) );
					fields_llvm_types.push_back( dependent_type_typeinfo.type.GetLLVMType()->getPointerTo() );
					fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo.llvm_value ) );
				}

				node_type_class.members->AddName(
					"is_this_call",
					Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.is_this_call ) ) );

				node_type_class.members->AddName(
					"is_generated",
					Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.is_generated ) ) );

				node_type_class.members->AddName(
					"is_deleted",
					Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.is_deleted ) ) );

				node_type_class.members->AddName(
					"is_virtual",
					Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.virtual_table_index != ~0u ) ) );

				CreateTypeinfoClassMembersListNodeCommonFields( *class_type, node_type, name, fields_llvm_types, fields_initializers );

				FinishTypeinfoClass( node_type, fields_llvm_types );

				list_elements.push_back(
					TypeinfoListElement{
						function.llvm_function->getName(), // Sort, using function mangled name.
						llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ),
						node_type } );
			} // for functions with same name
		};

	class_type->members->ForEachInThisScope( process_class_member );

	return FinalizeTypeinfoList( llvm_context_, list_elements );
}

Variable CodeBuilder::BuildTypeinfoClassParentsList( const ClassPtr& class_type, NamesScope& root_namespace )
{
	const Class& class_= *class_type;
	const llvm::StructLayout* const struct_layout= data_layout_.getStructLayout( class_.llvm_type );

	TupleType list_type;
	std::vector< llvm::Type* > list_elements_llvm_types;
	std::vector< llvm::Constant* > list_elements_initializers;
	list_type.elements.reserve( class_.parents.size() );
	list_elements_llvm_types.reserve( class_.parents.size() );
	list_elements_initializers.reserve( class_.parents.size() );

	for( size_t i= 0u; i < class_.parents.size(); ++i )
	{
		const ClassPtr node_type= CreateTypeinfoClass( root_namespace, class_type, g_typeinfo_class_parents_list_node_class_name + std::to_string(i) );
		Class& node_type_class= *node_type;

		ClassFieldsVector<llvm::Type*> fields_llvm_types;
		ClassFieldsVector<llvm::Constant*> fields_initializers;

		{
			const Variable parent_type_typeinfo= BuildTypeInfo( class_.parents[i].class_, root_namespace );
			ClassField field( node_type, parent_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

			node_type_class.members->AddName( g_type_field_name, Value( std::move(field), g_dummy_src_loc ) );
			fields_llvm_types.push_back( parent_type_typeinfo.type.GetLLVMType()->getPointerTo() );
			fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( parent_type_typeinfo.llvm_value ) );
		}

		const uint64_t parent_field_offset= struct_layout->getElementOffset( class_.parents[i].field_number );
		node_type_class.members->AddName(
			"offset",
			Value( ClassField( node_type, size_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
		fields_llvm_types.push_back( size_type_.GetLLVMType() );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( size_type_.GetLLVMType(), llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), parent_field_offset ) ) );

		FinishTypeinfoClass( node_type, fields_llvm_types );

		list_type.elements.push_back( node_type );
		list_elements_llvm_types.push_back( node_type_class.llvm_type );
		list_elements_initializers.push_back( llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );
	} // for parents

	list_type.llvm_type= llvm::StructType::get( llvm_context_, list_elements_llvm_types );
	llvm::Constant* const initializer= llvm::ConstantStruct::get( list_type.llvm_type, list_elements_initializers );

	return
		Variable(
			std::move(list_type),
			Variable::Location::LLVMRegister,
			ValueType::Value,
			initializer,
			initializer );
}

Variable CodeBuilder::BuildTypeinfoFunctionArguments( const FunctionType& function_type, NamesScope& root_namespace )
{
	TupleType list_type;
	std::vector< llvm::Type* > list_elements_llvm_types;
	std::vector< llvm::Constant* > list_elements_initializers;
	list_type.elements.reserve( function_type.params.size() );
	list_elements_llvm_types.reserve( function_type.params.size() );
	list_elements_initializers.reserve( function_type.params.size() );

	for( const FunctionType::Param& param : function_type.params )
	{
		const size_t param_index= size_t(&param - function_type.params.data());
		const ClassPtr node_type= CreateTypeinfoClass( root_namespace, function_type, g_typeinfo_function_arguments_list_node_class_name + std::to_string(param_index) );
		Class& node_type_class= *node_type;

		ClassFieldsVector<llvm::Type*> fields_llvm_types;
		ClassFieldsVector<llvm::Constant*> fields_initializers;

		{
			const Variable dependent_type_typeinfo= BuildTypeInfo( param.type, root_namespace );
			ClassField field( node_type, dependent_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

			node_type_class.members->AddName( g_type_field_name, Value( std::move(field), g_dummy_src_loc ) );
			fields_llvm_types.push_back( dependent_type_typeinfo.type.GetLLVMType()->getPointerTo() );
			fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo.llvm_value ) );
		}

		node_type_class.members->AddName(
			"is_reference",
			Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
		fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, param.value_type != ValueType::Value ) ) );

		node_type_class.members->AddName(
			"is_mutable",
			Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
		fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, param.value_type == ValueType::ReferenceMut ) ) );

		// SPRACHE_TODO - add reference pollution

		FinishTypeinfoClass( node_type, fields_llvm_types );

		list_type.elements.push_back( node_type );
		list_elements_llvm_types.push_back( node_type_class.llvm_type );
		list_elements_initializers.push_back( llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );
	}

	list_type.llvm_type= llvm::StructType::get( llvm_context_, list_elements_llvm_types );
	llvm::Constant* const initializer= llvm::ConstantStruct::get( list_type.llvm_type, list_elements_initializers );

	return
		Variable(
			std::move(list_type),
			Variable::Location::LLVMRegister,
			ValueType::Value,
			initializer,
			initializer );
}

Variable CodeBuilder::BuildTypeinfoTupleElements( const TupleType& tuple_type, NamesScope& root_namespace )
{
	TupleType list_type;
	std::vector< llvm::Type* > list_elements_llvm_types;
	std::vector< llvm::Constant* > list_elements_initializers;
	list_type.elements.reserve( tuple_type.elements.size() );
	list_elements_llvm_types.reserve( tuple_type.elements.size() );
	list_elements_initializers.reserve( tuple_type.elements.size() );

	const llvm::StructLayout* const struct_layout= data_layout_.getStructLayout( tuple_type.llvm_type );

	for( const Type& element_type : tuple_type.elements )
	{
		const size_t element_index= size_t( &element_type - tuple_type.elements.data() );

		const ClassPtr node_type= CreateTypeinfoClass( root_namespace, tuple_type, g_typeinfo_tuple_elements_list_node_class_name + std::to_string(element_index) );
		Class& node_type_class= *node_type;

		ClassFieldsVector<llvm::Type*> fields_llvm_types;
		ClassFieldsVector<llvm::Constant*> fields_initializers;

		{
			const Variable dependent_type_typeinfo= BuildTypeInfo( element_type, root_namespace );
			ClassField field( node_type, dependent_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

			node_type_class.members->AddName( g_type_field_name, Value( std::move(field), g_dummy_src_loc ) );
			fields_llvm_types.push_back( dependent_type_typeinfo.type.GetLLVMType()->getPointerTo() );
			fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo.llvm_value ) );
		}
		{
			node_type_class.members->AddName(
				"index",
				Value( ClassField( node_type, size_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( size_type_.GetLLVMType() );
			fields_initializers.push_back( llvm::Constant::getIntegerValue( size_type_.GetLLVMType(), llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), element_index ) ) );
		}
		{
			const auto offset= struct_layout->getElementOffset( static_cast<unsigned int>(element_index) );
			node_type_class.members->AddName(
				"offset",
				Value( ClassField( node_type, size_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( size_type_.GetLLVMType() );
			fields_initializers.push_back( llvm::Constant::getIntegerValue( size_type_.GetLLVMType(), llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), offset ) ) );
		}
		FinishTypeinfoClass( node_type, fields_llvm_types );

		list_type.elements.push_back( node_type );
		list_elements_llvm_types.push_back( node_type_class.llvm_type );
		list_elements_initializers.push_back( llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );
	}

	list_type.llvm_type= llvm::StructType::get( llvm_context_, list_elements_llvm_types );
	llvm::Constant* const initializer= llvm::ConstantStruct::get( list_type.llvm_type, list_elements_initializers );

	return
		Variable(
			std::move(list_type),
			Variable::Location::LLVMRegister,
			ValueType::Value,
			initializer,
			initializer );
}

} // namespace U
