#include "../lex_synt_lib/assert.hpp"
#include "mangling.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

static const ProgramString g_next_node_name= "next"_SpC;
static const ProgramString g_is_end_var_name= "is_end"_SpC;
static const ProgramString g_name_field_name= "name"_SpC;
static const ProgramString g_type_field_name= "type"_SpC;
static const FilePos g_dummy_file_pos{ 0u, 0u, 0u };

static std::string GetTypeinfoVariableName( const ClassProxyPtr& typeinfo_class )
{
	return "_val_of_" + std::string(typeinfo_class->class_->llvm_type->getName());
}

Value CodeBuilder::BuildTypeinfoOperator( const Synt::TypeInfo& typeinfo_op, NamesScope& names, FunctionContext& function_context )
{
	const Type type= PrepareType( *typeinfo_op.type_, names, function_context );
	if( type == invalid_type_ )
		return ErrorValue();

	return Value( BuildTypeInfo( type, *names.GetRoot() ), typeinfo_op.file_pos_ );
}

Variable CodeBuilder::BuildTypeInfo( const Type& type, NamesScope& root_namespace )
{
	// Search in cache.
	for( const auto& cache_value : typeinfo_cache_ )
		if( cache_value.first == type )
			return cache_value.second;

	typeinfo_cache_.push_back( std::make_pair( type, BuildTypeinfoPrototype( type, root_namespace ) ) );
	return typeinfo_cache_.back().second;
}

ClassProxyPtr CodeBuilder::CreateTypeinfoClass( NamesScope& root_namespace )
{
	// Currently, give "random" names for typeinfo classes.
	llvm::StructType* const llvm_type= llvm::StructType::create( llvm_context_ );

	const ProgramString typeinfo_class_name= "_typeinfo_"_SpC + ToProgramString(std::to_string(reinterpret_cast<uintptr_t>(llvm_type)));
	const ClassProxyPtr typeinfo_class_proxy= std::make_shared<ClassProxy>();
	typeinfo_class_table_[typeinfo_class_proxy].reset( new Class( typeinfo_class_name, &root_namespace ) );
	typeinfo_class_proxy->class_= typeinfo_class_table_[typeinfo_class_proxy].get();
	typeinfo_class_proxy->class_->llvm_type= llvm_type;

	llvm_type->setName( ToUTF8( typeinfo_class_name ) );

	typeinfo_class_proxy->class_->references_tags_count= 1u; // Almost all typeinfo have references to another typeinfo.
	typeinfo_class_proxy->class_->completeness=  TypeCompleteness::ReferenceTagsComplete;

	return typeinfo_class_proxy;
}

Variable CodeBuilder::BuildTypeinfoPrototype( const Type& type, NamesScope& root_namespace )
{
	U_UNUSED(type);

	const ClassProxyPtr typeinfo_class_proxy= CreateTypeinfoClass( root_namespace );
	typeinfo_class_proxy->class_->is_typeinfo= true;
	Variable result( typeinfo_class_proxy, Variable::Location::Pointer, ValueType::ConstReference );

	result.constexpr_value= llvm::UndefValue::get( typeinfo_class_proxy->class_->llvm_type ); // Currently uninitialized.
	result.llvm_value=
		CreateGlobalConstantVariable(
			result.type,
			GetTypeinfoVariableName( typeinfo_class_proxy ),
			result.constexpr_value );

	return result;
}

