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
const std::string g_typeinfo_function_params_list_node_class_name= "_TIAL_";
const std::string g_typeinfo_tuple_elements_list_node_class_name= "_TITL_";

} // namespace

VariablePtr CodeBuilder::BuildTypeInfo( const Type& type, NamesScope& root_namespace )
{
	if( const auto it= typeinfo_cache_.find( type ); it != typeinfo_cache_.end() )
		return it->second.variable;

	const auto result= BuildTypeinfoPrototype( type, root_namespace );
	typeinfo_cache_.emplace( type, TypeinfoCacheElement{ result } );
	return result;
}

ClassPtr CodeBuilder::CreateTypeinfoClass( NamesScope& root_namespace, const Type& src_type, std::string name )
{
	// Currently, give "random" names for typeinfo classes.
	// There is no reason to give meaningfull names to LLVM types, since these names are insignificant.
	// Avoiding calculation and storing of mangled names reduces compilation time and memory usage.
	llvm::StructType* const llvm_type= llvm::StructType::create( llvm_context_ );

	auto typeinfo_class_ptr= std::make_unique<Class>( std::move(name), &root_namespace );
	const ClassPtr typeinfo_class= typeinfo_class_ptr.get();
	typeinfo_class_table_.push_back( std::move(typeinfo_class_ptr) );

	typeinfo_class->llvm_type= llvm_type;
	typeinfo_class->generated_class_data= TypeinfoClassDescription{ src_type, false /* non-main by default */ };

	typeinfo_class->inner_references.push_back( InnerReferenceType::Imut ); // Almost all typeinfo have references to another typeinfo.

	return typeinfo_class;
}

VariableMutPtr CodeBuilder::BuildTypeinfoPrototype( const Type& type, NamesScope& root_namespace )
{
	const ClassPtr typeinfo_class= CreateTypeinfoClass( root_namespace, type, g_typeinfo_root_class_name );
	const VariableMutPtr result=
		Variable::Create(
			typeinfo_class,
			ValueType::ReferenceImut,
			Variable::Location::Pointer,
			"typeinfo</" + type.ToString() + "/>" );

	result->constexpr_value= llvm::UndefValue::get( typeinfo_class->llvm_type ); // Currently uninitialized.
	result->llvm_value= CreateGlobalConstantVariable( result->type, "", result->constexpr_value );

	// This allows to get typename itself, using typeinfo variable and use such type as normal.
	typeinfo_class->members->AddName( "src_type", NamesScopeValue( type, g_dummy_src_loc ) );

	// Mark this typeinfo class as main typeinfo class.
	std::get_if<TypeinfoClassDescription>( &typeinfo_class->generated_class_data )->is_main_class= true;

	return result;
}

