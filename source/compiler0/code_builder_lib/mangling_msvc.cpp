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
constexpr char g_array_type_name_in_templates_prefix[]= "$$B";
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
	case U_FundamentalType::void_: return "X";
	case U_FundamentalType::bool_: return "_N";
	case U_FundamentalType::i8_  : return "C"; // C++ "signed char"
	case U_FundamentalType::u8_  : return "E"; // C++ "unsigned char"
	case U_FundamentalType::i16_ : return "F"; // C++ "short"
	case U_FundamentalType::u16_ : return "G"; // C++ "unsigned short"
	case U_FundamentalType::i32_ : return "H"; // C++ "int"
	case U_FundamentalType::u32_ : return "I"; // C++ "unsigned short"
	case U_FundamentalType::i64_ : return "_J"; // C++ "int64_t"
	case U_FundamentalType::u64_ : return "_K"; // C++ "uint64_t"
	case U_FundamentalType::i128_: return "_L"; // C++ "__int128"
	case U_FundamentalType::u128_: return "_M"; // "unsigned __int128"
	case U_FundamentalType::ssize_type_: return "J"; // C++ "long"
	case U_FundamentalType::size_type_ : return "K"; // C++ "unsigned long"
	case U_FundamentalType::f32_: return "M";  // C++ "float"
	case U_FundamentalType::f64_: return "N"; // C++ "double"
	case U_FundamentalType::char8_ : return "D"; // C++ "char"
	case U_FundamentalType::char16_: return "_S"; // C++ "char16_t"
	case U_FundamentalType::char32_: return "_U"; // C++ "char32_t"
	// Encode "byte" types as regular structs in global namspace.
	case U_FundamentalType::byte8_  : return "Ubyte8@@"  ;
	case U_FundamentalType::byte16_ : return "Ubyte16@@" ;
	case U_FundamentalType::byte32_ : return "Ubyte32@@" ;
	case U_FundamentalType::byte64_ : return "Ubyte64@@" ;
	case U_FundamentalType::byte128_: return "Ubyte128@@";
	};

	U_ASSERT(false);
	return "";
}

std::string_view GetCallingConventionName( const llvm::CallingConv::ID calling_convention )
{
	switch(calling_convention)
	{
	case llvm::CallingConv::C:
		return "A";
	case llvm::CallingConv::Fast:
		return "I";
	case llvm::CallingConv::Cold:
		return "U";
	case llvm::CallingConv::X86_StdCall:
		return "G";
	case llvm::CallingConv::X86_ThisCall:
		return "E";
	case llvm::CallingConv::X86_VectorCall:
		return "Q";
	};
	U_ASSERT(false);
	return "A";
}

