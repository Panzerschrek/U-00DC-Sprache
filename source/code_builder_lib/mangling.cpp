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

// TODO
// Covert unicode characters of ProgramString to names charset correctly.

struct NamePair final
{
	ProgramString full;
	ProgramString compressed_and_escaped;
};

sprache_char Base36Digit( size_t value )
{
	value %= 36u;
	if( value < 10 )
		return sprache_char('0' + value);
	else
		return sprache_char('A' + ( value - 10 ) );
}

class NamesCache final
{
public:
	size_t GetRepalcement( const ProgramString& name )
	{
		for( const ProgramString& candidate : names_container_ )
			if( name == candidate )
				return size_t(&candidate - names_container_.data());

		return std::numeric_limits<size_t>::max();
	}

	void AddName( ProgramString name )
	{
		for( const ProgramString& candidate : names_container_ )
			if( candidate == name )
				return;
		names_container_.push_back( std::move(name) );
	}

	ProgramString RetReplacementString( size_t n )
	{
		U_ASSERT( n < names_container_.size() );
		if( n == 0u )
			return "S_"_SpC;

		--n;
		ProgramString result;

		do // Converto to 36-base number representation.
		{
			const size_t base36_digit= n % 36u;
			n/= 36u;
			result.insert( result.begin(), Base36Digit( base36_digit ) );
		}
		while( n > 0u );

		return "S"_SpC + result + "_"_SpC;
	}

private:
	std::vector<ProgramString> names_container_;
};

// Returns empty string if func_name is not operatorname.
const ProgramString& DecodeOperator( const ProgramString& func_name )
{
	static const ProgramStringMap<ProgramString> c_op_names
	{
		{ "+"_SpC, "pl"_SpC },
		{ "-"_SpC, "mi"_SpC },
		{ "*"_SpC, "ml"_SpC },
		{ "/"_SpC, "dv"_SpC },
		{ "%"_SpC, "rm"_SpC },

		{ "=="_SpC, "eq"_SpC },
		{ "!="_SpC, "ne"_SpC },
		{  ">"_SpC, "gt"_SpC },
		{ ">="_SpC, "ge"_SpC },
		{  "<"_SpC, "lt"_SpC },
		{ "<="_SpC, "le"_SpC },

		{ "&"_SpC, "an"_SpC },
		{ "|"_SpC, "or"_SpC },
		{ "^"_SpC, "eo"_SpC },

		{ "<<"_SpC, "ls"_SpC },
		{ ">>"_SpC, "rs"_SpC },

		{ "+="_SpC, "pL"_SpC },
		{ "-="_SpC, "mI"_SpC },
		{ "*="_SpC, "mL"_SpC },
		{ "/="_SpC, "dV"_SpC },
		{ "%="_SpC, "rM"_SpC },

		{ "&="_SpC, "aN"_SpC },
		{ "|="_SpC, "oR"_SpC },
		{ "^="_SpC, "eO"_SpC },

		{ "<<="_SpC, "lS"_SpC },
		{ ">>="_SpC, "rS"_SpC },

		{ "!"_SpC, "nt"_SpC },
		{ "~"_SpC, "co"_SpC },

		{ "="_SpC, "aS"_SpC },
		{ "++"_SpC, "pp"_SpC },
		{ "--"_SpC, "mm"_SpC },

		{ "()"_SpC, "cl"_SpC },
		{ "[]"_SpC, "ix"_SpC },
	};
	static const ProgramString c_empty;

	const auto it= c_op_names.find( func_name );
	if( it != c_op_names.end() )
		return it->second;

	return c_empty;
}

void GetNamespacePrefix_r( const NamesScope& names_scope, std::vector<ProgramString>& result )
{
	const NamesScope* const parent= names_scope.GetParent();
	if( parent != nullptr )
		GetNamespacePrefix_r( *parent, result );

	const ProgramString& name= names_scope.GetThisNamespaceName();
	if( !name.empty() )
	{
		result.push_back( ToProgramString( std::to_string( name.size() ) ) + name );
	}
}

