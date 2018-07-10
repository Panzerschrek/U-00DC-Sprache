#include "assert.hpp"
#include "mangling.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

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

	Variable result;
	result.type= typeinfo_class_proxy;
	result.location= Variable::Location::Pointer;
	result.value_type= ValueType::ConstReference;

	result.constexpr_value= llvm::UndefValue::get( typeinfo_class.llvm_type ); // Currently uninitialized.

	llvm::GlobalVariable* const global_variable=
		new llvm::GlobalVariable(
			*module_,
			typeinfo_class.llvm_type,
			true, // is constant
			llvm::GlobalValue::InternalLinkage, // We have no external variables, so, use internal linkage.
			result.constexpr_value,
			ToStdString( "_val_of_"_SpC + typeinfo_class_name ) );
	global_variable->setUnnamedAddr( true );

	result.llvm_value= global_variable;
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
		ClassField field;
		field.class_= typeinfo_class_proxy;
		field.type= bool_type_;
		field.index= static_cast<unsigned int>(fields_llvm_types.size());
		field.is_reference= false;
		field.is_mutable= true;

		typeinfo_class.members.AddName( name, Value( std::move(field), file_pos ) );
		fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( field.type.GetLLVMType(), llvm::APInt( 1u, uint64_t(value) ) ) );
	};

	const auto add_size_field=
	[&]( const ProgramString& name, const SizeType value )
	{
		ClassField field;
		field.class_= typeinfo_class_proxy;
		field.type= size_type_;
		field.index= static_cast<unsigned int>(fields_llvm_types.size());
		field.is_reference= false;
		field.is_mutable= true;

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

		ClassField field;
		field.class_= typeinfo_class_proxy;
		field.type= dependent_type_typeinfo.type;
		field.index= static_cast<unsigned int>(fields_llvm_types.size());
		field.is_reference= true;
		field.is_mutable= false;

		typeinfo_class.members.AddName( name, Value( std::move(field), file_pos ) );
		fields_llvm_types.push_back( llvm::PointerType::get( dependent_type_typeinfo.type.GetLLVMType(), 0u ) );
		fields_initializers.push_back( llvm::dyn_cast<llvm::Constant>( dependent_type_typeinfo.llvm_value ) );
	};

	const llvm::DataLayout& data_layout= module_->getDataLayout();
	add_size_field(  "size_of"_SpC, data_layout.getTypeAllocSize( type.GetLLVMType() ) );
	add_size_field( "align_of"_SpC, data_layout.getABITypeAlignment( type.GetLLVMType() ) ); // TODO - is this correct alignment?

	add_bool_field(      "is_fundamental"_SpC, type.GetFundamentalType()     != nullptr );
	add_bool_field(             "is_enum"_SpC, type.GetEnumType()            != nullptr );
	add_bool_field(            "is_array"_SpC, type.GetArrayType()           != nullptr );
	add_bool_field(            "is_class"_SpC, type.GetClassType()           != nullptr );
	add_bool_field( "is_function_pointer"_SpC, type.GetFunctionPointerType() != nullptr );

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

		// SPRACHE_TODO - add complete information about class type - fields, member types, functions, etc/
	}
	else if( type.GetFunctionPointerType() != nullptr )
	{
		// SPRACHE_DOTO - add complete information about function pointer type - arguments, return value, pollution, unsafe, etc.
	}
	else U_ASSERT(false);

	// TODO - add other stuff

	// Prepare class

	typeinfo_class.llvm_type->setBody( fields_llvm_types );
	typeinfo_class.kind= Class::Kind::Struct;
	typeinfo_class.completeness= Class::Completeness::Complete;
	typeinfo_class.can_be_constexpr= true;

	TryGenerateDefaultConstructor( typeinfo_class, typeinfo_class_proxy );
	TryGenerateDestructor( typeinfo_class, typeinfo_class_proxy );
	TryGenerateCopyConstructor( typeinfo_class, typeinfo_class_proxy );
	TryGenerateCopyAssignmentOperator( typeinfo_class, typeinfo_class_proxy );

	// Prepare result value
	typeinfo_variable.constexpr_value= llvm::ConstantStruct::get( typeinfo_class.llvm_type, fields_initializers );
	llvm::dyn_cast<llvm::GlobalVariable>(typeinfo_variable.llvm_value)->setInitializer( typeinfo_variable.constexpr_value );

	UpdateTypeinfoForDependentTypes( typeinfo_class_proxy );
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