void CodeBuilder::BuildFullTypeinfo( const Type& type, const VariableMutPtr& typeinfo_variable, NamesScope& root_namespace )
{
	if( !EnsureTypeComplete( type ) )
	{
		// Just ignore here incomplete types, report about error while building "typeinfo" operator.
		return;
	}

	U_ASSERT( typeinfo_variable->type.GetClassType() != nullptr );

	const ClassPtr typeinfo_class= typeinfo_variable->type.GetClassType();
	if( typeinfo_class->is_complete )
		return;

	ClassFieldsVector<llvm::Type*> fields_llvm_types;
	ClassFieldsVector<llvm::Constant*> fields_initializers;

	const auto add_bool_field=
	[&]( const std::string& name, const bool value )
	{
		typeinfo_class->members->AddName(
			name,
			NamesScopeValue( std::make_shared<ClassField>( typeinfo_class, bool_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
		fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, uint64_t(value) ) ) );
	};

	const auto add_size_field=
	[&]( const std::string& name, const uint64_t value )
	{
		typeinfo_class->members->AddName(
			name,
			NamesScopeValue( std::make_shared<ClassField>( typeinfo_class, size_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
		fields_llvm_types.push_back( size_type_.GetLLVMType() );
		fields_initializers.push_back(
			llvm::Constant::getIntegerValue(
				size_type_.GetLLVMType(),
				llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), value ) ) );
	};

	const auto add_typeinfo_field=
	[&]( const std::string& name, const Type& dependent_type )
	{
		const VariablePtr dependent_type_typeinfo= BuildTypeInfo( dependent_type, root_namespace );

		typeinfo_class->members->AddName(
			name,
			NamesScopeValue( std::make_shared<ClassField>( typeinfo_class, dependent_type_typeinfo->type, uint32_t(fields_llvm_types.size()), false, true ), g_dummy_src_loc ) );
		fields_llvm_types.push_back( dependent_type_typeinfo->type.GetLLVMType()->getPointerTo() );
		fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo->llvm_value ) );
	};

	// Fields sorted by alignment - first, "size_type" types and reference types, then, bool types.

	{
		llvm::Type* const llvm_type= type.GetLLVMType();
		// see llvm/lib/IR/DataLayout.cpp:40
		add_size_field( "size_of" , data_layout_.getTypeAllocSize   ( llvm_type ) );
		add_size_field( "align_of", data_layout_.getABITypeAlignment( llvm_type ) );
	}

	add_size_field( "reference_tag_count", type.ReferenceTagCount() );

	// TODO - rework this - provide array of bools with size equal to number of reference tag count.
	{
		bool contains_mutable_references= false;
		for( size_t i= 0, reference_tag_count= type.ReferenceTagCount(); i < reference_tag_count; ++i )
			contains_mutable_references |= type.GetInnerReferenceType(i) == InnerReferenceType::Mut;

		add_bool_field( "contains_mutable_references", contains_mutable_references );
	}

	add_bool_field( "is_fundamental"     , type.GetFundamentalType()     != nullptr );
	add_bool_field( "is_enum"            , type.GetEnumType()            != nullptr );
	add_bool_field( "is_array"           , type.GetArrayType()           != nullptr );
	add_bool_field( "is_tuple"           , type.GetTupleType()           != nullptr );
	add_bool_field( "is_class"           , type.GetClassType()           != nullptr );
	add_bool_field( "is_raw_pointer"     , type.GetRawPointerType()      != nullptr );
	add_bool_field( "is_function_pointer", type.GetFunctionPointerType() != nullptr );

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
		add_bool_field( "is_byte"            , IsByte           ( fundamental_type->fundamental_type ) );
		add_bool_field( "is_bool"            , fundamental_type->fundamental_type == U_FundamentalType::bool_ );
		add_bool_field( "is_void"            , fundamental_type->fundamental_type == U_FundamentalType::void_ );
	}
	else if( const Enum* const enum_type= type.GetEnumType() )
	{
		add_size_field( "element_count", enum_type->element_count );
		add_typeinfo_field( "underlying_type", enum_type->underlying_type );
	}
	else if( const ArrayType* const array_type= type.GetArrayType() )
	{
		add_size_field( "element_count", array_type->element_count );
		add_typeinfo_field( "element_type", array_type->element_type );
	}
	else if( const TupleType* const tuple_type= type.GetTupleType() )
	{
		add_size_field( "element_count", tuple_type->element_types.size() );
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

		if( is_polymorph )
		{
			U_ASSERT( class_type->polymorph_type_id_table != nullptr );
			typeinfo_class->members->AddName(
				"type_id",
				NamesScopeValue( std::make_shared<ClassField>( typeinfo_class, size_type_, uint32_t(fields_llvm_types.size()), false, true ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( fundamental_llvm_types_.int_ptr->getPointerTo() );

			// Take address of fist member of first element of typeinfo table, which is offset of type "int_ptr".
			llvm::Value* const gep_indices[]{ GetZeroGEPIndex(), GetZeroGEPIndex(), GetZeroGEPIndex() };
			const auto address=
				llvm::ConstantExpr::getGetElementPtr(
					class_type->polymorph_type_id_table->getInitializer()->getType(),
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

		add_bool_field( "is_typeinfo", std::get_if<TypeinfoClassDescription>( &class_type->generated_class_data ) != nullptr );

		if( const auto coroutine_type_description= std::get_if<CoroutineTypeDescription>( &class_type->generated_class_data ) )
		{
			add_bool_field( "is_coroutine", true );

			add_bool_field( "is_generator", coroutine_type_description->kind == CoroutineKind::Generator );
			add_bool_field( "is_async_func", coroutine_type_description->kind == CoroutineKind::AsyncFunc );

			add_typeinfo_field( "coroutine_return_type", coroutine_type_description->return_type );
			add_bool_field( "coroutine_return_value_is_reference", coroutine_type_description->return_value_type != ValueType::Value );
			add_bool_field( "coroutine_return_value_is_mutable"  , coroutine_type_description->return_value_type == ValueType::ReferenceMut );
		}
		else
			add_bool_field( "is_coroutine", false );

		add_bool_field( "is_lambda", std::holds_alternative<LambdaClassData>( class_type->generated_class_data ) );
	}
	else if( const RawPointerType* const raw_pointer_type= type.GetRawPointerType() )
	{
		add_typeinfo_field( "element_type", raw_pointer_type->element_type );
	}
	else if( const FunctionPointerType* const function_pointer_type= type.GetFunctionPointerType() )
	{
		const FunctionType& function_type= function_pointer_type->function_type;
		add_typeinfo_field( "return_type", function_type.return_type );
		add_bool_field( "return_value_is_reference", function_type.return_value_type != ValueType::Value );
		add_bool_field( "return_value_is_mutable"  , function_type.return_value_type == ValueType::ReferenceMut );
		add_bool_field( "unsafe"                   , function_type.unsafe );

		{
			const auto return_references_contant= GetReturnReferencesConstant( function_type.return_references );

			typeinfo_class->members->AddName(
				"return_references",
				NamesScopeValue( std::make_shared<ClassField>( typeinfo_class, return_references_contant.first, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( return_references_contant.first.GetLLVMType() );
			fields_initializers.push_back( return_references_contant.second );
		}
		{
			const auto return_inner_references_contant= GetReturnInnerReferencesConstant( function_type.return_inner_references );

			typeinfo_class->members->AddName(
				"return_inner_references",
				NamesScopeValue( std::make_shared<ClassField>( typeinfo_class, return_inner_references_contant.first, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( return_inner_references_contant.first.GetLLVMType() );
			fields_initializers.push_back( return_inner_references_contant.second );
		}

		// TODO - encode reference pollution.
	}
	else U_ASSERT(false);

	FinishTypeinfoClass( typeinfo_class, fields_llvm_types );

	// Prepare result value
	typeinfo_variable->constexpr_value= llvm::ConstantStruct::get( typeinfo_class->llvm_type, fields_initializers );
	llvm::dyn_cast<llvm::GlobalVariable>(typeinfo_variable->llvm_value)->setInitializer( typeinfo_variable->constexpr_value );
}

void CodeBuilder::FinishTypeinfoClass( const ClassPtr class_type, const ClassFieldsVector<llvm::Type*>& fields_llvm_types )
{
	Class& class_= *class_type;
	class_.llvm_type->setBody( fields_llvm_types );
	class_.kind= Class::Kind::Struct;
	class_.is_complete= true;
	class_.can_be_constexpr= true;

	// Generate no destructor. There is no reason to generate it for typeinfo class.
	// Avoiding destructor generation saves some compilation time and memory.
}

VariablePtr CodeBuilder::TryFetchTypeinfoClassLazyField( const Type& typeinfo_type, const std::string_view name )
{
	// Generate typeinfo list fields on-demand, instead of creating lists as typeinfo class fields as usual.
	// The main reason to do this - for faster compilation.
	// Building typeinfo lists is too costly - requires creation of bunch of node classes, each with members namespace, destructor, fields, etc.
	// So, avoid doing this unless typeinfo list isn't direclty accessed.
	// Such optimization also reduces total memory usage, that is especially important for the Language Server.

	const ClassPtr typeinfo_class_type= typeinfo_type.GetClassType();
	if( typeinfo_class_type == nullptr )
		return nullptr;

	const auto typeinfo_type_description= std::get_if<TypeinfoClassDescription>( &typeinfo_class_type->generated_class_data );
	if( typeinfo_type_description == nullptr )
		return nullptr;

	if( !typeinfo_type_description->is_main_class )
		return nullptr;

	const Type& source_type= typeinfo_type_description->source_type;

	const auto typeinfo_table_it= typeinfo_cache_.find( source_type );
	U_ASSERT( typeinfo_table_it != typeinfo_cache_.end() );
	// Take copy of cache element, in order to avoid reference invalidation in possible recursive calls.
	TypeinfoCacheElement cache_element= typeinfo_table_it->second;

	NamesScope& root_namespace= *typeinfo_class_type->members->GetRoot();

	if( source_type.GetFundamentalType() != nullptr ||
		source_type.GetArrayType() != nullptr ||
		source_type.GetRawPointerType() != nullptr )
	{
		// These kinds of types have no lists.
	}
	else if( const EnumPtr enum_type= source_type.GetEnumType() )
	{
		if( name == "elements_list" )
		{
			if( cache_element.elements_list == nullptr )
			{
				cache_element.elements_list= BuildTypeinfoEnumElementsList( enum_type, root_namespace );
				typeinfo_cache_[source_type]= cache_element;
			}
			return cache_element.elements_list;
		}
	}
	else if( const TupleType* const tuple_type= source_type.GetTupleType() )
	{
		if( name == "elements_list" )
		{
			if( cache_element.elements_list == nullptr )
			{
				cache_element.elements_list= BuildTypeinfoTupleElements( *tuple_type, root_namespace );
				typeinfo_cache_[source_type]= cache_element;
			}
			return cache_element.elements_list;
		}
	}
	else if( const ClassPtr class_type= source_type.GetClassType() )
	{
		if( name == "fields_list" )
		{
			if( cache_element.fields_list == nullptr )
			{
				cache_element.fields_list= BuildTypeinfoClassFieldsList( class_type, root_namespace );
				typeinfo_cache_[source_type]= cache_element;
			}
			return cache_element.fields_list;
		}
		if( name == "types_list" )
		{
			if( cache_element.types_list == nullptr )
			{
				cache_element.types_list= BuildTypeinfoClassTypesList( class_type, root_namespace );
				typeinfo_cache_[source_type]= cache_element;
			}
			return cache_element.types_list;
		}
		if( name == "functions_list" )
		{
			if( cache_element.functions_list == nullptr )
			{
				cache_element.functions_list= BuildTypeinfoClassFunctionsList( class_type, root_namespace );
				typeinfo_cache_[source_type]= cache_element;
			}
			return cache_element.functions_list;
		}
		if( name == "parents_list" )
		{
			if( cache_element.parents_list == nullptr )
			{
				cache_element.parents_list= BuildTypeinfoClassParentsList( class_type, root_namespace );
				typeinfo_cache_[source_type]= cache_element;
			}
			return cache_element.parents_list;
		}
	}
	else if( const FunctionPointerType* const function_pointer_type= source_type.GetFunctionPointerType() )
	{
		if( name == "params_list" )
		{
			if( cache_element.params_list == nullptr )
			{
				cache_element.params_list= BuildTypeinfoFunctionParams( function_pointer_type->function_type, root_namespace );
				typeinfo_cache_[source_type]= cache_element;
			}
			return cache_element.params_list;
		}
	}
	else U_ASSERT(false);

	// Unknown member.
	return nullptr;
}

VariablePtr CodeBuilder::CreateTypeinfoListVariable( llvm::SmallVectorImpl<TypeinfoListElement>& list )
{
	std::sort(
		list.begin(), list.end(),
		[]( const TypeinfoListElement& l, const TypeinfoListElement& r )
		{
			return l.name_for_ordering < r.name_for_ordering;
		} );

	TupleType list_type;
	llvm::SmallVector< llvm::Type*, 16 > list_elements_llvm_types;
	llvm::SmallVector< llvm::Constant*, 16 > list_elements_initializers;
	list_type.element_types.reserve( list.size() );
	list_elements_llvm_types.reserve( list.size() );
	list_elements_initializers.reserve( list.size() );

	for( const TypeinfoListElement& list_element : list )
	{
		list_type.element_types.emplace_back( list_element.type );
		list_elements_llvm_types.push_back( list_element.type->llvm_type );
		list_elements_initializers.push_back( list_element.initializer );
	}

	list_type.llvm_type= llvm::StructType::get( llvm_context_, list_elements_llvm_types );
	llvm::Constant* const initializer= llvm::ConstantStruct::get( list_type.llvm_type, list_elements_initializers );

	const VariableMutPtr result=
		Variable::Create(
			std::move(list_type),
			ValueType::ReferenceImut,
			Variable::Location::Pointer,
			"typeinfo_list",
			nullptr,
			initializer );

	result->llvm_value=
		CreateGlobalConstantVariable(
			result->type,
			"", // Save some space - avoid to give LLVM variable a name. This is irrelevant, since this variable has private visibility.
			initializer );

	return result;
}

VariablePtr CodeBuilder::BuildTypeinfoEnumElementsList( const EnumPtr enum_type, NamesScope& root_namespace )
{
	llvm::SmallVector<TypeinfoListElement, 16> list_elements;
	list_elements.reserve( enum_type->element_count );

	enum_type->members.ForEachInThisScope(
		[&]( const std::string_view name, const NamesScopeValue& enum_member )
		{
			llvm::Constant* const enum_member_value= enum_member.value.GetVariable()->constexpr_value;

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
				NamesScopeValue( std::make_shared<ClassField>( node_type, enum_type->underlying_type, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( enum_type->underlying_type.llvm_type );
			fields_initializers.push_back( enum_member_value );

			{
				ArrayType name_type;
				name_type.element_type= FundamentalType( U_FundamentalType::char8_, fundamental_llvm_types_.char8_ );
				name_type.element_count= name.size();
				name_type.llvm_type= llvm::ArrayType::get( name_type.element_type.GetLLVMType(), name_type.element_count );

				node_type_class.members->AddName( g_name_field_name, NamesScopeValue( std::make_shared<ClassField>( node_type, name_type, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
				fields_llvm_types.push_back( name_type.llvm_type );
				fields_initializers.push_back( llvm::ConstantDataArray::getString( llvm_context_, StringViewToStringRef(name), false /* not null terminated */ ) );
			}

			FinishTypeinfoClass( node_type, fields_llvm_types );

			list_elements.push_back(
				TypeinfoListElement{
					std::string(name),
					llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ),
					node_type } );
		} );

	return CreateTypeinfoListVariable( list_elements );
}

void CodeBuilder::CreateTypeinfoClassMembersListNodeCommonFields(
	const Class& class_, const ClassPtr node_class_type,
	const std::string_view member_name,
	ClassFieldsVector<llvm::Type*>& fields_llvm_types, ClassFieldsVector<llvm::Constant*>& fields_initializers )
{
	Class& node_class= *node_class_type;

	{
		ArrayType name_type;
		name_type.element_type= FundamentalType( U_FundamentalType::char8_, fundamental_llvm_types_.char8_ );
		name_type.element_count= member_name.size();
		name_type.llvm_type= llvm::ArrayType::get( name_type.element_type.GetLLVMType(), name_type.element_count );

		node_class.members->AddName(
			g_name_field_name,
			NamesScopeValue( std::make_shared<ClassField>( node_class_type, name_type, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
		fields_llvm_types.push_back( name_type.llvm_type );
		fields_initializers.push_back( llvm::ConstantDataArray::getString( llvm_context_, StringViewToStringRef(member_name), false /* not null terminated */ ) );
	}

	const ClassMemberVisibility member_visibility= class_.GetMemberVisibility( member_name );

	node_class.members->AddName(
		"is_public",
		NamesScopeValue( std::make_shared<ClassField>( node_class_type, bool_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
	fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
	fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, member_visibility == ClassMemberVisibility::Public    ) ) );

	node_class.members->AddName(
		"is_protected",
		NamesScopeValue( std::make_shared<ClassField>( node_class_type, bool_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
	fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
	fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, member_visibility == ClassMemberVisibility::Protected ) ) );

	node_class.members->AddName(
		"is_private",
		NamesScopeValue( std::make_shared<ClassField>( node_class_type, bool_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
	fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
	fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, member_visibility == ClassMemberVisibility::Private   ) ) );
}

VariablePtr CodeBuilder::BuildTypeinfoClassFieldsList( const ClassPtr class_type, NamesScope& root_namespace )
{
	llvm::SmallVector<TypeinfoListElement, 16> list_elements;

	const auto process_class_member=
		[&]( const std::string_view member_name, const NamesScopeValue& class_member )
		{
			const ClassFieldPtr class_field= class_member.value.GetClassField();
			if( class_field == nullptr )
				return;

			std::string node_class_name= g_typeinfo_class_fields_list_node_class_name;
			node_class_name+= member_name;
			const ClassPtr node_type= CreateTypeinfoClass( root_namespace, class_type, std::move(node_class_name) );
			Class& node_type_class= *node_type;

			ClassFieldsVector<llvm::Type*> fields_llvm_types;
			ClassFieldsVector<llvm::Constant*> fields_initializers;

			{
				const VariablePtr field_type_typeinfo= BuildTypeInfo( class_field->type, root_namespace );

				node_type_class.members->AddName( g_type_field_name, NamesScopeValue( std::make_shared<ClassField>( node_type, field_type_typeinfo->type, uint32_t(fields_llvm_types.size()), false, true ), g_dummy_src_loc ) );
				fields_llvm_types.push_back( field_type_typeinfo->type.GetLLVMType()->getPointerTo() );
				fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( field_type_typeinfo->llvm_value ) );
			}
			{
				const VariablePtr fields_class_type_typeinfo= BuildTypeInfo( class_field->class_, root_namespace );

				node_type_class.members->AddName( "class_type", NamesScopeValue( std::make_shared<ClassField>( node_type, fields_class_type_typeinfo->type, uint32_t(fields_llvm_types.size()), false, true ), g_dummy_src_loc ) );
				fields_llvm_types.push_back( fields_class_type_typeinfo->type.GetLLVMType()->getPointerTo() );
				fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( fields_class_type_typeinfo->llvm_value ) );
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
					NamesScopeValue( std::make_shared<ClassField>( node_type, size_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
				fields_llvm_types.push_back( size_type_.GetLLVMType() );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( size_type_.GetLLVMType(), llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), offset ) ) );
			}

			node_type_class.members->AddName(
				"is_reference",
				NamesScopeValue( std::make_shared<ClassField>( node_type, bool_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
			fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, class_field->is_reference ) ) );

			node_type_class.members->AddName(
				"is_mutable",
				NamesScopeValue( std::make_shared<ClassField>( node_type, bool_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
			fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, class_field->is_mutable ) ) );

			CreateTypeinfoClassMembersListNodeCommonFields( *class_type, node_type, member_name, fields_llvm_types, fields_initializers );

			FinishTypeinfoClass( node_type, fields_llvm_types );

			list_elements.push_back(
				TypeinfoListElement{
					std::string(member_name),
					llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ),
					node_type } );
		};

	class_type->members->ForEachInThisScope( process_class_member );

	return CreateTypeinfoListVariable( list_elements );
}

VariablePtr CodeBuilder::BuildTypeinfoClassTypesList( const ClassPtr class_type, NamesScope& root_namespace )
{
	llvm::SmallVector<TypeinfoListElement, 16> list_elements;

	const auto process_class_member=
		[&]( const std::string_view name, NamesScopeValue& class_member )
		{
			if( class_member.value.GetTypeAlias() != nullptr ) // Event in complete class type aliases may be not yet complete. Complete it now.
				GlobalThingBuildTypeAlias( *class_type->members, class_member.value );

			const Type* const class_inner_type= class_member.value.GetTypeName();
			if( class_inner_type == nullptr )
				return;

			std::string node_class_name= g_typeinfo_class_types_list_node_class_name;
			node_class_name+= name;
			const ClassPtr node_type= CreateTypeinfoClass( root_namespace, class_type, std::move(node_class_name) );
			Class& node_type_class= *node_type;

			ClassFieldsVector<llvm::Type*> fields_llvm_types;
			ClassFieldsVector<llvm::Constant*> fields_initializers;

			{
				const VariablePtr dependent_type_typeinfo= BuildTypeInfo( *class_inner_type, root_namespace );

				node_type_class.members->AddName( g_type_field_name, NamesScopeValue( std::make_shared<ClassField>( node_type, dependent_type_typeinfo->type, uint32_t(fields_llvm_types.size()), false, true ), g_dummy_src_loc ) );
				fields_llvm_types.push_back( dependent_type_typeinfo->type.GetLLVMType()->getPointerTo() );
				fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo->llvm_value ) );
			}

			CreateTypeinfoClassMembersListNodeCommonFields( *class_type, node_type, name, fields_llvm_types, fields_initializers );

			FinishTypeinfoClass( node_type, fields_llvm_types );

			list_elements.push_back(
				TypeinfoListElement{
					std::string(name),
					llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ),
					node_type } );
		};

	class_type->members->ForEachInThisScope( process_class_member );

	return CreateTypeinfoListVariable( list_elements );
}

VariablePtr CodeBuilder::BuildTypeinfoClassFunctionsList( const ClassPtr class_type, NamesScope& root_namespace )
{
	llvm::SmallVector<TypeinfoListElement, 16> list_elements;

	const auto process_class_member=
		[&]( const std::string_view name, const NamesScopeValue& class_member )
		{
			const OverloadedFunctionsSetConstPtr functions_set= class_member.value.GetFunctionsSet();
			if( functions_set == nullptr )
				return;

			for( const FunctionVariable& function : functions_set->functions )
			{
				const ClassPtr node_type=
					CreateTypeinfoClass(
						root_namespace,
						class_type,
						g_typeinfo_class_functions_list_node_class_name + std::string(function.llvm_function->name_mangled) ); // Use mangled name for type name.
				Class& node_type_class= *node_type;

				ClassFieldsVector<llvm::Type*> fields_llvm_types;
				ClassFieldsVector<llvm::Constant*> fields_initializers;

				{
					const VariablePtr dependent_type_typeinfo= BuildTypeInfo( FunctionTypeToPointer( function.type ), root_namespace );

					node_type_class.members->AddName( g_type_field_name, NamesScopeValue( std::make_shared<ClassField>( node_type, dependent_type_typeinfo->type, uint32_t(fields_llvm_types.size()), false, true ), g_dummy_src_loc ) );
					fields_llvm_types.push_back( dependent_type_typeinfo->type.GetLLVMType()->getPointerTo() );
					fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo->llvm_value ) );
				}

				node_type_class.members->AddName(
					"is_this_call",
					NamesScopeValue( std::make_shared<ClassField>( node_type, bool_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.is_this_call ) ) );

				node_type_class.members->AddName(
					"is_generated",
					NamesScopeValue( std::make_shared<ClassField>( node_type, bool_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.is_generated ) ) );

				node_type_class.members->AddName(
					"is_deleted",
					NamesScopeValue( std::make_shared<ClassField>( node_type, bool_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.is_deleted ) ) );

				node_type_class.members->AddName(
					"is_virtual",
					NamesScopeValue( std::make_shared<ClassField>( node_type, bool_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.virtual_table_index != ~0u ) ) );

				CreateTypeinfoClassMembersListNodeCommonFields( *class_type, node_type, name, fields_llvm_types, fields_initializers );

				FinishTypeinfoClass( node_type, fields_llvm_types );

				list_elements.push_back(
					TypeinfoListElement{
						function.llvm_function->name_mangled, // Sort, using function mangled name.
						llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ),
						node_type } );
			} // for functions with same name
		};

	class_type->members->ForEachInThisScope( process_class_member );

	return CreateTypeinfoListVariable( list_elements );
}

VariablePtr CodeBuilder::BuildTypeinfoClassParentsList( const ClassPtr class_type, NamesScope& root_namespace )
{
	const Class& class_= *class_type;
	const llvm::StructLayout* const struct_layout= data_layout_.getStructLayout( class_.llvm_type );

	llvm::SmallVector<TypeinfoListElement, 16> list_elements;
	list_elements.reserve( class_.parents.size() );

	for( size_t i= 0u; i < class_.parents.size(); ++i )
	{
		const ClassPtr node_type= CreateTypeinfoClass( root_namespace, class_type, g_typeinfo_class_parents_list_node_class_name + std::to_string(i) );
		Class& node_type_class= *node_type;

		ClassFieldsVector<llvm::Type*> fields_llvm_types;
		ClassFieldsVector<llvm::Constant*> fields_initializers;

		{
			const VariablePtr parent_type_typeinfo= BuildTypeInfo( class_.parents[i].class_, root_namespace );

			node_type_class.members->AddName( g_type_field_name, NamesScopeValue( std::make_shared<ClassField>( node_type, parent_type_typeinfo->type, uint32_t(fields_llvm_types.size()), false, true ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( parent_type_typeinfo->type.GetLLVMType()->getPointerTo() );
			fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( parent_type_typeinfo->llvm_value ) );
		}

		const uint64_t parent_field_offset= struct_layout->getElementOffset( class_.parents[i].field_number );
		node_type_class.members->AddName(
			"offset",
			NamesScopeValue( std::make_shared<ClassField>( node_type, size_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
		fields_llvm_types.push_back( size_type_.GetLLVMType() );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( size_type_.GetLLVMType(), llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), parent_field_offset ) ) );

		FinishTypeinfoClass( node_type, fields_llvm_types );

		list_elements.push_back( TypeinfoListElement{ std::to_string(i), llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ), node_type } );
	} // for parents

	return CreateTypeinfoListVariable( list_elements );
}

VariablePtr CodeBuilder::BuildTypeinfoFunctionParams( const FunctionType& function_type, NamesScope& root_namespace )
{
	llvm::SmallVector<TypeinfoListElement, 16> list_elements;

	const FunctionPointerType function_pointer_type= FunctionTypeToPointer( function_type );

	for( const FunctionType::Param& param : function_type.params )
	{
		const size_t param_index= size_t(&param - function_type.params.data());
		const ClassPtr node_type= CreateTypeinfoClass( root_namespace, function_pointer_type, g_typeinfo_function_params_list_node_class_name + std::to_string(param_index) );
		Class& node_type_class= *node_type;

		ClassFieldsVector<llvm::Type*> fields_llvm_types;
		ClassFieldsVector<llvm::Constant*> fields_initializers;

		{
			const VariablePtr dependent_type_typeinfo= BuildTypeInfo( param.type, root_namespace );

			node_type_class.members->AddName( g_type_field_name, NamesScopeValue( std::make_shared<ClassField>( node_type, dependent_type_typeinfo->type, uint32_t(fields_llvm_types.size()), false, true ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( dependent_type_typeinfo->type.GetLLVMType()->getPointerTo() );
			fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo->llvm_value ) );
		}

		node_type_class.members->AddName(
			"is_reference",
			NamesScopeValue( std::make_shared<ClassField>( node_type, bool_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
		fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, param.value_type != ValueType::Value ) ) );

		node_type_class.members->AddName(
			"is_mutable",
			NamesScopeValue( std::make_shared<ClassField>( node_type, bool_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
		fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, param.value_type == ValueType::ReferenceMut ) ) );

		// SPRACHE_TODO - add reference pollution

		FinishTypeinfoClass( node_type, fields_llvm_types );

		list_elements.push_back( TypeinfoListElement{ std::to_string(param_index), llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ), node_type } );
	}

	return CreateTypeinfoListVariable( list_elements );
}

VariablePtr CodeBuilder::BuildTypeinfoTupleElements( const TupleType& tuple_type, NamesScope& root_namespace )
{
	llvm::SmallVector<TypeinfoListElement, 16> list_elements;
	list_elements.reserve( tuple_type.element_types.size() );

	const llvm::StructLayout* const struct_layout= data_layout_.getStructLayout( tuple_type.llvm_type );

	for( const Type& element_type : tuple_type.element_types )
	{
		const size_t element_index= size_t( &element_type - tuple_type.element_types.data() );

		const ClassPtr node_type= CreateTypeinfoClass( root_namespace, tuple_type, g_typeinfo_tuple_elements_list_node_class_name + std::to_string(element_index) );
		Class& node_type_class= *node_type;

		ClassFieldsVector<llvm::Type*> fields_llvm_types;
		ClassFieldsVector<llvm::Constant*> fields_initializers;

		{
			const VariablePtr dependent_type_typeinfo= BuildTypeInfo( element_type, root_namespace );

			node_type_class.members->AddName( g_type_field_name, NamesScopeValue( std::make_shared<ClassField>( node_type, dependent_type_typeinfo->type, uint32_t(fields_llvm_types.size()), false, true ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( dependent_type_typeinfo->type.GetLLVMType()->getPointerTo() );
			fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo->llvm_value ) );
		}
		{
			node_type_class.members->AddName(
				"index",
				NamesScopeValue( std::make_shared<ClassField>( node_type, size_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( size_type_.GetLLVMType() );
			fields_initializers.push_back( llvm::Constant::getIntegerValue( size_type_.GetLLVMType(), llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), element_index ) ) );
		}
		{
			const auto offset= struct_layout->getElementOffset( uint32_t(element_index) );
			node_type_class.members->AddName(
				"offset",
				NamesScopeValue( std::make_shared<ClassField>( node_type, size_type_, uint32_t(fields_llvm_types.size()), true, false ), g_dummy_src_loc ) );
			fields_llvm_types.push_back( size_type_.GetLLVMType() );
			fields_initializers.push_back( llvm::Constant::getIntegerValue( size_type_.GetLLVMType(), llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), offset ) ) );
		}
		FinishTypeinfoClass( node_type, fields_llvm_types );

		list_elements.push_back( TypeinfoListElement{ std::to_string(element_index), llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ), node_type } );
	}

	return CreateTypeinfoListVariable( list_elements );
}

} // namespace U
