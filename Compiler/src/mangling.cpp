#include "assert.hpp"
#include "keywords.hpp"

#include "mangling.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

// TODO
// Covert unicode characters of ProgramString to names charset correctly.
// TODO
// Process scope encoding correctly.

typedef std::vector<ProgramString> NamesCache;

struct Name final
{
	ProgramString full;
	ProgramString compressed_and escaped;
};

static size_t GetRepalcement( const NamesCache& names_cache, ProgramString& name )
{
	for( const ProgramString& candidate : names_cache )
		if( name == candidate )
			return &candidate - names_cache.data();

	return std::numeric_limits<size_t>::max();
}

static void AddNameToCache( NamesCache& names_cache, ProgramString& name )
{
	for( const ProgramString& candidate : names_cache )
		if( name == candidate )
			return;

	names_cache.push_back( name );
}

static ProgramString RetReplacementString( const size_t n )
{
	if( n == 0u )
		return "S_"_SpC;
	return "S"_SpC + ToProgramString( std::to_string( n - 1u ).c_str() ) + "_"_SpC;
}

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

static ProgramString GetNestedName( const ProgramString& name, const NamesScope& parent_scope, const bool need_konst, NamesCache& names_cache, bool name_is_func_name= false )
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
		const size_t replacement_candidate= GetRepalcement( names_cache, name_combined );
		if( replacement_candidate != std::numeric_limits<size_t>::max() )
		{
			replacement_n= replacement_candidate;
			replacement_pos= size_t( &comp - result_splitted.data() );
		}

		if( !( name_is_func_name && &comp == &result_splitted.back() ) )
			AddNameToCache( names_cache, name_combined );
	}

	if( replacement_n == std::numeric_limits<size_t>::max() )
	{
		if( result_splitted.size() == 1u )
			return name_combined;
		return "N"_SpC + konst + name_combined + "E"_SpC;
	}

	if( replacement_pos + 1u == result_splitted.size() )
		return konst + RetReplacementString( replacement_n );
	else
	{
		name_combined= "N"_SpC + konst + RetReplacementString( replacement_n );
		for( size_t i= replacement_pos + 1u; i < result_splitted.size(); i++ )
			name_combined+= result_splitted[i];
		name_combined+= "E"_SpC;

		return name_combined;
	}
}

static ProgramString GetTypeName_r( const Type& type, NamesCache& names_cache )
{
	ProgramString result;

	if( const FundamentalType* const fundamental_type= boost::get<FundamentalType>( &type.one_of_type_kind ) )
	{
		switch( fundamental_type->fundamental_type )
		{
		case U_FundamentalType::InvalidType: break;
		case U_FundamentalType::LastType: break;
		case U_FundamentalType::Void: result+= "v"_SpC; break;
		case U_FundamentalType::Bool: result+= "b"_SpC; break;
		case U_FundamentalType:: i8: result+= "a"_SpC; break;
		case U_FundamentalType:: u8: result+= "h"_SpC; break;
		case U_FundamentalType::i16: result+= "s"_SpC; break;
		case U_FundamentalType::u16: result+= "t"_SpC; break;
		case U_FundamentalType::i32: result+= "i"_SpC; break;
		case U_FundamentalType::u32: result+= "j"_SpC; break;
		case U_FundamentalType::i64: result+= "x"_SpC; break;
		case U_FundamentalType::u64: result+= "y"_SpC; break;
		case U_FundamentalType::f32: result+= "f"_SpC; break;
		case U_FundamentalType::f64: result+= "d"_SpC; break;
		};
	}
	else if( const ArrayPtr* const array_type_ptr= boost::get<ArrayPtr>( &type.one_of_type_kind ) )
	{
		U_ASSERT( *array_type_ptr != nullptr );
		result+= "A"_SpC;
		result+= ToProgramString( std::to_string( (*array_type_ptr)->size ).c_str() );
		result+= GetTypeName_r( (*array_type_ptr)->type, names_cache );
	}
	else if( const ClassPtr* const class_type_ptr= boost::get<ClassPtr>( &type.one_of_type_kind ) )
	{
		U_ASSERT( *class_type_ptr != nullptr );
		const Class& class_type= **class_type_ptr;
		result+= GetNestedName( class_type.members.GetThisNamespaceName(), *class_type.members.GetParent(), false, names_cache );
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
		result+= GetNestedName( "C1"_SpC, parent_scope, false, names_cache, true );
	}
	else
		result+=
			GetNestedName(
				function_name,
				parent_scope,
				is_this_call_method && !function_type.args.front().is_mutable,
				names_cache,
				true );

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
		if( arg.is_reference )
			result+= "R"_SpC;
		if( !arg.is_mutable )
			result+= "K"_SpC;

		result+= GetTypeName_r( arg.type, names_cache );
	}
	if( arg_count == 0u )
		result+= "v"_SpC;

	return ToStdString( result );
}

std::string MangleClass(
	const NamesScope& parent_scope,
	const ProgramString& class_name )
{
	NamesCache names_cache;
	return ToStdString( GetNestedName( class_name, parent_scope, false, names_cache ) );
}

} // namespace CodeBuilderPrivate

} // namespace U