NamePair GetNestedName(
	const ProgramString& name, bool name_needs_num_prefix,
	const NamesScope& parent_scope, const bool need_konst, NamesCache& names_cache, bool name_is_func_name= false )
{
	// "N" - prefix for all names inside namespaces or classes.
	// "K" prefix for "this-call" methods with immutable "this".
	// "E" postfix fol all names inside namespaces or classes.

	std::vector<ProgramString> result_splitted;
	GetNamespacePrefix_r( parent_scope, result_splitted );
	if( name_needs_num_prefix )
		result_splitted.push_back( ToProgramString( std::to_string( name.size() ) ) + name );
	else
		result_splitted.push_back( name );

	ProgramString name_combined;
	const ProgramString konst= need_konst ? "K"_SpC : ""_SpC;

	size_t replacement_n= std::numeric_limits<size_t>::max();
	size_t replacement_pos= 0u;
	for( const ProgramString& comp : result_splitted )
	{
		name_combined+= comp;
		const size_t replacement_candidate= names_cache.GetRepalcement(name_combined );
		if( replacement_candidate != std::numeric_limits<size_t>::max() )
		{
			replacement_n= replacement_candidate;
			replacement_pos= size_t( &comp - result_splitted.data() );
		}

		if( !( name_is_func_name && &comp == &result_splitted.back() ) )
			names_cache.AddName( name_combined );
	}

	if( replacement_n == std::numeric_limits<size_t>::max() )
	{
		if( result_splitted.size() == 1u )
			return NamePair{ name_combined, name_combined };
		return NamePair{ konst + name_combined, "N"_SpC + konst + name_combined + "E"_SpC };
	}

	if( replacement_pos + 1u == result_splitted.size() )
		return NamePair{ konst + name_combined, konst + names_cache.RetReplacementString( replacement_n ) };
	else
	{
		ProgramString compressed_name;
		compressed_name= "N"_SpC + konst + names_cache.RetReplacementString( replacement_n );
		for( size_t i= replacement_pos + 1u; i < result_splitted.size(); i++ )
			compressed_name+= result_splitted[i];
		compressed_name+= "E"_SpC;

		return NamePair{ name_combined, compressed_name };
	}
}