void CodeBuilder::BuildFullTypeinfo( const Type& type, Variable& typeinfo_variable, NamesScope& root_namespace )
{
	if( type != void_type_ && !EnsureTypeCompleteness( type, TypeCompleteness::Complete ) )
	{
		REPORT_ERROR( UsingIncompleteType, root_namespace.GetErrors(), g_dummy_file_pos, type ); // TODO - use correct file_pos
		return;
	}

	U_ASSERT( typeinfo_variable.type.GetClassType() != nullptr );

	const ClassProxyPtr typeinfo_class_proxy= typeinfo_variable.type.GetClassTypeProxy();
	Class& typeinfo_class= *typeinfo_variable.type.GetClassType();

	ClassFieldsVector<llvm::Type*> fields_llvm_types;
	ClassFieldsVector<llvm::Constant*> fields_initializers;

	const auto add_bool_field=
	[&]( const ProgramString& name, const bool value )
	{
		typeinfo_class.members.AddName(
			name,
			Value(ClassField( typeinfo_class_proxy, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
		fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, uint64_t(value) ) ) );
	};

	const auto add_size_field=
	[&]( const ProgramString& name, const SizeType value )
	{
		typeinfo_class.members.AddName(
			name,
			Value( ClassField( typeinfo_class_proxy, size_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
		fields_llvm_types.push_back( size_type_.GetLLVMType() );
		fields_initializers.push_back(
			llvm::Constant::getIntegerValue(
				size_type_.GetLLVMType(),
				llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), value ) ) );
	};

	const auto add_typeinfo_field=
	[&]( const ProgramString& name, const Type& dependent_type )
	{
		const Variable dependent_type_typeinfo= BuildTypeInfo( dependent_type, root_namespace );

		typeinfo_class.members.AddName(
			name,
			Value( ClassField( typeinfo_class_proxy, dependent_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), g_dummy_file_pos ) );
		fields_llvm_types.push_back( llvm::PointerType::get( dependent_type_typeinfo.type.GetLLVMType(), 0u ) );
		fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo.llvm_value ) );
	};

	const auto add_list_head_field=
	[&]( const ProgramString& name, const Variable& variable )
	{
		typeinfo_class.members.AddName(
			name,
			Value( ClassField( typeinfo_class_proxy, variable.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), g_dummy_file_pos ) );
		fields_llvm_types.push_back( llvm::PointerType::get( variable.type.GetLLVMType(), 0u ) );
		fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( variable.llvm_value ) );
	};

	const llvm::DataLayout& data_layout= module_->getDataLayout();
	if( type.GetFunctionType() == nullptr )
	{
		llvm::Type* llvm_type= type.GetLLVMType();
		if( llvm_type == fundamental_llvm_types_.void_for_ret )
			llvm_type= fundamental_llvm_types_.void_;

		add_size_field( "size_of"_SpC , data_layout.getTypeAllocSize   ( llvm_type ) );
		add_size_field( "align_of"_SpC, data_layout.getABITypeAlignment( llvm_type ) ); // TODO - is this correct alignment?
	}

	add_bool_field( "is_fundamental"_SpC     , type.GetFundamentalType()     != nullptr );
	add_bool_field( "is_enum"_SpC            , type.GetEnumType()            != nullptr );
	add_bool_field( "is_array"_SpC           , type.GetArrayType()           != nullptr );
	add_bool_field( "is_class"_SpC           , type.GetClassType()           != nullptr );
	add_bool_field( "is_function_pointer"_SpC, type.GetFunctionPointerType() != nullptr );
	add_bool_field(  "is_function"_SpC       , type.GetFunctionType()        != nullptr );

	add_bool_field( "is_default_constructible"_SpC, type.IsDefaultConstructible() );
	add_bool_field( "is_copy_constructible"_SpC   , type.IsCopyConstructible()    );
	add_bool_field( "is_copy_assignable"_SpC      , type.IsCopyAssignable()       );

	add_size_field( "references_tags_count"_SpC, type.ReferencesTagsCount() );

	if( const FundamentalType* const fundamental_type= type.GetFundamentalType() )
	{
		add_bool_field( "is_integer"_SpC         , IsInteger        ( fundamental_type->fundamental_type ) );
		add_bool_field( "is_numeric"_SpC         , IsNumericType    ( fundamental_type->fundamental_type ) );
		add_bool_field( "is_signed_integer"_SpC  , IsSignedInteger  ( fundamental_type->fundamental_type ) );
		add_bool_field( "is_unsigned_integer"_SpC, IsUnsignedInteger( fundamental_type->fundamental_type ) );
		add_bool_field( "is_float"_SpC           , IsFloatingPoint  ( fundamental_type->fundamental_type ) );
		add_bool_field( "is_char"_SpC            , IsChar           ( fundamental_type->fundamental_type ) );
		add_bool_field( "is_bool"_SpC            , fundamental_type->fundamental_type == U_FundamentalType::Bool );
		add_bool_field( "is_void"_SpC            , fundamental_type->fundamental_type == U_FundamentalType::Void );
	}
	else if( const Enum* const enum_type= type.GetEnumType() )
	{
		add_size_field( "element_count"_SpC, enum_type->element_count );
		add_typeinfo_field( "underlaying_type"_SpC, enum_type->underlaying_type );
		add_list_head_field( "elements_list"_SpC, BuildTypeinfoEnumElementsList( *enum_type, root_namespace ) );
	}
	else if( const Array* const array_type= type.GetArrayType() )
	{
		add_size_field( "element_count"_SpC, array_type->ArraySizeOrZero() );
		add_typeinfo_field( "element_type"_SpC, array_type->type );
	}
	else if( const Class* const class_type= type.GetClassType() )
	{
		U_ASSERT( class_type->completeness == TypeCompleteness::Complete );

		add_size_field( "field_count"_SpC, class_type->field_count );
		add_size_field( "parent_count"_SpC, class_type->parents.size() );

		add_bool_field( "is_struct"_SpC, class_type->kind == Class::Kind::Struct );

		add_bool_field( "is_polymorph"_SpC,
			class_type->kind == Class::Kind::Interface ||
			class_type->kind == Class::Kind::Abstract ||
			class_type->kind == Class::Kind::PolymorphNonFinal ||
			class_type->kind == Class::Kind::PolymorphFinal );

		add_bool_field( "is_final"_SpC,
			class_type->kind == Class::Kind::Struct ||
			class_type->kind == Class::Kind::NonPolymorph ||
			class_type->kind == Class::Kind::PolymorphFinal );

		add_bool_field( "is_abstract"_SpC,
			class_type->kind == Class::Kind::Abstract ||
			class_type->kind == Class::Kind::Interface );

		add_bool_field( "is_interface"_SpC, class_type->kind == Class::Kind::Interface );

		const ClassProxyPtr class_proxy= type.GetClassTypeProxy();
		add_list_head_field( "fields_list"_SpC   , BuildTypeinfoClassFieldsList(    class_proxy, root_namespace ) );
		add_list_head_field( "types_list"_SpC    , BuildTypeinfoClassTypesList(     class_proxy, root_namespace ) );
		add_list_head_field( "functions_list"_SpC, BuildTypeinfoClassFunctionsList( class_proxy, root_namespace ) );
		add_list_head_field( "parents_list"_SpC  , BuildeTypeinfoClassParentsList(  class_proxy, root_namespace ) );

	}
	else if( const FunctionPointer* const function_pointer_type= type.GetFunctionPointerType() )
	{
		add_typeinfo_field( "element_type"_SpC, function_pointer_type->function );
	}
	else if( const Function* const function_type= type.GetFunctionType() )
	{
		add_typeinfo_field( "return_type"_SpC, function_type->return_type );
		add_bool_field( "return_value_is_reference"_SpC, function_type->return_value_is_reference );
		add_bool_field( "return_value_is_mutable"_SpC  , function_type->return_value_is_mutable );
		add_bool_field( "unsafe"_SpC                   , function_type->unsafe );
		add_list_head_field( "arguments_list"_SpC      , BuildTypeinfoFunctionArguments( *function_type, root_namespace ) );
		// SPRACHE_TODO - add also reference pollution.
	}
	else U_ASSERT(false);

	FinishTypeinfoClass( typeinfo_class, typeinfo_class_proxy, fields_llvm_types );

	// Prepare result value
	typeinfo_variable.constexpr_value= llvm::ConstantStruct::get( typeinfo_class.llvm_type, fields_initializers );
	llvm::dyn_cast<llvm::GlobalVariable>(typeinfo_variable.llvm_value)->setInitializer( typeinfo_variable.constexpr_value );
}

