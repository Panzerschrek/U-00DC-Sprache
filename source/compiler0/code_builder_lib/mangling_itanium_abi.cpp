#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"

#include "class.hpp"
#include "enum.hpp"
#include "template_types.hpp"
#include "mangling.hpp"

namespace U
{

namespace
{

class ManglerState
{
private:
	using LenType = uint16_t;

public:
	void Push( char c );
	void Push( std::string_view name );
	void PushLengthPrefixed( std::string_view name );

	std::string TakeResult();

public:
	class NodeHolder;

private:
	LenType GetCurrentPos() const;
	LenType GetCurrentCompressedPos() const;
	void FinalizePart( LenType start, LenType compressed_start );

private:
	struct Substitution
	{
		LenType start;
		LenType size;
	};

private:
	llvm::SmallVector<Substitution, 16> substitutions_;
	std::string result_full_;
	std::string result_compressed_;
};

// Mangling with Itanium ABI rules.
// Use class instead of set of free functions for possibility of reu-use of internal buffers.

class ManglerItaniumABI final : public IMangler
{
public:
	std::string MangleFunction(
		const NamesScope& parent_scope,
		std::string_view function_name,
		const FunctionType& function_type,
		std::optional<llvm::ArrayRef<TemplateArg>> template_args ) override;
	std::string MangleGlobalVariable( const NamesScope& parent_scope, std::string_view variable_name, const Type& type, bool is_constant ) override;
	std::string MangleType( const Type& type ) override;
	std::string MangleTemplateArgs( llvm::ArrayRef<TemplateArg> template_args ) override;
	std::string MangleVirtualTable( const Type& type ) override;

private:
	ManglerState state_;
};

char Base36Digit( const size_t value )
{
	U_ASSERT( value < 36u );
	if( value < 10 )
		return char('0' + value);
	else
		return char('A' + ( value - 10 ) );
}

//
// ManglerState
//

void ManglerState::Push( const char c )
{
	result_full_.push_back( c );
	result_compressed_.push_back( c );
}

void ManglerState::Push( const std::string_view name )
{
	result_full_+= name;
	result_compressed_+= name;
}

void ManglerState::PushLengthPrefixed( const std::string_view name )
{
	Push( std::to_string(name.size()) );
	Push( name );
}

std::string ManglerState::TakeResult()
{
	// Take copy for result. This allows us to re-use internal buffer for mangling of next name.
	std::string result= result_compressed_;

	substitutions_.clear();
	result_full_.clear();
	result_compressed_.clear();

	return result;
}

class ManglerState::NodeHolder
{
public:
	explicit NodeHolder( ManglerState& state )
		: state_(state), start_(state.GetCurrentPos()), compressed_start_(state.GetCurrentCompressedPos())
	{}

