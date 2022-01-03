#include "../../lex_synt_lib_common/assert.hpp"
#include "enum.hpp"
#include "keywords.hpp"
#include "mangling.hpp"

namespace U
{

namespace
{

constexpr size_t g_num_back_references= 10;
constexpr char g_name_prefix= '?'; // All names (function, variables) should start with it.
constexpr char g_terminator= '@';
constexpr char g_template_prefix[]= "?$";
constexpr char g_numeric_template_arg_prefix[]= "$0";
constexpr char g_class_type_prefix = 'U';
constexpr char g_reference_prefix = 'A';
constexpr char g_pointer_prefix = 'P';
constexpr char g_mut_prefix= 'A';
constexpr char g_imut_prefix= 'B';

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

// Use special class for backreferences table tracking and usage.
class ManglerState
{
public:
	ManglerState( std::string& res )
		: res_(res)
	{}

	ManglerState( const ManglerState& )= delete;
	ManglerState& operator=( const ManglerState& )= delete;

	// Push name and possible create or use backreferences.
	void EncodeName( const std::string_view str )
	{
		EncodeNameImpl( str, true );
	}

	void EncodeNameNoTerminator( const std::string_view str )
	{
		EncodeNameImpl( str, false );
	}

	// Push non-name element (no need to create backreferences for it).
	void PushElement( const std::string_view str )
	{
		res_+= str;
	}

