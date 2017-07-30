#include "assert.hpp"

#include "mangling.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

// TODO
// Covert unicode characters of ProgramString to names charset correctly.

static void GetNamespacePrefix_r( const NamesScope& names_scope, ProgramString& result )
{
	const NamesScope* const parent= names_scope.GetParent();
	if( parent != nullptr )
		GetNamespacePrefix_r( *parent, result );

	const ProgramString& name= names_scope.GetThisNamespaceName();
	if( !name.empty() )
	{
		result+= ToProgramString( std::to_string( name.size() ).c_str() );
		result+= name;
	}
}

// Returns true, if name is nested.
static bool GetNestedName( const NamesScope& parent_scope, ProgramString& result )
{
	U_ASSERT( result.empty() );

	// "_Z" - common prefix for all symbols.
	// "N" - prefix for all names inside namespaces or classes.
	result+= "_ZN"_SpC;
	GetNamespacePrefix_r( parent_scope, result );
	if( result == "_ZN"_SpC )
	{
		result.pop_back(); // "N" prefix for namespace doesn`t need, because namespace prefix is empty.
		return false;
	}

	return true;
}

std::string MangleFunction(
	const NamesScope& parent_scope,
	const ProgramString& function_name,
	const Function& function_type )
{
	// TODO - add args-dependent postfix.
	U_UNUSED( function_type );

	ProgramString result;
	const bool is_nested= GetNestedName( parent_scope, result );

	result+= ToProgramString( std::to_string( function_name.size() ).c_str() );
	result+= function_name;

	if( is_nested ) result+= "E"_SpC;

	return ToStdString( result );
}

std::string MangleClass(
	const NamesScope& parent_scope,
	const ProgramString& class_name )
{
	ProgramString result;
	const bool is_nested= GetNestedName( parent_scope, result );

	result+= ToProgramString( std::to_string( class_name.size() ).c_str() );
	result+= class_name;

	if( is_nested ) result+= "E"_SpC;

	return ToStdString( result );
}

} // namespace CodeBuilderPrivate

} // namespace U
