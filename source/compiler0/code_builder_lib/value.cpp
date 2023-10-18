#include "../../lex_synt_lib_common/assert.hpp"
#include "class.hpp"
#include "template_signature_param.hpp"
#include "enum.hpp"
#include "../../lex_synt_lib_common/size_assert.hpp"
#include "value.hpp"

namespace U
{

//
// FunctionVariable
//

bool FunctionVariable::VirtuallyEquals( const FunctionVariable& other ) const
{
	U_ASSERT( this->is_this_call && other.is_this_call );

	const FunctionType& l_type= this->type;
	const FunctionType& r_type= other.type;

	return
		l_type.return_type == r_type.return_type &&
		l_type.return_value_type == r_type.return_value_type &&
		l_type.return_references == r_type.return_references &&
		l_type.return_inner_references == r_type.return_inner_references &&
		l_type.references_pollution == r_type.references_pollution &&
		l_type.unsafe == r_type.unsafe &&
		l_type.calling_convention == r_type.calling_convention &&
		l_type.params.size() == r_type.params.size() &&
		l_type.params.size() > 0u && l_type.params.front().value_type == r_type.params.front().value_type &&
		std::equal( l_type.params.begin() + 1, l_type.params.end(), r_type.params.begin() + 1 );  // Compare args, except first.
}

//
// Variable
//

Variable::Variable(
	Type in_type,
	const ValueType in_value_type,
	const Location in_location,
	std::string in_name,
	llvm::Value* const in_llvm_value,
	llvm::Constant* const in_constexpr_value )
	: type(std::move(in_type))
	, llvm_value(in_llvm_value)
	, constexpr_value(in_constexpr_value)
	, name(std::move(in_name))
	, value_type(in_value_type)
	, location(in_location)
{
}

VariableMutPtr Variable::Create(
	Type type,
	const ValueType value_type,
	const Location location,
	std::string name,
	llvm::Value* const llvm_value,
	llvm::Constant* const constexpr_value )
{
	auto result= std::make_shared<Variable>( Variable( std::move(type), value_type, location, std::move(name), llvm_value, constexpr_value ) );

	if( result->type.ReferencesTagsCount() > 0 )
	{
		const auto inner_reference_node= std::make_shared<Variable>(
			Variable(
				FundamentalType( U_FundamentalType::InvalidType ),
				// Mutability of inner reference node is determined only by type properties itself.
				result->type.GetInnerReferenceType() == InnerReferenceType::Mut ? ValueType::ReferenceMut : ValueType::ReferenceImut,
				Variable::Location::Pointer,
				result->name + " inner reference",
				nullptr,
				nullptr ) );
		inner_reference_node->is_variable_inner_reference_node= result->value_type == ValueType::Value;

		result->inner_reference_node= inner_reference_node;
	}

	return result;
}

VariableMutPtr Variable::CreateChildNode(
	const VariablePtr& parent,
	Type type,
	const ValueType value_type,
	const Location location,
	std::string name,
	llvm::Value* const llvm_value,
	llvm::Constant* const constexpr_value )
{
	U_ASSERT( parent != nullptr );
	auto result= std::make_shared<Variable>( Variable( std::move(type), value_type, location, std::move(name), llvm_value, constexpr_value ) );
	result->parent= parent;

	// Child nodes reuse inner reference nodes of parents.
	if( type.ReferencesTagsCount() > 0 )
		result->inner_reference_node= parent->inner_reference_node;

	return result;
}

std::string ConstantVariableToString( const TemplateVariableArg& variable )
{
	if( variable.constexpr_value == nullptr )
		return "";

	if( const auto fundamental_type= variable.type.GetFundamentalType() )
	{
		if( fundamental_type->fundamental_type == U_FundamentalType::bool_ )
			return variable.constexpr_value->getUniqueInteger().isNullValue() ? "false" : "true";
		else if( IsFloatingPoint( fundamental_type->fundamental_type ) )
		{
			if( const auto constant_fp= llvm::dyn_cast<llvm::ConstantFP>( variable.constexpr_value ) )
			{
				llvm::SmallString<256> str;
				constant_fp->getValueAPF().toString(str);
				if( fundamental_type->fundamental_type == U_FundamentalType::f32_ )
					str+= "f";
				return str.str().str();
			}
		}
		else if( IsSignedInteger( fundamental_type->fundamental_type ) )
		{
			std::string res= std::to_string( variable.constexpr_value->getUniqueInteger().getSExtValue() );
			res+=
				fundamental_type->fundamental_type == U_FundamentalType::i32_
					? ""
					: GetFundamentalTypeName( fundamental_type->fundamental_type );

			return res;
		}
		else if( IsUnsignedInteger( fundamental_type->fundamental_type ) )
		{
			std::string res= std::to_string( variable.constexpr_value->getUniqueInteger().getZExtValue() );
			res+=
				fundamental_type->fundamental_type == U_FundamentalType::u32_
					? "u"
					: GetFundamentalTypeName( fundamental_type->fundamental_type );

			return res;
		}
		else if( IsChar( fundamental_type->fundamental_type ) )
		{
			std::string res= std::to_string( variable.constexpr_value->getUniqueInteger().getZExtValue() );

			if( fundamental_type->fundamental_type == U_FundamentalType::char8_  )
				res+= "c8" ;
			if( fundamental_type->fundamental_type == U_FundamentalType::char16_ )
				res+= "c16";
			if( fundamental_type->fundamental_type == U_FundamentalType::char32_ )
				res+= "c32";

			return res;
		}
		else if( IsByte( fundamental_type->fundamental_type ) )
		{
			std::string res;
			res+= GetFundamentalTypeName( fundamental_type->fundamental_type );
			res+= "( ";

			res+= std::to_string( variable.constexpr_value->getUniqueInteger().getZExtValue() );
			switch( fundamental_type->fundamental_type )
			{
			case U_FundamentalType::byte8_  : res+= GetFundamentalTypeName( U_FundamentalType::u8_   ); break;
			case U_FundamentalType::byte16_ : res+= GetFundamentalTypeName( U_FundamentalType::u16_  ); break;
			case U_FundamentalType::byte32_ : res+= GetFundamentalTypeName( U_FundamentalType::u32_  ); break;
			case U_FundamentalType::byte64_ : res+= GetFundamentalTypeName( U_FundamentalType::u64_  ); break;
			case U_FundamentalType::byte128_: res+= GetFundamentalTypeName( U_FundamentalType::u128_ ); break;
			default: U_ASSERT(false); break;
			}

			res+= " )";
			return res;
		}
	}
	else if( const auto enum_type= variable.type.GetEnumType() )
	{
		const llvm::APInt num_value= variable.constexpr_value->getUniqueInteger();
		std::string_view enum_member_name;
		enum_type->members.ForEachInThisScope(
			[&]( const std::string_view name, const NamesScopeValue& enum_member )
			{
				if( const VariablePtr enum_variable= enum_member.value.GetVariable() )
				{
					U_ASSERT( enum_variable->constexpr_value != nullptr );
					if( enum_variable->constexpr_value->getUniqueInteger().getLimitedValue() == num_value )
						enum_member_name= name;
				}
			});

		std::string res= enum_type->members.ToString() + "::";
		res+= enum_member_name;
		return res;
	}

	return "";
}

//
// ClassField
//

ClassField::ClassField( const ClassPtr in_class, Type in_type, const uint32_t in_index, const bool in_is_mutable, const bool in_is_reference )
	: type(std::move(in_type)), class_(in_class), index(in_index), is_mutable(in_is_mutable), is_reference(in_is_reference)
{}

const std::string ClassField::c_generated_field_name= "_generated";

//
// Value
//

SIZE_ASSERT( Value, 56u )
SIZE_ASSERT( NamesScopeValue, 72u )

Value::Value( VariablePtr variable )
{
	something_= std::move(variable);
}

Value::Value( VariableMutPtr variable )
{
	something_= std::move(variable);
}

Value::Value( OverloadedFunctionsSetPtr functions_set )
{
	something_= std::move(functions_set);
}

Value::Value( Type type )
{
	something_= std::move(type);
}

Value::Value( ClassFieldPtr class_field )
{
	something_= std::move( class_field );
}

Value::Value( ThisOverloadedMethodsSet this_overloaded_methods_set )
{
	something_= std::move( this_overloaded_methods_set );
}

Value::Value( NamesScopePtr namespace_ )
{
	U_ASSERT( namespace_ != nullptr );
	something_= std::move(namespace_);
}

Value::Value( TypeTemplatesSet type_templates )
{
	something_= std::move(type_templates);
}

Value::Value( StaticAssert static_assert_ )
{
	something_= std::move(static_assert_);
}

Value::Value( TypeAlias type_alias )
{
	something_= std::move(type_alias);
}

Value::Value( IncompleteGlobalVariable incomplete_global_variable )
{
	something_= std::move(incomplete_global_variable);
}

Value::Value( YetNotDeducedTemplateArg yet_not_deduced_template_arg )
{
	something_= std::move(yet_not_deduced_template_arg);
}

Value::Value( ErrorValue error_value )
{
	something_= std::move(error_value);
}

size_t Value::GetKindIndex() const
{
	return something_.index();
}

std::string_view Value::GetKindName() const
{
	struct Visitor final
	{
		std::string_view operator()( const VariablePtr& ) const { return "variable"; }
		std::string_view operator()( const FunctionVariable& ) const { return "function variable"; }
		std::string_view operator()( const OverloadedFunctionsSetPtr& ) const { return "functions set"; }
		std::string_view operator()( const Type& ) const { return "typename"; }
		std::string_view operator()( const ClassFieldPtr& ) const { return "class field"; }
		std::string_view operator()( const ThisOverloadedMethodsSet& ) const { return "this + functions set"; }
		std::string_view operator()( const NamesScopePtr& ) const { return "namespace"; }
		std::string_view operator()( const TypeTemplatesSet& ) const { return "type templates set"; }
		std::string_view operator()( const StaticAssert& ) const { return "static assert"; }
		std::string_view operator()( const TypeAlias& ) const { return "incomplete type alias"; }
		std::string_view operator()( const IncompleteGlobalVariable& ) const { return "incomplete global variable"; }
		std::string_view operator()( const YetNotDeducedTemplateArg& ) const { return "yet not deduced template arg"; }
		std::string_view operator()( const ErrorValue& ) const { return "error value"; }
	};

	return std::visit( Visitor(), something_ );
}

VariablePtr Value::GetVariable() const
{
	if( const auto ptr= std::get_if<VariablePtr>( &something_ ) )
		return *ptr;

	return nullptr;
}

OverloadedFunctionsSetPtr Value::GetFunctionsSet() const
{
	if( const auto ptr= std::get_if<OverloadedFunctionsSetPtr>( &something_ ) )
		return *ptr;

	return nullptr;
}

Type* Value::GetTypeName()
{
	return std::get_if<Type>( &something_ );
}

const Type* Value::GetTypeName() const
{
	return std::get_if<Type>( &something_ );
}

ClassFieldPtr Value::GetClassField() const
{
	if( const auto ptr= std::get_if<ClassFieldPtr>( &something_ ) )
		return *ptr;

	return nullptr;
}

ThisOverloadedMethodsSet* Value::GetThisOverloadedMethodsSet()
{
	return std::get_if<ThisOverloadedMethodsSet>( &something_ );
}

const ThisOverloadedMethodsSet* Value::GetThisOverloadedMethodsSet() const
{
	return std::get_if<ThisOverloadedMethodsSet>( &something_ );
}

NamesScopePtr Value::GetNamespace() const
{
	const NamesScopePtr* const namespace_= std::get_if<NamesScopePtr>( &something_ );
	if( namespace_ == nullptr )
		return nullptr;
	return *namespace_;
}

TypeTemplatesSet* Value::GetTypeTemplatesSet()
{
	return std::get_if<TypeTemplatesSet>( &something_ );
}

const TypeTemplatesSet* Value::GetTypeTemplatesSet() const
{
	return std::get_if<TypeTemplatesSet>( &something_ );
}

StaticAssert* Value::GetStaticAssert()
{
	return std::get_if<StaticAssert>( &something_ );
}

const StaticAssert* Value::GetStaticAssert() const
{
	return std::get_if<StaticAssert>( &something_ );
}

TypeAlias* Value::GetTypeAlias()
{
	return std::get_if<TypeAlias>( &something_ );
}

const TypeAlias* Value::GetTypeAlias() const
{
	return std::get_if<TypeAlias>( &something_ );
}

IncompleteGlobalVariable* Value::GetIncompleteGlobalVariable()
{
	return std::get_if<IncompleteGlobalVariable>( &something_ );
}

const IncompleteGlobalVariable* Value::GetIncompleteGlobalVariable() const
{
	return std::get_if<IncompleteGlobalVariable>( &something_ );
}

YetNotDeducedTemplateArg* Value::GetYetNotDeducedTemplateArg()
{
	return std::get_if<YetNotDeducedTemplateArg>( &something_ );
}

const YetNotDeducedTemplateArg* Value::GetYetNotDeducedTemplateArg() const
{
	return std::get_if<YetNotDeducedTemplateArg>( &something_ );
}

const ErrorValue* Value::GetErrorValue() const
{
	return std::get_if<ErrorValue>( &something_ );
}

} // namespace U
