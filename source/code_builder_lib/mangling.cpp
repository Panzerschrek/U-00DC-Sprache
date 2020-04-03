#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"

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

struct MangleGraphNode
{
	std::string prefix;
	std::vector<MangleGraphNode> childs;
	std::string postfix;
	bool cachable= true;
};

char Base36Digit( size_t value )
{
	value %= 36u;
	if( value < 10 )
		return char('0' + value);
	else
		return char('A' + ( value - 10 ) );
}

class NamesCache
{
public:
	void AddName( std::string name )
	{
		for( const std::string& candidate : names_container_ )
			if( candidate == name )
				return;
		names_container_.push_back( std::move(name) );
	}

	std::optional<std::string> GetReplacement( const std::string& name ) const
	{
		for( const std::string& candidate : names_container_ )
			if( name == candidate )
			{
				const size_t index= size_t( &candidate - names_container_.data() );

				if( index == 0u )
					return "S_";

				size_t n= index - 1u;
				std::string result;

				do // Converto to 36-base number representation.
				{
					const size_t base36_digit= n % 36u;
					n/= 36u;
					result.insert( result.begin(), Base36Digit( base36_digit ) );
				}
				while( n > 0u );

				return "S" + result + "_";
			}

		return std::nullopt;
	}

private:
	std::vector<std::string> names_container_;
};

struct NamePair
{
	std::string full;
	std::string compressed;
};

NamePair MangleGraphFinalize_r( NamesCache& names_cache, const MangleGraphNode& node )
{
	NamePair result;

	result.full+= node.prefix;
	result.compressed+= node.prefix;
	for( const MangleGraphNode& child_node : node.childs )
	{
		const NamePair child_node_result= MangleGraphFinalize_r( names_cache, child_node );
		result.full+= child_node_result.full;
		result.compressed+= child_node_result.compressed;
	}
	result.full+= node.postfix;
	result.compressed+= node.postfix;

	if( node.cachable )
	{
		if( const auto replacement = names_cache.GetReplacement( result.full ) )
			result.compressed= *replacement;
		else
			names_cache.AddName( result.full );
	}

	return result;
}

std::string MangleGraphFinalize( const MangleGraphNode& node )
{
	NamesCache names_cache;
	return MangleGraphFinalize_r( names_cache, node ).compressed;
}

MangleGraphNode GetTypeName( const Type& type );
MangleGraphNode GetNamespacePrefix_r( const NamesScope& names_scope );

MangleGraphNode EncodeTemplateParameters( const std::vector<TemplateParameter>& template_parameters )
{
	MangleGraphNode result;
	result.prefix= "I";

	for( const TemplateParameter& template_paremater : template_parameters )
	{
		if( const auto type= std::get_if<Type>( &template_paremater ) )
			result.childs.push_back( GetTypeName( *type ) );
		else if( const auto variable= std::get_if<Variable>( &template_paremater ) )
		{
			MangleGraphNode variable_param_node;
			variable_param_node.prefix= "L";
			variable_param_node.childs.push_back( GetTypeName( variable->type ) );

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
					variable_param_node.postfix= std::to_string( value_signed );
				else
					variable_param_node.postfix= "n" + std::to_string( -value_signed );
			}
			else
				variable_param_node.postfix= std::to_string( param_value.getZExtValue() );

			variable_param_node.postfix+= "E";

			variable_param_node.cachable= false;
			result.childs.push_back( std::move( variable_param_node ) );
		}
		else U_ASSERT(false);
	}

	result.postfix= "E";

	result.cachable= false;
	return result;
}

MangleGraphNode GetTemplateClassName( const Class& the_class )
{
	U_ASSERT( the_class.base_template != std::nullopt );

	MangleGraphNode name_node;
	const std::string& class_name= the_class.base_template->class_template->syntax_element->name_;
	name_node.postfix= std::to_string( class_name.size() ) + class_name;

	// Skip template parameters namespace.
	U_ASSERT( the_class.members.GetParent() != nullptr );
	if( const auto parent= the_class.members.GetParent()->GetParent() )
		if( !parent->GetThisNamespaceName().empty() )
			name_node.childs.push_back( GetNamespacePrefix_r( *parent ) );

	MangleGraphNode params_node= EncodeTemplateParameters( the_class.base_template->signature_parameters );

	MangleGraphNode result;
	result.childs.push_back( std::move( name_node ) );
	result.childs.push_back( std::move( params_node ) );
	return result;
}

MangleGraphNode GetNamespacePrefix_r( const NamesScope& names_scope )
{
	const std::string& name= names_scope.GetThisNamespaceName();
	if( name == Class::c_template_class_name )
	{
		// Assume, that "names_scope" is field "members" of "Class".
		const auto names_scope_address= reinterpret_cast<const std::byte*>(&names_scope);
		//const auto& the_class= *reinterpret_cast<const Class*>( names_scope_address - offsetof(Class, members) );
		const auto& the_class= *reinterpret_cast<const Class*>( names_scope_address - 0 );
		if( the_class.base_template != std::nullopt )
			return GetTemplateClassName( the_class );
	}

	MangleGraphNode result;

	if( const auto parent= names_scope.GetParent() )
		if( !parent->GetThisNamespaceName().empty() )
			result.childs.push_back( GetNamespacePrefix_r( *parent ) );

	result.postfix= std::to_string( name.size() ) + name;

	return result;
}