const Variable& CodeBuilder::GetTypeinfoListEndNode( NamesScope& root_namespace )
{
	if (typeinfo_list_end_node_ != boost::none )
		return *typeinfo_list_end_node_;

	const ClassProxyPtr node_type= CreateTypeinfoClass( root_namespace );
	Class& node_type_class= *node_type->class_;

	ClassFieldsVector<llvm::Type*> fields_llvm_types;
	ClassFieldsVector<llvm::Constant*> fields_initializers;

	AddTypeinfoNodeIsEndVariable( node_type_class, true );

	node_type_class.members.AddName(
		g_next_node_name,
		Value( ClassField( node_type, node_type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), g_dummy_file_pos ) );
	fields_llvm_types.push_back( llvm::PointerType::get( node_type_class.llvm_type, 0u ) );
	fields_initializers.push_back( nullptr );

	FinishTypeinfoClass( node_type_class, node_type, fields_llvm_types );

	llvm::GlobalVariable* const global_variable=
		CreateGlobalConstantVariable(
			node_type,
			GetTypeinfoVariableName( node_type ),
			nullptr );

	// Save self-reference.
	fields_initializers[0u]= global_variable;
	global_variable->setInitializer( llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );

	typeinfo_list_end_node_= Variable( node_type, Variable::Location::Pointer, ValueType::ConstReference, global_variable, global_variable->getInitializer() );
	return *typeinfo_list_end_node_;
}

