#include "assert.hpp"
#include "mangling.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

static const ProgramString g_next_node_name= "next"_SpC;
static const ProgramString g_is_end_var_name= "is_end"_SpC;
static const ProgramString g_name_field_name= "name"_SpC;


static bool TypeIsClassOrArrayOfClasses( const ClassProxyPtr& class_type, const Type& type )
{
	if( class_type == type )
		return true;
	if( const Array* const array_type= type.GetArrayType() )
		return TypeIsClassOrArrayOfClasses( class_type, array_type->type );

	return false;
}

Value CodeBuilder::BuildTypeinfoOperator( const Synt::TypeInfo& typeinfo_op, NamesScope& names )
{
	const Type type= PrepareType( typeinfo_op.type_, names );
	if( type == NontypeStub::ErrorValue || type == invalid_type_ )
		return ErrorValue();

	return Value( BuildTypeInfo( type, *names.GetRoot() ), typeinfo_op.file_pos_ );
}

Variable CodeBuilder::BuildTypeInfo( const Type& type, const NamesScope& root_namespace )
{
	// Search in cache.
	for( const auto& cache_value : typeinfo_cache_ )
		if( cache_value.first == type )
			return cache_value.second;

	typeinfo_cache_.push_back( std::make_pair( type, BuildTypeinfoPrototype( type, root_namespace ) ) );

	Variable result= typeinfo_cache_.back().second;

	if( !type.IsIncomplete() || type == void_type_ )
	{
		const size_t index= typeinfo_cache_.size() - 1u;
		BuildFullTypeinfo( type, result, root_namespace );
		typeinfo_cache_[index].second= result;
	}

	return result;
}

Variable CodeBuilder::BuildTypeinfoPrototype( const Type& type, const NamesScope& root_namespace )
{
	const ProgramString typeinfo_class_name= "_typeinfo_for_"_SpC + type.ToString();
	const auto typeinfo_class_proxy= std::make_shared<ClassProxy>( new Class( typeinfo_class_name, &root_namespace ) );
	Class& typeinfo_class= *typeinfo_class_proxy->class_;

	typeinfo_class.llvm_type= llvm::StructType::create( llvm_context_, MangleType( typeinfo_class_proxy ) );

	Variable result( typeinfo_class_proxy, Variable::Location::Pointer, ValueType::ConstReference );

	result.constexpr_value= llvm::UndefValue::get( typeinfo_class.llvm_type ); // Currently uninitialized.
	result.llvm_value=
		CreateGlobalConstantVariable(
			result.type,
			MangleGlobalVariable( root_namespace, "_val_of_"_SpC + typeinfo_class_name ),
			result.constexpr_value );

	return result;
}

