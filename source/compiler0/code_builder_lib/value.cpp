#include "../../lex_synt_lib_common/assert.hpp"
#include "class.hpp"
#include "template_signature_param.hpp"
#include "enum.hpp"
#include "value.hpp"

namespace U
{

//
// FunctionVariable
//

bool FunctionVariable::VirtuallyEquals( const FunctionVariable& other ) const
{
	U_ASSERT( this->is_this_call && other.is_this_call );

	const FunctionType& l_type= *this->type.GetFunctionType();
	const FunctionType& r_type= *other.type.GetFunctionType();

	return
		l_type.return_type == r_type.return_type &&
		l_type.return_value_type == r_type.return_value_type &&
		l_type.return_references == r_type.return_references &&
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
	const Location in_location, const ValueType in_value_type,
	llvm::Value* const in_llvm_value, llvm::Constant* const in_constexpr_value )
	: type(std::move(in_type)), location(in_location), value_type(in_value_type)
	, llvm_value(in_llvm_value), constexpr_value(in_constexpr_value)
{}

std::string ConstantVariableToString( const Variable& variable )
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
				return str.str();
			}
		}
		else if( IsSignedInteger( fundamental_type->fundamental_type ) )
		{
			const std::string suffix=
				fundamental_type->fundamental_type == U_FundamentalType::i32_
					? ""
					: GetFundamentalTypeName( fundamental_type->fundamental_type );

			return std::to_string( variable.constexpr_value->getUniqueInteger().getSExtValue() ) + suffix;
		}
		else if( IsUnsignedInteger( fundamental_type->fundamental_type ) )
		{
			const std::string suffix=
				fundamental_type->fundamental_type == U_FundamentalType::u32_
					? "u"
					: GetFundamentalTypeName( fundamental_type->fundamental_type );

			return std::to_string( variable.constexpr_value->getUniqueInteger().getZExtValue() ) + suffix;
		}
		else if( IsChar( fundamental_type->fundamental_type ) )
		{
			const char* suffix= "";
			if( fundamental_type->fundamental_type == U_FundamentalType::char8_  )
				suffix= "c8" ;
			if( fundamental_type->fundamental_type == U_FundamentalType::char16_ )
				suffix= "c16";
			if( fundamental_type->fundamental_type == U_FundamentalType::char32_ )
				suffix= "c32";

			return std::to_string( variable.constexpr_value->getUniqueInteger().getZExtValue() ) + suffix;
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
		std::string enum_member_name;
		enum_type->members.ForEachInThisScope(
			[&]( const std::string& name, const Value& enum_member )
			{
				if( const Variable* const enum_variable= enum_member.GetVariable() )
				{
					U_ASSERT( enum_variable->constexpr_value != nullptr );
					if( enum_variable->constexpr_value->getUniqueInteger().getLimitedValue() == num_value )
						enum_member_name= name;
				}
			});

		return enum_type->members.ToString() + "::" + enum_member_name;
	}

	return "";
}

//
// ClassField
//

ClassField::ClassField( const ClassPtr& in_class, Type in_type, const unsigned int in_index, const bool in_is_mutable, const bool in_is_reference )
	: type(std::move(in_type)), class_(in_class), index(in_index), is_mutable(in_is_mutable), is_reference(in_is_reference)
{}


//
// ThisOverloadedMethodsSet
//

ThisOverloadedMethodsSet::ThisOverloadedMethodsSet()
	: overloaded_methods_set_( std::make_unique<OverloadedFunctionsSet>() )
{}

ThisOverloadedMethodsSet::ThisOverloadedMethodsSet( const ThisOverloadedMethodsSet& other )
	: this_(other.this_), overloaded_methods_set_( std::make_unique<OverloadedFunctionsSet>( *other.overloaded_methods_set_ ) )
{}

ThisOverloadedMethodsSet& ThisOverloadedMethodsSet::operator=( const ThisOverloadedMethodsSet& other )
{
	this->this_= other.this_;
	*this->overloaded_methods_set_= *other.overloaded_methods_set_;
	return *this;
}

OverloadedFunctionsSet& ThisOverloadedMethodsSet::GetOverloadedFunctionsSet()
{
	return *overloaded_methods_set_;
}

const OverloadedFunctionsSet& ThisOverloadedMethodsSet::GetOverloadedFunctionsSet() const
{
	return *overloaded_methods_set_;
}

