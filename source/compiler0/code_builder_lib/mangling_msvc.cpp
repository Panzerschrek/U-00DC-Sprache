#include "../../lex_synt_lib_common/assert.hpp"
#include "enum.hpp"
#include "keywords.hpp"
#include "mangling.hpp"

namespace U
{

namespace
{

// TODO - maybe avoid hardcoding 'E' for 64-bit pointers?

constexpr size_t g_num_back_references= 10;
constexpr char g_name_prefix= '?'; // All names (function, variables) should start with it.
constexpr char g_terminator= '@';
constexpr char g_reference_mut_prefix[]= "AEA"; // 'A' for reference, 'E' for 64bit pointer, 'A' for const.
constexpr char g_reference_imut_prefix[]= "AEB"; // 'A' for reference, 'E' for 64bit pointer, 'B' for non-const.
constexpr char g_template_prefix[]= "?$";
constexpr char g_class_type_prefix = 'U';
constexpr char g_mut_flag= 'A';
constexpr char g_imut_flag= 'B';

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
		res+= g_terminator;
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

void EncodeTemplateArgs( std::string& res, ManglerState& mangler_state, const TemplateArgs& template_args );
void EncodeType( std::string& res, ManglerState& mangler_state, const Type& type );
void EncodeNamespacePostfix_r( std::string& res, ManglerState& mangler_state, const NamesScope& scope );

void EncodeTemplateClassName( std::string& res, ManglerState& mangler_state, const ClassPtr& the_class )
{
	U_ASSERT( the_class->base_template != std::nullopt );

	const TypeTemplatePtr& type_template= the_class->base_template->class_template;
	const auto namespace_containing_template= type_template->parent_namespace;

	// Use separate backreferences table.
	ManglerState template_mangler_state;

	res+= g_template_prefix;
	template_mangler_state.EncodeName( type_template->syntax_element->name_, res );
	EncodeTemplateArgs( res, template_mangler_state, the_class->base_template->signature_args );

	if( namespace_containing_template->GetParent() != nullptr )
		EncodeNamespacePostfix_r( res, mangler_state, *namespace_containing_template );
}

void EncodeNamespacePostfix_r( std::string& res, ManglerState& mangler_state, const NamesScope& scope )
{
	if( scope.GetParent() == nullptr ) // Root namespace.
		return;

	if( const ClassPtr the_class= scope.GetClass() )
	{
		if( the_class->base_template != std::nullopt )
		{
			EncodeTemplateClassName( res, mangler_state, the_class );
			return;
		}
	}

	mangler_state.EncodeName( scope.GetThisNamespaceName(), res );

	EncodeNamespacePostfix_r( res, mangler_state, *scope.GetParent() );
}

void EncodeFullName( std::string& res, ManglerState& mangler_state, const std::string_view name, const NamesScope& scope )
{
	mangler_state.EncodeName( name, res );
	EncodeNamespacePostfix_r( res, mangler_state, scope );
	// Finish list of name components.
	res+= g_terminator;
}

void EncodeNumber( std::string& res, const llvm::APInt& num, const bool is_signed )
{
	uint64_t abs_value= 0;
	if( is_signed )
	{
		const int64_t value_signed= num.getSExtValue();
		if( value_signed >= 0 )
			abs_value= value_signed;
		else
		{
			res+= "?";
			abs_value= -value_signed;
		}
	}
	else
		abs_value= num.getZExtValue();

	if( abs_value == 0 )
		res+= "A@";
	else if( abs_value <= 10 )
		res+= char(abs_value - 1 + '0');
	else
	{
		// Use hex numbers with digits in range [A;Q)
		int64_t hex_digit= 15;
		while((abs_value & (uint64_t(0xF) << (hex_digit << 2))) == 0)
			--hex_digit; // It's impossible to reach zero here since "abs_value" is non-zero.

		while(hex_digit >= 0)
		{
			res+= char('A' + ((abs_value >> (hex_digit << 2)) & 0xF));
			--hex_digit;
		}

		// Finish list of digits.
		res+= g_terminator;
	}
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
					res+= param.is_mutable ? g_reference_mut_prefix : g_reference_imut_prefix;

				EncodeType( res, mangler_state, param.type );

				if( back_references.size() < g_num_back_references )
					back_references.push_back( std::move(param_copy) );
			}
		}
	}
}