	void PushElement( const char c )
	{
		res_+= c;
	}

private:
	void EncodeNameImpl( const std::string_view str, const bool use_terminator )
	{
		for( size_t i= 0; i < g_num_back_references; ++i )
		{
			BackReference& br= back_references_[i];
			if( std::string_view(res_).substr( br.pos, br.count ) == str )
			{
				res_+= char(i + '0');
				return;
			}
			if( br.count == 0 )
			{
				// Reached empty space - fill it.
				br.pos= uint32_t(res_.size());
				br.count= uint32_t(str.size());
				break;
			}
		}

		// Not found or reached backreferences limit.
		res_+= str;
		if( use_terminator )
			res_+= g_terminator;
	}

private:
	// Store backreference as position and length in destination string.
	// Do not use pointers (or std::string_view) because it's not possible because container may be reallocated.
	struct BackReference{ uint32_t pos= 0; uint32_t count= 0; };

private:
	BackReference back_references_[g_num_back_references];
	std::string& res_;
};

class ManglerMSVC final : public IMangler
{
public:
	explicit ManglerMSVC(bool is_32_bit);

public: // IMangler
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
	void EncodeType( ManglerState& mangler_state, const Type& type ) const;
	void EncodeFunctionType( ManglerState& mangler_state, const FunctionType& function_type, bool encode_full_type ) const;
	void EncodeFunctionParams( ManglerState& mangler_state, const ArgsVector<FunctionType::Param>& params ) const;
	void EncodeTemplateArgs( ManglerState& mangler_state, const TemplateArgs& template_args ) const;
	void EncodeFullName( ManglerState& mangler_state, const std::string_view name, const NamesScope& scope ) const;
	void EncodeNamespacePostfix_r( ManglerState& mangler_state, const NamesScope& scope ) const;
	void EncodeTemplateClassName( ManglerState& mangler_state, const ClassPtr& the_class ) const;
	void EncodeNumber( ManglerState& mangler_state, const llvm::APInt& num, bool is_signed ) const;

private:
	const std::string pointer_types_modifier_;
	// Reuse mangler state to reduce number of allocations.
};

ManglerMSVC::ManglerMSVC(const bool is_32_bit)
	: pointer_types_modifier_(is_32_bit ? "" : "E")
{}

std::string ManglerMSVC::MangleFunction(
	const NamesScope& parent_scope,
	const std::string& function_name,
	const FunctionType& function_type,
	const TemplateArgs* const template_args )
{
	// For class methods do not encode stuff like access labels, or methods-related stuff.
	// Just encode class methods as regular functions inside namespaces, with "this" as regular param.

	std::string res;
	ManglerState mangler_state( res );

	mangler_state.PushElement( g_name_prefix );

	const std::string& op_name= DecodeOperator( function_name );
	if( template_args != nullptr )
	{
		// Use separate backreferences table.
		std::string template_name;
		ManglerState template_mangler_state( template_name );
		{
			template_mangler_state.PushElement( g_template_prefix );
			if( !op_name.empty() )
				template_mangler_state.PushElement( op_name );
			else
				template_mangler_state.EncodeName( function_name );
			EncodeTemplateArgs( template_mangler_state, *template_args );
		}
		// Do not create backreference for template name, just push template name instead.
		mangler_state.PushElement( template_name );
	}
	else
	{
		if( !op_name.empty() )
			mangler_state.PushElement( op_name );
		else
			mangler_state.EncodeName( function_name );
	}
	EncodeNamespacePostfix_r( mangler_state, parent_scope );
	// Finish list of name components.
	mangler_state.PushElement( g_terminator );

	// Access label. Use global access modifier. There is no reason to use real access modifiers for class members
	mangler_state.PushElement( "Y" );

	// No need to encode full function type, like "unsafe" flag or return references/references pollution,
	// since it's not possible to overload function unsing only such data.
	const bool encode_full_function_type= false;
	EncodeFunctionType( mangler_state, function_type, encode_full_function_type );

	return res;
}

std::string ManglerMSVC::MangleGlobalVariable( const NamesScope& parent_scope, const std::string& variable_name, const Type& type, const bool is_constant )
{
	std::string res;
	ManglerState mangler_state( res );

	mangler_state.PushElement( g_name_prefix );
	EncodeFullName( mangler_state, variable_name, parent_scope );

	mangler_state.PushElement( "3" ); // Special name for global variables.
	EncodeType( mangler_state, type );
	mangler_state.PushElement( is_constant ? g_imut_prefix : g_mut_prefix );

	return res;
}

std::string ManglerMSVC::MangleType( const Type& type )
{
	std::string res;
	ManglerState mangler_state( res );
	EncodeType( mangler_state, type );
	return res;
}

std::string ManglerMSVC::MangleTemplateArgs( const TemplateArgs& template_args )
{
	std::string res;
	ManglerState mangler_state( res );
	EncodeTemplateArgs( mangler_state, template_args );
	return res;
}

std::string ManglerMSVC::MangleVirtualTable( const Type& type )
{
	std::string res;
	ManglerState mangler_state( res );
	mangler_state.PushElement( g_name_prefix );
	mangler_state.PushElement( "?_7" ); // Special name for virtual functions table.
	EncodeNamespacePostfix_r( mangler_state, *type.GetClassType()->members );
	mangler_state.PushElement( g_terminator ); // Finish list of name components
	mangler_state.PushElement( "6" ); // "6" for "vftable"
	mangler_state.PushElement( g_imut_prefix );
	mangler_state.PushElement( g_terminator );
	return res;
}

void ManglerMSVC::EncodeType( ManglerState& mangler_state, const Type& type ) const
{
	if( const auto fundamental_type= type.GetFundamentalType() )
		mangler_state.PushElement( GetFundamentalTypeMangledName( fundamental_type->fundamental_type ) );
	else if( type.GetArrayType() != nullptr )
	{
		// Process nested arrays.
		llvm::SmallVector<uint64_t, 8> dimensions;

		const Type* element_type= &type;
		while( true )
		{
			if( const auto element_type_as_array_type= element_type->GetArrayType() )
			{
				dimensions.push_back( element_type_as_array_type->size );
				element_type= &element_type_as_array_type->type;
			}
			else
				break;
		}

		mangler_state.PushElement( "Y" );
		EncodeNumber( mangler_state, llvm::APInt(64, dimensions.size()), false );
		for( const uint64_t dimension_size : dimensions )
			EncodeNumber( mangler_state, llvm::APInt(54, dimension_size), false );
		EncodeType( mangler_state, *element_type );
	}
	else if( const auto tuple_type= type.GetTupleType() )
	{
		// Encode tuples, like type templates.
		TemplateArgs template_args;
		template_args.reserve( tuple_type->elements.size() );
		for( const Type& element : tuple_type->elements )
			template_args.push_back( element );

		mangler_state.PushElement( g_class_type_prefix );

		// Use separate backreferences table.
		std::string template_name;
		{
			ManglerState template_mangler_state(template_name );

			template_mangler_state.PushElement( g_template_prefix );
			template_mangler_state.EncodeName( Keyword( Keywords::tup_ ) );
			EncodeTemplateArgs( template_mangler_state, template_args );
		}
		mangler_state.EncodeNameNoTerminator( template_name );
		// Finish class name.
		mangler_state.PushElement( g_terminator );
	}
	else if( const auto class_type= type.GetClassType() )
	{
		mangler_state.PushElement( g_class_type_prefix );

		if( class_type->typeinfo_type != std::nullopt )
		{
			// Encode typeinfo, like type template.

			// Use separate backreferences table.
			std::string template_name;
			{
				ManglerState template_mangler_state( template_name );

				template_mangler_state.PushElement( g_template_prefix );
				template_mangler_state.EncodeName( class_type->members->GetThisNamespaceName() );
				EncodeType( template_mangler_state, *class_type->typeinfo_type );
				// Finish list of template arguments.
				template_mangler_state.PushElement( g_terminator );
			}
			mangler_state.EncodeNameNoTerminator( template_name );
			// Finish class name.
			mangler_state.PushElement( g_terminator );
		}
		else if( class_type->base_template != std::nullopt )
		{
			EncodeTemplateClassName( mangler_state, class_type );
			// Finish list of name components.
			mangler_state.PushElement( g_terminator );
		}
		else
			EncodeFullName( mangler_state, class_type->members->GetThisNamespaceName(), *class_type->members->GetParent() );
	}
	else if( const auto enum_type= type.GetEnumType() )
	{
		mangler_state.PushElement( "W" );
		mangler_state.PushElement( "4" ); // Underlaying type. Modern MSVC uses "4" for all enums independent on underlaying type.
		EncodeFullName( mangler_state, enum_type->members.GetThisNamespaceName(), *enum_type->members.GetParent() );
	}
	else if( const auto raw_pointer= type.GetRawPointerType() )
	{
		mangler_state.PushElement( g_pointer_prefix );
		mangler_state.PushElement( pointer_types_modifier_ );
		mangler_state.PushElement( g_mut_prefix );
		EncodeType( mangler_state, raw_pointer->type );
	}
	else if( const auto function_pointer= type.GetFunctionPointerType() )
	{
		mangler_state.PushElement( g_pointer_prefix );
		mangler_state.PushElement( "6" );
		EncodeFunctionType(  mangler_state, function_pointer->function, true );
	}
	else if( const auto function= type.GetFunctionType() )
		EncodeFunctionType( mangler_state, *function, true );
	else U_ASSERT(false);
}

void ManglerMSVC::EncodeFunctionType( ManglerState& mangler_state, const FunctionType& function_type, const bool encode_full_type ) const
{
	// Calling convention code
	mangler_state.PushElement( "A" );

	if( function_type.return_value_is_reference )
	{
		mangler_state.PushElement( g_reference_prefix );
		mangler_state.PushElement( pointer_types_modifier_ );
		mangler_state.PushElement( function_type.return_value_is_mutable ? g_mut_prefix : g_imut_prefix );
	}
	else if(
		function_type.return_type.GetClassType() != nullptr ||
		function_type.return_type.GetEnumType() != nullptr ||
		function_type.return_type.GetTupleType() != nullptr )
	{
		mangler_state.PushElement( "?" );
		mangler_state.PushElement( g_mut_prefix ); // Return value is mutable
	}

	EncodeType( mangler_state, function_type.return_type );

	EncodeFunctionParams( mangler_state, function_type.params );

	bool params_empty= function_type.params.empty();

	if( encode_full_type )
	{
		// Encode additional function properties as params.
		if( function_type.unsafe )
		{
			// Encode "unsafe" flag as param of type "unsafe".
			params_empty= false;

			mangler_state.PushElement( g_class_type_prefix );
			mangler_state.PushElement( Keyword( Keywords::unsafe_ ) );
			// Finish list of name components.
			mangler_state.PushElement( g_terminator );
			// Finish class name.
			mangler_state.PushElement( g_terminator );
		}
		if( !function_type.return_references.empty() )
		{
			// Encode return references, like template class with special name and numeric args.
			params_empty= false;

			mangler_state.PushElement( g_class_type_prefix );

			// Use separate backreferences table.
			std::string template_name;
			{
				ManglerState template_mangler_state( template_name );

				template_mangler_state.PushElement( g_template_prefix );
				template_mangler_state.EncodeName( "_RR" );
				for( const FunctionType::ParamReference& arg_and_tag : function_type.return_references )
				{
					template_mangler_state.PushElement( g_numeric_template_arg_prefix );
					EncodeNumber( template_mangler_state, llvm::APInt( 64, arg_and_tag.first ), false );
					template_mangler_state.PushElement( g_numeric_template_arg_prefix );
					EncodeNumber( template_mangler_state, llvm::APInt( 64, arg_and_tag.second), true  );
				}
				// Finish list of template args.
				template_mangler_state.PushElement( g_terminator );
			}
			mangler_state.EncodeNameNoTerminator( template_name );
			// Finish class name.
			mangler_state.PushElement( g_terminator );
		}
		if( !function_type.references_pollution.empty() )
		{
			// Encode references pollution like template class with special name and numeric args.
			params_empty= false;

			mangler_state.PushElement( g_class_type_prefix );

			// Use separate backreferences table.
			std::string template_name;
			{
				ManglerState template_mangler_state( template_name );

				template_mangler_state.PushElement( g_template_prefix );
				template_mangler_state.EncodeName( "_RP" );
				for( const FunctionType::ReferencePollution& pollution : function_type.references_pollution )
				{
					template_mangler_state.PushElement( g_numeric_template_arg_prefix );
					EncodeNumber( template_mangler_state, llvm::APInt( 64, pollution.dst.first ), false );
					template_mangler_state.PushElement( g_numeric_template_arg_prefix );
					EncodeNumber( template_mangler_state, llvm::APInt( 64, pollution.dst.second), true  );
					template_mangler_state.PushElement( g_numeric_template_arg_prefix );
					EncodeNumber( template_mangler_state, llvm::APInt( 64, pollution.src.first ), false );
					template_mangler_state.PushElement( g_numeric_template_arg_prefix );
					EncodeNumber( template_mangler_state, llvm::APInt( 64, pollution.src.second), true  );
				}
				// Finish list of template args.
				template_mangler_state.PushElement( g_terminator );
			}
			mangler_state.EncodeNameNoTerminator( template_name );
			// Finish class name.
			mangler_state.PushElement( g_terminator );
		}
	}

	if( params_empty )
		mangler_state.PushElement( GetFundamentalTypeMangledName( U_FundamentalType::Void ) ); // In case of empty params just leave single type - "void" without terminator symbol.
	else
		mangler_state.PushElement( g_terminator ); // Finish list of params.

	mangler_state.PushElement( "Z" );
}

void ManglerMSVC::EncodeFunctionParams( ManglerState& mangler_state, const ArgsVector<FunctionType::Param>& params ) const
{
	ArgsVector<FunctionType::Param> back_references;

	for( const FunctionType::Param& param : params )
	{
		if( !param.is_reference && param.type.GetFundamentalType() != nullptr )
		{
			// For trivial params (fundamentals with no reference modifiers) do not create backreferences.
			EncodeType( mangler_state, param.type );
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
					mangler_state.PushElement( char(i + '0') );
					found= true;
					break;
				}
			}

			if( !found )
			{
				if( param.is_reference )
				{
					mangler_state.PushElement( g_reference_prefix );
					mangler_state.PushElement(pointer_types_modifier_ );
					mangler_state.PushElement( param.is_mutable ? g_mut_prefix : g_imut_prefix );
				}

				EncodeType( mangler_state, param.type );

				if( back_references.size() < g_num_back_references )
					back_references.push_back( std::move(param_copy) );
			}
		}
	}
}