void CodeBuilder::BuildFullTypeinfo( const Type& type, Variable& typeinfo_variable, const NamesScope& root_namespace )
{
	U_ASSERT( !type.IsIncomplete() || type == void_type_ );
	U_ASSERT( typeinfo_variable.type.GetClassType() != nullptr );

	const ClassProxyPtr typeinfo_class_proxy= typeinfo_variable.type.GetClassTypeProxy();
	Class& typeinfo_class= *typeinfo_variable.type.GetClassType();
	const FilePos& file_pos= typeinfo_class.forward_declaration_file_pos;

	std::vector<llvm::Type*> fields_llvm_types;
	std::vector<llvm::Constant*> fields_initializers;

	const auto add_bool_field=
	[&]( const ProgramString& name, const bool value )
	{
		ClassField field( typeinfo_class_proxy, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false );

		typeinfo_class.members.AddName( name, Value( std::move(field), file_pos ) );
		fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( field.type.GetLLVMType(), llvm::APInt( 1u, uint64_t(value) ) ) );
	};

	const auto add_size_field=
	[&]( const ProgramString& name, const SizeType value )
	{
		ClassField field( typeinfo_class_proxy, size_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false );

		typeinfo_class.members.AddName( name, Value( std::move(field), file_pos ) );
		fields_llvm_types.push_back( field.type.GetLLVMType() );
		fields_initializers.push_back(
			llvm::Constant::getIntegerValue(
				size_type_.GetLLVMType(),
				llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), value ) ) );
	};

	const auto add_typeinfo_field=
	[&]( const ProgramString& name, const Type& dependent_type )
	{
		const Variable dependent_type_typeinfo= BuildTypeInfo( dependent_type, root_namespace );
		ClassField field( typeinfo_class_proxy, dependent_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

		typeinfo_class.members.AddName( name, Value( std::move(field), file_pos ) );
		fields_llvm_types.push_back( llvm::PointerType::get( dependent_type_typeinfo.type.GetLLVMType(), 0u ) );
		fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo.llvm_value ) );
	};

	const auto add_list_head_field=
	[&]( const ProgramString& name, const Variable& variable )
	{
		ClassField field( typeinfo_class_proxy, variable.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

		typeinfo_class.members.AddName( name, Value( std::move(field), file_pos ) );
		fields_llvm_types.push_back( llvm::PointerType::get( variable.type.GetLLVMType(), 0u ) );
		fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( variable.llvm_value ) );
	};

	const llvm::DataLayout& data_layout= module_->getDataLayout();
	if( type.GetFunctionType() == nullptr )
	{
		llvm::Type* llvm_type= type.GetLLVMType();
		if( llvm_type == fundamental_llvm_types_.void_for_ret_ )
			llvm_type= fundamental_llvm_types_.void_;

		add_size_field(  "size_of"_SpC, data_layout.getTypeAllocSize( llvm_type ) );
		add_size_field( "align_of"_SpC, data_layout.getABITypeAlignment( llvm_type ) ); // TODO - is this correct alignment?
	}

	add_bool_field(      "is_fundamental"_SpC, type.GetFundamentalType()     != nullptr );
	add_bool_field(             "is_enum"_SpC, type.GetEnumType()            != nullptr );
	add_bool_field(            "is_array"_SpC, type.GetArrayType()           != nullptr );
	add_bool_field(            "is_class"_SpC, type.GetClassType()           != nullptr );
	add_bool_field( "is_function_pointer"_SpC, type.GetFunctionPointerType() != nullptr );
	add_bool_field(         "is_function"_SpC, type.GetFunctionType()        != nullptr );

	add_bool_field( "is_default_constructible"_SpC, type.IsDefaultConstructible() );
	add_bool_field(    "is_copy_constructible"_SpC, type.IsCopyConstructible()    );
	add_bool_field(       "is_copy_assignable"_SpC, type.IsCopyAssignable()       );

	if( const FundamentalType* const fundamental_type= type.GetFundamentalType() )
	{
		add_bool_field( "is_integer"_SpC         , IsInteger        ( fundamental_type->fundamental_type ) );
		add_bool_field( "is_numeric"_SpC         , IsNumericType    ( fundamental_type->fundamental_type ) );
		add_bool_field( "is_signed_integer"_SpC  , IsSignedInteger  ( fundamental_type->fundamental_type ) );
		add_bool_field( "is_unsigned_integer"_SpC, IsUnsignedInteger( fundamental_type->fundamental_type ) );
		add_bool_field( "is_float"_SpC           , IsFloatingPoint  ( fundamental_type->fundamental_type ) );
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
		U_ASSERT( class_type->completeness == Class::Completeness::Complete );

		add_size_field( "field_count"_SpC, class_type->field_count );

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
		add_list_head_field( "fields_list"_SpC, BuildTypeinfoClassFieldsList( class_proxy, root_namespace ) );
		add_list_head_field( "types_list"_SpC, BuildTypeinfoClassTypesList( class_proxy, root_namespace ) );
		add_list_head_field( "functions_list"_SpC, BuildTypeinfoClassFunctionsList( class_proxy, root_namespace ) );
	}
	else if( const FunctionPointer* const function_pointer_type= type.GetFunctionPointerType() )
	{
		add_typeinfo_field( "element_type"_SpC, function_pointer_type->function );
	}
	else if( const Function* const function_type= type.GetFunctionType() )
	{
		add_typeinfo_field( "return_type"_SpC, function_type->return_type );
		add_bool_field( "return_value_is_reference"_SpC, function_type->return_value_is_reference );
		add_bool_field( "return_value_is_mutable"_SpC, function_type->return_value_is_mutable );
		add_bool_field( "unsafe"_SpC, function_type->unsafe );
		add_list_head_field( "arguments_list"_SpC, BuildTypeinfoFunctionArguments( *function_type, root_namespace ) );
		// SPRACHE_TODO - add also reference pollution.
	}
	else U_ASSERT(false);

	FinishTypeinfoClass( typeinfo_class, typeinfo_class_proxy, fields_llvm_types );

	// Prepare result value
	typeinfo_variable.constexpr_value= llvm::ConstantStruct::get( typeinfo_class.llvm_type, fields_initializers );
	llvm::dyn_cast<llvm::GlobalVariable>(typeinfo_variable.llvm_value)->setInitializer( typeinfo_variable.constexpr_value );

	UpdateTypeinfoForDependentTypes( typeinfo_class_proxy );
}