void EncodeFunctionType( std::string& res, ManglerState& mangler_state, const FunctionType& function_type )
{
	// Calling convention code
	res+= "A";

	if( function_type.return_value_is_reference )
		res+= function_type.return_value_is_mutable ? g_reference_mut_prefix : g_reference_imut_prefix;
	else if(
		function_type.return_type.GetClassType() != nullptr ||
		function_type.return_type.GetEnumType() != nullptr ||
		function_type.return_type.GetTupleType() != nullptr )
	{
		res += "?";
		res+= g_mut_flag; // Return value is mutable
	}

	EncodeType( res, mangler_state, function_type.return_type );

	EncodeFunctionParams( res, mangler_state, function_type.params );

	if( !function_type.params.empty() )
		res+= g_terminator; // Finish list of params.
	else
		res+= GetFundamentalTypeMangledName( U_FundamentalType::Void ); // In case of empty params just leave single type - "void" without terminator symbol.

	res+= "Z";

	// TODO - encode unsafe flag, return references, references pollution
}

void EncodeType( std::string& res, ManglerState& mangler_state, const Type& type )
{
	if( const auto fundamental_type= type.GetFundamentalType() )
	{
		res+= GetFundamentalTypeMangledName( fundamental_type->fundamental_type );
	}
	else if( type.GetArrayType() != nullptr )
	{
		// Process nested arrays.
		llvm::SmallVector<uint64_t, 8> dimensions;

		const Type* element_type= &type;
		while( true )
		{
			if( const auto element_type_as_array_type= element_type->GetArrayType() )
			{
				dimensions.push_back(element_type_as_array_type->size);
				element_type= &element_type_as_array_type->type;
			}
			else
				break;
		}

		res+= "Y";
		EncodeNumber( res, llvm::APInt(64, dimensions.size()), false );
		for( const uint64_t dimension_size : dimensions )
			EncodeNumber( res, llvm::APInt(54, dimension_size), false );
		EncodeType( res, mangler_state, *element_type );
	}
	else if( const auto tuple_type= type.GetTupleType() )
	{
		// Encode tuples, like type templates.
		TemplateArgs template_args;
		template_args.reserve( tuple_type->elements.size() );
		for( const Type& element : tuple_type->elements )
			template_args.push_back( element );

		// Use separate backreferences table.
		ManglerState template_mangler_state;

		res+= g_class_type_prefix;
		res+= g_template_prefix;
		template_mangler_state.EncodeName( Keyword( Keywords::tup_ ), res );
		EncodeTemplateArgs( res, template_mangler_state, template_args );
		// Finish list of dimensions.
		res+= g_terminator;
	}
	else if( const auto class_type= type.GetClassType() )
	{
		res += g_class_type_prefix;

		if( class_type->typeinfo_type != std::nullopt )
		{
			// TODO
		}
		else if( class_type->base_template != std::nullopt )
		{
			EncodeTemplateClassName( res, mangler_state, class_type );
			// Finish list of name components.
			res+= g_terminator;
		}
		else
			EncodeFullName( res, mangler_state, class_type->members->GetThisNamespaceName(), *class_type->members->GetParent() );
	}
	else if( const auto enum_type= type.GetEnumType() )
	{
		res+= "W";
		res+= "4"; // Underlaying type. Modern MSVC uses "4" for all enums independent on underlaying type.
		EncodeFullName( res, mangler_state, enum_type->members.GetThisNamespaceName(), *enum_type->members.GetParent() );
	}
	else if( const auto raw_pointer= type.GetRawPointerType() )
	{
		res+= "PEA"; // P for pointer, 'E' for 64 bit, 'A' for non-const.
		EncodeType( res, mangler_state, raw_pointer->type );
	}
	else if( const auto function_pointer= type.GetFunctionPointerType() )
	{
		res+= "P";
		res+= "6";
		EncodeFunctionType( res, mangler_state, function_pointer->function );
	}
	else if( const auto function= type.GetFunctionType() )
	{
		EncodeFunctionType( res, mangler_state, *function );
	}
	else U_ASSERT(false);
}

