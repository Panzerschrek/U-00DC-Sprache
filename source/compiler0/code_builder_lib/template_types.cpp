#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Hashing.h>
#include <llvm/ADT/SmallString.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "../../lex_synt_lib_common/assert.hpp"

#include "enum.hpp"
#include "template_types.hpp"

namespace U
{

namespace
{

size_t TemplateArgHashImpl( const TemplateVariableArg& template_variable_arg )
{
	return
		llvm::hash_combine(
			template_variable_arg.type.Hash(),
			// LLVM constants are deduplicated, so, hashing pointers should work.
			template_variable_arg.constexpr_value );
}

size_t TemplateArgHashImpl( const Type& template_type_arg )
{
	return template_type_arg.Hash();
}

size_t TemplateArgHashImpl( const TypeTemplatePtr& type_template_arg )
{
	// Use pointer for hasing, since type templates are identified internally by their pointer.
	return reinterpret_cast<uintptr_t>( type_template_arg.get() );
}

size_t TemplateArgHash( const TemplateArg& arg )
{
	return std::visit( []( const auto& el ) { return TemplateArgHashImpl(el); }, arg );
}

} // namespace

bool operator==( const TemplateVariableArg& l, const TemplateVariableArg& r )
{
	U_ASSERT( l.constexpr_value != nullptr );
	U_ASSERT( r.constexpr_value != nullptr );
	return
		l.type == r.type &&
		// LLVM constants are deduplicated, so, comparing pointers should work.
		l.constexpr_value == r.constexpr_value;
}

void TemplateVariableArg::AppendToString( std::string& s ) const
{
	if( constexpr_value == nullptr )
		return;

	if( const auto fundamental_type= type.GetFundamentalType() )
	{
		if( fundamental_type->fundamental_type == U_FundamentalType::bool_ )
			s+= constexpr_value->getUniqueInteger().isZero() ? "false" : "true";
		else if( IsFloatingPoint( fundamental_type->fundamental_type ) )
		{
			if( const auto constant_fp= llvm::dyn_cast<llvm::ConstantFP>( constexpr_value ) )
			{
				llvm::SmallString<64> str;
				constant_fp->getValueAPF().toString(str);
				s+= str;
				if( fundamental_type->fundamental_type == U_FundamentalType::f32_ )
					s+= "f";
			}
		}
		else if( IsSignedInteger( fundamental_type->fundamental_type ) )
		{
			s+= std::to_string( constexpr_value->getUniqueInteger().getSExtValue() );
			s+=
				fundamental_type->fundamental_type == U_FundamentalType::i32_
					? ""
					: GetFundamentalTypeName( fundamental_type->fundamental_type );;
		}
		else if( IsUnsignedInteger( fundamental_type->fundamental_type ) )
		{
			s+= std::to_string( constexpr_value->getUniqueInteger().getZExtValue() );
			s+=
				fundamental_type->fundamental_type == U_FundamentalType::u32_
					? "u"
					: GetFundamentalTypeName( fundamental_type->fundamental_type );
		}
		else if( IsChar( fundamental_type->fundamental_type ) )
		{
			s+= GetFundamentalTypeName( fundamental_type->fundamental_type );
			s.push_back( '(' );
			s+= std::to_string( constexpr_value->getUniqueInteger().getZExtValue() );
			s.push_back( ')' );
		}
		else if( IsByte( fundamental_type->fundamental_type ) )
		{
			s+= GetFundamentalTypeName( fundamental_type->fundamental_type );
			s+= "( ";

			s+= std::to_string( constexpr_value->getUniqueInteger().getZExtValue() );
			switch( fundamental_type->fundamental_type )
			{
			case U_FundamentalType::byte8_  : s+= GetFundamentalTypeName( U_FundamentalType::u8_   ); break;
			case U_FundamentalType::byte16_ : s+= GetFundamentalTypeName( U_FundamentalType::u16_  ); break;
			case U_FundamentalType::byte32_ : s+= GetFundamentalTypeName( U_FundamentalType::u32_  ); break;
			case U_FundamentalType::byte64_ : s+= GetFundamentalTypeName( U_FundamentalType::u64_  ); break;
			case U_FundamentalType::byte128_: s+= GetFundamentalTypeName( U_FundamentalType::u128_ ); break;
			default: U_ASSERT(false); break;
			}

			s+= " )";
		}
	}
	else if( const auto enum_type= type.GetEnumType() )
	{
		const llvm::APInt num_value= constexpr_value->getUniqueInteger();
		std::string_view enum_member_name;
		enum_type->members.ForEachInThisScope(
			[&]( const std::string_view name, const NamesScopeValue& enum_member )
			{
				if( const VariablePtr enum_variable= enum_member.value.GetVariable() )
				{
					U_ASSERT( enum_variable->constexpr_value != nullptr );
					if( enum_variable->constexpr_value->getUniqueInteger() == num_value )
						enum_member_name= name;
				}
			});

		type.AppendToString( s );
		s+= "::";
		s+= enum_member_name;
	}
	else if( const auto array_type= type.GetArrayType() )
	{
		// For now we have no special syntax for array literals. So, use just a comma-separated list in [].
		s+= type.ToString();
		s+= "[ ";

		TemplateVariableArg element;
		element.type= array_type->element_type;
		for( uint64_t i= 0; i < array_type->element_count; ++i )
		{
			element.constexpr_value= constexpr_value->getAggregateElement( uint32_t(i) );
			element.AppendToString( s );
			if( i + 1 < array_type->element_count )
				s+= ", ";
		}

		s+= "]";
	}
	else if( const auto tuple_type= type.GetTupleType() )
	{
		// For now we have no special syntax for tuple literals. So, use just a comma-separated list in [].
		s+= type.ToString();
		s+= "[ ";

		for( size_t i= 0; i < tuple_type->element_types.size(); ++i )
		{
			TemplateVariableArg element;
			element.type= tuple_type->element_types[i];
			element.constexpr_value= constexpr_value->getAggregateElement( uint32_t(i) );
			element.AppendToString( s );
			if( i + 1 < tuple_type->element_types.size() )
				s+= ", ";
		}

		s+= "]";
	}
}

void TypeTemplate::AppendToString( std::string& s ) const
{
	if( parent_namespace->GetParent() != nullptr )
	{
		parent_namespace->AppendToString( s );
		s+= "::";
	}
	s+= syntax_element->name;
}

void FunctionTemplate::AppendToString( std::string& s ) const
{
	if( parent_namespace->GetParent() != nullptr )
	{
		parent_namespace->AppendToString( s );
		s+= "::";
	}
	s+= syntax_element->function->name.back().name;
}

size_t TemplateKey::Hash() const
{
	size_t hash= size_t( reinterpret_cast<uintptr_t>( template_.get() ) );

	for( const TemplateArg& arg : args )
		hash= llvm::hash_combine( hash, TemplateArgHash(arg) );

	return hash;
}

bool operator==( const TemplateKey& l, const TemplateKey& r )
{
	return l.template_ == r.template_ && l.args == r.args;
}

size_t ParameterizedFunctionTemplateKey::Hash() const
{
	size_t hash= size_t( reinterpret_cast<uintptr_t>( functions_set.get() ) );

	for( const TemplateArg& arg : args )
		hash= llvm::hash_combine( hash, TemplateArgHash(arg) );

	return hash;
}

bool operator==( const ParameterizedFunctionTemplateKey& l, const ParameterizedFunctionTemplateKey& r )
{
	return l.functions_set == r.functions_set && l.args == r.args;
}

} // namespace U