const Variable& CodeBuilder::GetTypeinfoListEndNode( const NamesScope& root_namespace )
{
	if (typeinfo_list_end_node_ != boost::none )
		return *typeinfo_list_end_node_;

	const ProgramString node_class_name= "_end_node"_SpC;
	const ClassProxyPtr node_type= std::make_shared<ClassProxy>( new Class( node_class_name, &root_namespace ) );
	Class& node_type_class= *node_type->class_;
	node_type_class.llvm_type= llvm::StructType::create( llvm_context_, MangleType( node_type ) );

	const FilePos file_pos{ 0u, 0u, 0u };

	std::vector<llvm::Type*> fields_llvm_types;
	std::vector<llvm::Constant*> fields_initializers;

	AddTypeinfoNodeIsEndVariable( node_type_class, true );

	node_type_class.members.AddName(
		g_next_node_name,
		Value( ClassField( node_type, node_type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), file_pos ) );
	fields_llvm_types.push_back( llvm::PointerType::get( node_type_class.llvm_type, 0u ) );
	fields_initializers.push_back( nullptr );

	FinishTypeinfoClass( node_type_class, node_type, fields_llvm_types );

	llvm::GlobalVariable* const global_variable=
		CreateGlobalConstantVariable(
			node_type,
			MangleGlobalVariable( root_namespace, "_val_of_"_SpC + node_class_name ),
			nullptr );

	// Save self-reference.
	fields_initializers[0u]= global_variable;
	global_variable->setInitializer( llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );

	typeinfo_list_end_node_= Variable( node_type, Variable::Location::Pointer, ValueType::ConstReference, global_variable, global_variable->getInitializer() );
	return *typeinfo_list_end_node_;
}

void CodeBuilder::AddTypeinfoNodeIsEndVariable( Class& node_class, const bool is_end )
{
	Variable var( bool_type_, Variable::Location::Pointer, ValueType::ConstReference );

	var.constexpr_value= llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, is_end ) );
	var.llvm_value=
		CreateGlobalConstantVariable(
			var.type,
			MangleGlobalVariable( node_class.members, g_is_end_var_name ),
			var.constexpr_value );

	node_class.members.AddName( g_is_end_var_name, Value( std::move(var), FilePos() ) );
}

void CodeBuilder::FinishTypeinfoClass( Class& class_, const ClassProxyPtr class_proxy, const std::vector<llvm::Type*>& fields_llvm_types )
{
	class_.llvm_type->setBody( fields_llvm_types );
	class_.kind= Class::Kind::Struct;
	class_.completeness= Class::Completeness::Complete;
	class_.can_be_constexpr= true;

	// Generate only destructor, because almost all structs and classes must have it.
	// Other methods - constructors, assignment operators does not needs for typeinfo classes.
	TryGenerateDestructor( class_, class_proxy );
}