void EncodeTemplateArgs( std::string& res, ManglerState& mangler_state, const TemplateArgs& template_args )
{
	for( const TemplateArg& template_arg : template_args )
	{
		if( const auto type= std::get_if<Type>(&template_arg) )
		{
			if( type->GetArrayType() != nullptr )
				res+= "$$B";

			EncodeType( res, mangler_state, *type );
		}
		else if( const auto variable= std::get_if<Variable>(&template_arg) )
		{
			res+= "$0";

			bool is_signed= false;
			if( const auto fundamental_type= variable->type.GetFundamentalType() )
				is_signed= IsSignedInteger( fundamental_type->fundamental_type );
			else if( const auto enum_type= variable->type.GetEnumType() )
				is_signed= IsSignedInteger( enum_type->underlaying_type.fundamental_type );
			else U_ASSERT(false);

			U_ASSERT( variable->constexpr_value != nullptr );
			EncodeNumber( res, variable->constexpr_value->getUniqueInteger(), is_signed );
		}
		else U_ASSERT(false);
	}

	// Finish list of arguments.
	res+= g_terminator;
}

const ProgramStringMap<std::string> g_op_names
{
	{ "+", "?H" },
	{ "-", "?G" },
	{ "*", "?D" },
	{ "/", "?K" },
	{ "%", "?L" },

	{ "==", "?8" },
	{ "!=", "?9" },
	{  ">", "?O" },
	{ ">=", "?P" },
	{  "<", "?M" },
	{ "<=", "?N" },

	{ "&", "?I" },
	{ "|", "?U" },
	{ "^", "?T" },

	{ "<<", "?6" },
	{ ">>", "?5" },

	{ "+=", "?Y" },
	{ "-=", "?Z" },
	{ "*=", "?X" },
	{ "/=", "?_0" },
	{ "%=", "?_1" },

	{ "&=", "?_4" },
	{ "|=", "?_5" },
	{ "^=", "?_6" },

	{ "<<=", "?_3" },
	{ ">>=", "?_2" },

	{ "!", "?7" },
	{ "~", "?S" },

	{ "=", "?4" },
	{ "++", "?E" },
	{ "--", "?F" },

	{ "()", "?R" },
	{ "[]", "?A" },
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

std::string ManglerMSVC::MangleFunction(
	const NamesScope& parent_scope,
	const std::string& function_name,
	const FunctionType& function_type,
	const TemplateArgs* const template_args )
{
	// For class methods do not encode stuff like access labels, or methods-related stuff.
	// Just encode class methods as regular functions inside namespaces, with "this" as regular param.

	mangler_state_.Clear();

	std::string res;

	res+= g_name_prefix;

	const std::string& op_name= DecodeOperator( function_name );
	if( template_args != nullptr )
	{
		// Use separate backreferences table.
		ManglerState template_mangler_state;

		res+= g_template_prefix;
		if( !op_name.empty() )
			res+= op_name;
		else
			template_mangler_state.EncodeName( function_name, res );
		EncodeTemplateArgs( res, template_mangler_state, *template_args );
	}
	else
	{
		if( !op_name.empty() )
			res+= op_name;
		else
			mangler_state_.EncodeName( function_name, res );
	}
	EncodeNamespacePostfix_r( res, mangler_state_, parent_scope );
	// Finish list of name components.
	res+= g_terminator;

	// Access label. Use global access modifier. There is no reason to use real access modifiers for class members
	res+= "Y";

	EncodeFunctionType( res, mangler_state_, function_type );

	return res;
}

std::string ManglerMSVC::MangleGlobalVariable( const NamesScope& parent_scope, const std::string& variable_name, const Type& type, const bool is_constant )
{
	mangler_state_.Clear();

	std::string res;

	res+= g_name_prefix;
	EncodeFullName( res, mangler_state_, variable_name, parent_scope );

	res+= "3"; // Special name for global variables.
	EncodeType( res, mangler_state_, type );
	res+= is_constant ? g_imut_flag : g_mut_flag;

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

	std::string res;
	EncodeTemplateArgs( res, mangler_state_, template_args );
	return res;
}

std::string ManglerMSVC::MangleVirtualTable( const Type& type )
{
	mangler_state_.Clear();

	std::string res;
	res+= g_name_prefix;
	res+= "?_7"; // Special name for virtual functions table.
	EncodeType( res, mangler_state_, type );
	res+= "6"; // "6" for "vftable"
	res+= g_imut_flag;
	return res;
}

} // namespace

std::unique_ptr<IMangler> CreateManglerMSVC()
{
	return std::make_unique<ManglerMSVC>();
}

} // namespace U