const std::unordered_map<std::string_view, std::string_view> g_op_names
{
	{ "+", "?H" },
	{ "-", "?G" },
	{ "*", "?D" },
	{ "/", "?K" },
	{ "%", "?L" },

	{ "==", "?8" },
	{ "<=>", "?__M" }, // C++ spaceship operator

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

// Returns empty string if func_name is not special.
std::string_view DecodeOperator( std::string_view func_name )
{
	const auto it= g_op_names.find( func_name );
	if( it != g_op_names.end() )
		return it->second;

	return "";
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
		std::string_view function_name,
		const FunctionType& function_type,
		std::optional<llvm::ArrayRef<TemplateArg>> template_args ) override;
	std::string MangleGlobalVariable( const NamesScope& parent_scope, std::string_view variable_name, const Type& type, bool is_constant ) override;
	std::string MangleType( const Type& type ) override;
	std::string MangleVirtualTable( const Type& type ) override;

private:
	void EncodeType( ManglerState& mangler_state, const Type& type ) const;
	void EncodeFunctionType( ManglerState& mangler_state, const FunctionType& function_type, bool encode_full_type ) const;
	void EncodeFunctionParams( ManglerState& mangler_state, llvm::ArrayRef<FunctionType::Param> params ) const;
	void EncodeTemplateArgs( ManglerState& mangler_state, llvm::ArrayRef<TemplateArg> template_args ) const;
	void EncodeTemplateArgImpl( ManglerState& mangler_state, const Type& type ) const;
	void EncodeTemplateArgImpl( ManglerState& mangler_state, const TemplateVariableArg& variable ) const;
	void EncodeTemplateArgImpl( ManglerState& mangler_state, const TypeTemplatePtr& type_template ) const;
	void EncodeConstexprValue( ManglerState& mangler_state, const Type& type, const llvm::Constant* constexpr_value ) const;
	void EncodeFullName( ManglerState& mangler_state, const std::string_view name, const NamesScope& names_scope ) const;
	void EncodeNamespacePostfix_r( ManglerState& mangler_state, const NamesScope& names_scope ) const;
	void EncodeTemplateClassName( ManglerState& mangler_state, ClassPtr the_class ) const;
	void EncodeLambdaClassName( ManglerState& mangler_state, ClassPtr the_class ) const;
	void EncodeCoroutineClassName( ManglerState& mangler_state, ClassPtr the_class ) const;
	void EncodeNumber( ManglerState& mangler_state, const llvm::APInt& num, bool is_signed ) const;
	void EncodeReferencePollution( ManglerState& mangler_state, const FunctionType::ReferencesPollution& references_pollution ) const;
	void EncodeReturnReferences( ManglerState& mangler_state, const FunctionType::ReturnReferences& return_references ) const;
	void EncodeReturnInnerReferences( ManglerState& mangler_state, const FunctionType::ReturnInnerReferences& return_inner_references ) const;
	void EncodeParamReference( ManglerState& mangler_state, const FunctionType::ParamReference& param_reference ) const;

private:
	const std::string_view pointer_types_modifier_;
};

ManglerMSVC::ManglerMSVC(const bool is_32_bit)
	: pointer_types_modifier_(is_32_bit ? "" : "E")
{}

std::string ManglerMSVC::MangleFunction(
	const NamesScope& parent_scope,
	const std::string_view function_name,
	const FunctionType& function_type,
	const std::optional<llvm::ArrayRef<TemplateArg>> template_args )
{
	// For class methods do not encode stuff like access labels, or methods-related stuff.
	// Just encode class methods as regular functions inside namespaces, with "this" as regular param.

	std::string res;
	ManglerState mangler_state( res );

	mangler_state.PushElement( g_name_prefix );

	const std::string_view op_name= DecodeOperator( function_name );
	if( template_args != std::nullopt )
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

std::string ManglerMSVC::MangleGlobalVariable( const NamesScope& parent_scope, const std::string_view variable_name, const Type& type, const bool is_constant )
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
				dimensions.push_back( element_type_as_array_type->element_count );
				element_type= &element_type_as_array_type->element_type;
			}
			else
				break;
		}

		mangler_state.PushElement( "Y" );
		EncodeNumber( mangler_state, llvm::APInt(64, dimensions.size()), false );
		for( const uint64_t dimension_size : dimensions )
			EncodeNumber( mangler_state, llvm::APInt(64, dimension_size), false );
		EncodeType( mangler_state, *element_type );
	}
	else if( const auto tuple_type= type.GetTupleType() )
	{
		// Encode tuples, like type templates.
		llvm::SmallVector<TemplateArg, 8> template_args;
		template_args.reserve( tuple_type->element_types.size() );
		for( const Type& element : tuple_type->element_types )
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

		if( const auto typeinfo_class_description= std::get_if<TypeinfoClassDescription>( &class_type->generated_class_data ) )
		{
			// Encode typeinfo, like type template.

			// Use separate backreferences table.
			std::string template_name;
			{
				ManglerState template_mangler_state( template_name );

				template_mangler_state.PushElement( g_template_prefix );
				template_mangler_state.EncodeName( class_type->members->GetThisNamespaceName() );
				EncodeType( template_mangler_state, typeinfo_class_description->source_type );
				// Finish list of template arguments.
				template_mangler_state.PushElement( g_terminator );
			}
			mangler_state.EncodeNameNoTerminator( template_name );
			// Finish class name.
			mangler_state.PushElement( g_terminator );
		}
		else if( std::get_if< Class::BaseTemplate >( &class_type->generated_class_data ) != nullptr )
		{
			EncodeTemplateClassName( mangler_state, class_type );
			// Finish list of name components.
			mangler_state.PushElement( g_terminator );
		}
		else if( std::get_if< CoroutineTypeDescription >( &class_type->generated_class_data ) != nullptr )
		{
			EncodeCoroutineClassName( mangler_state, class_type );
			// Finish list of name components.
			mangler_state.PushElement( g_terminator );
		}
		else if( std::holds_alternative<LambdaClassData>( class_type->generated_class_data ) )
		{
			EncodeLambdaClassName( mangler_state, class_type );
			// Finish list of name components.
			mangler_state.PushElement( g_terminator );
		}
		else
			EncodeFullName( mangler_state, class_type->members->GetThisNamespaceName(), *class_type->members->GetParent() );
	}
	else if( const auto enum_type= type.GetEnumType() )
	{
		mangler_state.PushElement( "W" );
		mangler_state.PushElement( "4" ); // Underlying type. Modern MSVC uses "4" for all enums independent on underlying type.
		EncodeFullName( mangler_state, enum_type->members.GetThisNamespaceName(), *enum_type->members.GetParent() );
	}
	else if( const auto raw_pointer= type.GetRawPointerType() )
	{
		mangler_state.PushElement( g_pointer_prefix );
		mangler_state.PushElement( pointer_types_modifier_ );
		mangler_state.PushElement( g_mut_prefix );
		EncodeType( mangler_state, raw_pointer->element_type );
	}
	else if( const auto function_pointer= type.GetFunctionPointerType() )
	{
		mangler_state.PushElement( g_pointer_prefix );
		mangler_state.PushElement( "6" );
		EncodeFunctionType( mangler_state, function_pointer->function_type, true );
	}
	else U_ASSERT(false);
}