Variable CodeBuilder::BuildTypeinfoEnumElementsList( const Enum& enum_type, const NamesScope& root_namespace )
{
	const FilePos file_pos{ 0u, 0u, 0u };

	Variable head= GetTypeinfoListEndNode( root_namespace );

	enum_type.members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& enum_member )
		{
			const Variable& var= *enum_member.second.GetVariable();

			const ProgramString node_class_name= "_node_"_SpC + enum_member.first + "_of_"_SpC + enum_type.members.GetThisNamespaceName();
			const ClassProxyPtr node_type= std::make_shared<ClassProxy>( new Class( node_class_name, &root_namespace ) );
			Class& node_type_class= *node_type->class_;
			node_type_class.llvm_type= llvm::StructType::create( llvm_context_, MangleType( node_type ) );

			std::vector<llvm::Type*> fields_llvm_types;
			std::vector<llvm::Constant*> fields_initializers;

			// TODO - maybe reorder fields for better result struct layout?
			AddTypeinfoNodeIsEndVariable( node_type_class );

			node_type_class.members.AddName(
				g_next_node_name,
				Value( ClassField( node_type, head.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), file_pos ) );
			fields_llvm_types.push_back( llvm::PointerType::get( head.type.GetLLVMType(), 0u ) );
			fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>(head.llvm_value) );

			node_type_class.members.AddName(
				"value"_SpC,
				Value( ClassField( node_type, enum_type.underlaying_type, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), file_pos ) );
			fields_llvm_types.push_back( enum_type.underlaying_type.llvm_type );
			fields_initializers.push_back( var.constexpr_value );

			{
				const std::string name_str= ToUTF8( enum_member.first );
				Array name_type;
				name_type.type= FundamentalType( U_FundamentalType::char8, fundamental_llvm_types_.char8 );
				name_type.size= name_str.size();
				name_type.llvm_type= llvm::ArrayType::get( name_type.type.GetLLVMType(), name_type.size );

				ClassField field( node_type, name_type, static_cast<unsigned int>(fields_llvm_types.size()), true, false );

				node_type_class.members.AddName( g_name_field_name, Value( std::move(field), file_pos ) );
				fields_llvm_types.push_back( name_type.llvm_type );
				fields_initializers.push_back( llvm::ConstantDataArray::getString( llvm_context_, name_str, false /* not null terminated */ ) );
			}

			FinishTypeinfoClass( node_type_class, node_type, fields_llvm_types );

			llvm::GlobalVariable* const global_variable=
				CreateGlobalConstantVariable(
					node_type,
					MangleGlobalVariable( root_namespace, "_val_of_"_SpC + node_class_name ),
					llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );

			head= Variable( node_type, Variable::Location::Pointer, ValueType::ConstReference, global_variable, global_variable->getInitializer() );
		}); // for enum elements

	return head;
}