MangleGraphNode GetNestedName(
	const std::string& name,
	const NamesScope& parent_scope,
	const bool no_name_num_prefix= false )
{
	MangleGraphNode result;

	const std::string num_prefix=  no_name_num_prefix ? "" : std::to_string( name.size() );
	if( parent_scope.GetParent() != nullptr )
	{
		result.prefix= "N";
		result.childs.push_back( GetNamespacePrefix_r( parent_scope ) );
		result.postfix= num_prefix + name + "E";
	}
	else
		result.postfix= num_prefix + name;

	return result;
}

MangleGraphNode GetTypeName( const Type& type )
{
	MangleGraphNode result;

	if( const auto fundamental_type= type.GetFundamentalType() )
	{
		switch( fundamental_type->fundamental_type )
		{
		case U_FundamentalType::InvalidType: break;
		case U_FundamentalType::LastType: break;
		case U_FundamentalType::Void: result.prefix= "v"; break;
		case U_FundamentalType::Bool: result.prefix= "b"; break;
		case U_FundamentalType:: i8: result.prefix= "a"; break; // C++ signed char
		case U_FundamentalType:: u8: result.prefix= "h"; break; // C++ unsigned char
		case U_FundamentalType::i16: result.prefix= "s"; break;
		case U_FundamentalType::u16: result.prefix= "t"; break;
		case U_FundamentalType::i32: result.prefix= "i"; break;
		case U_FundamentalType::u32: result.prefix= "j"; break;
		case U_FundamentalType::i64: result.prefix= "x"; break;
		case U_FundamentalType::u64: result.prefix= "y"; break;
		case U_FundamentalType::i128: result.prefix= "n"; break;
		case U_FundamentalType::u128: result.prefix= "o"; break;
		case U_FundamentalType::f32: result.prefix= "f"; break;
		case U_FundamentalType::f64: result.prefix= "d"; break;
		case U_FundamentalType::char8 : result.prefix= "c"; break; // C++ char
		case U_FundamentalType::char16: result.prefix= "Ds"; break; // C++ char16_t
		case U_FundamentalType::char32: result.prefix= "Di"; break;  // C++ char32_t
		};

		result.cachable= false; // Do not replace names of fundamental types
	}
	else if( const auto array_type= type.GetArrayType() )
	{
		result.prefix= "A" + std::to_string( array_type->size ) + "_";
		result.childs.push_back( GetTypeName( array_type->type ) );
	}
	else if( const Tuple* const tuple_type= type.GetTupleType() )
	{
		// Encode tuples, like in "Rust".
		result.prefix= "T";

		result.childs.reserve( tuple_type->elements.size() );
		for( const Type& element_type : tuple_type->elements )
			result.childs.push_back( GetTypeName( element_type ) );

		result.postfix= "E";
	}
	else if( const auto class_type= type.GetClassType() )
	{
		if( class_type->typeinfo_type != std::nullopt )
		{
			result.prefix= class_type->members.GetThisNamespaceName();
			result.childs.push_back( GetTypeName( *class_type->typeinfo_type ) );
		}
		else if( class_type->base_template != std::nullopt )
			result= GetTemplateClassName( *class_type );
		else
			result= GetNestedName( class_type->members.GetThisNamespaceName(), *class_type->members.GetParent() );
	}
	else if( const auto enum_type= type.GetEnumType() )
	{
		result= GetNestedName( enum_type->members.GetThisNamespaceName(), *enum_type->members.GetParent() );
	}
	else if( const auto function_pointer= type.GetFunctionPointerType() )
	{
		result.prefix= "P";
		result.childs.push_back( GetTypeName( function_pointer->function ) );
	}
	else if( const auto function= type.GetFunctionType() )
	{
		std::vector<Function::Arg> signature;
		{
			Function::Arg ret;
			ret.is_mutable= function->return_value_is_mutable;
			ret.is_reference= function->return_value_is_reference;
			ret.type= function->return_type;
			signature.push_back(ret);
		}
		if( function->args.empty() )
		{
			Function::Arg arg;
			arg.is_mutable= false;
			arg.is_reference= false;
			arg.type= FundamentalType( U_FundamentalType::Void );
			signature.push_back(arg);
		}
		signature.insert( signature.end(), function->args.begin(), function->args.end() );

		result.childs.reserve( signature.size() );
		for( const Function::Arg& arg : signature )
		{
			MangleGraphNode arg_node= GetTypeName( arg.type );
			if( !arg.is_mutable && arg.is_reference ) // push "Konst" for reference immutable arguments
			{
				MangleGraphNode konst_node;
				konst_node.prefix= "K";
				konst_node.childs.push_back( std::move( arg_node ) );
				arg_node= std::move( konst_node );
			}
			if( arg.is_reference )
			{
				MangleGraphNode ref_node;
				ref_node.prefix= "R";
				ref_node.childs.push_back( std::move( arg_node ) );
				arg_node= std::move( ref_node );
			}

			result.childs.push_back( std::move(arg_node) );
		}

		if( !function->return_references.empty() )
		{
			MangleGraphNode rr_node;
			rr_node.prefix= "_RR";

			U_ASSERT( function->return_references.size() < 36u );
			rr_node.prefix.push_back( Base36Digit(function->return_references.size()) );

			for( const Function::ArgReference& arg_and_tag : function->return_references )
			{
				U_ASSERT( arg_and_tag.first  < 36u );
				U_ASSERT( arg_and_tag.second < 36u || arg_and_tag.second == Function::c_arg_reference_tag_number );

				rr_node.prefix.push_back( Base36Digit(arg_and_tag.first) );
				rr_node.prefix.push_back(
					arg_and_tag.second == Function::c_arg_reference_tag_number
					? '_'
					: Base36Digit(arg_and_tag.second) );
			}

			result.childs.push_back( std::move( rr_node ) );
		}
		if( !function->references_pollution.empty() )
		{
			MangleGraphNode rp_node;
			rp_node.prefix= "_RP";

			U_ASSERT( function->references_pollution.size() < 36u );
			rp_node.prefix.push_back( Base36Digit(function->references_pollution.size()) );

			for( const Function::ReferencePollution& pollution : function->references_pollution )
			{
				U_ASSERT( pollution.dst.first  < 36u );
				U_ASSERT( pollution.dst.second < 36u || pollution.dst.second == Function::c_arg_reference_tag_number );
				U_ASSERT( pollution.src.first  < 36u );
				U_ASSERT( pollution.src.second < 36u || pollution.src.second == Function::c_arg_reference_tag_number );

				rp_node.prefix.push_back( Base36Digit(pollution.dst.first) );
				rp_node.prefix.push_back(
					pollution.dst.second == Function::c_arg_reference_tag_number
					? '_'
					: Base36Digit(pollution.dst.second) );
				rp_node.prefix.push_back( Base36Digit(pollution.src.first) );
				rp_node.prefix.push_back(
					pollution.src.second == Function::c_arg_reference_tag_number
					? '_'
					: Base36Digit(pollution.src.second) );
				rp_node.prefix.push_back( pollution.src_is_mutable ? '1' : '0' );
			}

			result.childs.push_back( std::move( rp_node ) );
		}
		if( function->unsafe )
		{
			MangleGraphNode unsafe_node;
			unsafe_node.prefix= "unsafe";
			result.childs.push_back( std::move(unsafe_node) );
		}

		result.prefix= "F";
		result.postfix= "E";
	}
	else U_ASSERT(false);

	return result;
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
	const std::vector<TemplateParameter>* template_parameters )
{
	MangleGraphNode result;
	const std::string& operator_decoded= DecodeOperator( function_name );
	if( !operator_decoded.empty() )
		result.childs.push_back( GetNestedName( operator_decoded, parent_scope, true ) );
	else
		result.childs.push_back( GetNestedName( function_name, parent_scope ) );
	result.childs.back().cachable= false;

	// Normally we should use "T_" instead of "S_" for referencing template parameters in function signature.
	// But without "T_" it works fine too.

	if( template_parameters != nullptr )
	{
		MangleGraphNode params_node= EncodeTemplateParameters( *template_parameters );
		params_node.postfix+= "v"; // I have no idea why, but this needed.

		MangleGraphNode combined_node;
		combined_node.childs.push_back( std::move(result) );
		combined_node.childs.push_back( std::move(params_node) );

		result= std::move(combined_node);
	}

	for( const Function::Arg& arg : function_type.args )
	{
		MangleGraphNode arg_node= GetTypeName( arg.type );
		if( !arg.is_mutable && arg.is_reference ) // push "Konst" for reference immutable arguments
		{
			MangleGraphNode konst_node;
			konst_node.prefix= "K";
			konst_node.childs.push_back( std::move( arg_node ) );
			arg_node= std::move( konst_node );
		}
		if( arg.is_reference )
		{
			MangleGraphNode ref_node;
			ref_node.prefix= "R";
			ref_node.childs.push_back( std::move( arg_node ) );
			arg_node= std::move( ref_node );
		}

		result.childs.push_back( std::move(arg_node) );
	}
	if( function_type.args.empty() )
	{
		MangleGraphNode empty_args_node;
		empty_args_node.postfix= "v";
		result.childs.push_back( std::move( empty_args_node ) );
	}

	return "_Z" + MangleGraphFinalize( result );
}

std::string MangleGlobalVariable(
	const NamesScope& parent_scope,
	const std::string& variable_name )
{
	// Variables inside global namespace have simple names.
	if( parent_scope.GetParent() == nullptr )
		return variable_name;

	return "_Z" + MangleGraphFinalize( GetNestedName( variable_name, parent_scope ) );
}

std::string MangleType( const Type& type )
{
	return MangleGraphFinalize( GetTypeName( type ) );
}

} // namespace CodeBuilderPrivate

} // namespace U
