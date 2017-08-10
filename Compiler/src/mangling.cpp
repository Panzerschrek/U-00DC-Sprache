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
static bool GetNestedName( const NamesScope& parent_scope, const bool need_konst, ProgramString& result )
{
	// "N" - prefix for all names inside namespaces or classes.
	// "K" prefix for "this-call" methods with immutable "this".
	result+= "N"_SpC;
	if( need_konst )
		result+= "K"_SpC;

	const size_t size_before_scope= result.size();
	GetNamespacePrefix_r( parent_scope, result );
	if( result.size() == size_before_scope )
	{
		if( need_konst )
			result[ result.size() - 2u ]= result.back();

		result.pop_back(); // "N" prefix for namespace doesn`t need, because namespace prefix is empty.
		return false;
	}

	return true;
}

static void MangleClassImpl(
	const NamesScope& parent_scope,
	const ProgramString& class_name,
	 ProgramString& result )
{
	const bool is_nested= GetNestedName( parent_scope, false, result );

	result+= ToProgramString( std::to_string( class_name.size() ).c_str() );
	result+= class_name;

	if( is_nested ) result+= "E"_SpC;
}

static void GetTypeName_r( const Type& type, ProgramString& result )
{
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
		GetTypeName_r( (*array_type_ptr)->type, result );
	}
	else if( const ClassPtr* const class_type_ptr= boost::get<ClassPtr>( &type.one_of_type_kind ) )
	{
		U_ASSERT( *class_type_ptr != nullptr );
		const Class& class_type= **class_type_ptr;
		MangleClassImpl( *class_type.members.GetParent(), class_type.members.GetThisNamespaceName(), result );
	}
}

std::string MangleFunction(
	const NamesScope& parent_scope,
	const ProgramString& function_name,
	const Function& function_type,
	bool is_this_call_method )
{
	U_ASSERT( !( is_this_call_method && function_type.args.empty() ) );

	ProgramString result;

	// "_Z" - common prefix for all symbols.
	result+= "_Z"_SpC;

	const bool is_nested=
		GetNestedName(
			parent_scope,
			is_this_call_method && !function_type.args.front().is_mutable,
			result );

	if( function_name == Keywords::constructor_ )
	{
		// TODO
		// Itanium ABI requires at least 2  cnstructors - "C1" and "C2". We should generate both.
		result+= "C1"_SpC;
	}
	else
	{
		result+= ToProgramString( std::to_string( function_name.size() ).c_str() );
		result+= function_name;
	}

	if( is_nested ) result+= "E"_SpC;

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

		GetTypeName_r( arg.type, result );
	}
	if( arg_count == 0u )
		result+= "v"_SpC;

	return ToStdString( result );
}

std::string MangleClass(
	const NamesScope& parent_scope,
	const ProgramString& class_name )
{
	ProgramString result;
	MangleClassImpl( parent_scope, class_name, result );

	return ToStdString( result );
}

} // namespace CodeBuilderPrivate

} // namespace U