Variable CodeBuilder::BuildTypeinfoClassFieldsList( const ClassProxyPtr& class_type, const NamesScope& root_namespace )
{
	const FilePos file_pos{ 0u, 0u, 0u };

	Variable head= GetTypeinfoListEndNode( root_namespace );

	class_type->class_->members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& class_member )
		{
			const ClassField* const class_field= class_member.second.GetClassField();
			if( class_field == nullptr || class_field->class_.lock() != class_type )
				return;

			// TODO - maybe add fileds of parents and type of field`s parent class?

			const ProgramString node_class_name= "_node_"_SpC + class_member.first + "_of_"_SpC + class_type->class_->members.GetThisNamespaceName();
			const ClassProxyPtr node_type= std::make_shared<ClassProxy>( new Class( node_class_name, &root_namespace ) );
			Class& node_type_class= *node_type->class_;
			node_type_class.llvm_type= llvm::StructType::create( llvm_context_, MangleType( node_type ) );

			std::vector<llvm::Type*> fields_llvm_types;
			std::vector<llvm::Constant*> fields_initializers;

			// TODO - maybe reorder fields for better result struct layout?
			AddTypeinfoNodeIsEndVariable( node_type_class );

			node_type_class.members.AddName(
				g_next_node_name,
				Value( ClassField( node_type, head.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), file_pos ) );
			fields_llvm_types.push_back( llvm::PointerType::get( head.type.GetLLVMType(), 0u ) );
			fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>(head.llvm_value) );

			{
				const Variable field_type_typeinfo= BuildTypeInfo( class_field->type, root_namespace );
				ClassField field( node_type, field_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

				node_type_class.members.AddName( "type"_SpC, Value( std::move(field), file_pos ) );
				fields_llvm_types.push_back( llvm::PointerType::get( field_type_typeinfo.type.GetLLVMType(), 0u ) );
				fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( field_type_typeinfo.llvm_value ) );
			}
			// TODO - add offset, number.
			{
				const std::string name_str= ToUTF8( class_member.first );
				Array name_type;
				name_type.type= FundamentalType( U_FundamentalType::char8, fundamental_llvm_types_.char8 );
				name_type.size= name_str.size();
				name_type.llvm_type= llvm::ArrayType::get( name_type.type.GetLLVMType(), name_type.size );

				ClassField field( node_type, name_type, static_cast<unsigned int>(fields_llvm_types.size()), true, false );

				node_type_class.members.AddName( g_name_field_name, Value( std::move(field), file_pos ) );
				fields_llvm_types.push_back( name_type.llvm_type );
				fields_initializers.push_back( llvm::ConstantDataArray::getString( llvm_context_, name_str, false /* not null terminated */ ) );
			}

			node_type_class.members.AddName(
				"is_reference"_SpC,
				Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), file_pos ) );
			fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
			fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, class_field->is_reference ) ) );

			node_type_class.members.AddName(
				"is_mutable"_SpC,
				Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), file_pos ) );
			fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
			fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, class_field->is_mutable ) ) );

			FinishTypeinfoClass( node_type_class, node_type, fields_llvm_types );

			llvm::GlobalVariable* const global_variable=
				CreateGlobalConstantVariable(
					node_type,
					MangleGlobalVariable( root_namespace, "_val_of_"_SpC + node_class_name ),
					llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );

			head= Variable( node_type, Variable::Location::Pointer, ValueType::ConstReference, global_variable, global_variable->getInitializer() );
		} ); // for class elements

	return head;
}

Variable CodeBuilder::BuildTypeinfoClassTypesList( const ClassProxyPtr& class_type, const NamesScope& root_namespace )
{
	const FilePos file_pos{ 0u, 0u, 0u };

	Variable head= GetTypeinfoListEndNode( root_namespace );

	class_type->class_->members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& class_member )
		{
			const Type* const class_inner_type= class_member.second.GetTypeName();
			if( class_inner_type == nullptr )
				return;

			const ProgramString node_class_name= "_node_"_SpC + class_member.first + "_of_"_SpC + class_type->class_->members.GetThisNamespaceName();
			const ClassProxyPtr node_type= std::make_shared<ClassProxy>( new Class( node_class_name, &root_namespace ) );
			Class& node_type_class= *node_type->class_;
			node_type_class.llvm_type= llvm::StructType::create( llvm_context_, MangleType( node_type ) );

			std::vector<llvm::Type*> fields_llvm_types;
			std::vector<llvm::Constant*> fields_initializers;

			// TODO - maybe reorder fields for better result struct layout?
			AddTypeinfoNodeIsEndVariable( node_type_class );

			node_type_class.members.AddName(
				g_next_node_name,
				Value( ClassField( node_type, head.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), file_pos ) );
			fields_llvm_types.push_back( llvm::PointerType::get( head.type.GetLLVMType(), 0u ) );
			fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>(head.llvm_value) );

			{
				const Variable dependent_type_typeinfo= BuildTypeInfo( *class_inner_type, root_namespace );
				ClassField field( node_type, dependent_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

				node_type_class.members.AddName( "type"_SpC, Value( std::move(field), file_pos ) );
				fields_llvm_types.push_back( llvm::PointerType::get( dependent_type_typeinfo.type.GetLLVMType(), 0u ) );
				fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo.llvm_value ) );
			}
			{
				std::string name_str= ToUTF8( class_member.first );
				Array name_type;
				name_type.type= FundamentalType( U_FundamentalType::char8, fundamental_llvm_types_.char8 );
				name_type.size= name_str.size();
				name_type.llvm_type= llvm::ArrayType::get( name_type.type.GetLLVMType(), name_type.size );

				ClassField field( node_type, name_type, static_cast<unsigned int>(fields_llvm_types.size()), true, false );

				node_type_class.members.AddName( g_name_field_name, Value( std::move(field), file_pos ) );
				fields_llvm_types.push_back( name_type.llvm_type );
				fields_initializers.push_back( llvm::ConstantDataArray::getString( llvm_context_, name_str, false /* not null terminated */ ) );
			}

			FinishTypeinfoClass( node_type_class, node_type, fields_llvm_types );

			llvm::GlobalVariable* const global_variable=
				CreateGlobalConstantVariable(
					node_type,
					MangleGlobalVariable( root_namespace, "_val_of_"_SpC + node_class_name ),
					llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );

			head= Variable( node_type, Variable::Location::Pointer, ValueType::ConstReference, global_variable, global_variable->getInitializer() );
		} ); // for class elements

	return head;
}