NamePair GetTypeName_r( const Type& type, NamesCache& names_cache )
{
	NamePair result;

	if( const FundamentalType* const fundamental_type= type.GetFundamentalType() )
	{
		switch( fundamental_type->fundamental_type )
		{
		case U_FundamentalType::InvalidType: break;
		case U_FundamentalType::LastType: break;
		case U_FundamentalType::Void: result.full= "v"_SpC; break;
		case U_FundamentalType::Bool: result.full= "b"_SpC; break;
		case U_FundamentalType:: i8: result.full= "a"_SpC; break; // C++ signed char
		case U_FundamentalType:: u8: result.full= "h"_SpC; break; // C++ unsigned char
		case U_FundamentalType::i16: result.full= "s"_SpC; break;
		case U_FundamentalType::u16: result.full= "t"_SpC; break;
		case U_FundamentalType::i32: result.full= "i"_SpC; break;
		case U_FundamentalType::u32: result.full= "j"_SpC; break;
		case U_FundamentalType::i64: result.full= "x"_SpC; break;
		case U_FundamentalType::u64: result.full= "y"_SpC; break;
		case U_FundamentalType::i128: result.full= "n"_SpC; break;
		case U_FundamentalType::u128: result.full= "o"_SpC; break;
		case U_FundamentalType::f32: result.full= "f"_SpC; break;
		case U_FundamentalType::f64: result.full= "d"_SpC; break;
		case U_FundamentalType::char8 : result.full= "c"_SpC; break; // C++ char
		case U_FundamentalType::char16: result.full= "Ds"_SpC; break; // C++ char16_t
		case U_FundamentalType::char32: result.full= "Di"_SpC; break;  // C++ char32_t
		};
		result.compressed_and_escaped= result.full;
	}
	else if( const Array* const array_type= type.GetArrayType() )
	{
		ProgramString array_prefix= "A"_SpC + ToProgramString( std::to_string( array_type->size ) ) + "_"_SpC;

		const NamePair type_name= GetTypeName_r( array_type->type, names_cache );
		result.full= array_prefix + type_name.full;

		const size_t replacement_candidate= names_cache.GetRepalcement( result.full );
		if( replacement_candidate != std::numeric_limits<size_t>::max() )
			result.compressed_and_escaped= names_cache.RetReplacementString( replacement_candidate );
		else
		{
			result.compressed_and_escaped= array_prefix + type_name.compressed_and_escaped;
			names_cache.AddName( result.full );
		}
	}
	else if( const Tuple* const tuple_type= type.GetTupleType() )
	{
		// Encode tuples, like in "Rust".
		result.full= "T"_SpC;
		for( const Type& element_type : tuple_type->elements )
		{
			const NamePair element_type_name= GetTypeName_r( element_type, names_cache );
			result.full+= element_type_name.full;
			result.compressed_and_escaped+= element_type_name.compressed_and_escaped;
		}
		result.full+= "E"_SpC;

		const size_t replacement_candidate= names_cache.GetRepalcement( result.full );
		if( replacement_candidate != std::numeric_limits<size_t>::max() )
			result.compressed_and_escaped= names_cache.RetReplacementString( replacement_candidate );
		else
			names_cache.AddName( result.full );
	}
	else if( const Class* const class_type= type.GetClassType() )
	{
		if( class_type->typeinfo_type != std::nullopt )
		{
			result.full= class_type->members.GetThisNamespaceName();
			result.compressed_and_escaped= result.full;
			const NamePair dependent_type_name= GetTypeName_r( *class_type->typeinfo_type, names_cache );
			result.full+= dependent_type_name.full;
			result.compressed_and_escaped+= dependent_type_name.compressed_and_escaped;

			const size_t replacement_candidate= names_cache.GetRepalcement( result.full );
			if( replacement_candidate != std::numeric_limits<size_t>::max() )
				result.compressed_and_escaped= names_cache.RetReplacementString( replacement_candidate );
			else
				names_cache.AddName( result.full );
		}
		else
			result= GetNestedName( class_type->members.GetThisNamespaceName(), true, *class_type->members.GetParent(), false, names_cache );
	}
	else if( const Enum* const enum_type= type.GetEnumType() )
	{
		result= GetNestedName( enum_type->members.GetThisNamespaceName(), true, *enum_type->members.GetParent(), false, names_cache );
	}
	else if( const FunctionPointer* const function_pointer= type.GetFunctionPointerType() )
	{
		const ProgramString prefix= "P"_SpC;
		const NamePair function_type_name= GetTypeName_r( function_pointer->function, names_cache );
		result.full= prefix + function_type_name.full;

		const size_t replacement_candidate= names_cache.GetRepalcement( result.full );
		if( replacement_candidate != std::numeric_limits<size_t>::max() )
			result.compressed_and_escaped= names_cache.RetReplacementString( replacement_candidate );
		else
		{
			result.compressed_and_escaped= prefix + function_type_name.compressed_and_escaped;
			names_cache.AddName( result.full );
		}
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

		for( const Function::Arg& arg : signature )
		{
			NamePair type_name= GetTypeName_r( arg.type, names_cache );
			if( !arg.is_mutable && arg.is_reference ) // push "Konst" for reference immutable arguments
			{
				const ProgramString prefix= "K"_SpC;
				const ProgramString prefixed_type_name= prefix + type_name.full;
				type_name.full= prefixed_type_name;

				const size_t replacement_candidate= names_cache.GetRepalcement( prefixed_type_name );
				if( replacement_candidate != std::numeric_limits<size_t>::max() )
					type_name.compressed_and_escaped= names_cache.RetReplacementString( replacement_candidate );
				else
				{
					type_name.compressed_and_escaped= prefix + type_name.compressed_and_escaped;
					names_cache.AddName( prefixed_type_name );
				}
			}
			if( arg.is_reference )
			{
				const ProgramString prefix= "R"_SpC;
				const ProgramString prefixed_type_name= prefix + type_name.full;
				type_name.full= prefixed_type_name;

				const size_t replacement_candidate= names_cache.GetRepalcement( prefixed_type_name );
				if( replacement_candidate != std::numeric_limits<size_t>::max() )
					type_name.compressed_and_escaped= names_cache.RetReplacementString( replacement_candidate );
				else
				{
					type_name.compressed_and_escaped= prefix + type_name.compressed_and_escaped;
					names_cache.AddName( prefixed_type_name );
				}
			}

			result.full+= type_name.full;
			result.compressed_and_escaped+= type_name.compressed_and_escaped;
		}

		if( !function->return_references.empty() )
		{
			ProgramString rr;
			rr+= "_RR"_SpC;

			U_ASSERT( function->return_references.size() < 36u );
			rr.push_back( Base36Digit(function->return_references.size()) );

			for( const Function::ArgReference& arg_and_tag : function->return_references )
			{
				U_ASSERT( arg_and_tag.first  < 36u );
				U_ASSERT( arg_and_tag.second < 36u || arg_and_tag.second == Function::c_arg_reference_tag_number );

				rr.push_back( Base36Digit(arg_and_tag.first) );
				rr.push_back(
					arg_and_tag.second == Function::c_arg_reference_tag_number
					? sprache_char('_') :
					Base36Digit(arg_and_tag.second) );
			}

			result.full+= rr;
			result.compressed_and_escaped+= rr;
		}
		if( !function->references_pollution.empty() )
		{
			ProgramString rp;
			rp+= "_RP"_SpC;

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
					? sprache_char('_') :
					Base36Digit(pollution.dst.second) );
				rp.push_back( Base36Digit(pollution.src.first) );
				rp.push_back(
					pollution.src.second == Function::c_arg_reference_tag_number
					? sprache_char('_') :
					Base36Digit(pollution.src.second) );
				rp.push_back( pollution.src_is_mutable ? '1' : '0' );
			}

			result.full+= rp;
			result.compressed_and_escaped+= rp;
		}

		const ProgramString function_prefix= "F"_SpC + (function->unsafe ? "unsafe"_SpC : ""_SpC);
		const ProgramString function_postfix= "E"_SpC;
		const ProgramString prefixed_type_name= function_prefix + result.full + function_postfix;
		result.full= prefixed_type_name;

		const size_t replacement_candidate= names_cache.GetRepalcement( prefixed_type_name );
		if( replacement_candidate != std::numeric_limits<size_t>::max() )
			result.compressed_and_escaped= names_cache.RetReplacementString( replacement_candidate );
		else
		{
			result.compressed_and_escaped= function_prefix + result.compressed_and_escaped + function_postfix;
			names_cache.AddName( prefixed_type_name );
		}
	}
	else U_ASSERT(false);

	return result;
}

} // namespace