void CodeBuilder::AddTypeinfoNodeIsEndVariable( Class& node_class, const bool is_end )
{
	// Reuse llvm global variable for "is_end" variables.
	// TODO - maybe reuse all scalar constants?

	const unsigned int i= is_end ? 1u : 0u;
	if( typeinfo_is_end_variable_[i] == nullptr )
		typeinfo_is_end_variable_[i]=
			CreateGlobalConstantVariable(
				bool_type_,
				MangleGlobalVariable( node_class.members, g_is_end_var_name ),
				llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, i ) ) );

	node_class.members.AddName(
		g_is_end_var_name,
		Value(
			Variable(
				bool_type_,
				Variable::Location::Pointer, ValueType::ConstReference,
				typeinfo_is_end_variable_[i], typeinfo_is_end_variable_[i]->getInitializer() ),
			FilePos() ) );
}

void CodeBuilder::FinishTypeinfoClass( Class& class_, const ClassProxyPtr class_proxy, const ClassFieldsVector<llvm::Type*>& fields_llvm_types )
{
	class_.llvm_type->setBody( fields_llvm_types );
	class_.kind= Class::Kind::Struct;
	class_.completeness= TypeCompleteness::Complete;
	class_.can_be_constexpr= true;

	// Generate only destructor, because almost all structs and classes must have it.
	// Other methods - constructors, assignment operators does not needs for typeinfo classes.
	TryGenerateDestructor( class_, class_proxy );
}