Variable CodeBuilder::BuildTypeinfoClassFunctionsList( const ClassProxyPtr& class_type, const NamesScope& root_namespace )
{
	const FilePos file_pos{ 0u, 0u, 0u };

	Variable head= GetTypeinfoListEndNode( root_namespace );

	class_type->class_->members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& class_member )
		{
			const OverloadedFunctionsSet* const functions_set= class_member.second.GetFunctionsSet();
			if( functions_set == nullptr )
				return;
			size_t i= 0u;
			for( const FunctionVariable& function : functions_set->functions )
			{
				const ProgramString node_class_name= "_node_"_SpC + class_member.first + "_"_SpC + ToProgramString(std::to_string(i).c_str()) + "_of_"_SpC + class_type->class_->members.GetThisNamespaceName();
				const ClassProxyPtr node_type= std::make_shared<ClassProxy>( new Class( node_class_name, &root_namespace ) );
				Class& node_type_class= *node_type->class_;
				node_type_class.llvm_type= llvm::StructType::create( llvm_context_, MangleType( node_type ) );

				++i;

				std::vector<llvm::Type*> fields_llvm_types;
				std::vector<llvm::Constant*> fields_initializers;

				// TODO - maybe reorder fields for better result struct layout?
				AddTypeinfoNodeIsEndVariable( node_type_class );

				node_type_class.members.AddName(
					g_next_node_name,
					Value( ClassField( node_type, head.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), file_pos ) );
				fields_llvm_types.push_back( llvm::PointerType::get( head.type.GetLLVMType(), 0u ) );
				fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>(head.llvm_value) );

				{
					const Variable dependent_type_typeinfo= BuildTypeInfo( function.type, root_namespace );
					ClassField field( node_type, dependent_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

					node_type_class.members.AddName( "type"_SpC, Value( std::move(field), file_pos ) );
					fields_llvm_types.push_back( llvm::PointerType::get( dependent_type_typeinfo.type.GetLLVMType(), 0u ) );
					fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo.llvm_value ) );
				}
				{
					std::string name_str= ToUTF8( class_member.first );
					Array name_type;
					name_type.type= FundamentalType( U_FundamentalType::char8, fundamental_llvm_types_.char8 );
					name_type.size= name_str.size();
					name_type.llvm_type= llvm::ArrayType::get( name_type.type.GetLLVMType(), name_type.size );

					ClassField field( node_type, name_type, static_cast<unsigned int>(fields_llvm_types.size()), true, false );

					node_type_class.members.AddName( g_name_field_name, Value( std::move(field), file_pos ) );
					fields_llvm_types.push_back( name_type.llvm_type );
					fields_initializers.push_back( llvm::ConstantDataArray::getString( llvm_context_, name_str, false /* not null terminated */ ) );
				}

				node_type_class.members.AddName(
					"is_this_call"_SpC,
					Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), file_pos ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.is_this_call ) ) );

				node_type_class.members.AddName(
					"is_generated"_SpC,
					Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), file_pos ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.is_generated ) ) );

				node_type_class.members.AddName(
					"is_deleted"_SpC,
					Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), file_pos ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.is_deleted ) ) );

				node_type_class.members.AddName(
					"is_virtual"_SpC,
					Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), file_pos ) );
				fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
				fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, function.virtual_table_index != ~0u ) ) );

				FinishTypeinfoClass( node_type_class, node_type, fields_llvm_types );

				llvm::GlobalVariable* const global_variable=
					CreateGlobalConstantVariable(
						node_type,
						MangleGlobalVariable( root_namespace, "_val_of_"_SpC + node_class_name ),
						llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );

				head= Variable( node_type, Variable::Location::Pointer, ValueType::ConstReference, global_variable, global_variable->getInitializer() );
			} // for functions
		} ); // for class elements

	return head;
}