void ManglerMSVC::EncodeTemplateArgs( ManglerState& mangler_state, const TemplateArgs& template_args ) const
{
	for( const TemplateArg& template_arg : template_args )
	{
		if( const auto type= std::get_if<Type>(&template_arg) )
		{
			if( type->GetArrayType() != nullptr )
				mangler_state.PushElement( "$$B" );

			EncodeType( mangler_state, *type );
		}
		else if( const auto variable= std::get_if<Variable>(&template_arg) )
		{
			// HACK!
			// This is not how C++ compiler encodes value template args.
			// In C++ this is just numbers.
			// In Ü it's possible to create several type templates with same name and single value template param
			// but with different param type.
			// And it's possible to use same numeric value with diffirent types for instantiation of different type templates.
			// So, we need to distinguish between such template types.
			// Because of that prefix each numeric arg with type, like this is just hidden type param for each value param.
			EncodeType( mangler_state, variable->type );

			mangler_state.PushElement( g_numeric_template_arg_prefix );

			bool is_signed= false;
			if( const auto fundamental_type= variable->type.GetFundamentalType() )
				is_signed= IsSignedInteger( fundamental_type->fundamental_type );
			else if( const auto enum_type= variable->type.GetEnumType() )
				is_signed= IsSignedInteger( enum_type->underlaying_type.fundamental_type );
			else U_ASSERT(false);

			U_ASSERT( variable->constexpr_value != nullptr );
			EncodeNumber( mangler_state, variable->constexpr_value->getUniqueInteger(), is_signed );
		}
		else U_ASSERT(false);
	}

	// Finish list of arguments.
	mangler_state.PushElement( g_terminator );
}