Variable CodeBuilder::BuildTypeinfoEnumElementsList( const Enum& enum_type, NamesScope& root_namespace )
{
	Variable head= GetTypeinfoListEndNode( root_namespace );

	enum_type.members.ForEachInThisScope(
		[&]( const ProgramString& name, const Value& enum_member )
		{
			const ClassProxyPtr node_type= CreateTypeinfoClass( root_namespace );
			Class& node_type_class= *node_type->class_;

			ClassFieldsVector<llvm::Type*> fields_llvm_types;
			ClassFieldsVector<llvm::Constant*> fields_initializers;

			AddTypeinfoNodeIsEndVariable( node_type_class );

			node_type_class.members.AddName(
				g_next_node_name,
				Value( ClassField( node_type, head.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), g_dummy_file_pos ) );
			fields_llvm_types.push_back( llvm::PointerType::get( head.type.GetLLVMType(), 0u ) );
			fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>(head.llvm_value) );

			node_type_class.members.AddName(
				"value"_SpC,
				Value( ClassField( node_type, enum_type.underlaying_type, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
			fields_llvm_types.push_back( enum_type.underlaying_type.llvm_type );
			fields_initializers.push_back( enum_member.GetVariable()->constexpr_value );

			{
				const std::string name_str= ToUTF8( name );
				Array name_type;
				name_type.type= FundamentalType( U_FundamentalType::char8, fundamental_llvm_types_.char8 );
				name_type.size= name_str.size();
				name_type.llvm_type= llvm::ArrayType::get( name_type.type.GetLLVMType(), name_type.size );

				ClassField field( node_type, name_type, static_cast<unsigned int>(fields_llvm_types.size()), true, false );

				node_type_class.members.AddName( g_name_field_name, Value( std::move(field), g_dummy_file_pos ) );
				fields_llvm_types.push_back( name_type.llvm_type );
				fields_initializers.push_back( llvm::ConstantDataArray::getString( llvm_context_, name_str, false /* not null terminated */ ) );
			}

			FinishTypeinfoClass( node_type_class, node_type, fields_llvm_types );

			llvm::GlobalVariable* const global_variable=
				CreateGlobalConstantVariable(
					node_type,
					GetTypeinfoVariableName( node_type ),
					llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );

			head= Variable( node_type, Variable::Location::Pointer, ValueType::ConstReference, global_variable, global_variable->getInitializer() );
		}); // for enum elements

	return head;
}

void CodeBuilder::CreateTypeinfoClassMembersListNodeCommonFields(
	const Class& class_, const ClassProxyPtr& node_class_proxy,
	const ProgramString& member_name,
	ClassFieldsVector<llvm::Type*>& fields_llvm_types, ClassFieldsVector<llvm::Constant*>& fields_initializers )
{
	Class& node_class= *node_class_proxy->class_;

	{
		const std::string name_str= ToUTF8( member_name );
		Array name_type;
		name_type.type= FundamentalType( U_FundamentalType::char8, fundamental_llvm_types_.char8 );
		name_type.size= name_str.size();
		name_type.llvm_type= llvm::ArrayType::get( name_type.type.GetLLVMType(), name_type.size );

		node_class.members.AddName(
			g_name_field_name,
			Value( ClassField( node_class_proxy, name_type, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
		fields_llvm_types.push_back( name_type.llvm_type );
		fields_initializers.push_back( llvm::ConstantDataArray::getString( llvm_context_, name_str, false /* not null terminated */ ) );
	}

	const ClassMemberVisibility member_visibility= class_.GetMemberVisibility( member_name );

	node_class.members.AddName(
		"is_public"_SpC,
		Value( ClassField( node_class_proxy, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
	fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
	fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, member_visibility == ClassMemberVisibility::Public    ) ) );

	node_class.members.AddName(
		"is_protected"_SpC,
		Value( ClassField( node_class_proxy, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
	fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
	fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, member_visibility == ClassMemberVisibility::Protected ) ) );

	node_class.members.AddName(
		"is_private"_SpC,
		Value( ClassField( node_class_proxy, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
	fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
	fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, member_visibility == ClassMemberVisibility::Private   ) ) );
}

Variable CodeBuilder::BuildTypeinfoClassFieldsList( const ClassProxyPtr& class_type, NamesScope& root_namespace )
{
	Variable head= GetTypeinfoListEndNode( root_namespace );

	const llvm::DataLayout& data_layout= module_->getDataLayout();

	class_type->class_->members.ForEachValueInThisScope(
		[&]( const Value& class_member )
		{
			const ClassField* const class_field= class_member.GetClassField();
			if( class_field == nullptr )
				return;

			const ClassProxyPtr node_type= CreateTypeinfoClass( root_namespace );
			Class& node_type_class= *node_type->class_;

			ClassFieldsVector<llvm::Type*> fields_llvm_types;
			ClassFieldsVector<llvm::Constant*> fields_initializers;

			AddTypeinfoNodeIsEndVariable( node_type_class );

			node_type_class.members.AddName(
				g_next_node_name,
				Value( ClassField( node_type, head.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), g_dummy_file_pos ) );
			fields_llvm_types.push_back( llvm::PointerType::get( head.type.GetLLVMType(), 0u ) );
			fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>(head.llvm_value) );

			{
				const Variable field_type_typeinfo= BuildTypeInfo( class_field->type, root_namespace );
				ClassField field( node_type, field_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

				node_type_class.members.AddName( g_type_field_name, Value( std::move(field), g_dummy_file_pos ) );
				fields_llvm_types.push_back( llvm::PointerType::get( field_type_typeinfo.type.GetLLVMType(), 0u ) );
				fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( field_type_typeinfo.llvm_value ) );
			}
			{
				const Variable fields_class_type_typeinfo= BuildTypeInfo( class_field->class_.lock(), root_namespace );
				ClassField field( node_type, fields_class_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

				node_type_class.members.AddName( "class_type"_SpC, Value( std::move(field), g_dummy_file_pos ) );
				fields_llvm_types.push_back( llvm::PointerType::get( fields_class_type_typeinfo.type.GetLLVMType(), 0u ) );
				fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( fields_class_type_typeinfo.llvm_value ) );
			}
			{
				// For ancestor fields accumulate offset.
				uint64_t offset= 0u;
				ClassProxyPtr class_for_field_search= class_type;
				while(true)
				{
					if( class_for_field_search == class_field->class_.lock() )
					{
						offset+= data_layout.getStructLayout( class_for_field_search->class_->llvm_type )->getElementOffset( class_field->index );
						break;
					}
					else
					{
						U_ASSERT( class_for_field_search->class_->base_class != nullptr );
						offset+= data_layout.getStructLayout( class_for_field_search->class_->llvm_type )->getElementOffset( class_for_field_search->class_->base_class_field_number );
						class_for_field_search= class_for_field_search->class_->base_class;
					}
				}

				node_type_class.members.AddName(
					"offset"_SpC,
					Value( ClassField( node_type, size_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
				fields_llvm_types.push_back( size_type_.GetLLVMType() );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( size_type_.GetLLVMType(), llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), offset ) ) );
			}

			node_type_class.members.AddName(
				"is_reference"_SpC,
				Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
			fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
			fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, class_field->is_reference ) ) );

			node_type_class.members.AddName(
				"is_mutable"_SpC,
				Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
			fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
			fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, class_field->is_mutable ) ) );

			CreateTypeinfoClassMembersListNodeCommonFields( *class_type->class_, node_type, class_field->syntax_element->name, fields_llvm_types, fields_initializers );

			FinishTypeinfoClass( node_type_class, node_type, fields_llvm_types );

			llvm::GlobalVariable* const global_variable=
				CreateGlobalConstantVariable(
					node_type,
					GetTypeinfoVariableName( node_type ),
					llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );

			head= Variable( node_type, Variable::Location::Pointer, ValueType::ConstReference, global_variable, global_variable->getInitializer() );
		} ); // for class elements

	return head;
}

