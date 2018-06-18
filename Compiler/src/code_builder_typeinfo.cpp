#include "mangling.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

Value CodeBuilder::BuildTypeinfoOperator( const Synt::TypeInfo& typeinfo_op, NamesScope& names )
{
	const Type type= PrepareType( typeinfo_op.type_, names );
	if( type == NontypeStub::ErrorValue || type == invalid_type_ )
		return ErrorValue();

	if( type.IsIncomplete() )
	{
		errors_.push_back( ReportUsingIncompleteType( typeinfo_op.file_pos_, type.ToString() ) );
		return ErrorValue();
	}

	// TODO - cache typeinfo values.

	const ProgramString typeinfo_class_name= "_typeinfo_for_"_SpC + type.ToString();
	const auto typeinfo_class_proxy= std::make_shared<ClassProxy>( new Class( typeinfo_class_name, names.GetRoot() ) );
	Class& typeinfo_class= *typeinfo_class_proxy->class_;

	std::vector<llvm::Type*> fields_llvm_types;
	std::vector<llvm::Constant*> fields_initializers;

	const auto add_bool_field=
	[&]( const ProgramString& name, const bool value )
	{
		ClassField field;
		field.class_= typeinfo_class_proxy;
		field.type= bool_type_;
		field.index= fields_llvm_types.size();
		field.is_reference= false;
		field.is_mutable= false;

		typeinfo_class.members.AddName( name, Value( std::move(field), typeinfo_op.file_pos_ ) );
		fields_llvm_types.push_back( fundamental_llvm_types_.bool_ );
		fields_initializers.push_back( llvm::Constant::getIntegerValue( field.type.GetLLVMType(), llvm::APInt( 1u, uint64_t(value) ) ) );
	};

	add_bool_field(      "is_fundamental"_SpC, type.GetFundamentalType()     != nullptr );
	add_bool_field(             "is_enum"_SpC, type.GetEnumType()            != nullptr );
	add_bool_field(            "is_array"_SpC, type.GetArrayType()           != nullptr );
	add_bool_field(            "is_class"_SpC, type.GetClassType()           != nullptr );
	add_bool_field( "is_function_pointer"_SpC, type.GetFunctionPointerType() != nullptr );


	// TODO - add other stuff

	// Prepare class

	typeinfo_class.llvm_type= llvm::StructType::create( fields_llvm_types, MangleType( typeinfo_class_proxy ) );
	typeinfo_class.kind= Class::Kind::Struct;
	typeinfo_class.completeness= Class::Completeness::Complete;
	typeinfo_class.can_be_constexpr= true;

	TryGenerateDefaultConstructor( typeinfo_class, typeinfo_class_proxy );
	TryGenerateDestructor( typeinfo_class, typeinfo_class_proxy );
	TryGenerateCopyConstructor( typeinfo_class, typeinfo_class_proxy );
	TryGenerateCopyAssignmentOperator( typeinfo_class, typeinfo_class_proxy );

	// Prepare result value

	Variable result;
	result.type= typeinfo_class_proxy;
	result.location= Variable::Location::Pointer;
	result.value_type= ValueType::ConstReference;

	result.constexpr_value= llvm::ConstantStruct::get( typeinfo_class.llvm_type, fields_initializers );
	result.llvm_value=
		CreateGlobalConstantVariable(
			typeinfo_class_proxy,
			ToStdString( "_val_of_"_SpC + typeinfo_class_name ),
			result.constexpr_value );

	return Value( result, typeinfo_op.file_pos_ );
}

} // namespace CodeBuilderPrivate

} // namespace U