void ManglerMSVC::EncodeFullName( ManglerState& mangler_state, const std::string_view name, const NamesScope& scope ) const
{
	mangler_state.EncodeName( name );
	EncodeNamespacePostfix_r( mangler_state, scope );
	// Finish list of name components.
	mangler_state.PushElement(g_terminator );
}

void ManglerMSVC::EncodeNamespacePostfix_r( ManglerState& mangler_state, const NamesScope& scope ) const
{
	if( scope.GetParent() == nullptr ) // Root namespace.
		return;

	if( const ClassPtr the_class= scope.GetClass() )
	{
		if( the_class->base_template != std::nullopt )
		{
			EncodeTemplateClassName( mangler_state, the_class );
			return;
		}
	}

	mangler_state.EncodeName( scope.GetThisNamespaceName() );

	EncodeNamespacePostfix_r( mangler_state, *scope.GetParent() );
}

void ManglerMSVC::EncodeTemplateClassName( ManglerState& mangler_state, const ClassPtr& the_class ) const
{
	U_ASSERT( the_class->base_template != std::nullopt );

	const TypeTemplatePtr& type_template= the_class->base_template->class_template;
	const auto namespace_containing_template= type_template->parent_namespace;

	// Use separate backreferences table.
	std::string template_name;
	{
		ManglerState template_mangler_state( template_name );

		template_mangler_state.PushElement( g_template_prefix );
		template_mangler_state.EncodeName( type_template->syntax_element->name_ );
		EncodeTemplateArgs( template_mangler_state, the_class->base_template->signature_args );
	}
	mangler_state.EncodeNameNoTerminator( template_name );

	if( namespace_containing_template->GetParent() != nullptr )
		EncodeNamespacePostfix_r( mangler_state, *namespace_containing_template );
}

