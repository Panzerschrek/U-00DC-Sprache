#include <ostream>

#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "program_string.hpp"
#include "program_writer.hpp"

namespace U
{

namespace Synt
{

namespace
{

class ProgramWriter
{
public:
	ProgramWriter( std::ostream& stream )
		: stream_(stream)
	{}

private:
	std::ostream& stream_;

private:

void WriteStringEscaped( const std::string_view s ) const
{
	std::string escaped;
	for( const char c : s )
	{
		switch(c)
		{
		case '"':
			escaped.push_back( '\\' );
			escaped.push_back( '\"' );
			break;
		case '\'':
			escaped.push_back( '\'' );
			break;
		case '\\':
			escaped.push_back( '\\' );
			escaped.push_back( '\\' );
			break;
		case '\b':
			escaped.push_back( '\\' );
			escaped.push_back( 'b' );
			break;
		case '\f':
			escaped.push_back( '\\' );
			escaped.push_back( 'f' );
			break;
		case '\n':
			escaped.push_back( '\\' );
			escaped.push_back( 'n' );
			break;
		case '\r':
			escaped.push_back( '\\' );
			escaped.push_back( 'r' );
			break;
		case '\t':
			escaped.push_back( '\\' );
			escaped.push_back( 't' );
			break;
		case '\0':
			escaped.push_back( '\\' );
			escaped.push_back( '0' );
			break;
		default:
			if( sprache_char(c) < 32 )
			{
				escaped.push_back('\\');
				escaped.push_back('u');
				for( uint32_t i= 0u; i < 4u; ++i )
				{
					const sprache_char val= ( sprache_char(c) >> ((3u-i) * 4u ) ) & 15u;
					if( val < 10u )
						escaped.push_back( char( '0' + int(val) ) );
					else
						escaped.push_back( char( 'a' + int(val-10u) ) );
				}
			}
			else
				escaped.push_back(c);
			break;
		};
	}
	stream_ << "\"" << escaped << "\"";
}

void WriteRawMixin( const Mixin& mixin ) const
{
	stream_ << Keyword( Keywords::mixin_ );
	stream_ << "( ";
	ElementWrite( mixin.expression );
	stream_ << " )";
}

public:

template<typename T>
void ElementWrite( const std::unique_ptr<const T>& el ) const
{
	ElementWrite( *el );
}

template<typename T>
void ElementWrite( const std::unique_ptr<T>& el ) const
{
	ElementWrite( *el );
}

template<typename ... Args>
void ElementWriteVariant( const std::variant<Args...>& v ) const
{
	std::visit( [&]( const auto& el ) { ElementWrite( el ); }, v );
}

void ElementWrite( const ComplexName& complex_name ) const
{
	ElementWriteVariant( complex_name );
}

void ElementWrite( const TypeName& type_name ) const
{
	if( const auto mixin_ptr= std::get_if< std::unique_ptr<const Mixin> >( &type_name ) )
	{
		// Hack to distinguish between mixins in type names and in blocks/global space.
		WriteRawMixin( **mixin_ptr );
	}
	else
		ElementWriteVariant( type_name );
}

void ElementWrite( const Expression& expression ) const
{
	if( const auto mixin_ptr= std::get_if< std::unique_ptr<const Mixin> >( &expression ) )
	{
		// Hack to distinguish between mixins in expressions and in blocks/global space.
		WriteRawMixin( **mixin_ptr );
	}
	else
		ElementWriteVariant( expression );
}

void ElementWrite( const EmptyVariant& )  const {}

void ElementWrite( const RootNamespaceNameLookup& root_namespace_lookup ) const
{
	stream_ << "::" << root_namespace_lookup.name;
}

void ElementWrite( const RootNamespaceNameLookupCompletion& root_namespace_lookup_completion ) const
{
	stream_ << "::" << root_namespace_lookup_completion.name;
}

void ElementWrite( const NameLookup& name_lookup ) const
{
	stream_ << name_lookup.name;
}

void ElementWrite( const NameLookupCompletion& name_lookup_completion ) const
{
	stream_ << name_lookup_completion.name;
}

void ElementWrite( const NamesScopeNameFetch& names_scope_fetch ) const
{
	ElementWrite( names_scope_fetch.base );
	stream_ << "::" << names_scope_fetch.name;
}

void ElementWrite( const NamesScopeNameFetchCompletion& names_scope_fetch_completion ) const
{
	ElementWrite( names_scope_fetch_completion.base );
	stream_ << "::" << names_scope_fetch_completion.name;
}

void ElementWrite( const TemplateParameterization& template_parameterization ) const
{
	ElementWrite( template_parameterization.base );

	stream_ << "</ ";
	for( const Expression& expr : template_parameterization.template_args )
	{
		ElementWrite( expr );
		if( &expr != &template_parameterization.template_args.back() )
			stream_ << ", ";
	}
	stream_ << " />";
}

void ElementWrite( const ArrayTypeName& array_type_name ) const
{
	stream_ << "[ ";
	ElementWrite( array_type_name.element_type );
	stream_ << ", ";
	ElementWrite( array_type_name.size );
	stream_ << " ]";
}

void ElementWrite( const TupleType& tuple_type_name ) const
{
	stream_ << Keyword( Keywords::tup_ );
	if( tuple_type_name.element_types.empty() )
		stream_ << "[]";
	else
	{
		stream_ << "[ ";
		for( const TypeName& element_type : tuple_type_name.element_types )
		{
			ElementWrite( element_type );
			if( &element_type != &tuple_type_name.element_types.back() )
				stream_ << ", ";
		}
		stream_ << " ]";
	}
}

void ElementWrite( const RawPointerType& raw_pointer_type_name ) const
{
	stream_ << "$";
	stream_ << "(";
	ElementWrite( raw_pointer_type_name.element_type );
	stream_ << ")";
}

void ElementWrite( const CoroutineType& coroutine_name ) const
{
	switch( coroutine_name.kind )
	{
	case CoroutineType::Kind::Generator:
		stream_ << Keyword( Keywords::generator_ );
		break;
	case CoroutineType::Kind::AsyncFunc:
		stream_ << Keyword( Keywords::async_ );
		break;
	};

	if( !coroutine_name.inner_references.empty() )
	{
		stream_ << "'";
		for( size_t i= 0; i < coroutine_name.inner_references.size(); ++i )
		{
			ElementWrite( coroutine_name.inner_references[i] );
			if( i + 1 < coroutine_name.inner_references.size() )
				stream_ << ", ";
		}
		stream_ << "'";
	}

	ElementWrite( coroutine_name.non_sync_tag );

	stream_ << ":";
	ElementWrite( coroutine_name.return_type );

	ElementWrite( coroutine_name.return_value_reference_modifier );
	ElementWrite( coroutine_name.return_value_mutability_modifier );
}

void ElementWrite( const TypeofTypeName& typeof_type_name ) const
{
	stream_ << Keyword( Keywords::typeof_ ) << "( ";
	ElementWrite( typeof_type_name.expression );
	stream_ << " )";
}

void ElementWrite( const FunctionType& function_type_name ) const
{
	stream_ << "( " << Keyword( Keywords::fn_ );
	WriteFunctionParamsList( function_type_name );
	WriteFunctionTypeEnding( function_type_name );
	stream_ << " )";
}

void ElementWrite( const FunctionParam& param ) const
{
	if( param.name == Keywords::this_ )
	{
		if( param.reference_modifier == ReferenceModifier::None )
			stream_ << Keyword( Keywords::byval_ ) << " ";
		if( param.mutability_modifier != MutabilityModifier::None )
		{
			ElementWrite( param.mutability_modifier );
			stream_ << " ";
		}
		stream_ << Keyword( Keywords::this_ );
	}
	else
	{
		ElementWrite( param.type );

		stream_ << " ";

		ElementWrite( param.reference_modifier );

		ElementWrite( param.mutability_modifier );

		if( param.mutability_modifier != MutabilityModifier::None )
			stream_ << " ";
		stream_ << param.name;
	}
}

void ElementWrite( const BinaryOperator& binary_operator ) const
{
	// Add () to ensure order of evaluation is correct.
	stream_ << "( ";
	ElementWrite( binary_operator.left  );
	stream_ << " " << BinaryOperatorToString(binary_operator.operator_type) << " ";
	ElementWrite( binary_operator.right );
	stream_ << " )";
}

void ElementWrite( const TernaryOperator& ternary_operator ) const
{
	stream_ << "( ";
	ElementWrite( ternary_operator.condition );
	stream_ << " ? ";
	ElementWrite( ternary_operator.branches[0] );
	stream_ << " : ";
	ElementWrite( ternary_operator.branches[1] );
	stream_ << " )";
}

void ElementWrite( const ReferenceToRawPointerOperator& reference_to_raw_pointer_operator ) const
{
	stream_ << "$<";
	stream_ << "(";
	ElementWrite( reference_to_raw_pointer_operator.expression );
	stream_ << ")";
}

void ElementWrite( const RawPointerToReferenceOperator& raw_pointer_to_reference_operator ) const
{
	stream_ << "$>";
	stream_ << "(";
	ElementWrite( raw_pointer_to_reference_operator.expression );
	stream_ << ")";
}

void ElementWrite( const IntegerNumericConstant& numeric_constant ) const
{
	stream_ << numeric_constant.num.value;
	stream_ << numeric_constant.num.type_suffix.data();
}

void ElementWrite( const FloatingPointNumericConstant& numeric_constant ) const
{
	stream_.precision( std::numeric_limits<double>::digits10 + 1 );
	stream_.flags( stream_.flags() | std::ios_base::showpoint );
	stream_ << numeric_constant.num.value;
	stream_ << numeric_constant.num.type_suffix.data();
}

void ElementWrite( const StringLiteral& string_literal ) const
{
	WriteStringEscaped( string_literal.value );
	stream_ << string_literal.type_suffix;
}

void ElementWrite( const CharLiteral& char_literal ) const
{
	stream_ << "'";
	switch(char_literal.code_point)
	{
	case '"': stream_ << "\""; break;
	case '\'': stream_ << "\\'"; break;
	case '\\': stream_ << "\\\\"; break;
	case '\b': stream_ << "\\b"; break;
	case '\f': stream_ << "\\f"; break;
	case '\n': stream_ << "\\n"; break;
	case '\r': stream_ << "\\r"; break;
	case '\t': stream_ << "\\t"; break;
	case '\0': stream_ << "\\0"; break;
	default:
		if( char_literal.code_point < 32 )
		{
			stream_ << "\\u";
			for( uint32_t i= 0u; i < 4u; ++i )
			{
				const sprache_char val= ( sprache_char(char_literal.code_point) >> ((3u-i) * 4u ) ) & 15u;
				if( val < 10u )
					stream_ << char( '0' + int(val) );
				else
					stream_ << char( 'a' + int(val-10u) );
			}
		}
		if( char_literal.code_point <= 127 )
			stream_ << char(char_literal.code_point);
		else
		{
			std::string s;
			PushCharToUTF8String( char_literal.code_point, s );
			stream_ << s;
		}

		break;
	};
	stream_ << "'";

	if( char_literal.type_suffix[0] != 0 )
		stream_ << char_literal.type_suffix.data();
}

void ElementWrite( const BooleanConstant& boolean_constant ) const
{
	stream_ << ( boolean_constant.value ? Keyword( Keywords::true_ ) : Keyword( Keywords::false_ ) );
}

void ElementWrite( const MoveOperator& move_operator ) const
{
	stream_ << Keyword( Keywords::move_ ) << "( " << move_operator.var_name << " )";
}

void ElementWrite( const MoveOperatorCompletion& move_operator_completion ) const
{
	stream_ << Keyword( Keywords::move_ ) << "( " << move_operator_completion.var_name << " )";
}

void ElementWrite( const TakeOperator& take_operator ) const
{
	stream_ << Keyword( Keywords::take_ ) << "( ";
	ElementWrite( take_operator.expression );
	stream_ << " )";
}

void ElementWrite( const Lambda& lambda ) const
{
	U_UNUSED(lambda);
	// For now do not print whole lambda - just create dummy.
	stream_ << Keyword( Keywords::lambda_ ) << "(){}";
}

void ElementWrite( const CastRef& cast_ref ) const
{
	stream_ << Keyword( Keywords::cast_ref_ ) << "</ ";
	ElementWrite( cast_ref.type );
	stream_ << " />( ";
	ElementWrite( cast_ref.expression );
	stream_ << " )";
}

void ElementWrite( const CastRefUnsafe& cast_ref_unsafe ) const
{
	stream_ << Keyword( Keywords::cast_ref_unsafe_ ) << "</ ";
	ElementWrite( cast_ref_unsafe.type );
	stream_ << " />( ";
	ElementWrite( cast_ref_unsafe.expression );
	stream_ << " )";
}

void ElementWrite( const CastImut& cast_imut ) const
{
	stream_ << Keyword( Keywords::cast_imut_ ) << "( ";
	ElementWrite( cast_imut.expression );
	stream_ << " )";
}

void ElementWrite( const CastMut& cast_mut ) const
{
	stream_ << Keyword( Keywords::cast_mut_ ) << "( ";
	ElementWrite( cast_mut.expression );
	stream_ << " )";
}

void ElementWrite( const Embed& embed ) const
{
	stream_ << Keyword( Keywords::embed_ );

	if( embed.element_type != std::nullopt )
	{
		stream_ << "</";
		ElementWrite( *embed.element_type );
		stream_ << "/>";
	}

	stream_ << "( ";
	ElementWrite( embed.expression );
	stream_ << " )";
}

void ElementWrite( const ExternalFunctionAccess& external_function_access ) const
{
	stream_ << Keyword( Keywords::import_ ) << " " << Keyword( Keywords::fn_ );

	stream_ << "</";
	ElementWrite( external_function_access.type );
	stream_ << "/>";

	stream_ << "( ";
	WriteStringEscaped( external_function_access.name );
	stream_ << " )";
}

void ElementWrite( const ExternalVariableAccess& external_variable_access ) const
{
	stream_ << Keyword( Keywords::import_ ) << " " << Keyword( Keywords::var_ );

	stream_ << "</";
	ElementWrite( external_variable_access.type );
	stream_ << "/>";

	stream_ << "( ";
	WriteStringEscaped( external_variable_access.name );
	stream_ << " )";
}

void ElementWrite( const TypeInfo& typeinfo_ ) const
{
	stream_ << Keyword( Keywords::typeinfo_ ) << "</ ";
	ElementWrite( typeinfo_.type );
	stream_ << " />";
}

void ElementWrite( const SameType& same_type ) const
{
	stream_ << Keyword( Keywords::same_type_ ) << "</ ";
	ElementWrite( same_type.l );
	stream_ << ", ";
	ElementWrite( same_type.r );
	stream_ << " />";
}

void ElementWrite( const NonSyncExpression& non_sync_expression ) const
{
	stream_ << "</ ";
	ElementWrite( non_sync_expression.type );
	stream_ << " />";
}

void ElementWrite( const SafeExpression& safe_expression ) const
{
	stream_ << Keyword( Keywords::safe_ );
	stream_ << "( ";
	ElementWrite( safe_expression.expression );
	stream_ << " )";
}

void ElementWrite( const UnsafeExpression& unsafe_expression ) const
{
	stream_ << Keyword( Keywords::unsafe_ );
	stream_ << "( ";
	ElementWrite( unsafe_expression.expression );
	stream_ << " )";
}

void ElementWrite( const UnaryMinus& unary_minus ) const
{
	// Add () to ensure order of evaluation is correct.
	stream_ << "( ";
	stream_ << OverloadedOperatorToString( OverloadedOperator::Sub );
	ElementWrite( unary_minus.expression );
	stream_ << " )";
}

void ElementWrite( const LogicalNot& logical_not ) const
{
	// Add () to ensure order of evaluation is correct.
	stream_ << "( ";
	stream_ << OverloadedOperatorToString( OverloadedOperator::LogicalNot );
	ElementWrite( logical_not.expression );
	stream_ << " )";
}

void ElementWrite( const BitwiseNot& bitwise_not ) const
{
	// Add () to ensure order of evaluation is correct.
	stream_ << "( ";
	stream_ << OverloadedOperatorToString( OverloadedOperator::BitwiseNot );
	ElementWrite( bitwise_not.expression );
	stream_ << " )";
}

void ElementWrite( const SubscriptOperator& subscript_operator ) const
{
	// Add () to ensure order of evaluation is correct.
	stream_ << "( ";

	ElementWrite( subscript_operator.expression );
	stream_ << "[ ";
	ElementWrite( subscript_operator.index );
	stream_ << " ]";

	stream_ << " )";
}

void ElementWrite( const MemberAccessOperator& member_access_operator ) const
{
	// Add () to ensure order of evaluation is correct.
	stream_ << "( ";

	ElementWrite( member_access_operator.expression );

	stream_ << "." << member_access_operator.member_name;
	if( member_access_operator.has_template_args )
	{
		stream_ << "</";
		for( const Expression& template_arg : member_access_operator.template_args )
		{
			ElementWrite( template_arg );
			if( &template_arg != &member_access_operator.template_args.back() )
				stream_ << ", ";
		}
		stream_ << "/>";
	}

	stream_ << " )";
}

void ElementWrite( const AwaitOperator& await_operator ) const
{
	// Add () to ensure order of evaluation is correct.
	stream_ << "( ";
	ElementWrite( await_operator.expression );
	stream_ << "." << Keyword( Keywords::await_ );
	stream_ << " )";
}

void ElementWrite( const VariableInitialization& variable_initialization ) const
{
	// Add () to ensure order of evaluation is correct.
	stream_ << "( ";
	ElementWrite( variable_initialization.type );
	ElementWrite( variable_initialization.initializer );
	stream_ << " )";
}

void ElementWrite( const MemberAccessOperatorCompletion& member_access_operator_completion ) const
{
	// Add () to ensure order of evaluation is correct.
	stream_ << "( ";
	ElementWrite( member_access_operator_completion.expression );
	stream_ << " )";

	stream_ << "." << member_access_operator_completion.member_name;
}

void ElementWrite( const CallOperator& call_operator ) const
{
	// Add () to ensure order of evaluation is correct.
	stream_ << "( ";

	ElementWrite( call_operator.expression );

	stream_ << "( ";
	for( const Expression& arg : call_operator.arguments )
	{
		ElementWrite( arg );

		if( &arg != &call_operator.arguments.back() )
			stream_ << ", ";
	}
	stream_ << ")";

	stream_ << " )";
}

void ElementWrite( const CallOperatorSignatureHelp& call_operator_signature_help ) const
{
	ElementWrite( call_operator_signature_help.expression );
	stream_ << "( ";
	stream_ << ")";
}

void ElementWrite( const Initializer& initializer ) const
{
	if( const auto constructor_initializer= std::get_if<ConstructorInitializer>( &initializer ) )
	{
		if( constructor_initializer->arguments.empty() )
		{
			stream_ << "()";
		}
		else
		{
			stream_ << "( ";
			for( const Expression& arg :constructor_initializer->arguments )
			{
				ElementWrite( arg );
				if( &arg != &constructor_initializer->arguments.back() )
					stream_ << ", ";
			}
			stream_ << " )";
		}
	}
	else if( const auto constructor_initializer_signature_help= std::get_if<ConstructorInitializerSignatureHelp>( &initializer ) )
	{
		(void)constructor_initializer_signature_help;
		stream_ << "()";
	}
	else if( const auto struct_named_initializer= std::get_if<StructNamedInitializer>( &initializer ) )
	{
		stream_ << "{ ";
		for( const StructNamedInitializer::MemberInitializer& member_initializer : struct_named_initializer->members_initializers )
		{
			stream_ << "." << member_initializer.name;
			ElementWrite( member_initializer.initializer );
			if( &member_initializer != &struct_named_initializer->members_initializers.back() )
				stream_ << ", ";
		}
		stream_ << "}";
	}
	else if( const auto sequence_initializer= std::get_if<SequenceInitializer>( &initializer ) )
	{
		if( sequence_initializer->initializers.empty() )
			stream_ << "[]";
		else
		{
			stream_ << "[ ";

			for( const Initializer& element_initializer : sequence_initializer->initializers )
			{
				ElementWrite( element_initializer );

				if( &element_initializer != &sequence_initializer->initializers.back() )
					stream_ << ", ";
			}

			stream_ << " ]";
		}
	}
	else
	{
		U_ASSERT(false);
		// Not implemented yet.
	}
}

void ElementWrite( const ReferenceModifier& reference_modifier ) const
{
	if( reference_modifier == ReferenceModifier::Reference )
		stream_ << "&";
}

void ElementWrite( const MutabilityModifier& mutability_modifier ) const
{
	switch(mutability_modifier)
	{
	case MutabilityModifier::None:
		break;
	case MutabilityModifier::Mutable:
		stream_ << Keyword( Keywords::mut_ );
		break;
	case MutabilityModifier::Immutable:
		stream_ << Keyword( Keywords::imut_ );
		break;
	case MutabilityModifier::Constexpr:
		stream_ << Keyword( Keywords::constexpr_ );
		break;
	}
}

void ElementWrite( const Function& function ) const
{
	WriteFunctionDeclaration( function );

	if( function.block == nullptr )
		stream_ << ";\n";
	else
	{} // TODO
}

void ElementWrite( const Class& class_ ) const
{
	stream_ << Keyword(class_.kind_attribute == ClassKindAttribute::Struct ? Keywords::struct_ : Keywords::class_ );
	stream_ << " " << class_.name;

	switch( class_.kind_attribute )
	{
	case ClassKindAttribute::Struct:
	case ClassKindAttribute::Class:
		break;
	case ClassKindAttribute::Final:
		stream_ << " " << Keyword( Keywords::final_ );
		break;
	case ClassKindAttribute::Polymorph:
		stream_ << " " << Keyword( Keywords::polymorph_ );
		break;
	case ClassKindAttribute::Interface:
		stream_ << " " << Keyword( Keywords::interface_ );
		break;
	case ClassKindAttribute::Abstract:
		stream_ << " " << Keyword( Keywords::abstract_ );
		break;
	};

	if( !class_.parents.empty() )
	{
		stream_ << " : ";
		for( const ComplexName& parent : class_.parents )
		{
			ElementWrite( parent );
			if( &parent != &class_.parents.back() )
				stream_ << ", ";
		}
	}

	ElementWrite( class_.non_sync_tag );

	if( class_.keep_fields_order )
		stream_ << " " << Keyword( Keywords::ordered_ );

	if( class_.no_discard )
		stream_ << " " << Keyword( Keywords::nodiscard_ );

	stream_ << "\n{\n";
	ElementWrite( class_.elements );
	stream_ << "}\n";
}

void ElementWrite( const NonSyncTag& non_sync_tag ) const
{
	if( std::holds_alternative<NonSyncTagNone>( non_sync_tag ) )
	{}
	else if( std::holds_alternative<NonSyncTagTrue>( non_sync_tag ) )
		stream_ << " " << Keyword( Keywords::non_sync_ );
	else if( const auto expression_ptr = std::get_if< std::unique_ptr<const Expression> >( &non_sync_tag ) )
	{
		stream_ << Keyword( Keywords::non_sync_ ) << " ( ";
		ElementWrite( **expression_ptr );
		stream_ << " )";
	}
	else U_ASSERT(false);
}

void ElementWrite( const Namespace& namespace_ ) const
{
	stream_ << Keyword( Keywords::namespace_ ) << " " << namespace_.name << "\n{\n\n";
	ElementWrite( namespace_.elements );
	stream_ << "\n}\n";
}

void ElementWrite( const VariablesDeclaration& variables_declaration ) const
{
	stream_ << Keyword( Keywords::var_ ) << " ";
	ElementWrite( variables_declaration.type );

	if( variables_declaration.variables.size () <= 1 )
	{
		stream_ << " ";
		for( const VariablesDeclaration::VariableEntry& var : variables_declaration.variables )
		{
			ElementWrite( var.reference_modifier );
			ElementWrite( var.mutability_modifier );
			stream_ << " " << var.name;

			if( var.initializer != nullptr )
				ElementWrite( *var.initializer );
		}
	}
	else
	{
		stream_ << "\n";
		for( const VariablesDeclaration::VariableEntry& var : variables_declaration.variables )
		{
			stream_ << "\t";
			ElementWrite( var.reference_modifier );
			ElementWrite( var.mutability_modifier );
			stream_ << " " << var.name;

			if( var.initializer != nullptr )
				ElementWrite( *var.initializer );

			if( &var != &variables_declaration.variables.back() )
				stream_ << ",\n";
		}
	}

	stream_ << ";\n";
}

void ElementWrite( const AutoVariableDeclaration& auto_variable_declaration ) const
{
	stream_ << Keyword( Keywords::auto_ );
	ElementWrite( auto_variable_declaration.reference_modifier );
	stream_ << " ";
	ElementWrite( auto_variable_declaration.mutability_modifier );
	if( auto_variable_declaration.mutability_modifier != MutabilityModifier::None )
		stream_ << " ";

	stream_ << auto_variable_declaration.name;
	stream_ << " = ";
	ElementWrite( auto_variable_declaration.initializer_expression );
	stream_ << ";\n";
}

void ElementWrite( const StaticAssert& static_assert_ ) const
{
	U_UNUSED(static_assert_);
	U_ASSERT(false);
	// Not implemented yet.
}

void ElementWrite( const Enum& enum_ ) const
{
	stream_ << Keyword( Keywords::enum_ ) << " " << enum_.name;

	if( enum_.underlying_type_name != std::nullopt )
	{
		stream_ << " : ";
		ElementWrite( *enum_.underlying_type_name );
	}

	if( enum_.no_discard )
		stream_ << " " << Keyword( Keywords::nodiscard_ );

	stream_ << "\n{\n";
	for( const Enum::Member& enum_member : enum_.members )
		stream_ << "\t" << enum_member.name << ",\n";
	stream_ << "}\n";
}

void ElementWrite( const TypeAlias& type_alias ) const
{
	stream_ << Keyword( Keywords::type_ ) << " " << type_alias.name << " = ";
	ElementWrite( type_alias.value );
	stream_ << ";\n";
}

void ElementWrite( const TypeTemplate& type_template ) const
{
	U_UNUSED(type_template);
	U_ASSERT(false);
	// Not implemented yet.
}

void ElementWrite( const ClassField& class_field ) const
{
	stream_ << "\t";
	ElementWrite( class_field.type );

	if( !std::holds_alternative< EmptyVariant >( class_field.inner_reference_tags_expression ) )
	{
		stream_ << " @(";
		ElementWrite( class_field.inner_reference_tags_expression );
		stream_ << ") ";
	}

	ElementWrite( class_field.reference_modifier );
	stream_ << " ";

	ElementWrite( class_field.mutability_modifier );
	if( class_field.mutability_modifier != MutabilityModifier::None )
		stream_ << " ";

	if( !std::holds_alternative< EmptyVariant >( class_field.reference_tag_expression ) )
	{
		stream_ << " @(";
		ElementWrite( class_field.reference_tag_expression );
		stream_ << ") ";
	}

	stream_ << class_field.name << ";\n";
}

void ElementWrite( const ClassVisibilityLabel& visibility_label ) const
{
	switch( visibility_label.visibility )
	{
	case ClassMemberVisibility::Public:
		stream_ << Keyword( Keywords::public_ ) << ": \n";
		break;
	case ClassMemberVisibility::Protected:
		stream_ << Keyword( Keywords::protected_ ) << ": \n";
		break;
	case ClassMemberVisibility::Private:
		stream_ << Keyword( Keywords::private_ ) << ": \n";
		break;
	};
}

void ElementWrite( const Mixin& mixin ) const
{
	WriteRawMixin( mixin );
	stream_ << ";";
}

void ElementWrite( const ClassElementsList& class_elements ) const
{
	class_elements.Iter( [&]( const auto& el ) { ElementWrite(el); } );
}

void ElementWrite( const ProgramElementsList& elements ) const
{
	elements.Iter( [&]( const auto& el ) { ElementWrite(el); } );
}

void WriteFunctionDeclaration( const Function& function ) const
{
	if( function.overloaded_operator == OverloadedOperator::None )
		stream_ << Keyword( Keywords::fn_ );
	else
		stream_ << Keyword( Keywords::op_ );
	stream_ << " ";

	switch( function.virtual_function_kind )
	{
	case VirtualFunctionKind::None:
		break;
	case VirtualFunctionKind::DeclareVirtual:
		stream_ << Keyword( Keywords::virtual_ ) << " ";
		break;
	case VirtualFunctionKind::VirtualOverride:
		stream_ << Keyword( Keywords::override_ ) << " ";
		break;
	case VirtualFunctionKind::VirtualFinal:
		stream_ << Keyword( Keywords::final_ ) << " ";
		break;
	case VirtualFunctionKind::VirtualPure:
		stream_ << Keyword( Keywords::pure_ ) << " ";
		break;
	};

	if( function.kind == Function::Kind::Generator )
		stream_ << Keyword( Keywords::generator_ ) << " ";
	else if( function.kind == Function::Kind::Async )
		stream_ << Keyword( Keywords::async_ ) << " ";

	ElementWrite( function.coroutine_non_sync_tag );

	if( function.constexpr_ )
		stream_ << Keyword( Keywords::constexpr_ ) << " ";
	if( function.no_mangle )
		stream_ << Keyword( Keywords::nomangle_ ) << " ";
	if( function.no_discard )
		stream_ << Keyword( Keywords::nodiscard_ ) << " ";

	if( !std::holds_alternative<EmptyVariant>(function.condition) )
	{
		stream_ << Keyword( Keywords::enable_if_ ) << "( ";
		ElementWrite( function.condition );
		stream_ << " ) ";
	}

	for( const Function::NameComponent& component : function.name )
	{
		stream_ << component.name;
		if( &component != &function.name.back() )
			stream_ << "::";
	}

	WriteFunctionParamsList( function.type );

	WriteFunctionTypeEnding( function.type );

	switch( function.body_kind )
	{
	case Function::BodyKind::None:
		break;
	case Function::BodyKind::BodyGenerationRequired:
		stream_ << "= " << Keyword( Keywords::default_ );
		break;
	case Function::BodyKind::BodyGenerationDisabled:
		stream_ << "= " << Keyword( Keywords::delete_ );
		break;
	}
}

void WriteFunctionParamsList( const FunctionType& function_type ) const
{
	if( function_type.params.empty() )
		stream_ << "()";
	else
	{
		stream_ << "( ";
		for( const FunctionParam& param : function_type.params )
		{
			ElementWrite( param );
			if( &param != &function_type.params.back() )
				stream_ << ", ";
		}
		stream_ << " )";
	}
	stream_ << " ";
}

void WriteFunctionTypeEnding( const FunctionType& function_type ) const
{
	if( function_type.references_pollution_expression != nullptr )
	{
		stream_ << " @( ";
		ElementWrite( *function_type.references_pollution_expression );
		stream_ << " )";
	}

	if( function_type.unsafe )
		stream_ << Keyword( Keywords::unsafe_ ) << " ";

	if( function_type.calling_convention != nullptr )
	{
		stream_ << Keyword( Keywords::call_conv_ );
		stream_ << "( ";
		ElementWrite( *function_type.calling_convention );
		stream_ << " ) ";
	}

	stream_ << ": ";
	if( function_type.return_type != nullptr )
		ElementWrite( *function_type.return_type );
	else
		stream_ << Keyword( Keywords::void_ );

	if( function_type.return_value_inner_references_expression != nullptr )
	{
		stream_ << " @( ";
		ElementWrite( *function_type.return_value_inner_references_expression );
		stream_ << " )";
	}

	if( function_type.return_value_reference_modifier != ReferenceModifier::None )
	{
		stream_ << " ";

		ElementWrite( function_type.return_value_reference_modifier );
		ElementWrite( function_type.return_value_mutability_modifier );

		if( function_type.return_value_reference_expression != nullptr )
		{
			stream_ << " @( ";
			ElementWrite( *function_type.return_value_reference_expression );
			stream_ << " )";
		}
	}
}

void ElementWrite( const FunctionTemplate& function_template ) const
{
	stream_ << Keyword( Keywords::template_ );
	stream_ << "</ ";

	for( const TemplateParam& param : function_template.params )
	{
		if( const auto variable_param_data= std::get_if<TemplateParam::VariableParamData>( &param.kind_data ) )
		{
			ElementWrite( variable_param_data->type );
			stream_ << " ";
		}
		else if( std::holds_alternative<TemplateParam::TypeParamData>( param.kind_data ) )
			stream_ << Keyword( Keywords::type_ ) << " ";
		else if( std::holds_alternative<TemplateParam::TypeTemplateParamData>( param.kind_data ) )
			stream_ << Keyword( Keywords::type_ ) << " " << Keyword( Keywords::template_ ) << " ";
		else U_ASSERT(false);

		stream_ << param.name;

		if( &param != &function_template.params.back() )
			stream_ << ", ";
	}

	stream_ << " /> ";

	if( function_template.function != nullptr )
		ElementWrite( *function_template.function );
}

};

} // namespace

void WriteProgram( const ProgramElementsList& program_elements, std::ostream& stream )
{
	ProgramWriter(stream).ElementWrite( program_elements );
}

void WriteExpression( const Expression& expression, std::ostream& stream )
{
	ProgramWriter(stream).ElementWrite( expression );
}

void WriteTypeName( const TypeName& type_name, std::ostream& stream )
{
	ProgramWriter(stream).ElementWrite( type_name );
}

void WriteFunctionDeclaration( const Function& function, std::ostream& stream )
{
	ProgramWriter(stream).ElementWrite( function );
}

void WriteFunctionParamsList( const FunctionType& function_type, std::ostream& stream )
{
	ProgramWriter(stream).WriteFunctionParamsList( function_type );
}

void WriteFunctionTypeEnding( const FunctionType& function_type, std::ostream& stream )
{
	ProgramWriter(stream).WriteFunctionTypeEnding( function_type );
}

void WriteFunctionTemplate( const FunctionTemplate& function_template, std::ostream& stream )
{
	ProgramWriter(stream).ElementWrite( function_template );
}

} // namespace Synt

} // namespace U