Variable CodeBuilder::BuildTypeinfoFunctionArguments( const Function& function_type, const NamesScope& root_namespace )
{
	const FilePos file_pos{ 0u, 0u, 0u };

	Variable head= GetTypeinfoListEndNode( root_namespace );
	for( const Function::Arg& arg : function_type.args )
	{
		const ProgramString node_class_name= "_node_"_SpC; // TODO - set correct name.
		const ClassProxyPtr node_type= std::make_shared<ClassProxy>( new Class( node_class_name, &root_namespace ) );
		Class& node_type_class= *node_type->class_;
		node_type_class.llvm_type= llvm::StructType::create( llvm_context_, MangleType( node_type ) );

		std::vector<llvm::Type*> fields_llvm_types;
		std::vector<llvm::Constant*> fields_initializers;

		// TODO - maybe reorder fields for better result struct layout?
		AddTypeinfoNodeIsEndVariable( node_type_class );

		node_type_class.members.AddName(
			g_next_node_name,
			Value( ClassField( node_type, head.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true ), file_pos ) );
		fields_llvm_types.push_back( llvm::PointerType::get( head.type.GetLLVMType(), 0u ) );
		fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>(head.llvm_value) );

		{
			const Variable dependent_type_typeinfo= BuildTypeInfo( arg.type, root_namespace );
			ClassField field( node_type, dependent_type_typeinfo.type, static_cast<unsigned int>(fields_llvm_types.size()), false, true );

			node_type_class.members.AddName( "type"_SpC, Value( std::move(field), file_pos ) );
			fields_llvm_types.push_back( llvm::PointerType::get( dependent_type_typeinfo.type.GetLLVMType(), 0u ) );
			fields_initializers.push_back( llvm::dyn_cast<llvm::GlobalVariable>( dependent_type_typeinfo.llvm_value ) );
		}

		node_type_class.members.AddName(
			"is_reference"_SpC,
			Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), file_pos ) );
		fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, arg.is_reference ) ) );

		node_type_class.members.AddName(
			"is_mutable"_SpC,
			Value( ClassField( node_type, bool_type_, static_cast<unsigned int>(fields_llvm_types.size()), true, false ), file_pos ) );
		fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( fundamental_llvm_types_.bool_, llvm::APInt( 1u, arg.is_mutable ) ) );

		// SPRACHE_TODO - add reference pollution

		FinishTypeinfoClass( node_type_class, node_type, fields_llvm_types );

		llvm::GlobalVariable* const global_variable=
			CreateGlobalConstantVariable(
				node_type,
				MangleGlobalVariable( root_namespace, "_val_of_"_SpC + node_class_name ),
				llvm::ConstantStruct::get( node_type_class.llvm_type, fields_initializers ) );

		head= Variable( node_type, Variable::Location::Pointer, ValueType::ConstReference, global_variable, global_variable->getInitializer() );
	}

	return head;
}

void CodeBuilder::UpdateTypeinfoForDependentTypes( const ClassProxyPtr& class_type )
{
	for( size_t i= 0u; i < typeinfo_cache_.size(); ++i )
	{
		if( TypeIsClassOrArrayOfClasses( class_type, typeinfo_cache_[i].first ) )
		{
			if( typeinfo_cache_[i].second.type.IsIncomplete() )
			{
				Variable typeinfo_variable= typeinfo_cache_[i].second;
				BuildFullTypeinfo( typeinfo_cache_[i].first, typeinfo_variable, *typeinfo_variable.type.GetClassType()->members.GetRoot() );
				typeinfo_cache_[i].second= typeinfo_variable;
			}
		}
	}
}

} // namespace CodeBuilderPrivate

} // namespace U