Variable CodeBuilder::BuildTypeinfoClassTypesList( const ClassProxyPtr& class_type, NamesScope& root_namespace )
{
	Variable head= GetTypeinfoListEndNode( root_namespace );

	class_type->class_->members.ForEachInThisScope(
		[&]( const ProgramString& name, Value& class_member )
		{
			if( class_member.GetTypedef() != nullptr ) // Event in complete class typedefs may be not yet complete. Complete it now.
				GlobalThingBuildTypedef( class_type->class_->members, class_member );

			const Type* const class_inner_type= class_member.GetTypeName();
			if( class_inner_type == nullptr )
				return;

			const ClassProxyPtr node_type= CreateTypeinfoClass( root_namespace );
			Class& node_type_class= *node_type->class_;

			ClassFieldsVector<llvm::Type*> fields_llvm_types;
			ClassFieldsVector<llvm::Constant*> fields_initializers;

			AddTypeinfoNodeIsEndVariable( node_type_class );

			node_type_class.members.AddName(
				g_next_node_name,
				Value( ClassField( node_type, head.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), g_dummy_file_pos ) );
			fields_llvm_types.push_back( llvm::PointerType::get( head.type.GetLLVMType(), 0u ) );
			fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>(head.llvm_value) );

			{
				const Variable dependent_type_typeinfo= BuildTypeInfo( *class_inner_type, root_namespace );
				ClassField field( node_type, dependent_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

				node_type_class.members.AddName( g_type_field_name, Value( std::move(field), g_dummy_file_pos ) );
				fields_llvm_types.push_back( llvm::PointerType::get( dependent_type_typeinfo.type.GetLLVMType(), 0u ) );
				fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo.llvm_value ) );
			}

			CreateTypeinfoClassMembersListNodeCommonFields( *class_type->class_, node_type, name, fields_llvm_types, fields_initializers );

			FinishTypeinfoClass( node_type_class, node_type, fields_llvm_types );

			llvm::GlobalVariable* const global_variable=
				CreateGlobalConstantVariable(
					node_type,
					GetTypeinfoVariableName( node_type ),
					llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );

			head= Variable( node_type, Variable::Location::Pointer, ValueType::ConstReference, global_variable, global_variable->getInitializer() );
		} ); // for class elements

	return head;
}

