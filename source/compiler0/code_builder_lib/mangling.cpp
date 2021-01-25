#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"

#include "class.hpp"
#include "enum.hpp"
#include "template_types.hpp"
#include "mangling.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

namespace
{

char Base36Digit( const size_t value )
{
	U_ASSERT( value < 36u );
	if( value < 10 )
		return char('0' + value);
	else
		return char('A' + ( value - 10 ) );
}

class ManglerState
{
private:
	using LenType = uint16_t;

public:
	void PushName( const char c )
	{
		result_name_full_.push_back( c );
		result_name_compressed_.push_back( c );
	}

	void PushName( const std::string_view name )
	{
		result_name_full_+= name;
		result_name_compressed_+= name;
	}

	std::string TakeResult()
	{
		return std::move(result_name_compressed_);
	}

public:
	class NodeHolder
	{
	public:
		explicit NodeHolder( ManglerState& state )
			: state_(state), start_(state.GetCurrentPos()), compressed_start_(state.GetCurrentCompressedPos())
		{}

		~NodeHolder()
		{
			state_.FinalizePart( start_, compressed_start_ );
		}

	private:
		ManglerState& state_;
		const LenType start_;
		const LenType compressed_start_;
	};

private:
	LenType GetCurrentPos() const
	{
		return LenType( result_name_full_.size() );
	}

	LenType GetCurrentCompressedPos() const
	{
		return LenType( result_name_compressed_.size() );
	}

