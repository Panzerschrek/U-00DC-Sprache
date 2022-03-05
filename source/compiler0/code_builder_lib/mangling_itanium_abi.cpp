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
	std::vector<Substitution> substitutions_;
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
		const std::string& function_name,
		const FunctionType& function_type,
		const TemplateArgs* template_args ) override;
	std::string MangleGlobalVariable( const NamesScope& parent_scope, const std::string& variable_name, const Type& type, bool is_constant ) override;
	std::string MangleType( const Type& type ) override;
	std::string MangleTemplateArgs( const TemplateArgs& template_args ) override;
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
				size_t n= i - 1u;
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
void EncodeNamespacePrefix_r( ManglerState& mangler_state, const NamesScope& names_scope );

void EncodeTemplateArgs( ManglerState& mangler_state, const TemplateArgs& template_args )
{
	mangler_state.Push( "I" );

	for( const TemplateArg& template_arg : template_args )
	{
		if( const auto type= std::get_if<Type>( &template_arg ) )
			EncodeTypeName( mangler_state, *type );
		else if( const auto variable= std::get_if<Variable>( &template_arg ) )
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
	U_ASSERT( the_class.base_template != std::nullopt );

	{
		const ManglerState::NodeHolder name_node( mangler_state );

		// Skip template parameters namespace.
		U_ASSERT( the_class.members->GetParent() != nullptr );
		if( const auto parent= the_class.members->GetParent()->GetParent() )
			if( !parent->GetThisNamespaceName().empty() )
				EncodeNamespacePrefix_r( mangler_state, *parent );

		mangler_state.PushLengthPrefixed( the_class.base_template->class_template->syntax_element->name_ );
	}

	EncodeTemplateArgs( mangler_state, the_class.base_template->signature_args );
}

void EncodeNamespacePrefix_r( ManglerState& mangler_state, const NamesScope& names_scope )
{
	if( const ClassPtr the_class= names_scope.GetClass() )
	{
		if( the_class->base_template != std::nullopt )
		{
			const ManglerState::NodeHolder result_node( mangler_state );
			EncodeTemplateClassName( mangler_state, *the_class );
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

void EncodeNestedName( ManglerState& mangler_state, const std::string& name, const NamesScope& parent_scope )
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
	case U_FundamentalType::Void: return "v";
	case U_FundamentalType::Bool: return "b";
	case U_FundamentalType:: i8: return "a"; // C++ signed char
	case U_FundamentalType:: u8: return "h"; // C++ unsigned char
	case U_FundamentalType::i16: return "s";
	case U_FundamentalType::u16: return "t";
	case U_FundamentalType::i32: return "i";
	case U_FundamentalType::u32: return "j";
	case U_FundamentalType::i64: return "x";
	case U_FundamentalType::u64: return "y";
	case U_FundamentalType::i128: return "n";
	case U_FundamentalType::u128: return "o";
	case U_FundamentalType::f32: return "f";
	case U_FundamentalType::f64: return "d";
	case U_FundamentalType::char8 : return "c"; // C++ char
	case U_FundamentalType::char16: return "Ds"; // C++ char16_t
	case U_FundamentalType::char32: return "Di"; // C++ char32_t
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

void EncodeFunctionParams( ManglerState& mangler_state, const ArgsVector<FunctionType::Param>& params )
{
	for( const FunctionType::Param& param : params )
	{
		if(
			param.value_type == ValueType::Value &&
			param.type.GetFundamentalType() != nullptr &&
			param.type.GetFundamentalType()->fundamental_type == U_FundamentalType::Void )
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

void EncodeTypeName( ManglerState& mangler_state, const Type& type )
{
	if( const auto fundamental_type= type.GetFundamentalType() )
		mangler_state.Push( EncodeFundamentalType( fundamental_type->fundamental_type ) );
	else if( const auto array_type= type.GetArrayType() )
	{
		const ManglerState::NodeHolder result_node( mangler_state );
		mangler_state.Push( "A" );
		mangler_state.Push( std::to_string( array_type->size ) );
		mangler_state.Push( "_" );
		EncodeTypeName( mangler_state, array_type->type );
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
		for( const Type& element_type : tuple_type->elements )
			EncodeTypeName( mangler_state, element_type );
		mangler_state.Push( "E" );
	}
	else if( const auto class_type= type.GetClassType() )
	{
		if( class_type->typeinfo_type != std::nullopt )
		{
			const ManglerState::NodeHolder result_node( mangler_state );
			{
				const ManglerState::NodeHolder name_node( mangler_state );
				mangler_state.PushLengthPrefixed( class_type->members->GetThisNamespaceName() );
			}
			{
				const ManglerState::NodeHolder args_node( mangler_state );

				TemplateArgs typeinfo_pseudo_args;
				typeinfo_pseudo_args.push_back( *class_type->typeinfo_type );
				EncodeTemplateArgs( mangler_state, typeinfo_pseudo_args );
			}
		}
		else if( class_type->base_template != std::nullopt )
		{
			const ManglerState::NodeHolder result_node( mangler_state );
			if( class_type->base_template->class_template->parent_namespace->GetParent() != nullptr )
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
		EncodeTypeName( mangler_state, raw_pointer->type );
	}
	else if( const auto function_pointer= type.GetFunctionPointerType() )
	{
		const ManglerState::NodeHolder result_node( mangler_state );
		mangler_state.Push( "P" );
		EncodeTypeName( mangler_state, function_pointer->function );
	}
	else if( const auto function= type.GetFunctionType() )
	{
		const ManglerState::NodeHolder function_node( mangler_state );

		if( function->calling_convention == llvm::CallingConv::C ){}
		else if( function->calling_convention == llvm::CallingConv::Cold )
		{
			mangler_state.Push( "U" );
			mangler_state.PushLengthPrefixed( "cold" );
		}
		else if( function->calling_convention == llvm::CallingConv::Fast )
		{
			mangler_state.Push( "U" );
			mangler_state.PushLengthPrefixed( "fast" );
		}
		else if( function->calling_convention == llvm::CallingConv::X86_StdCall )
		{
			mangler_state.Push( "U" );
			mangler_state.PushLengthPrefixed( "stdcall" );
		}
		else U_ASSERT(false);

		mangler_state.Push( "F" );

		{
			FunctionType::Param ret;
			ret.value_type= function->return_value_type;
			ret.type= function->return_type;
			EncodeFunctionParam( mangler_state, ret );
		}

		EncodeFunctionParams( mangler_state, function->params );

		if( !function->return_references.empty() )
		{
			const ManglerState::NodeHolder rr_node( mangler_state );
			mangler_state.Push( "_RR" );
			mangler_state.Push( Base36Digit(function->return_references.size()) );

			for( const FunctionType::ParamReference& arg_and_tag : function->return_references )
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
		if( !function->references_pollution.empty() )
		{
			const ManglerState::NodeHolder rp_node( mangler_state );
			mangler_state.Push( "_RP" );
			U_ASSERT( function->references_pollution.size() < 36u );
			mangler_state.Push( Base36Digit(function->references_pollution.size()) );

			for( const FunctionType::ReferencePollution& pollution : function->references_pollution )
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
		if( function->unsafe )
		{
			const ManglerState::NodeHolder unsafe_node( mangler_state );
			mangler_state.Push( "unsafe" );
		}

		mangler_state.Push( "E" );
	}
	else U_ASSERT(false);
}

const ProgramStringMap<std::string> g_op_names
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

const std::string g_empty_op_name;

// Returns empty string if func_name is not special.
const std::string& DecodeOperator( const std::string& func_name )
{
	const auto it= g_op_names.find( func_name );
	if( it != g_op_names.end() )
		return it->second;

	return g_empty_op_name;
}

} // namespace

std::string ManglerItaniumABI::MangleFunction(
	const NamesScope& parent_scope,
	const std::string& function_name,
	const FunctionType& function_type,
	const TemplateArgs* const template_args )
{
	state_.Push( "_Z" );

	std::string name_prefixed= DecodeOperator( function_name );
	if( name_prefixed.empty() )
	{
		name_prefixed= std::to_string( function_name.size() );
		name_prefixed+= function_name;
	}

	// Normally we should use "T_" instead of "S_" for referencing template parameters in function signature.
	// But without "T_" it works fine too.
	if( template_args != nullptr )
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

std::string ManglerItaniumABI::MangleGlobalVariable( const NamesScope& parent_scope, const std::string& variable_name, const Type& type, const bool is_constant )
{
	(void)type;
	(void)is_constant;

	// Variables inside global namespace have simple names.
	if( parent_scope.GetParent() == nullptr )
		return variable_name;

	state_.Push( "_Z" );
	EncodeNestedName( state_, variable_name, parent_scope );

	return state_.TakeResult();
}

std::string ManglerItaniumABI::MangleType( const Type& type )
{
	EncodeTypeName( state_, type );
	return state_.TakeResult();
}

std::string ManglerItaniumABI::MangleTemplateArgs( const TemplateArgs& template_parameters )
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
