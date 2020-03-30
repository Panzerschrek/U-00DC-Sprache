#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"

#include "class.hpp"
#include "enum.hpp"
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

class NamesCache final
{
public:
	static constexpr size_t c_no_replacement= std::numeric_limits<size_t>::max();

	void AddName( std::string name )
	{
		for( const std::string& candidate : names_container_ )
			if( candidate == name )
				return;
		names_container_.push_back( std::move(name) );
	}


	std::optional<std::string> GetReplacement( const std::string& name )
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

struct NamePair final
{
	std::string full;
	std::string compressed_and_escaped;
};

NamePair MangleGraphFinalize_r( NamesCache& names_cache, const MangleGraphNode& node )
{
	NamePair result;

	result.full+= node.prefix;
	result.compressed_and_escaped+= node.prefix;
	for( const MangleGraphNode& child_node : node.childs )
	{
		const NamePair child_node_result= MangleGraphFinalize_r( names_cache, child_node );
		result.full+= child_node_result.full;
		result.compressed_and_escaped+= child_node_result.compressed_and_escaped;
	}
	result.full+= node.postfix;
	result.compressed_and_escaped+= node.postfix;

	if( node.cachable )
	{
		if( const auto replacement = names_cache.GetReplacement( result.full ) )
			result.compressed_and_escaped= *replacement;
		else
			names_cache.AddName( result.full );
	}

	return result;
}

std::string MangleGraphFinalize( const MangleGraphNode& node )
{
	NamesCache names_cache;
	return MangleGraphFinalize_r( names_cache, node ).compressed_and_escaped;
}

MangleGraphNode GetNamespacePrefix_r( const NamesScope& names_scope )
{
	MangleGraphNode result;

	if( const auto parent= names_scope.GetParent() )
		if( !parent->GetThisNamespaceName().empty() )
			result.childs.push_back( GetNamespacePrefix_r( *parent ) );

	const std::string& name= names_scope.GetThisNamespaceName();
	result.postfix= std::to_string( name.size() ) + name;
	return result;
}

MangleGraphNode GetNestedName( const std::string& name, const NamesScope& parent_scope )
{
	MangleGraphNode result;

	if( parent_scope.GetParent() != nullptr )
	{
		result.prefix= "N";
		result.childs.push_back( GetNamespacePrefix_r( parent_scope ) );
		result.postfix= std::to_string( name.size() ) + name + "E";
	}
	else
		result.postfix= std::to_string( name.size() ) + name;

	return result;
}

MangleGraphNode GetTypeName( const Type& type )
{
	MangleGraphNode result;

	if( const FundamentalType* const fundamental_type= type.GetFundamentalType() )
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
		result.prefix= "T";

		result.childs.reserve( tuple_type->elements.size() );
		for( const Type& element_type : tuple_type->elements )
			result.childs.push_back( GetTypeName( element_type ) );

		result.postfix= "E";
	}
	else if( const Class* const class_type= type.GetClassType() )
	{
		result= GetNestedName( class_type->members.GetThisNamespaceName(), *class_type->members.GetParent() );
	}
	else if( const Enum* const enum_type= type.GetEnumType() )
	{
		result= GetNestedName( enum_type->members.GetThisNamespaceName(), *enum_type->members.GetParent() );
	}
	else if( const FunctionPointer* const function_pointer= type.GetFunctionPointerType() )
	{
		result.prefix= "P";
		result.childs.push_back( GetTypeName( function_pointer->function ) );
	}
	else if( const Function* const function= type.GetFunctionType() )
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
			// TODO
			/*
			std::string rr;
			rr+= "_RR";

			U_ASSERT( function->return_references.size() < 36u );
			rr.push_back( Base36Digit(function->return_references.size()) );

			for( const Function::ArgReference& arg_and_tag : function->return_references )
			{
				U_ASSERT( arg_and_tag.first  < 36u );
				U_ASSERT( arg_and_tag.second < 36u || arg_and_tag.second == Function::c_arg_reference_tag_number );

				rr.push_back( Base36Digit(arg_and_tag.first) );
				rr.push_back(
					arg_and_tag.second == Function::c_arg_reference_tag_number
					? '_' :
					Base36Digit(arg_and_tag.second) );
			}

			result.full+= rr;
			result.compressed_and_escaped+= rr;
			*/
		}
		if( !function->references_pollution.empty() )
		{
			// TODO
			/*
			std::string rp;
			rp+= "_RP";

			U_ASSERT( function->references_pollution.size() < 36u );
			rp.push_back( Base36Digit(function->references_pollution.size()) );

			for( const Function::ReferencePollution& pollution : function->references_pollution )
			{
				U_ASSERT( pollution.dst.first  < 36u );
				U_ASSERT( pollution.dst.second < 36u || pollution.dst.second == Function::c_arg_reference_tag_number );
				U_ASSERT( pollution.src.first  < 36u );
				U_ASSERT( pollution.src.second < 36u || pollution.src.second == Function::c_arg_reference_tag_number );

				rp.push_back( Base36Digit(pollution.dst.first) );
				rp.push_back(
					pollution.dst.second == Function::c_arg_reference_tag_number
					? '_' :
					Base36Digit(pollution.dst.second) );
				rp.push_back( Base36Digit(pollution.src.first) );
				rp.push_back(
					pollution.src.second == Function::c_arg_reference_tag_number
					? '_' :
					Base36Digit(pollution.src.second) );
				rp.push_back( pollution.src_is_mutable ? '1' : '0' );
			}

			result.full+= rp;
			result.compressed_and_escaped+= rp;
			*/
		}

		result.prefix= "F";
		result.postfix= "E";
		// TODO - unsafe
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

// Returns empty string if func_name is not operatorname.
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
	const Function& function_type )
{
	const std::string& operator_decoded= DecodeOperator( function_name );
	const std::string& real_function_name= operator_decoded.empty() ? function_name : operator_decoded;

	MangleGraphNode result;
	result.childs.push_back( GetNestedName( real_function_name, parent_scope ) );
	result.childs.back().cachable= false;

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

	MangleGraphNode result;
	result.childs.push_back( GetNestedName( variable_name, parent_scope ) );

	return "_Z" + MangleGraphFinalize( result );
}

std::string MangleType( const Type& type )
{
	return MangleGraphFinalize( GetTypeName( type ) );
}

} // namespace CodeBuilderPrivate

} // namespace U