	~NodeHolder()
	{
		state_.FinalizePart( start_, compressed_start_ );
	}

private:
	ManglerState& state_;
	const LenType start_;
	const LenType compressed_start_;
};

ManglerState::LenType ManglerState::GetCurrentPos() const
{
	return LenType( result_full_.size() );
}

ManglerState::LenType ManglerState::GetCurrentCompressedPos() const
{
	return LenType( result_compressed_.size() );
}

void ManglerState::FinalizePart( const LenType start, const LenType compressed_start )
{
	U_ASSERT( start <= result_full_.size() );
	U_ASSERT( compressed_start <= result_compressed_.size() );
	const auto size= LenType( result_full_.size() - start );

	const std::string_view current_part= std::string_view(result_full_).substr( start, size );

	// Search for replacement.
	for( size_t i= 0; i < substitutions_.size(); ++i )
	{
		const Substitution& substitution= substitutions_[i];
		if( current_part == std::string_view(result_full_).substr( substitution.start, substitution.size ) )
		{
			result_compressed_.resize( compressed_start );
			result_compressed_.push_back( 'S' );

			if( i > 0u )
			{
				// Use 32-bit division, which is significantly faster, than 64-bit.
				const uint32_t n= uint32_t(i) - 1u;
				if( n < 36 )
					result_compressed_.push_back( Base36Digit( n ) );
				else if( n < 36 * 36 )
				{
					result_compressed_.push_back( Base36Digit( n / 36 ) );
					result_compressed_.push_back( Base36Digit( n % 36 ) );
				}
				else if( n < 36 * 36 * 36 )
				{
					result_compressed_.push_back( Base36Digit( n / ( 36 * 36 ) ) );
					result_compressed_.push_back( Base36Digit( n / 36 % 36 ) );
					result_compressed_.push_back( Base36Digit( n % 36 ) );
				}
				else U_ASSERT(false); // Too much substitutions.
			}
			result_compressed_.push_back( '_' );
			return;
		}
	}

	// Not found replacement - add new substitution.
	substitutions_.push_back( Substitution{ start, size } );
}

namespace
{

void EncodeTypeName( ManglerState& mangler_state, const Type& type );
void EncodeFunctionTypeName( ManglerState& mangler_state, const FunctionType& function_type );
void EncodeNamespacePrefix_r( ManglerState& mangler_state, const NamesScope& names_scope );
void EncodeCoroutineType( ManglerState& mangler_state, ClassPtr class_type );

void EncodeTemplateArgs( ManglerState& mangler_state, const llvm::ArrayRef<TemplateArg> template_args )
{
	mangler_state.Push( "I" );

	for( const TemplateArg& template_arg : template_args )
	{
		if( const auto type= std::get_if<Type>( &template_arg ) )
			EncodeTypeName( mangler_state, *type );
		else if( const auto variable= std::get_if<TemplateVariableArg>( &template_arg ) )
		{
			mangler_state.Push( "L" );

			EncodeTypeName( mangler_state, variable->type );

			bool is_signed= false;
			if( const auto fundamental_type= variable->type.GetFundamentalType() )
				is_signed= IsSignedInteger( fundamental_type->fundamental_type );
			else if( const auto enum_type= variable->type.GetEnumType() )
				is_signed= IsSignedInteger( enum_type->underlaying_type.fundamental_type );
			else U_ASSERT(false);

			U_ASSERT( variable->constexpr_value != nullptr );
			const llvm::APInt arg_value= variable->constexpr_value->getUniqueInteger();
			if( is_signed )
			{
				const int64_t value_signed= arg_value.getSExtValue();
				if( value_signed >= 0 )
					mangler_state.Push( std::to_string( value_signed ) );
				else
				{
					mangler_state.Push( "n" );
					mangler_state.Push( std::to_string( -value_signed ) );
				}
			}
			else
				mangler_state.Push( std::to_string( arg_value.getZExtValue() ) );

			mangler_state.Push( "E" );
		}
		else U_ASSERT(false);
	}

	mangler_state.Push( "E" );
}

void EncodeTemplateClassName( ManglerState& mangler_state, const Class& the_class )
{
	const auto base_template= std::get_if<Class::BaseTemplate>( &the_class.generated_class_data );
	U_ASSERT( base_template != nullptr );

	{
		const ManglerState::NodeHolder name_node( mangler_state );

		// Skip template parameters namespace.
		U_ASSERT( the_class.members->GetParent() != nullptr );
		if( const auto parent= the_class.members->GetParent()->GetParent() )
			if( !parent->GetThisNamespaceName().empty() )
				EncodeNamespacePrefix_r( mangler_state, *parent );

		mangler_state.PushLengthPrefixed( base_template->class_template->syntax_element->name );
	}

	EncodeTemplateArgs( mangler_state, base_template->signature_args );
}

void EncodeNamespacePrefix_r( ManglerState& mangler_state, const NamesScope& names_scope )
{
	if( const ClassPtr the_class= names_scope.GetClass() )
	{
		if( std::get_if<Class::BaseTemplate>( &the_class->generated_class_data ) != nullptr )
		{
			const ManglerState::NodeHolder result_node( mangler_state );
			EncodeTemplateClassName( mangler_state, *the_class );
			return;
		}
		else if( std::get_if<CoroutineTypeDescription>( &the_class->generated_class_data ) != nullptr )
		{
			EncodeCoroutineType( mangler_state, the_class );
			return;
		}
	}

	const std::string& name= names_scope.GetThisNamespaceName();

	const ManglerState::NodeHolder result_node( mangler_state );

	if( const auto parent= names_scope.GetParent() )
		if( !parent->GetThisNamespaceName().empty() )
			EncodeNamespacePrefix_r( mangler_state, *parent );

	mangler_state.PushLengthPrefixed( name );
}

void EncodeNestedName( ManglerState& mangler_state, const std::string_view name, const NamesScope& parent_scope )
{
	const ManglerState::NodeHolder result_node( mangler_state );

	if( parent_scope.GetParent() != nullptr )
	{
		mangler_state.Push( "N" );
		EncodeNamespacePrefix_r( mangler_state, parent_scope );
		mangler_state.PushLengthPrefixed( name );
		mangler_state.Push( "E" );
	}
	else
		mangler_state.PushLengthPrefixed( name );
}

std::string_view EncodeFundamentalType( const U_FundamentalType t )
{
	switch( t )
	{
	case U_FundamentalType::InvalidType:
	case U_FundamentalType::LastType:
		return "";
	case U_FundamentalType::void_: return "v";
	case U_FundamentalType::bool_: return "b";
	case U_FundamentalType::i8_  : return "a"; // C++ signed char
	case U_FundamentalType::u8_  : return "h"; // C++ unsigned char
	case U_FundamentalType::i16_ : return "s";
	case U_FundamentalType::u16_ : return "t";
	case U_FundamentalType::i32_ : return "i";
	case U_FundamentalType::u32_ : return "j";
	case U_FundamentalType::i64_ : return "x";
	case U_FundamentalType::u64_ : return "y";
	case U_FundamentalType::i128_: return "n";
	case U_FundamentalType::u128_: return "o";
	case U_FundamentalType::f32_: return "f";
	case U_FundamentalType::f64_: return "d";
	case U_FundamentalType::char8_ : return "c"; // C++ char
	case U_FundamentalType::char16_: return "Ds"; // C++ char16_t
	case U_FundamentalType::char32_: return "Di"; // C++ char32_t
	// Use vendor-extended types for "byte" types.
	case U_FundamentalType::byte8_  : return "u5byte8"  ;
	case U_FundamentalType::byte16_ : return "u6byte16" ;
	case U_FundamentalType::byte32_ : return "u6byte32" ;
	case U_FundamentalType::byte64_ : return "u6byte64" ;
	case U_FundamentalType::byte128_: return "u7byte128";
	};

	U_ASSERT(false);
	return "";
}

void EncodeFunctionParam( ManglerState& mangler_state, const FunctionType::Param& param )
{
	if( param.value_type != ValueType::Value )
	{
		const ManglerState::NodeHolder ref_node( mangler_state );
		mangler_state.Push( "R" );
		if( param.value_type == ValueType::ReferenceMut )
			EncodeTypeName( mangler_state, param.type );
		else
		{
			const ManglerState::NodeHolder konst_node( mangler_state );
			mangler_state.Push( "K" );
			EncodeTypeName( mangler_state, param.type );
		}
	}
	else
		EncodeTypeName( mangler_state, param.type );
}

void EncodeFunctionParams( ManglerState& mangler_state, const llvm::ArrayRef<FunctionType::Param> params )
{
	for( const FunctionType::Param& param : params )
	{
		if(
			param.value_type == ValueType::Value &&
			param.type.GetFundamentalType() != nullptr &&
			param.type.GetFundamentalType()->fundamental_type == U_FundamentalType::void_ )
		{
			// We need to distinguish between function with no params (with "v" for params) and function with single "void" param.
			// So, mark real "void: params with "const".
			// Normaly we do not use "konst" prefix for value params, so, "void" type is single exception.
			const ManglerState::NodeHolder konst_node( mangler_state );
			mangler_state.Push( "K" );
			EncodeTypeName( mangler_state, param.type );
		}
		else
			EncodeFunctionParam( mangler_state, param );
	}

	if( params.empty() )
		mangler_state.Push( "v" );
}

void EncodeCoroutineType( ManglerState& mangler_state, const ClassPtr class_type )
{
	const auto coroutine_type_description= std::get_if<CoroutineTypeDescription>( &class_type->generated_class_data );
	U_ASSERT( coroutine_type_description != nullptr );

	const ManglerState::NodeHolder result_node( mangler_state );
	{
		const ManglerState::NodeHolder name_node( mangler_state );
		mangler_state.PushLengthPrefixed( class_type->members->GetThisNamespaceName() );
	}

	// Encode coroutine type as template with several arguments.
	mangler_state.Push( "I" );

	if( coroutine_type_description->return_value_type == ValueType::Value )
		EncodeTypeName( mangler_state, coroutine_type_description->return_type );
	else
	{
		const ManglerState::NodeHolder ref_node( mangler_state );
		mangler_state.Push( "R" );
		if( coroutine_type_description->return_value_type == ValueType::ReferenceMut )
			EncodeTypeName( mangler_state, coroutine_type_description->return_type );
		else
		{
			const ManglerState::NodeHolder konst_node( mangler_state );
			mangler_state.Push( "K" );
			EncodeTypeName( mangler_state, coroutine_type_description->return_type );
		}
	}

	// Encode non-sync tag, if it exists.
	if( coroutine_type_description->non_sync )
	{
		mangler_state.Push( "L" );
		mangler_state.Push( EncodeFundamentalType( U_FundamentalType::bool_ ) );
		mangler_state.Push( "1" );
		mangler_state.Push( "E" );
	}

	// Encode inner reference kind as variable template parameter.
	mangler_state.Push( "L" );
	mangler_state.Push( EncodeFundamentalType( U_FundamentalType::u32_ ) );
	mangler_state.Push( std::to_string( size_t(coroutine_type_description->inner_reference_type ) ) );
	mangler_state.Push( "E" );

	// Do not encode coroutine kind here, because coroutine class name contains kind.

	mangler_state.Push( "E" );
}

void EncodeTypeName( ManglerState& mangler_state, const Type& type )
{
	if( const auto fundamental_type= type.GetFundamentalType() )
		mangler_state.Push( EncodeFundamentalType( fundamental_type->fundamental_type ) );
	else if( const auto array_type= type.GetArrayType() )
	{
		const ManglerState::NodeHolder result_node( mangler_state );
		mangler_state.Push( "A" );
		mangler_state.Push( std::to_string( array_type->element_count ) );
		mangler_state.Push( "_" );
		EncodeTypeName( mangler_state, array_type->element_type );
	}
	else if( const auto tuple_type= type.GetTupleType() )
	{
		// Encode tuples, like type templates.
		const ManglerState::NodeHolder result_node( mangler_state );
		{
			const ManglerState::NodeHolder name_node( mangler_state );
			mangler_state.PushLengthPrefixed( Keyword( Keywords::tup_ ) );
		}

		mangler_state.Push( "I" );
		for( const Type& element_type : tuple_type->element_types )
			EncodeTypeName( mangler_state, element_type );
		mangler_state.Push( "E" );
	}
	else if( const auto class_type= type.GetClassType() )
	{
		if( const auto typeinfo_class_description= std::get_if<TypeinfoClassDescription>( &class_type->generated_class_data ) )
		{
			const ManglerState::NodeHolder result_node( mangler_state );
			{
				const ManglerState::NodeHolder name_node( mangler_state );
				mangler_state.PushLengthPrefixed( class_type->members->GetThisNamespaceName() );
			}
			{
				const ManglerState::NodeHolder args_node( mangler_state );

				TemplateArgs typeinfo_pseudo_args;
				typeinfo_pseudo_args.push_back( typeinfo_class_description->source_type );
				EncodeTemplateArgs( mangler_state, typeinfo_pseudo_args );
			}
		}
		else if( std::get_if< CoroutineTypeDescription >( &class_type->generated_class_data ) != nullptr )
			EncodeCoroutineType( mangler_state, class_type );
		else if( const auto base_template= std::get_if< Class::BaseTemplate >( &class_type->generated_class_data ) )
		{
			const ManglerState::NodeHolder result_node( mangler_state );
			if( base_template->class_template->parent_namespace->GetParent() != nullptr )
			{
				mangler_state.Push( "N" );
				EncodeTemplateClassName( mangler_state, *class_type );
				mangler_state.Push( "E" );
			}
			else
				EncodeTemplateClassName( mangler_state, *class_type );
		}
		else
			EncodeNestedName( mangler_state, class_type->members->GetThisNamespaceName(), *class_type->members->GetParent() );
	}
	else if( const auto enum_type= type.GetEnumType() )
		EncodeNestedName( mangler_state, enum_type->members.GetThisNamespaceName(), *enum_type->members.GetParent() );
	else if( const auto raw_pointer= type.GetRawPointerType() )
	{
		const ManglerState::NodeHolder result_node( mangler_state );
		mangler_state.Push( "P" );
		EncodeTypeName( mangler_state, raw_pointer->element_type );
	}
	else if( const auto function_pointer= type.GetFunctionPointerType() )
	{
		const ManglerState::NodeHolder result_node( mangler_state );
		mangler_state.Push( "P" );
		EncodeFunctionTypeName( mangler_state, function_pointer->function_type );
	}
	else U_ASSERT(false);
}

void EncodeFunctionTypeName( ManglerState& mangler_state, const FunctionType& function_type )
{
	const ManglerState::NodeHolder function_node( mangler_state );

	if( function_type.calling_convention == llvm::CallingConv::C ){}
	else if( function_type.calling_convention == llvm::CallingConv::Cold )
	{
		mangler_state.Push( "U" );
		mangler_state.PushLengthPrefixed( "cold" );
	}
	else if( function_type.calling_convention == llvm::CallingConv::Fast )
	{
		mangler_state.Push( "U" );
		mangler_state.PushLengthPrefixed( "fast" );
	}
	else if( function_type.calling_convention == llvm::CallingConv::X86_StdCall )
	{
		mangler_state.Push( "U" );
		mangler_state.PushLengthPrefixed( "stdcall" );
	}
	else U_ASSERT(false);

	mangler_state.Push( "F" );

	{
		FunctionType::Param ret;
		ret.value_type= function_type.return_value_type;
		ret.type= function_type.return_type;
		EncodeFunctionParam( mangler_state, ret );
	}

	EncodeFunctionParams( mangler_state, function_type.params );

	if( !function_type.return_references.empty() )
	{
		const ManglerState::NodeHolder rr_node( mangler_state );
		mangler_state.Push( "_RR" );
		mangler_state.Push( Base36Digit(function_type.return_references.size()) );

		for( const FunctionType::ParamReference& arg_and_tag : function_type.return_references )
		{
			U_ASSERT( arg_and_tag.first  < 36u );
			U_ASSERT( arg_and_tag.second < 36u || arg_and_tag.second == FunctionType::c_arg_reference_tag_number );

			mangler_state.Push( Base36Digit(arg_and_tag.first) );
			mangler_state.Push(
				arg_and_tag.second == FunctionType::c_arg_reference_tag_number
				? '_'
				: Base36Digit(arg_and_tag.second) );
		}
	}
	if( !function_type.references_pollution.empty() )
	{
		const ManglerState::NodeHolder rp_node( mangler_state );
		mangler_state.Push( "_RP" );
		U_ASSERT( function_type.references_pollution.size() < 36u );
		mangler_state.Push( Base36Digit(function_type.references_pollution.size()) );

		for( const FunctionType::ReferencePollution& pollution : function_type.references_pollution )
		{
			U_ASSERT( pollution.dst.first  < 36u );
			U_ASSERT( pollution.dst.second < 36u || pollution.dst.second == FunctionType::c_arg_reference_tag_number );
			U_ASSERT( pollution.src.first  < 36u );
			U_ASSERT( pollution.src.second < 36u || pollution.src.second == FunctionType::c_arg_reference_tag_number );

			mangler_state.Push( Base36Digit(pollution.dst.first) );
			mangler_state.Push(
				pollution.dst.second == FunctionType::c_arg_reference_tag_number
				? '_'
				: Base36Digit(pollution.dst.second) );
			mangler_state.Push( Base36Digit(pollution.src.first) );
			mangler_state.Push(
				pollution.src.second == FunctionType::c_arg_reference_tag_number
				? '_'
				: Base36Digit(pollution.src.second) );
		}
	}
	if( function_type.unsafe )
	{
		const ManglerState::NodeHolder unsafe_node( mangler_state );
		mangler_state.Push( "unsafe" );
	}

	mangler_state.Push( "E" );
}

const std::unordered_map<std::string_view, std::string_view> g_op_names
{
	{ "+", "pl" },
	{ "-", "mi" },
	{ "*", "ml" },
	{ "/", "dv" },
	{ "%", "rm" },

	{ "==", "eq" },
	{ "<=>", "ss" }, // C++ spaceship operator

	{ "&", "an" },
	{ "|", "or" },
	{ "^", "eo" },

	{ "<<", "ls" },
	{ ">>", "rs" },

	{ "+=", "pL" },
	{ "-=", "mI" },
	{ "*=", "mL" },
	{ "/=", "dV" },
	{ "%=", "rM" },

	{ "&=", "aN" },
	{ "|=", "oR" },
	{ "^=", "eO" },

	{ "<<=", "lS" },
	{ ">>=", "rS" },

	{ "!", "nt" },
	{ "~", "co" },

	{ "=", "aS" },
	{ "++", "pp" },
	{ "--", "mm" },

	{ "()", "cl" },
	{ "[]", "ix" },
};

// Returns empty string if func_name is not special.
std::string_view DecodeOperator( const std::string_view func_name )
{
	const auto it= g_op_names.find( func_name );
	if( it != g_op_names.end() )
		return it->second;

	return "";
}

} // namespace

std::string ManglerItaniumABI::MangleFunction(
	const NamesScope& parent_scope,
	const std::string_view function_name,
	const FunctionType& function_type,
	const std::optional<llvm::ArrayRef<TemplateArg>> template_args )
{
	state_.Push( "_Z" );

	std::string name_prefixed( DecodeOperator( function_name ) );
	if( name_prefixed.empty() )
	{
		name_prefixed= std::to_string( function_name.size() );
		name_prefixed+= function_name;
	}

	// Normally we should use "T_" instead of "S_" for referencing template parameters in function signature.
	// But without "T_" it works fine too.
	if( template_args != std::nullopt )
	{
		const ManglerState::NodeHolder result_node( state_ );

		if( parent_scope.GetParent() != nullptr )
		{
			state_.Push( "N" );
			{
				const ManglerState::NodeHolder name_node( state_ );
				EncodeNamespacePrefix_r( state_, parent_scope );
				state_.Push( name_prefixed );
			}

			EncodeTemplateArgs( state_, *template_args );
			state_.Push( "Ev" );
		}
		else
		{
			{
				const ManglerState::NodeHolder name_node( state_ );
				state_.Push( name_prefixed );
			}

			EncodeTemplateArgs( state_, *template_args );
			state_.Push( "v" );
		}
	}
	else
	{
		if( parent_scope.GetParent() != nullptr )
		{
			state_.Push( "N" );
			EncodeNamespacePrefix_r( state_, parent_scope );
			state_.Push( name_prefixed );
			state_.Push( "E" );
		}
		else
			state_.Push( name_prefixed );
	}

	EncodeFunctionParams( state_, function_type.params );

	return state_.TakeResult();
}

std::string ManglerItaniumABI::MangleGlobalVariable( const NamesScope& parent_scope, const std::string_view variable_name, const Type& type, const bool is_constant )
{
	(void)type;
	(void)is_constant;

	// Variables inside global namespace have simple names.
	if( parent_scope.GetParent() == nullptr )
		return std::string(variable_name);

	state_.Push( "_Z" );
	EncodeNestedName( state_, variable_name, parent_scope );

	return state_.TakeResult();
}

std::string ManglerItaniumABI::MangleType( const Type& type )
{
	EncodeTypeName( state_, type );
	return state_.TakeResult();
}

std::string ManglerItaniumABI::MangleTemplateArgs( const llvm::ArrayRef<TemplateArg> template_parameters )
{
	EncodeTemplateArgs( state_, template_parameters );
	return state_.TakeResult();
}

std::string ManglerItaniumABI::MangleVirtualTable( const Type& type )
{
	state_.Push( "_ZTV" );
	EncodeTypeName( state_, type );
	return state_.TakeResult();
}

} // namespace

std::unique_ptr<IMangler> CreateManglerItaniumABI()
{
	return std::make_unique<ManglerItaniumABI>();
}

} // namespace U