void ManglerMSVC::EncodeFunctionType( ManglerState& mangler_state, const FunctionType& function_type, const bool encode_full_type ) const
{
	mangler_state.PushElement( GetCallingConventionName(function_type.calling_convention) );

	if( function_type.return_value_type != ValueType::Value )
	{
		mangler_state.PushElement( g_reference_prefix );
		mangler_state.PushElement( pointer_types_modifier_ );
		mangler_state.PushElement( function_type.return_value_type == ValueType::ReferenceMut ? g_mut_prefix : g_imut_prefix );
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
		if( !function_type.references_pollution.empty() )
		{
			params_empty= false;
			EncodeReferencePollution( mangler_state, function_type.references_pollution );
		}
		if( !function_type.return_references.empty() )
		{
			params_empty= false;
			EncodeReturnReferences( mangler_state, function_type.return_references );
		}
		if( !function_type.return_inner_references.empty() )
		{
			params_empty= false;
			EncodeReturnInnerReferences( mangler_state, function_type.return_inner_references );
		}
	}

	if( params_empty )
		mangler_state.PushElement( GetFundamentalTypeMangledName( U_FundamentalType::void_ ) ); // In case of empty params just leave single type - "void" without terminator symbol.
	else
		mangler_state.PushElement( g_terminator ); // Finish list of params.

	mangler_state.PushElement( "Z" );
}

void ManglerMSVC::EncodeFunctionParams( ManglerState& mangler_state, const llvm::ArrayRef<FunctionType::Param> params ) const
{
	ArgsVector<FunctionType::Param> back_references;

	for( const FunctionType::Param& param : params )
	{
		if( param.value_type == ValueType::Value && param.type.GetFundamentalType() != nullptr )
		{
			// For trivial params (fundamentals with no reference modifiers) do not create backreferences.
			EncodeType( mangler_state, param.type );
		}
		else
		{
			bool found = false;
			for( size_t i= 0; i < back_references.size(); ++i )
			{
				if( param == back_references[i] )
				{
					mangler_state.PushElement( char(i + '0') );
					found= true;
					break;
				}
			}

			if( !found )
			{
				if( param.value_type != ValueType::Value )
				{
					mangler_state.PushElement( g_reference_prefix );
					mangler_state.PushElement( pointer_types_modifier_ );
					mangler_state.PushElement( param.value_type == ValueType::ReferenceMut ? g_mut_prefix : g_imut_prefix );
				}

				EncodeType( mangler_state, param.type );

				if( back_references.size() < g_num_back_references )
					back_references.push_back( param );
			}
		}
	}
}