	void FinalizePart( const LenType start, const LenType compressed_start )
	{
		U_ASSERT( start <= result_name_full_.size() );
		const auto size= LenType( result_name_full_.size() - start );
		if( size == 0 )
			return;
		U_ASSERT( compressed_start <= result_name_compressed_.size() );

		const std::string_view current_part= std::string_view(result_name_full_).substr( start, size );

		// Search for replacement.
		for( size_t i= 0; i < substitutions_.size(); ++i )
		{
			const Substitution& part= substitutions_[i];
			const std::string_view prev_part= std::string_view(result_name_full_).substr( part.start, part.size );
			if( prev_part == current_part )
			{
				result_name_compressed_.resize( compressed_start );
				result_name_compressed_.push_back( 'S' );

				if( i > 0u )
				{
					size_t n= i - 1u;
					if( n < 36 )
						result_name_compressed_.push_back( Base36Digit( n ) );
					else if( n < 36 * 36 )
					{
						result_name_compressed_.push_back( Base36Digit( n / 36 ) );
						result_name_compressed_.push_back( Base36Digit( n % 36 ) );
					}
					else if( n < 36 * 36 * 36 )
					{
						result_name_compressed_.push_back( Base36Digit( n / ( 36 * 36 ) ) );
						result_name_compressed_.push_back( Base36Digit( n / 36 % 36 ) );
						result_name_compressed_.push_back( Base36Digit( n % 36 ) );
					}
					else U_ASSERT(false); // TODO
				}
				result_name_compressed_.push_back( '_' );
				return;
			}
		}

		// Not found replacement - add new part.
		substitutions_.push_back( Substitution{ start, size } );
	}

private:
	struct Substitution
	{
		LenType start;
		LenType size;
	};

private:
	std::vector<Substitution> substitutions_;
	std::string result_name_full_;
	std::string result_name_compressed_;
};

void GetTypeName( ManglerState& mangler_state, const Type& type );
void GetNamespacePrefix_r( ManglerState& mangler_state, const NamesScope& names_scope );

void EncodeTemplateArgs( ManglerState& mangler_state, const TemplateArgs& template_args )
{
	mangler_state.PushName( "I" );

	for( const TemplateArg& template_arg : template_args )
	{
		if( const auto type= std::get_if<Type>( &template_arg ) )
			GetTypeName( mangler_state, *type );
		else if( const auto variable= std::get_if<Variable>( &template_arg ) )
		{
			mangler_state.PushName( "L" );

			GetTypeName( mangler_state, variable->type );

			bool is_signed= false;
			if( const auto fundamental_type= variable->type.GetFundamentalType() )
				is_signed= IsSignedInteger( fundamental_type->fundamental_type );
			else if( const auto enum_type= variable->type.GetEnumType() )
				is_signed= IsSignedInteger( enum_type->underlaying_type.fundamental_type );
			else U_ASSERT(false);

			U_ASSERT( variable->constexpr_value != nullptr );
			const llvm::APInt param_value= variable->constexpr_value->getUniqueInteger();
			if( is_signed )
			{
				const int64_t value_signed= param_value.getSExtValue();
				if( value_signed >= 0 )
					mangler_state.PushName( std::to_string( value_signed ) );
				else
				{
					mangler_state.PushName( "n" );
					mangler_state.PushName( std::to_string( -value_signed ) );
				}
			}
			else
				mangler_state.PushName( std::to_string( param_value.getZExtValue() ) );

			mangler_state.PushName( "E" );
		}
		else U_ASSERT(false);
	}

	mangler_state.PushName( "E" );
}

void GetTemplateClassName( ManglerState& mangler_state, const Class& the_class )
{
	U_ASSERT( the_class.base_template != std::nullopt );

	{
		ManglerState::NodeHolder name_node( mangler_state );

		// Skip template parameters namespace.
		U_ASSERT( the_class.members.GetParent() != nullptr );
		if( const auto parent= the_class.members.GetParent()->GetParent() )
			if( !parent->GetThisNamespaceName().empty() )
				GetNamespacePrefix_r( mangler_state, *parent );

		const std::string& class_name= the_class.base_template->class_template->syntax_element->name_;
		mangler_state.PushName( std::to_string( class_name.size() ) );
		mangler_state.PushName( class_name );
	}

	EncodeTemplateArgs( mangler_state, the_class.base_template->signature_args );
}

void GetNamespacePrefix_r( ManglerState& mangler_state, const NamesScope& names_scope )
{
	const std::string& name= names_scope.GetThisNamespaceName();
	if( name == Class::c_template_class_name )
	{
		// Assume, that "names_scope" is field "members" of "Class".
		const auto names_scope_address= reinterpret_cast<const std::byte*>(&names_scope);
		//const auto& the_class= *reinterpret_cast<const Class*>( names_scope_address - offsetof(Class, members) );
		const auto& the_class= *reinterpret_cast<const Class*>( names_scope_address - 0 );
		if( the_class.base_template != std::nullopt )
		{
			ManglerState::NodeHolder result_node( mangler_state );

			GetTemplateClassName( mangler_state, the_class );
			return;
		}
	}

	ManglerState::NodeHolder result_node( mangler_state );

	if( const auto parent= names_scope.GetParent() )
		if( !parent->GetThisNamespaceName().empty() )
			GetNamespacePrefix_r( mangler_state, *parent );

	mangler_state.PushName( std::to_string( name.size() ) );
	mangler_state.PushName( name );
}

void GetNestedName( ManglerState& mangler_state, const std::string& name, const NamesScope& parent_scope )
{
	ManglerState::NodeHolder result_node( mangler_state );

	const std::string num_prefix= std::to_string( name.size() );
	if( parent_scope.GetParent() != nullptr )
	{
		mangler_state.PushName( "N" );
		GetNamespacePrefix_r( mangler_state, parent_scope );
		mangler_state.PushName( num_prefix );
		mangler_state.PushName( name );
		mangler_state.PushName( "E" );
	}
	else
	{
		mangler_state.PushName( num_prefix );
		mangler_state.PushName( name );
	}
}

std::string_view EncodeFundamentalType( const U_FundamentalType t )
{
	switch( t )
	{
	case U_FundamentalType::InvalidType:
	case U_FundamentalType::LastType:
		return "";
	case U_FundamentalType::Void: return "v";
	case U_FundamentalType::Bool: return "b";
	case U_FundamentalType:: i8: return "a"; // C++ signed char
	case U_FundamentalType:: u8: return "h"; // C++ unsigned char
	case U_FundamentalType::i16: return "s";
	case U_FundamentalType::u16: return "t";
	case U_FundamentalType::i32: return "i";
	case U_FundamentalType::u32: return "j";
	case U_FundamentalType::i64: return "x";
	case U_FundamentalType::u64: return "y";
	case U_FundamentalType::i128: return "n";
	case U_FundamentalType::u128: return "o";
	case U_FundamentalType::f32: return "f";
	case U_FundamentalType::f64: return "d";
	case U_FundamentalType::char8 : return "c"; // C++ char
	case U_FundamentalType::char16: return "Ds"; // C++ char16_t
	case U_FundamentalType::char32: return "Di"; // C++ char32_t
	};

	U_ASSERT(false);
	return "";
}

void GetParamName( ManglerState& mangler_state, const Function::Arg& param )
{
	if( param.is_reference )
	{
		ManglerState::NodeHolder ref_node( mangler_state );
		mangler_state.PushName( "R" );
		if( param.is_mutable )
			GetTypeName( mangler_state, param.type );
		else
		{
			ManglerState::NodeHolder konst_node( mangler_state );
			mangler_state.PushName( "K" );
			GetTypeName( mangler_state, param.type );
		}
	}
	else
		GetTypeName( mangler_state, param.type );
}

void GetTypeName( ManglerState& mangler_state, const Type& type )
{
	if( const auto fundamental_type= type.GetFundamentalType() )
		mangler_state.PushName( EncodeFundamentalType( fundamental_type->fundamental_type ) );
	else if( const auto array_type= type.GetArrayType() )
	{
		ManglerState::NodeHolder result_node( mangler_state );
		mangler_state.PushName( "A" );
		mangler_state.PushName( std::to_string( array_type->size ) );
		mangler_state.PushName( "_" );
		GetTypeName( mangler_state, array_type->type );
	}
	else if( const auto tuple_type= type.GetTupleType() )
	{
		// Encode tuples, like type templates.
		ManglerState::NodeHolder result_node( mangler_state );
		{
			ManglerState::NodeHolder name_node( mangler_state );

			const std::string& keyword= Keyword( Keywords::tup_ );
			mangler_state.PushName( std::to_string(keyword.size()) );
			mangler_state.PushName( keyword );
		}

		mangler_state.PushName( "I" );
		for( const Type& element_type : tuple_type->elements )
			GetTypeName( mangler_state, element_type );
		mangler_state.PushName( "E" );
	}
	else if( const auto class_type= type.GetClassType() )
	{
		if( class_type->typeinfo_type != std::nullopt )
		{
			ManglerState::NodeHolder result_node( mangler_state );
			{
				ManglerState::NodeHolder name_node( mangler_state );

				const std::string& class_name= class_type->members.GetThisNamespaceName();
				mangler_state.PushName( std::to_string( class_name.size() ) );
				mangler_state.PushName( class_name );
			}
			{
				ManglerState::NodeHolder args_node( mangler_state );

				TemplateArgs typeinfo_pseudo_args;
				typeinfo_pseudo_args.push_back( *class_type->typeinfo_type );
				EncodeTemplateArgs( mangler_state, typeinfo_pseudo_args );
			}
		}
		else if( class_type->base_template != std::nullopt )
		{
			ManglerState::NodeHolder result_node( mangler_state );
			if( class_type->base_template->class_template->parent_namespace->GetParent() != nullptr )
			{
				mangler_state.PushName( "N" );
				GetTemplateClassName( mangler_state, *class_type );
				mangler_state.PushName( "E" );
			}
			else
				GetTemplateClassName( mangler_state, *class_type );
		}
		else
			GetNestedName( mangler_state, class_type->members.GetThisNamespaceName(), *class_type->members.GetParent() );
	}
	else if( const auto enum_type= type.GetEnumType() )
		GetNestedName( mangler_state, enum_type->members.GetThisNamespaceName(), *enum_type->members.GetParent() );
	else if( const auto raw_pointer= type.GetRawPointerType() )
	{
		ManglerState::NodeHolder result_node( mangler_state );
		mangler_state.PushName( "P" );
		GetTypeName( mangler_state, raw_pointer->type );
	}
	else if( const auto function_pointer= type.GetFunctionPointerType() )
	{
		ManglerState::NodeHolder result_node( mangler_state );
		mangler_state.PushName( "P" );
		GetTypeName( mangler_state, function_pointer->function );
	}
	else if( const auto function= type.GetFunctionType() )
	{
		ManglerState::NodeHolder function_node( mangler_state );
		mangler_state.PushName( "F" );

		{
			Function::Arg ret;
			ret.is_mutable= function->return_value_is_mutable;
			ret.is_reference= function->return_value_is_reference;
			ret.type= function->return_type;
			GetParamName( mangler_state, ret );
		}
		if( function->args.empty() )
		{
			Function::Arg param;
			param.is_mutable= false;
			param.is_reference= false;
			param.type= FundamentalType( U_FundamentalType::Void );
			GetParamName( mangler_state, param );
		}
		for( const Function::Arg& param : function->args )
			GetParamName( mangler_state, param );

		if( !function->return_references.empty() )
		{
			ManglerState::NodeHolder rr_node( mangler_state );
			mangler_state.PushName( "_RR" );
			mangler_state.PushName( Base36Digit(function->return_references.size()) );

			for( const Function::ArgReference& arg_and_tag : function->return_references )
			{
				U_ASSERT( arg_and_tag.first  < 36u );
				U_ASSERT( arg_and_tag.second < 36u || arg_and_tag.second == Function::c_arg_reference_tag_number );

				mangler_state.PushName( Base36Digit(arg_and_tag.first) );
				mangler_state.PushName(
					arg_and_tag.second == Function::c_arg_reference_tag_number
					? '_'
					: Base36Digit(arg_and_tag.second) );
			}
		}
		if( !function->references_pollution.empty() )
		{
			ManglerState::NodeHolder rp_node( mangler_state );
			mangler_state.PushName( "_RP" );
			U_ASSERT( function->references_pollution.size() < 36u );
			mangler_state.PushName( Base36Digit(function->references_pollution.size()) );

			for( const Function::ReferencePollution& pollution : function->references_pollution )
			{
				U_ASSERT( pollution.dst.first  < 36u );
				U_ASSERT( pollution.dst.second < 36u || pollution.dst.second == Function::c_arg_reference_tag_number );
				U_ASSERT( pollution.src.first  < 36u );
				U_ASSERT( pollution.src.second < 36u || pollution.src.second == Function::c_arg_reference_tag_number );

				mangler_state.PushName( Base36Digit(pollution.dst.first) );
				mangler_state.PushName(
					pollution.dst.second == Function::c_arg_reference_tag_number
					? '_'
					: Base36Digit(pollution.dst.second) );
				mangler_state.PushName( Base36Digit(pollution.src.first) );
				mangler_state.PushName(
					pollution.src.second == Function::c_arg_reference_tag_number
					? '_'
					: Base36Digit(pollution.src.second) );
			}
		}
		if( function->unsafe )
		{
			ManglerState::NodeHolder unsafe_node( mangler_state );
			mangler_state.PushName( "unsafe" );
		}

		mangler_state.PushName( "E" );
	}
	else U_ASSERT(false);
}

const ProgramStringMap<std::string> g_op_names
{
	{ "+", "pl" },
	{ "-", "mi" },
	{ "*", "ml" },
	{ "/", "dv" },
	{ "%", "rm" },

	{ "==", "eq" },
	{ "!=", "ne" },
	{  ">", "gt" },
	{ ">=", "ge" },
	{  "<", "lt" },
	{ "<=", "le" },

	{ "&", "an" },
	{ "|", "or" },
	{ "^", "eo" },

	{ "<<", "ls" },
	{ ">>", "rs" },

	{ "+=", "pL" },
	{ "-=", "mI" },
	{ "*=", "mL" },
	{ "/=", "dV" },
	{ "%=", "rM" },

	{ "&=", "aN" },
	{ "|=", "oR" },
	{ "^=", "eO" },

	{ "<<=", "lS" },
	{ ">>=", "rS" },

	{ "!", "nt" },
	{ "~", "co" },

	{ "=", "aS" },
	{ "++", "pp" },
	{ "--", "mm" },

	{ "()", "cl" },
	{ "[]", "ix" },
};

const std::string g_empty_op_name;

// Returns empty string if func_name is not special.
const std::string& DecodeOperator( const std::string& func_name )
{
	const auto it= g_op_names.find( func_name );
	if( it != g_op_names.end() )
		return it->second;

	return g_empty_op_name;
}

} // namespace

