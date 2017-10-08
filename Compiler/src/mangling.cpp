#include "assert.hpp"
#include "keywords.hpp"

#include "mangling.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

// TODO
// Covert unicode characters of ProgramString to names charset correctly.

struct NamePair final
{
	ProgramString full;
	ProgramString compressed_and_escaped;
};

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

			if( base36_digit <= 9u )
				result.insert( result.begin(), '0' + base36_digit );
			else
				result.insert( result.begin(), 'A' + ( base36_digit - 10u ) );
		}
		while( n > 0u );

		return "S"_SpC + result + "_"_SpC;
	}

private:
	std::vector<ProgramString> names_container_;
};


static void GetNamespacePrefix_r( const NamesScope& names_scope, std::vector<ProgramString>& result )
{
	const NamesScope* const parent= names_scope.GetParent();
	if( parent != nullptr )
		GetNamespacePrefix_r( *parent, result );

	const ProgramString& name= names_scope.GetThisNamespaceName();
	if( !name.empty() )
	{
		result.push_back( ToProgramString( std::to_string( name.size() ).c_str() ) + name );
	}
}

static NamePair GetNestedName( const ProgramString& name, const NamesScope& parent_scope, const bool need_konst, NamesCache& names_cache, bool name_is_func_name= false )
{
	// "N" - prefix for all names inside namespaces or classes.
	// "K" prefix for "this-call" methods with immutable "this".
	// "E" postfix fol all names inside namespaces or classes.

	std::vector<ProgramString> result_splitted;
	GetNamespacePrefix_r( parent_scope, result_splitted );
	result_splitted.push_back( ToProgramString( std::to_string( name.size() ).c_str() ) + name );

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

static NamePair GetTypeName_r( const Type& type, NamesCache& names_cache )
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
		case U_FundamentalType:: i8: result.full= "a"_SpC; break;
		case U_FundamentalType:: u8: result.full= "h"_SpC; break;
		case U_FundamentalType::i16: result.full= "s"_SpC; break;
		case U_FundamentalType::u16: result.full= "t"_SpC; break;
		case U_FundamentalType::i32: result.full= "i"_SpC; break;
		case U_FundamentalType::u32: result.full= "j"_SpC; break;
		case U_FundamentalType::i64: result.full= "x"_SpC; break;
		case U_FundamentalType::u64: result.full= "y"_SpC; break;
		case U_FundamentalType::f32: result.full= "f"_SpC; break;
		case U_FundamentalType::f64: result.full= "d"_SpC; break;
		};
		result.compressed_and_escaped= result.full;
	}
	else if( const Array* const array_type= type.GetArrayType() )
	{
		ProgramString array_prefix= "A"_SpC + ToProgramString( std::to_string( array_type->size ).c_str() ) + "_"_SpC;

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
	else if( const ClassPtr class_type= type.GetClassType() )
	{
		result= GetNestedName( class_type->members.GetThisNamespaceName(), *class_type->members.GetParent(), false, names_cache );
	}

	return result;
}

std::string MangleFunction(
	const NamesScope& parent_scope,
	const ProgramString& function_name,
	const Function& function_type,
	bool is_this_call_method )
{
	U_ASSERT( !( is_this_call_method && function_type.args.empty() ) );

	NamesCache names_cache;
	ProgramString result;

	// "_Z" - common prefix for all symbols.
	result+= "_Z"_SpC;

	if( function_name == Keywords::constructor_ )
	{
		// TODO
		// Itanium ABI requires at least 2  cnstructors - "C1" and "C2". We should generate both.
		result+= GetNestedName( "C1"_SpC, parent_scope, false, names_cache, true ).compressed_and_escaped;
	}
	else
		result+=
			GetNestedName(
				function_name,
				parent_scope,
				is_this_call_method && !function_type.args.front().is_mutable,
				names_cache,
				true ).compressed_and_escaped;

	size_t arg_count= function_type.args.size();
	const Function::Arg* args= function_type.args.data();
	if( is_this_call_method )
	{
		arg_count--;
		args++;
	}
	for( size_t i= 0u; i < arg_count; i++ )
	{
		const Function::Arg& arg= args[i];

		NamePair type_name= GetTypeName_r( arg.type, names_cache );

		if( !arg.is_mutable )
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
	if( arg_count == 0u )
		result+= "v"_SpC;

	return ToStdString( result );
}

std::string MangleGlobalVariable(
	const NamesScope& parent_scope,
	const ProgramString& variable_name )
{
	// Variables inside global namespace have simple names.
	if( parent_scope.GetParent() == nullptr )
		return ToStdString( variable_name );

	NamesCache names_cache;
	ProgramString result= "_Z"_SpC;

	result+= GetNestedName( variable_name, parent_scope, false, names_cache ).compressed_and_escaped;

	return ToStdString( result );
}

std::string MangleType(
	const NamesScope& parent_scope,
	const ProgramString& class_name )
{
	NamesCache names_cache;
	return ToStdString( GetNestedName( class_name, parent_scope, false, names_cache ).compressed_and_escaped );
}

} // namespace CodeBuilderPrivate

} // namespace U