void ManglerMSVC::EncodeTemplateArgs( ManglerState& mangler_state, const llvm::ArrayRef<TemplateArg> template_args ) const
{
	for( const TemplateArg& template_arg : template_args )
		std::visit( [&]( const auto& el ) { EncodeTemplateArgImpl( mangler_state, el ); }, template_arg );

	// Finish list of arguments.
	mangler_state.PushElement( g_terminator );
}

void ManglerMSVC::EncodeTemplateArgImpl( ManglerState& mangler_state, const Type& type ) const
{
	if( type.GetArrayType() != nullptr )
		mangler_state.PushElement( g_array_type_name_in_templates_prefix );

	EncodeType( mangler_state, type );
}

void ManglerMSVC::EncodeTemplateArgImpl( ManglerState& mangler_state, const TemplateVariableArg& variable ) const
{
	if( variable.type.GetArrayType() != nullptr || variable.type.GetTupleType() != nullptr )
	{
		mangler_state.PushElement( "$" );
		EncodeConstexprValue( mangler_state, variable.type, variable.constexpr_value );
	}
	else
	{
		// HACK!
		// This is not how C++ compiler encodes value template args.
		// In C++ this is just numbers.
		// In Ü it's possible to create several type templates with same name and single value template param
		// but with different param type.
		// And it's possible to use same numeric value with diffirent types for instantiation of different type templates.
		// So, we need to distinguish between such template types.
		// Because of that prefix each numeric arg with type, like this is just hidden type param for each value param.
		EncodeType( mangler_state, variable.type );

		mangler_state.PushElement( g_numeric_template_arg_prefix );

		bool is_signed= false;
		if( const auto fundamental_type= variable.type.GetFundamentalType() )
			is_signed= IsSignedInteger( fundamental_type->fundamental_type );
		else if( const auto enum_type= variable.type.GetEnumType() )
			is_signed= IsSignedInteger( enum_type->underlying_type.fundamental_type );
		else U_ASSERT(false);

		U_ASSERT( variable.constexpr_value != nullptr );
		EncodeNumber( mangler_state, variable.constexpr_value->getUniqueInteger(), is_signed );
	}
}

void ManglerMSVC::EncodeTemplateArgImpl( ManglerState& mangler_state, const TypeTemplatePtr& type_template ) const
{
	// TODO
	(void)mangler_state;
	(void)type_template;
}

void ManglerMSVC::EncodeConstexprValue( ManglerState& mangler_state, const Type& type, const llvm::Constant* const constexpr_value ) const
{
	if( const auto array_type= type.GetArrayType() )
	{
		mangler_state.PushElement( "2" );

		mangler_state.PushElement( g_array_type_name_in_templates_prefix ); // Prefix array type names in templates.
		EncodeType( mangler_state, type );

		for( uint64_t i= 0; i < array_type->element_count; ++i )
			EncodeConstexprValue( mangler_state, array_type->element_type, constexpr_value->getAggregateElement( uint32_t(i) ) );

		mangler_state.PushElement( g_terminator );
	}
	else if( const auto tuple_type= type.GetTupleType() )
	{
		mangler_state.PushElement( "2" );

		EncodeType( mangler_state, type );

		for( size_t i= 0; i < tuple_type->element_types.size(); ++i )
			EncodeConstexprValue( mangler_state, tuple_type->element_types[i], constexpr_value->getAggregateElement( uint32_t(i) ) );

		mangler_state.PushElement( g_terminator );
	}
	else
	{
		EncodeType( mangler_state, type );

		mangler_state.PushElement( '0' );

		bool is_signed= false;
		if( const auto fundamental_type= type.GetFundamentalType() )
			is_signed= IsSignedInteger( fundamental_type->fundamental_type );
		else if( const auto enum_type= type.GetEnumType() )
			is_signed= IsSignedInteger( enum_type->underlying_type.fundamental_type );
		else U_ASSERT(false);

		U_ASSERT( constexpr_value != nullptr );
		EncodeNumber( mangler_state, constexpr_value->getUniqueInteger(), is_signed );
	}
}