std::string MangleFunction(
	const NamesScope& parent_scope,
	const std::string& function_name,
	const Function& function_type,
	const TemplateArgs* const template_args )
{
	ManglerState mangler_state;

	mangler_state.PushName( "_Z" );

	std::string name_prefixed= DecodeOperator( function_name );
	if( name_prefixed.empty() )
	{
		name_prefixed= std::to_string( function_name.size() );
		name_prefixed+= function_name;
	}

	// Normally we should use "T_" instead of "S_" for referencing template parameters in function signature.
	// But without "T_" it works fine too.
	if( template_args != nullptr )
	{
		ManglerState::NodeHolder result_node( mangler_state );

		if( parent_scope.GetParent() != nullptr )
		{
			mangler_state.PushName( "N" );
			{
				ManglerState::NodeHolder name_node( mangler_state );
				GetNamespacePrefix_r( mangler_state, parent_scope );
				mangler_state.PushName( name_prefixed );
			}

			EncodeTemplateArgs( mangler_state, *template_args );
			mangler_state.PushName( "Ev" );
		}
		else
		{
			{
				ManglerState::NodeHolder name_node( mangler_state );
				mangler_state.PushName( name_prefixed );
			}

			EncodeTemplateArgs( mangler_state, *template_args );
			mangler_state.PushName( "v" );
		}
	}
	else
	{
		if( parent_scope.GetParent() != nullptr )
		{
			mangler_state.PushName( "N" );
			GetNamespacePrefix_r( mangler_state, parent_scope );
			mangler_state.PushName( name_prefixed );
			mangler_state.PushName( "E" );
		}
		else
			mangler_state.PushName( name_prefixed );
	}

	for( const Function::Arg& param : function_type.args )
		GetParamName( mangler_state, param );

	if( function_type.args.empty() )
		mangler_state.PushName( "v" );

	return mangler_state.TakeResult();
}

std::string MangleGlobalVariable( const NamesScope& parent_scope, const std::string& variable_name )
{
	// Variables inside global namespace have simple names.
	if( parent_scope.GetParent() == nullptr )
		return variable_name;

	ManglerState mangler_state;
	mangler_state.PushName( "_Z" );
	GetNestedName( mangler_state, variable_name, parent_scope );

	return mangler_state.TakeResult();
}

std::string MangleType( const Type& type )
{
	ManglerState mangler_state;
	GetTypeName( mangler_state, type );
	return mangler_state.TakeResult();
}

std::string MangleTemplateArgs( const TemplateArgs& template_parameters )
{
	ManglerState mangler_state;
	EncodeTemplateArgs( mangler_state, template_parameters );
	return mangler_state.TakeResult();
}

std::string MangleVirtualTable( const Type& type )
{
	ManglerState mangler_state;
	mangler_state.PushName( "_ZTV" );
	GetTypeName( mangler_state, type );
	return mangler_state.TakeResult();
}

} // namespace CodeBuilderPrivate

} // namespace U