Variable CodeBuilder::BuildTypeinfoClassFunctionsList( const ClassProxyPtr& class_type, NamesScope& root_namespace )
{
	Variable head= GetTypeinfoListEndNode( root_namespace );

	class_type->class_->members.ForEachInThisScope(
		[&]( const ProgramString& name, const Value& class_member )
		{
			const OverloadedFunctionsSet* const functions_set= class_member.GetFunctionsSet();
			if( functions_set == nullptr )
				return;
			for( const FunctionVariable& function : functions_set->functions )
			{
				const ClassProxyPtr node_type= CreateTypeinfoClass( root_namespace );
				Class& node_type_class= *node_type->class_;

				ClassFieldsVector<llvm::Type*> fields_llvm_types;
				ClassFieldsVector<llvm::Constant*> fields_initializers;

				AddTypeinfoNodeIsEndVariable( node_type_class );

				node_type_class.members.AddName(
					g_next_node_name,
					Value( ClassField( node_type, head.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), g_dummy_file_pos ) );
				fields_llvm_types.push_back( llvm::PointerType::get( head.type.GetLLVMType(), 0u ) );
				fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>(head.llvm_value) );

				{
					const Variable dependent_type_typeinfo= BuildTypeInfo( function.type, root_namespace );
					ClassField field( node_type, dependent_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

					node_type_class.members.AddName( g_type_field_name, Value( std::move(field), g_dummy_file_pos ) );
					fields_llvm_types.push_back( llvm::PointerType::get( dependent_type_typeinfo.type.GetLLVMType(), 0u ) );
					fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo.llvm_value ) );
				}

				node_type_class.members.AddName(
					"is_this_call"_SpC,
					Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.is_this_call ) ) );

				node_type_class.members.AddName(
					"is_generated"_SpC,
					Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.is_generated ) ) );

				node_type_class.members.AddName(
					"is_deleted"_SpC,
					Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.is_deleted ) ) );

				node_type_class.members.AddName(
					"is_virtual"_SpC,
					Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.virtual_table_index != ~0u ) ) );

				CreateTypeinfoClassMembersListNodeCommonFields( *class_type->class_, node_type, name, fields_llvm_types, fields_initializers );

				FinishTypeinfoClass( node_type_class, node_type, fields_llvm_types );

				llvm::GlobalVariable* const global_variable=
					CreateGlobalConstantVariable(
						node_type,
						GetTypeinfoVariableName( node_type ),
						llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );

				head= Variable( node_type, Variable::Location::Pointer, ValueType::ConstReference, global_variable, global_variable->getInitializer() );
			} // for functions
		} ); // for class elements

	return head;
}