std::string MangleFunction(
	const NamesScope& parent_scope,
	const ProgramString& function_name,
	const Function& function_type )
{
	NamesCache names_cache;
	ProgramString result;

	// "_Z" - common prefix for all symbols.
	result+= "_Z"_SpC;

	if( function_name == Keywords::constructor_ )
	{
		// Itanium ABI requires at least 2  cnstructors - "C1" and "C2".
		// 2 constructors required for virtual inheritance. Ü has no virtual inheritanse, so, second constructor does not needed.
		result+= GetNestedName( "C1"_SpC, false, parent_scope, false, names_cache, true ).compressed_and_escaped;
	}
	else if( function_name == Keywords::destructor_ )
	{
		result+= GetNestedName( "D0"_SpC, false, parent_scope, false, names_cache, true ).compressed_and_escaped;
	}
	else
	{
		const ProgramString& op_name= DecodeOperator( function_name );
		const bool is_op= !op_name.empty();

		result+=
			GetNestedName(
				is_op ? op_name : function_name,
				!is_op,
				parent_scope,
				false,
				names_cache,
				true ).compressed_and_escaped;
	}

	for( const Function::Arg& arg : function_type.args )
	{
		// We breaking some mangling rules here.
		// In C++ "this" argument skipped. In Ü "this" processed as regular argument.

		NamePair type_name= GetTypeName_r( arg.type, names_cache );

		if( !arg.is_mutable && arg.is_reference ) // push "Konst" for reference immutable arguments
		{
			const ProgramString prefix= "K"_SpC;
			const ProgramString prefixed_type_name= prefix + type_name.full;

			type_name.full= prefixed_type_name;

			const size_t replacement_candidate= names_cache.GetRepalcement( prefixed_type_name );
			if( replacement_candidate != std::numeric_limits<size_t>::max() )
				type_name.compressed_and_escaped= names_cache.RetReplacementString( replacement_candidate );
			else
			{
				type_name.compressed_and_escaped= prefix + type_name.compressed_and_escaped;
				names_cache.AddName( prefixed_type_name );
			}
		}
		if( arg.is_reference )
		{
			const ProgramString prefix= "R"_SpC;
			const ProgramString prefixed_type_name= prefix + type_name.full;

			type_name.full= prefixed_type_name;

			const size_t replacement_candidate= names_cache.GetRepalcement( prefixed_type_name );
			if( replacement_candidate != std::numeric_limits<size_t>::max() )
				type_name.compressed_and_escaped= names_cache.RetReplacementString( replacement_candidate );
			else
			{
				type_name.compressed_and_escaped= prefix + type_name.compressed_and_escaped;
				names_cache.AddName( prefixed_type_name );
			}
		}

		result+= type_name.compressed_and_escaped;
	}
	if( function_type.args.empty() )
		result+= "v"_SpC;

	return ToUTF8( result );
}

std::string MangleGlobalVariable(
	const NamesScope& parent_scope,
	const ProgramString& variable_name )
{
	// Variables inside global namespace have simple names.
	if( parent_scope.GetParent() == nullptr )
		return ToUTF8( variable_name );

	NamesCache names_cache;
	ProgramString result= "_Z"_SpC;

	result+= GetNestedName( variable_name, true, parent_scope, false, names_cache ).compressed_and_escaped;

	return ToUTF8( result );
}

std::string MangleType( const Type& type )
{
	NamesCache names_cache;
	return ToUTF8( GetTypeName_r( type, names_cache ).compressed_and_escaped );
}

} // namespace CodeBuilderPrivate

} // namespace U
