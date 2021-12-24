#include "../../lex_synt_lib_common/assert.hpp"
#include "mangling.hpp"

namespace U
{

namespace
{

class ManglerMSVC final : public IMangler
{
public:
	std::string MangleFunction(
		const NamesScope& parent_scope,
		const std::string& function_name,
		const FunctionType& function_type,
		const TemplateArgs* template_args ) override;
	std::string MangleGlobalVariable( const NamesScope& parent_scope, const std::string& variable_name )  override;
	std::string MangleType( const Type& type ) override;
	std::string MangleTemplateArgs( const TemplateArgs& template_args ) override;
	std::string MangleVirtualTable( const Type& type ) override;

private:
};

void EncodeNamespacePrefix_r(
	const NamesScope& scope,
	std::string& res )
{
	if( scope.GetParent() == nullptr ) // Root namespace.
		return;

	EncodeNamespacePrefix_r( *scope.GetParent(), res );

	res+= "@";
	res+= scope.GetThisNamespaceName();
}

std::string_view EncodeFundamentalType( const U_FundamentalType t )
{
	switch( t )
	{
	case U_FundamentalType::InvalidType:
	case U_FundamentalType::LastType:
		return "";
	case U_FundamentalType::Void: return "X";
	case U_FundamentalType::Bool: return "_N";
	case U_FundamentalType:: i8: return "C"; // C++ "signed char"
	case U_FundamentalType:: u8: return "E"; // C++ "unsigned char"
	case U_FundamentalType::i16: return "F"; // C++ "short"
	case U_FundamentalType::u16: return "G"; // C++ "unsigned short"
	case U_FundamentalType::i32: return "H"; // C++ "int"
	case U_FundamentalType::u32: return "I"; // C++ "unsigned short"
	case U_FundamentalType::i64: return "_J"; // C++ "int64_t"
	case U_FundamentalType::u64: return "_K"; // C++ "uuint64_t"
	case U_FundamentalType::i128: return "_L"; // C++ "__int128"
	case U_FundamentalType::u128: return "_M"; // "unsigned __int128"
	case U_FundamentalType::f32: return "M";  // C++ "float"
	case U_FundamentalType::f64: return "N"; // C++ "double"
	case U_FundamentalType::char8 : return "D"; // C++ "char"
	case U_FundamentalType::char16: return "_S"; // C++ "char16_t"
	case U_FundamentalType::char32: return "_U"; // C++ "char32_t"
	};

	U_ASSERT(false);
	return "";
}

void EncodeType( const Type& type, std::string& res )
{
	if( const auto fundamental_type= type.GetFundamentalType() )
	{
		res+= EncodeFundamentalType( fundamental_type->fundamental_type );
	}
	else if( const auto array_type= type.GetArrayType() )
	{
	}
	else if( const auto tuple_type= type.GetTupleType() )
	{
	}
	else if( const auto class_type= type.GetClassType() )
	{
	}
	else if( const auto enum_type= type.GetEnumType() )
	{
	}
	else if( const auto raw_pointer= type.GetRawPointerType() )
	{
		res+= "PEA";
		EncodeType( raw_pointer->type, res );
	}
	else if( const auto function_pointer= type.GetFunctionPointerType() )
	{
	}
	else if( const auto function= type.GetFunctionType() )
	{
	}
	else U_ASSERT(false);
}

std::string ManglerMSVC::MangleFunction(
	const NamesScope& parent_scope,
	const std::string& function_name,
	const FunctionType& function_type,
	const TemplateArgs* const template_args )
{
	(void)template_args; // TODO - use this

	std::string res;

	res+= "?";
	res+= function_name;
	if( parent_scope.GetParent() != nullptr )
	{
		res+= "@";
		EncodeNamespacePrefix_r( parent_scope, res );
	}
	res+= "@@";

	// Access label
	res+= "Y";

	// Calling convention code
	res+= "A";

	// Encode return type
	if( function_type.return_value_is_reference )
	{
		if( function_type.return_value_is_mutable )
			res+= "AEA";
		else
			res+= "AEB";
	}

	EncodeType( function_type.return_type, res );

	// Encode params
	for( const FunctionType::Param& param : function_type.params )
	{
		if( param.is_reference )
		{
			if( param.is_mutable )
				res+= "AEA";
			else
				res+= "AEB";
		}

		EncodeType( param.type, res );
	}

	if( function_type.params.empty() )
		res+= EncodeFundamentalType( U_FundamentalType::Void );
	else
		res+= "@"; // Terminate list of params in case of non-empty params list.

	// Finish name
	res+= "Z";

	return res;
}

std::string ManglerMSVC::MangleGlobalVariable( const NamesScope& parent_scope, const std::string& variable_name )
{
	std::string res;

	res+= "?";
	res+= variable_name;
	if( parent_scope.GetParent() != nullptr )
	{
		res+= "@";
		EncodeNamespacePrefix_r( parent_scope, res );
	}
	res+= "@@";

	// In Ü there is no reason to use real type for variables since we have no global variables overloading.
	// TODO - use real type anyway?
	res+= EncodeFundamentalType(U_FundamentalType::Void);

	res+= "3"; // Means "global variable"
	res+= "A"; // const/non-const flag. TODO - set this?

	return res;
}

std::string ManglerMSVC::MangleType( const Type& type )
{
	std::string res;
	EncodeType( type, res );
	return res;
}

std::string ManglerMSVC::MangleTemplateArgs( const TemplateArgs& template_args )
{
	// TODO
	(void)template_args;
	return "";
}

std::string ManglerMSVC::MangleVirtualTable( const Type& type )
{
	std::string res;
	res+= "??_7"; // "_7" is special name for virtual functions table
	EncodeType( type, res );
	res+= "6B"; // "6" for "vftable" and "B" for "const"
	return res;
}

} // namespace

std::unique_ptr<IMangler> CreateManglerMSVC()
{
	return std::make_unique<ManglerMSVC>();
}

} // namespace U