void ManglerMSVC::EncodeFullName( ManglerState& mangler_state, const std::string_view name, const NamesScope& names_scope ) const
{
	mangler_state.EncodeName( name );
	EncodeNamespacePostfix_r( mangler_state, names_scope );
	// Finish list of name components.
	mangler_state.PushElement(g_terminator );
}

void ManglerMSVC::EncodeNamespacePostfix_r( ManglerState& mangler_state, const NamesScope& names_scope ) const
{
	if( names_scope.GetParent() == nullptr ) // Root namespace.
		return;

	if( const ClassPtr the_class= names_scope.GetClass() )
	{
		if( std::get_if<Class::BaseTemplate>( &the_class->generated_class_data ) != nullptr )
		{
			EncodeTemplateClassName( mangler_state, the_class );
			return;
		}
		if(std::get_if<CoroutineTypeDescription>( &the_class->generated_class_data ) != nullptr )
		{
			EncodeCoroutineClassName( mangler_state, the_class );
			return;
		}
		if( std::holds_alternative<LambdaClassData>( the_class->generated_class_data ) )
		{
			EncodeLambdaClassName( mangler_state, the_class );
			return;
		}
	}

	mangler_state.EncodeName( names_scope.GetThisNamespaceName() );

	EncodeNamespacePostfix_r( mangler_state, *names_scope.GetParent() );
}

void ManglerMSVC::EncodeTemplateClassName( ManglerState& mangler_state, const ClassPtr the_class ) const
{
	const auto base_template= std::get_if<Class::BaseTemplate>( &the_class->generated_class_data );
	U_ASSERT( base_template != nullptr );

	const TypeTemplatePtr& type_template= base_template->class_template;
	const auto namespace_containing_template= type_template->parent_namespace;

	// Use separate backreferences table.
	std::string template_name;
	{
		ManglerState template_mangler_state( template_name );

		template_mangler_state.PushElement( g_template_prefix );
		template_mangler_state.EncodeName( type_template->syntax_element->name );
		EncodeTemplateArgs( template_mangler_state, base_template->signature_args );
	}
	mangler_state.EncodeNameNoTerminator( template_name );

	if( namespace_containing_template->GetParent() != nullptr )
		EncodeNamespacePostfix_r( mangler_state, *namespace_containing_template );
}

void ManglerMSVC::EncodeLambdaClassName( ManglerState& mangler_state, const ClassPtr the_class ) const
{
	const auto lambda_class_data= std::get_if<LambdaClassData>( &the_class->generated_class_data );
	U_ASSERT( lambda_class_data != nullptr );

	if( lambda_class_data->template_args.empty() )
	{
		mangler_state.EncodeName( the_class->members->GetThisNamespaceName() );
		EncodeNamespacePostfix_r( mangler_state, *the_class->members->GetParent() );
	}
	else
	{
		// Use separate backreferences table.
		std::string template_name;
		{
			ManglerState template_mangler_state( template_name );

			template_mangler_state.PushElement( g_template_prefix );
			template_mangler_state.EncodeName( the_class->members->GetThisNamespaceName() );
			EncodeTemplateArgs( template_mangler_state, lambda_class_data->template_args );
		}
		mangler_state.EncodeNameNoTerminator( template_name );

		EncodeNamespacePostfix_r( mangler_state, *the_class->members->GetParent()->GetParent() );
	}
}