//
// Value
//

static_assert( sizeof(Value) <= 152u, "Value is too heavy!" );

Value::Value( Variable variable, const SrcLoc& src_loc )
	: src_loc_(src_loc)
{
	something_= std::move(variable);
}

Value::Value( OverloadedFunctionsSet functions_set )
{
	something_= std::move(functions_set);
}

Value::Value( Type type, const SrcLoc& src_loc )
	: src_loc_(src_loc)
{
	something_= std::move(type);
}

Value::Value( ClassField class_field, const SrcLoc& src_loc )
	: src_loc_(src_loc)
{
	something_= std::move( class_field );
}

Value::Value( ThisOverloadedMethodsSet this_overloaded_methods_set )
{
	something_= std::move( this_overloaded_methods_set );
}

Value::Value( const NamesScopePtr& namespace_, const SrcLoc& src_loc )
	: src_loc_(src_loc)
{
	U_ASSERT( namespace_ != nullptr );
	something_= namespace_;
}

Value::Value( TypeTemplatesSet type_templates, const SrcLoc& src_loc )
	: src_loc_(src_loc)
{
	something_= std::move(type_templates);
}


Value::Value( StaticAssert static_assert_, const SrcLoc& src_loc )
	: src_loc_(src_loc)
{
	something_= std::move(static_assert_);
}

Value::Value( Typedef typedef_, const SrcLoc& src_loc )
	: src_loc_(src_loc)
{
	something_= std::move(typedef_);
}

Value::Value( IncompleteGlobalVariable incomplete_global_variable, const SrcLoc& src_loc )
	: src_loc_(src_loc)
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

std::string Value::GetKindName() const
{
	struct Visitor final
	{
		std::string operator()( const Variable& ) const { return "variable"; }
		std::string operator()( const FunctionVariable& ) const { return "function variable"; }
		std::string operator()( const OverloadedFunctionsSet& ) const { return "functions set"; }
		std::string operator()( const Type& ) const { return "typename"; }
		std::string operator()( const ClassField& ) const { return "class field"; }
		std::string operator()( const ThisOverloadedMethodsSet& ) const { return "this + functions set"; }
		std::string operator()( const NamesScopePtr& ) const { return "namespace"; }
		std::string operator()( const TypeTemplatesSet& ) const { return "type templates set"; }
		std::string operator()( const StaticAssert& ) const { return "static assert"; }
		std::string operator()( const Typedef& ) const { return "incomplete typedef"; }
		std::string operator()( const IncompleteGlobalVariable& ) const { return "incomplete global variable"; }
		std::string operator()( const YetNotDeducedTemplateArg& ) const { return "yet not deduced template arg"; }
		std::string operator()( const ErrorValue& ) const { return "error value"; }
	};

	return std::visit( Visitor(), something_ );
}

const SrcLoc& Value::GetSrcLoc() const
{
	return src_loc_;
}

Variable* Value::GetVariable()
{
	return std::get_if<Variable>( &something_ );
}

const Variable* Value::GetVariable() const
{
	return std::get_if<Variable>( &something_ );
}

OverloadedFunctionsSet* Value::GetFunctionsSet()
{
	return std::get_if<OverloadedFunctionsSet>( &something_ );
}

const OverloadedFunctionsSet* Value::GetFunctionsSet() const
{
	return std::get_if<OverloadedFunctionsSet>( &something_ );
}

Type* Value::GetTypeName()
{
	return std::get_if<Type>( &something_ );
}

const Type* Value::GetTypeName() const
{
	return std::get_if<Type>( &something_ );
}

ClassField* Value::GetClassField()
{
	return std::get_if<ClassField>( &something_ );
}

const ClassField* Value::GetClassField() const
{
	return std::get_if<ClassField>( &something_ );
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

Typedef* Value::GetTypedef()
{
	return std::get_if<Typedef>( &something_ );
}

const Typedef* Value::GetTypedef() const
{
	return std::get_if<Typedef>( &something_ );
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

ErrorValue* Value::GetErrorValue()
{
	return std::get_if<ErrorValue>( &something_ );
}

const ErrorValue* Value::GetErrorValue() const
{
	return std::get_if<ErrorValue>( &something_ );
}

} // namespace U
