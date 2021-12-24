#include "../../lex_synt_lib_common/assert.hpp"
#include "mangling.hpp"

namespace U
{

namespace
{

constexpr size_t g_num_back_references= 10;

class ManglerState
{
public:
	void EncodeName( const std::string_view str, std::string& res )
	{
		for( size_t i= 0; i < g_num_back_references; ++i )
		{
			if( back_references_[i] == str )
			{
				res+= char(i + '0');
				return;
			}
			if( back_references_[i].empty() )
			{
				// Reached empty space - fill it.
				back_references_[i].assign(str);
				break;
			}
		}

		// Not found or reached backreferences limit.
		res+= str;
		res+= "@";
	}

	void Clear()
	{
		for( std::string& back_reference : back_references_ )
			back_reference.clear();
	}

private:
	std::string back_references_[g_num_back_references];
};

class ManglerMSVC final : public IMangler
{
public:
	std::string MangleFunction(
		const NamesScope& parent_scope,
		const std::string& function_name,
		const FunctionType& function_type,
		const TemplateArgs* template_args ) override;
	std::string MangleGlobalVariable( const NamesScope& parent_scope, const std::string& variable_name, const Type& type, bool is_constant ) override;
	std::string MangleType( const Type& type ) override;
	std::string MangleTemplateArgs( const TemplateArgs& template_args ) override;
	std::string MangleVirtualTable( const Type& type ) override;

private:
	// Reuse mangler state to reduce number of allocations.
	ManglerState mangler_state_;
};

void EncodeNamespacePostfix_r( std::string& res, ManglerState& mangler_state, const NamesScope& scope )
{
	if( scope.GetParent() == nullptr ) // Root namespace.
		return;

	mangler_state.EncodeName( scope.GetThisNamespaceName(), res );

	EncodeNamespacePostfix_r( res, mangler_state, *scope.GetParent() );
}

void EncodeName( std::string& res, ManglerState& mangler_state, const std::string_view name, const NamesScope& scope )
{
	mangler_state.EncodeName( name, res );
	EncodeNamespacePostfix_r( res, mangler_state, scope );
	res+= "@";
}

std::string_view GetFundamentalTypeMangledName( const U_FundamentalType t )
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

void EncodeType( std::string& res, ManglerState& mangler_state, const Type& type )
{
	if( const auto fundamental_type= type.GetFundamentalType() )
	{
		res+= GetFundamentalTypeMangledName( fundamental_type->fundamental_type );
	}
	else if( const auto array_type= type.GetArrayType() )
	{
	}
	else if( const auto tuple_type= type.GetTupleType() )
	{
	}
	else if( const auto class_type= type.GetClassType() )
	{
		res+= "U";
		EncodeName( res, mangler_state, class_type->members->GetThisNamespaceName(), *class_type->members->GetParent() );
	}
	else if( const auto enum_type= type.GetEnumType() )
	{
	}
	else if( const auto raw_pointer= type.GetRawPointerType() )
	{
		res+= "PEA";
		EncodeType( res, mangler_state, raw_pointer->type );
	}
	else if( const auto function_pointer= type.GetFunctionPointerType() )
	{
	}
	else if( const auto function= type.GetFunctionType() )
	{
	}
	else U_ASSERT(false);
}

void EncodeFunctionParams( std::string& res, ManglerState& mangler_state, const ArgsVector<FunctionType::Param>& params )
{
	ArgsVector<FunctionType::Param> back_references;

	for( const FunctionType::Param& param : params )
	{
		if( !param.is_reference && param.type.GetFundamentalType() != nullptr )
		{
			// For trivial params (fundamentals with no reference modifiers) do not create backreferences.
			EncodeType( res, mangler_state, param.type );
		}
		else
		{
			FunctionType::Param param_copy= param;
			if( !param_copy.is_reference )
				param_copy.is_mutable= false; // We do not care about mutability modifier for value params.

			bool found = false;
			for( size_t i= 0; i < back_references.size(); ++i )
			{
				if( param_copy == back_references[i] )
				{
					res+= char(i + '0');
					found= true;
					break;
				}
			}

			if( !found )
			{
				if( param.is_reference )
				{
					if( param.is_mutable )
						res+= "AEA";
					else
						res+= "AEB";
				}

				EncodeType( res, mangler_state, param.type );

				if( back_references.size() < g_num_back_references )
					back_references.push_back( std::move(param_copy) );
			}
		}
	}
}

std::string ManglerMSVC::MangleFunction(
	const NamesScope& parent_scope,
	const std::string& function_name,
	const FunctionType& function_type,
	const TemplateArgs* const template_args )
{
	mangler_state_.Clear();

	(void)template_args; // TODO - use this

	std::string res;

	res+= "?";
	EncodeName( res, mangler_state_, function_name, parent_scope );

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
	else if( function_type.return_type.GetClassType() != nullptr )
	{
		res += "?";
		res+= "A"; // Return value is mutable
	}

	EncodeType( res, mangler_state_, function_type.return_type );

	EncodeFunctionParams( res, mangler_state_, function_type.params );

	if( function_type.params.empty() )
		res+= GetFundamentalTypeMangledName( U_FundamentalType::Void );
	else
		res+= "@"; // Terminate list of params in case of non-empty params list.

	// Finish name
	res+= "Z";

	return res;
}

std::string ManglerMSVC::MangleGlobalVariable( const NamesScope& parent_scope, const std::string& variable_name, const Type& type, const bool is_constant )
{
	mangler_state_.Clear();

	std::string res;

	res+= "?";
	EncodeName( res, mangler_state_, variable_name, parent_scope );

	res+= "3"; // Means "global variable"
	EncodeType( res, mangler_state_, type );
	res+= is_constant ? "B" : "A";

	return res;
}

std::string ManglerMSVC::MangleType( const Type& type )
{
	mangler_state_.Clear();

	std::string res;
	EncodeType( res, mangler_state_, type );
	return res;
}

std::string ManglerMSVC::MangleTemplateArgs( const TemplateArgs& template_args )
{
	mangler_state_.Clear();

	// TODO
	(void)template_args;
	return "";
}

std::string ManglerMSVC::MangleVirtualTable( const Type& type )
{
	mangler_state_.Clear();

	std::string res;
	res+= "??_7"; // "_7" is special name for virtual functions table
	EncodeType( res, mangler_state_, type );
	res+= "6B"; // "6" for "vftable" and "B" for "const"
	return res;
}

} // namespace

std::unique_ptr<IMangler> CreateManglerMSVC()
{
	return std::make_unique<ManglerMSVC>();
}

} // namespace U