void ManglerMSVC::EncodeCoroutineClassName( ManglerState& mangler_state, const ClassPtr the_class ) const
{
	const auto coroutine_type_description= std::get_if<CoroutineTypeDescription>( &the_class->generated_class_data );
	U_ASSERT( coroutine_type_description != nullptr );

	// Encode coroutine as template.
	// Use separate backreferences table.
	std::string template_name;
	{
		ManglerState template_mangler_state( template_name );

		template_mangler_state.PushElement( g_template_prefix );
		template_mangler_state.EncodeName( the_class->members->GetThisNamespaceName() );

		// Return value.
		if( coroutine_type_description->return_value_type != ValueType::Value )
		{
			template_mangler_state.PushElement( g_reference_prefix );
			template_mangler_state.PushElement( pointer_types_modifier_ );
			template_mangler_state.PushElement( coroutine_type_description->return_value_type == ValueType::ReferenceMut ? g_mut_prefix : g_imut_prefix );
		}
		EncodeType( template_mangler_state, coroutine_type_description->return_type );

		// non-sync tag.
		if( coroutine_type_description->non_sync )
		{
			template_mangler_state.PushElement( GetFundamentalTypeMangledName( U_FundamentalType::bool_ ) );
			template_mangler_state.PushElement( g_numeric_template_arg_prefix );
			EncodeNumber( template_mangler_state, llvm::APInt( 1u, uint64_t(1) ), false );
		}

		// Inner references.
		for( const InnerReferenceKind inner_reference : coroutine_type_description->inner_references )
		{
			template_mangler_state.PushElement( GetFundamentalTypeMangledName( U_FundamentalType::u32_ ) );
			template_mangler_state.PushElement( g_numeric_template_arg_prefix );
			EncodeNumber( template_mangler_state, llvm::APInt( 64u, uint64_t(inner_reference) ), false );
		}

		if( !coroutine_type_description->return_references.empty() )
			EncodeReturnReferences( template_mangler_state, coroutine_type_description->return_references );
		if( !coroutine_type_description->return_inner_references.empty() )
			EncodeReturnInnerReferences( template_mangler_state, coroutine_type_description->return_inner_references );

		// Finish list of template arguments.
		template_mangler_state.PushElement( g_terminator );
	}
	mangler_state.EncodeNameNoTerminator( template_name );
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

void ManglerMSVC::EncodeReferencePollution( ManglerState& mangler_state, const FunctionType::ReferencesPollution& references_pollution ) const
{
	// Encode references pollution like template class with special name and numeric args.
	mangler_state.PushElement( g_class_type_prefix );

	// Use separate backreferences table.
	std::string template_name;
	{
		ManglerState template_mangler_state( template_name );

		template_mangler_state.PushElement( g_template_prefix );
		template_mangler_state.EncodeName( "_RP" );

		template_mangler_state.PushElement( "$" );
		template_mangler_state.PushElement( "2" );
		template_mangler_state.PushElement( g_array_type_name_in_templates_prefix );
		template_mangler_state.PushElement( "Y2" );
		EncodeNumber( template_mangler_state, llvm::APInt( 64, references_pollution.size() ), false );
		EncodeNumber( template_mangler_state, llvm::APInt( 64, 2 ), false );
		EncodeNumber( template_mangler_state, llvm::APInt( 64, 2 ), false );
		template_mangler_state.PushElement( GetFundamentalTypeMangledName( U_FundamentalType::char8_ ) );

		for( const auto& pollution_element : references_pollution )
		{
			template_mangler_state.PushElement( "2" );
			template_mangler_state.PushElement( g_array_type_name_in_templates_prefix );
			template_mangler_state.PushElement( "Y1" );
			EncodeNumber( template_mangler_state, llvm::APInt( 64, 2 ), false );
			EncodeNumber( template_mangler_state, llvm::APInt( 64, 2 ), false );
			template_mangler_state.PushElement( GetFundamentalTypeMangledName( U_FundamentalType::char8_ ) );

			EncodeParamReference( template_mangler_state, pollution_element.dst );
			EncodeParamReference( template_mangler_state, pollution_element.src );

			template_mangler_state.PushElement( "@" );
		}

		template_mangler_state.PushElement( "@" );

		// Finish list of template args.
		template_mangler_state.PushElement( g_terminator );
	}
	mangler_state.EncodeNameNoTerminator( template_name );
	// Finish class name.
	mangler_state.PushElement( g_terminator );
}

void ManglerMSVC::EncodeReturnReferences( ManglerState& mangler_state, const FunctionType::ReturnReferences& return_references ) const
{
	// Encode return references, like template class with special name and numeric args.

	mangler_state.PushElement( g_class_type_prefix );

	// Use separate backreferences table.
	std::string template_name;
	{
		ManglerState template_mangler_state( template_name );

		template_mangler_state.PushElement( g_template_prefix );
		template_mangler_state.EncodeName( "_RR" );

		template_mangler_state.PushElement( "$" );
		template_mangler_state.PushElement( "2" );
		template_mangler_state.PushElement( g_array_type_name_in_templates_prefix );
		template_mangler_state.PushElement( "Y1" );
		EncodeNumber( template_mangler_state, llvm::APInt( 64, return_references.size() ), false );
		EncodeNumber( template_mangler_state, llvm::APInt( 64, 2 ), false );
		template_mangler_state.PushElement( GetFundamentalTypeMangledName( U_FundamentalType::char8_ ) );

		for( const FunctionType::ParamReference& param_reference : return_references )
			EncodeParamReference( template_mangler_state, param_reference );

		template_mangler_state.PushElement( g_terminator );

		// Finish list of template args.
		template_mangler_state.PushElement( g_terminator );
	}
	mangler_state.EncodeNameNoTerminator( template_name );
	// Finish class name.
	mangler_state.PushElement( g_terminator );
}

void ManglerMSVC::EncodeReturnInnerReferences( ManglerState& mangler_state, const FunctionType::ReturnInnerReferences& return_inner_references ) const
{
	// Encode return inner references, like template class with special name and numeric args.
	mangler_state.PushElement( g_class_type_prefix );

	// Use separate backreferences table.
	std::string template_name;
	{
		ManglerState template_mangler_state( template_name );

		template_mangler_state.PushElement( g_template_prefix );
		template_mangler_state.EncodeName( "_RIR" );

		template_mangler_state.PushElement( "$" );

		template_mangler_state.PushElement( "2" );

		// Hack! Just use "tup" as type name, without specifying exact values.
		template_mangler_state.PushElement( "U" );
		template_mangler_state.PushElement( Keyword( Keywords::tup_ ) );
		template_mangler_state.PushElement( g_terminator );
		template_mangler_state.PushElement( g_terminator );

		for( const auto& return_references : return_inner_references )
		{
			template_mangler_state.PushElement( "2" );
			template_mangler_state.PushElement( g_array_type_name_in_templates_prefix );
			template_mangler_state.PushElement( "Y1" );
			EncodeNumber( template_mangler_state, llvm::APInt( 64, return_references.size() ), false );
			EncodeNumber( template_mangler_state, llvm::APInt( 64, 2 ), false );
			template_mangler_state.PushElement( GetFundamentalTypeMangledName( U_FundamentalType::char8_ ) );

			for( const FunctionType::ParamReference& param_reference : return_references )
				EncodeParamReference( template_mangler_state, param_reference );

			template_mangler_state.PushElement( g_terminator );
		}

		template_mangler_state.PushElement( g_terminator );

		// Finish list of template args.
		template_mangler_state.PushElement( g_terminator );
	}
	mangler_state.EncodeNameNoTerminator( template_name );
	// Finish class name.
	mangler_state.PushElement( g_terminator );
}

void ManglerMSVC::EncodeParamReference( ManglerState& mangler_state, const FunctionType::ParamReference& param_reference ) const
{
	// HACK! Use encoding as for structs, instead as for arrays, because old versions of "undname.exe" can't parse arrays.
	mangler_state.PushElement( "2" );
	mangler_state.PushElement( g_array_type_name_in_templates_prefix );
	mangler_state.PushElement( "Y0" );
	EncodeNumber( mangler_state, llvm::APInt( 64, 2 ), false );
	mangler_state.PushElement( GetFundamentalTypeMangledName( U_FundamentalType::char8_ ) );

	mangler_state.PushElement( GetFundamentalTypeMangledName( U_FundamentalType::char8_ ) );
	mangler_state.PushElement( '0' );
	EncodeNumber( mangler_state, llvm::APInt( 64, uint32_t('0' + param_reference.first) ), false );

	mangler_state.PushElement( GetFundamentalTypeMangledName( U_FundamentalType::char8_ ) );
	mangler_state.PushElement( '0' );
	const uint32_t param_reference_char= param_reference.second == FunctionType::c_param_reference_number ? '_' : ('a' + param_reference.second);
	EncodeNumber( mangler_state, llvm::APInt( 64, param_reference_char ), false );

	mangler_state.PushElement( g_terminator );
}

} // namespace

std::unique_ptr<IMangler> CreateManglerMSVC(const bool is_32_bit)
{
	return std::make_unique<ManglerMSVC>(is_32_bit);
}

} // namespace U