Variable CodeBuilder::BuildeTypeinfoClassParentsList( const ClassProxyPtr& class_type, NamesScope& root_namespace )
{
	Variable head= GetTypeinfoListEndNode( root_namespace );

	const Class& class_= *class_type->class_;
	const llvm::StructLayout* const struct_layout= module_->getDataLayout().getStructLayout( class_.llvm_type );

	U_ASSERT( class_.parents.size() == class_.parents_fields_numbers.size() );
	for( size_t i= 0u; i < class_.parents.size(); ++i )
	{
		const ClassProxyPtr node_type= CreateTypeinfoClass( root_namespace );
		Class& node_type_class= *node_type->class_;

		ClassFieldsVector<llvm::Type*> fields_llvm_types;
		ClassFieldsVector<llvm::Constant*> fields_initializers;

		AddTypeinfoNodeIsEndVariable( node_type_class );

		node_type_class.members.AddName(
			g_next_node_name,
			Value( ClassField( node_type, head.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), g_dummy_file_pos ) );
		fields_llvm_types.push_back( llvm::PointerType::get( head.type.GetLLVMType(), 0u ) );
		fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>(head.llvm_value) );

		{
			const Variable parent_type_typeinfo= BuildTypeInfo( class_.parents[i], root_namespace );
			ClassField field( node_type, parent_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

			node_type_class.members.AddName( g_type_field_name, Value( std::move(field), g_dummy_file_pos ) );
			fields_llvm_types.push_back( llvm::PointerType::get( parent_type_typeinfo.type.GetLLVMType(), 0u ) );
			fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( parent_type_typeinfo.llvm_value ) );
		}

		const uint64_t parent_field_offset= struct_layout->getElementOffset( class_.parents_fields_numbers[i] );
		node_type_class.members.AddName(
			"offset"_SpC,
			Value( ClassField( node_type, size_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
		fields_llvm_types.push_back( size_type_.GetLLVMType() );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( size_type_.GetLLVMType(), llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), parent_field_offset ) ) );

		FinishTypeinfoClass( node_type_class, node_type, fields_llvm_types );

		llvm::GlobalVariable* const global_variable=
			CreateGlobalConstantVariable(
				node_type,
				GetTypeinfoVariableName( node_type ),
				llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );

		head= Variable( node_type, Variable::Location::Pointer, ValueType::ConstReference, global_variable, global_variable->getInitializer() );
	} // for parents

	return head;
}

Variable CodeBuilder::BuildTypeinfoFunctionArguments( const Function& function_type, NamesScope& root_namespace )
{
	Variable head= GetTypeinfoListEndNode( root_namespace );
	for( const Function::Arg& arg : function_type.args )
	{
		const ClassProxyPtr node_type= CreateTypeinfoClass( root_namespace );
		Class& node_type_class= *node_type->class_;

		ClassFieldsVector<llvm::Type*> fields_llvm_types;
		ClassFieldsVector<llvm::Constant*> fields_initializers;

		AddTypeinfoNodeIsEndVariable( node_type_class );

		node_type_class.members.AddName(
			g_next_node_name,
			Value( ClassField( node_type, head.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), g_dummy_file_pos ) );
		fields_llvm_types.push_back( llvm::PointerType::get( head.type.GetLLVMType(), 0u ) );
		fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>(head.llvm_value) );

		{
			const Variable dependent_type_typeinfo= BuildTypeInfo( arg.type, root_namespace );
			ClassField field( node_type, dependent_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

			node_type_class.members.AddName( g_type_field_name, Value( std::move(field), g_dummy_file_pos ) );
			fields_llvm_types.push_back( llvm::PointerType::get( dependent_type_typeinfo.type.GetLLVMType(), 0u ) );
			fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo.llvm_value ) );
		}

		node_type_class.members.AddName(
			"is_reference"_SpC,
			Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
		fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, arg.is_reference ) ) );

		node_type_class.members.AddName(
			"is_mutable"_SpC,
			Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), g_dummy_file_pos ) );
		fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, arg.is_mutable ) ) );

		// SPRACHE_TODO - add reference pollution

		FinishTypeinfoClass( node_type_class, node_type, fields_llvm_types );

		llvm::GlobalVariable* const global_variable=
			CreateGlobalConstantVariable(
				node_type,
				GetTypeinfoVariableName( node_type ),
				llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );

		head= Variable( node_type, Variable::Location::Pointer, ValueType::ConstReference, global_variable, global_variable->getInitializer() );
	}

	return head;
}

} // namespace CodeBuilderPrivate

} // namespace U