void ManglerMSVC::EncodeNumber( ManglerState& mangler_state, const llvm::APInt& num, const bool is_signed ) const
{
	uint64_t abs_value= 0;
	if( is_signed )
	{
		const int64_t value_signed= num.getSExtValue();
		if( value_signed >= 0 )
			abs_value= uint64_t(value_signed);
		else
		{
			mangler_state.PushElement( "?" );
			abs_value= uint64_t(-value_signed);
		}
	}
	else
		abs_value= num.getZExtValue();

	if( abs_value == 0 )
		mangler_state.PushElement( "A@" );
	else if( abs_value <= 10 )
		mangler_state.PushElement( char(abs_value - 1 + '0') );
	else
	{
		// Use hex numbers with digits in range [A;Q)
		int64_t hex_digit= 15;
		while((abs_value & (uint64_t(0xF) << (hex_digit << 2))) == 0)
			--hex_digit; // It's impossible to reach zero here since "abs_value" is non-zero.

		while(hex_digit >= 0)
		{
			mangler_state.PushElement( char('A' + ((abs_value >> (hex_digit << 2)) & 0xF)) );
			--hex_digit;
		}

		// Finish list of digits.
		mangler_state.PushElement( g_terminator );
	}
}

} // namespace

std::unique_ptr<IMangler> CreateManglerMSVC(const bool is_32_bit)
{
	return std::make_unique<ManglerMSVC>(is_32_bit);
}

} // namespace U
