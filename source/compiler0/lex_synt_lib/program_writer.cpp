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

// Prototypes start
void ElementWrite( const EmptyVariant& empty_variant, std::ostream& stream );
void ElementWrite( const RootNamespaceNameLookup& root_namespace_lookup, std::ostream& stream );
void ElementWrite( const RootNamespaceNameLookupCompletion& root_namespace_lookup_completion, std::ostream& stream );
void ElementWrite( const NameLookup& name_lookup, std::ostream& stream );
void ElementWrite( const NameLookupCompletion& name_lookup_completion, std::ostream& stream );
void ElementWrite( const NamesScopeNameFetch& names_scope_fetch, std::ostream& stream );
void ElementWrite( const NamesScopeNameFetchCompletion& names_scope_fetch_completion, std::ostream& stream );
void ElementWrite( const TemplateParameterization& template_parameterization, std::ostream& stream );
void ElementWrite( const ComplexName& complex_name, std::ostream& stream );
void ElementWrite( const ArrayTypeName& array_type_name, std::ostream& stream );
void ElementWrite( const TupleType& tuple_type_name, std::ostream& stream );
void ElementWrite( const RawPointerType& raw_pointer_type_name, std::ostream& stream );
void ElementWrite( const CoroutineType& coroutine_type_name, std::ostream& stream );
void ElementWrite( const TypeofTypeName& typeof_type_name, std::ostream& stream );
void ElementWrite( const FunctionType& function_type_name, std::ostream& stream );
void ElementWrite( const FunctionParam& param, std::ostream& stream );
void ElementWrite( const TypeName& type_name, std::ostream& stream );
void ElementWrite( const Expression& expression, std::ostream& stream );
void ElementWrite( const Initializer& initializer, std::ostream& stream );
void ElementWrite( const ReferenceModifier& reference_modifier, std::ostream& stream );
void ElementWrite( const MutabilityModifier& mutability_modifier, std::ostream& stream );
void ElementWrite( const Function& function, std::ostream& stream );
void ElementWrite( const Class& class_, std::ostream& stream );
void ElementWrite( const NonSyncTag& non_sync_tag, std::ostream& stream );
void ElementWrite( const Namespace& namespace_, std::ostream& stream );
void ElementWrite( const VariablesDeclaration& variables_declaration, std::ostream& stream );
void ElementWrite( const AutoVariableDeclaration& auto_variable_declaration, std::ostream& stream );
void ElementWrite( const StaticAssert& static_assert_, std::ostream& stream );
void ElementWrite( const Enum& enum_, std::ostream& stream );
void ElementWrite( const TypeAlias& type_alias, std::ostream& stream );
void ElementWrite( const TypeTemplate& type_template, std::ostream& stream );
void ElementWrite( const FunctionTemplate& function_template, std::ostream& stream );
void ElementWrite( const ClassField& class_field, std::ostream& stream );
void ElementWrite( const ClassVisibilityLabel& visibility_label, std::ostream& stream );
void ElementWrite( const Mixin& mixin, std::ostream& stream );
void ElementWrite( const ClassElementsList& class_elements, std::ostream& stream );
void ElementWrite( const ProgramElementsList& elements, std::ostream& stream );
// Prototypes end

class UniversalVisitor
{
public:
	explicit UniversalVisitor( std::ostream& in_stream ) : stream(in_stream) {}

	template<class T>
	void operator()( const std::unique_ptr<T>& ptr ) const
	{
		if( ptr != nullptr )
			ElementWrite( *ptr, stream );
	}

	template<class T>
	void operator()( const T& el ) const
	{
		ElementWrite( el, stream );
	}
private:
	std::ostream& stream;
};

void ElementWrite( const EmptyVariant&, std::ostream& ) {}

void ElementWrite( const RootNamespaceNameLookup& root_namespace_lookup, std::ostream& stream )
{
	stream << "::" << root_namespace_lookup.name;
}

void ElementWrite( const RootNamespaceNameLookupCompletion& root_namespace_lookup_completion, std::ostream& stream )
{
	stream << "::" << root_namespace_lookup_completion.name;
}

void ElementWrite( const NameLookup& name_lookup, std::ostream& stream )
{
	stream << name_lookup.name;
}

void ElementWrite( const NameLookupCompletion& name_lookup_completion, std::ostream& stream )
{
	stream << name_lookup_completion.name;
}

void ElementWrite( const NamesScopeNameFetch& names_scope_fetch, std::ostream& stream )
{
	ElementWrite( names_scope_fetch.base, stream );
	stream << "::" << names_scope_fetch.name;
}

void ElementWrite( const NamesScopeNameFetchCompletion& names_scope_fetch_completion, std::ostream& stream )
{
	ElementWrite( names_scope_fetch_completion.base, stream );
	stream << "::" << names_scope_fetch_completion.name;
}

void ElementWrite( const TemplateParameterization& template_parameterization, std::ostream& stream )
{
	ElementWrite( template_parameterization.base, stream );

	stream << "</ ";
	for( const Expression& expr : template_parameterization.template_args )
	{
		ElementWrite( expr, stream );
		if( &expr != &template_parameterization.template_args.back() )
			stream << ", ";
	}
	stream << " />";
}

void ElementWrite( const ComplexName& complex_name, std::ostream& stream )
{
	return std::visit( UniversalVisitor(stream), complex_name );
}

void ElementWrite( const ArrayTypeName& array_type_name, std::ostream& stream )
{
	stream << "[ ";
	ElementWrite( array_type_name.element_type, stream );
	stream << ", ";
	ElementWrite( array_type_name.size, stream );
	stream << " ]";
}

void ElementWrite( const TupleType& tuple_type_name, std::ostream& stream )
{
	stream << Keyword( Keywords::tup_ );
	if( tuple_type_name.element_types.empty() )
		stream << "[]";
	else
	{
		stream << "[ ";
		for( const TypeName& element_type : tuple_type_name.element_types )
		{
			ElementWrite( element_type, stream );
			if( &element_type != &tuple_type_name.element_types.back() )
				stream << ", ";
		}
		stream << " ]";
	}
}

void ElementWrite( const RawPointerType& raw_pointer_type_name, std::ostream& stream )
{
	stream << "$";
	stream << "(";
	ElementWrite( raw_pointer_type_name.element_type, stream );
	stream << ")";
}

void ElementWrite( const CoroutineType& coroutine_name, std::ostream& stream )
{
	switch( coroutine_name.kind )
	{
	case CoroutineType::Kind::Generator:
		stream << Keyword( Keywords::generator_ );
		break;
	case CoroutineType::Kind::AsyncFunc:
		stream << Keyword( Keywords::async_ );
		break;
	};

	if( !coroutine_name.inner_references.empty() )
	{
		stream << "'";
		for( size_t i= 0; i < coroutine_name.inner_references.size(); ++i )
		{
			ElementWrite( coroutine_name.inner_references[i], stream );
			if( i + 1 < coroutine_name.inner_references.size() )
				stream << ", ";
		}
		stream << "'";
	}

	ElementWrite( coroutine_name.non_sync_tag, stream );

	stream << ":";
	ElementWrite( coroutine_name.return_type, stream );

	ElementWrite( coroutine_name.return_value_reference_modifier, stream );
	ElementWrite( coroutine_name.return_value_mutability_modifier, stream );
}

void ElementWrite( const TypeofTypeName& typeof_type_name, std::ostream& stream )
{
	stream << Keyword( Keywords::typeof_ ) << "( ";
	ElementWrite( typeof_type_name.expression, stream );
	stream << " )";
}

void ElementWrite( const FunctionType& function_type_name, std::ostream& stream )
{
	stream << "( " << Keyword( Keywords::fn_ );
	WriteFunctionParamsList( function_type_name, stream );
	WriteFunctionTypeEnding( function_type_name, stream );
	stream << " )";
}

void ElementWrite( const FunctionParam& param, std::ostream& stream )
{
	if( param.name == Keywords::this_ )
	{
		if( param.reference_modifier == ReferenceModifier::None )
			stream << Keyword( Keywords::byval_ ) << " ";
		if( param.mutability_modifier != MutabilityModifier::None )
		{
			ElementWrite( param.mutability_modifier, stream );
			stream << " ";
		}
		stream << Keyword( Keywords::this_ );
	}
	else
	{
		ElementWrite( param.type, stream );

		stream << " ";

		ElementWrite( param.reference_modifier, stream );

		ElementWrite( param.mutability_modifier, stream );

		if( param.mutability_modifier != MutabilityModifier::None )
			stream << " ";
		stream << param.name;
	}
}

void ElementWrite( const TypeName& type_name, std::ostream& stream )
{
	std::visit( UniversalVisitor(stream), type_name );
}

void ElementWrite( const Expression& expression, std::ostream& stream )
{
	class Visitor
	{
	public:
		explicit Visitor( std::ostream& in_stream ) : stream(in_stream) {}

		void operator()( const EmptyVariant& ) const
		{
			return;
		}
		void operator()( const std::unique_ptr<const BinaryOperator>& binary_operator ) const
		{
			ElementWrite( binary_operator->left , stream );
			stream << " " << BinaryOperatorToString(binary_operator->operator_type) << " ";
			ElementWrite( binary_operator->right, stream );
		}
		void operator()( const ComplexName& complex_name ) const
		{
			ElementWrite( complex_name, stream );
		}
		void operator()( const std::unique_ptr<const TernaryOperator>& ternary_operator ) const
		{
			stream << Keyword( Keywords::select_ ) << "( ";
			ElementWrite( ternary_operator->condition, stream );
			stream << " ? ";
			ElementWrite( ternary_operator->branches[0], stream );
			stream << " : ";
			ElementWrite( ternary_operator->branches[1], stream );
			stream << " )";
		}
		void operator()( const std::unique_ptr<const ReferenceToRawPointerOperator>& reference_to_raw_pointer_operator ) const
		{
			stream << "$<";
			stream << "(";
			ElementWrite( reference_to_raw_pointer_operator->expression, stream );
			stream << ")";
		}
		void operator()( const std::unique_ptr<const RawPointerToReferenceOperator>& raw_pointer_to_reference_operator ) const
		{
			stream << "$>";
			stream << "(";
			ElementWrite( raw_pointer_to_reference_operator->expression, stream );
			stream << ")";
		}
		void operator()( const NumericConstant& numeric_constant ) const
		{
			stream.precision( std::numeric_limits<double>::digits10 + 1 );
			if( numeric_constant.has_fractional_point )
			{
				stream.flags( stream.flags() | std::ios_base::showpoint );
				stream << numeric_constant.value_double;
			}
			else
			{
				stream.flags( stream.flags() & (~std::ios_base::showpoint) );
				stream << numeric_constant.value_int;
			}
			stream << numeric_constant.type_suffix.data();
		}
		void operator()( const std::unique_ptr<const StringLiteral>& string_literal ) const
		{
			std::string escaped;
			for( const char c : string_literal->value )
			{
				switch(c)
				{
				case '\"':
					escaped.push_back( '\\' );
					escaped.push_back( '\"' );
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
			stream << "\"" << escaped << "\"" << string_literal->type_suffix;
		}
		void operator()( const BooleanConstant& boolean_constant ) const
		{
			stream << ( boolean_constant.value ? Keyword( Keywords::true_ ) : Keyword( Keywords::false_ ) );
		}
		void operator()( const MoveOperator& move_operator ) const
		{
			stream << Keyword( Keywords::move_ ) << "( " << move_operator.var_name << " )";
		}
		void operator()( const MoveOperatorCompletion& move_operator_completion ) const
		{
			stream << Keyword( Keywords::move_ ) << "( " << move_operator_completion.var_name << " )";
		}
		void operator()( const std::unique_ptr<const TakeOperator>& take_operator ) const
		{
			stream << Keyword( Keywords::take_ ) << "( ";
			ElementWrite( take_operator->expression, stream );
			stream << " )";
		}
		void operator()( const std::unique_ptr<const Lambda>& lambda ) const
		{
			U_UNUSED(lambda);
			// For now do not print whole lambda - just create dummy.
			stream << Keyword( Keywords::lambda_ ) << "(){}";
		}
		void operator()( const std::unique_ptr<const CastRef>& cast_ref ) const
		{
			stream << Keyword( Keywords::cast_ref_ ) << "</ ";
			ElementWrite( cast_ref->type, stream );
			stream << " />( ";
			ElementWrite( cast_ref->expression, stream );
			stream << " )";
		}
		void operator()( const std::unique_ptr<const CastRefUnsafe>& cast_ref_unsafe ) const
		{
			stream << Keyword( Keywords::cast_ref_unsafe_ ) << "</ ";
			ElementWrite( cast_ref_unsafe->type, stream );
			stream << " />( ";
			ElementWrite( cast_ref_unsafe->expression, stream );
			stream << " )";
		}
		void operator()( const std::unique_ptr<const CastImut>& cast_imut ) const
		{
			stream << Keyword( Keywords::cast_imut_ ) << "( ";
			ElementWrite( cast_imut->expression, stream );
			stream << " )";
		}
		void operator()( const std::unique_ptr<const CastMut>& cast_mut ) const
		{
			stream << Keyword( Keywords::cast_mut_ ) << "( ";
			ElementWrite( cast_mut->expression, stream );
			stream << " )";
		}
		void operator()( const std::unique_ptr<const Embed>& embed ) const
		{
			stream << Keyword( Keywords::embed_ );

			if( embed->element_type != std::nullopt )
			{
				stream << "</";
				ElementWrite( *embed->element_type, stream );
				stream << "/>";
			}

			stream << "( ";
			ElementWrite( embed->expression, stream );
			stream << " )";
		}
		void operator()( const std::unique_ptr<const TypeInfo>& typeinfo_ ) const
		{
			stream << "</ ";
			ElementWrite( typeinfo_->type, stream );
			stream << " />";
		}
		void operator()( const std::unique_ptr<const SameType>& same_type ) const
		{
			stream << Keyword( Keywords::same_type_ ) << "</ ";
			ElementWrite( same_type->l, stream );
			stream << ", ";
			ElementWrite( same_type->r, stream );
			stream << " />";
		}
		void operator()( const std::unique_ptr<const NonSyncExpression>& non_sync_expression ) const
		{
			stream << "</ ";
			ElementWrite( non_sync_expression->type, stream );
			stream << " />";
		}
		void operator()( const std::unique_ptr<const SafeExpression>& safe_expression ) const
		{
			stream << Keyword( Keywords::safe_ );
			stream << "( ";
			ElementWrite( safe_expression->expression, stream );
			stream << " )";
		}
		void operator()( const std::unique_ptr<const UnsafeExpression>& unsafe_expression ) const
		{
			stream << Keyword( Keywords::unsafe_ );
			stream << "( ";
			ElementWrite( unsafe_expression->expression, stream );
			stream << " )";
		}
		void operator()( const std::unique_ptr<const UnaryMinus>& unary_minus ) const
		{
			stream << OverloadedOperatorToString( OverloadedOperator::Sub );
			ElementWrite( unary_minus->expression, stream );
		}
		void operator()( const std::unique_ptr<const LogicalNot>& logical_not ) const
		{
			stream << OverloadedOperatorToString( OverloadedOperator::LogicalNot );
			ElementWrite( logical_not->expression, stream );
		}
		void operator()( const std::unique_ptr<const BitwiseNot>& bitwise_not ) const
		{
			stream << OverloadedOperatorToString( OverloadedOperator::BitwiseNot );
			ElementWrite( bitwise_not->expression, stream );
		}
		void operator()( const std::unique_ptr<const IndexationOperator>& indexation_operator )
		{
			ElementWrite( indexation_operator->expression, stream );
			stream << "[ ";
			ElementWrite( indexation_operator->index, stream );
			stream << " ]";
		}
		void operator()( const std::unique_ptr<const MemberAccessOperator>& member_access_operator ) const
		{
			ElementWrite( member_access_operator->expression, stream );
			stream << "." << member_access_operator->member_name;
			if( member_access_operator->template_args != std::nullopt )
			{
				stream << "</";
				for( const Expression& template_arg : *member_access_operator->template_args )
				{
					ElementWrite( template_arg, stream );
					if( &template_arg != &member_access_operator->template_args->back() )
						stream<< ", ";
				}
				stream << "/>";
			}
		}
		void operator()( const std::unique_ptr<const AwaitOperator>& await_operator ) const
		{
			ElementWrite( await_operator->expression, stream );
			stream << "." << Keyword( Keywords::await_ );
		}
		void operator()( const std::unique_ptr<const VariableInitialization>& variable_initialization ) const
		{
			ElementWrite( variable_initialization->type, stream );
			ElementWrite( variable_initialization->initializer, stream );
		}
		void operator()( const std::unique_ptr<const MemberAccessOperatorCompletion>& member_access_operator_completion ) const
		{
			ElementWrite( member_access_operator_completion->expression, stream );
			stream << "." << member_access_operator_completion->member_name;
		}
		void operator()( const std::unique_ptr<const CallOperator>& call_operator ) const
		{
			ElementWrite( call_operator->expression, stream );
			stream << "( ";
			for( const Expression& arg : call_operator->arguments )
			{
				ElementWrite( arg, stream );

				if( &arg != &call_operator->arguments.back() )
					stream << ", ";
			}
			stream << ")";
		}
		void operator()( const std::unique_ptr<const CallOperatorSignatureHelp>& call_operator_signature_help ) const
		{
			ElementWrite( call_operator_signature_help->expression, stream );
			stream << "( ";
			stream << ")";
		}
		void operator()( const std::unique_ptr<const NamesScopeNameFetch>& names_scope_fetch ) const
		{
			ElementWrite( *names_scope_fetch, stream );
		}
		void operator()( const std::unique_ptr<const TypeofTypeName>& typeof_type_name ) const
		{
			ElementWrite( *typeof_type_name, stream );
		}
		void operator()( const std::unique_ptr<const NamesScopeNameFetchCompletion>& names_scope_fetch_completion ) const
		{
			ElementWrite( *names_scope_fetch_completion, stream );
		}
		void operator()( const std::unique_ptr<const TemplateParameterization>& template_parameterization ) const
		{
			ElementWrite( *template_parameterization, stream );
		}
		void operator()( const std::unique_ptr<const ArrayTypeName>& array_type_name ) const
		{
			ElementWrite( *array_type_name, stream );
		}
		void operator()( const std::unique_ptr<const FunctionType>& function_type ) const
		{
			ElementWrite( *function_type, stream );
		}
		void operator()( const TupleType& tuple_type ) const
		{
			ElementWrite( tuple_type, stream );
		}
		void operator()( const std::unique_ptr<const RawPointerType>& raw_pointer_type ) const
		{
			ElementWrite( *raw_pointer_type, stream );
		}
		void operator()( const std::unique_ptr<const CoroutineType>& coroutine_type ) const
		{
			ElementWrite( *coroutine_type, stream );
		}
		void operator()( const std::unique_ptr<const Mixin>& mixin ) const
		{
			stream << Keyword( Keywords::mixin_ );
			stream << "( ";
			ElementWrite( mixin->expression, stream );
			stream << " )";
		}

	private:
		std::ostream& stream;
	};

	std::visit( Visitor(stream), expression );
}

void ElementWrite( const Initializer& initializer, std::ostream& stream )
{
	if( const auto constructor_initializer= std::get_if<ConstructorInitializer>( &initializer ) )
	{
		if( constructor_initializer->arguments.empty() )
		{
			stream << "()";
		}
		else
		{
			stream << "( ";
			for( const Expression& arg :constructor_initializer->arguments )
			{
				ElementWrite( arg, stream );
				if( &arg != &constructor_initializer->arguments.back() )
					stream << ", ";
			}
			stream << " )";
		}
	}
	else if( const auto constructor_initializer_signature_help= std::get_if<ConstructorInitializerSignatureHelp>( &initializer ) )
	{
		(void)constructor_initializer_signature_help;
		stream << "()";
	}
	else if( const auto struct_named_initializer= std::get_if<StructNamedInitializer>( &initializer ) )
	{
		stream << "{ ";
		for( const StructNamedInitializer::MemberInitializer& member_initializer : struct_named_initializer->members_initializers )
		{
			stream << "." << member_initializer.name;
			ElementWrite( member_initializer.initializer, stream );
			if( &member_initializer != &struct_named_initializer->members_initializers.back() )
				stream << ", ";
		}
		stream << "}";
	}
	else
	{
		U_ASSERT(false);
		// Not implemented yet.
	}
}

void ElementWrite( const ReferenceModifier& reference_modifier, std::ostream& stream )
{
	if( reference_modifier == ReferenceModifier::Reference )
		stream << "&";
}

void ElementWrite( const MutabilityModifier& mutability_modifier, std::ostream& stream )
{
	switch(mutability_modifier)
	{
	case MutabilityModifier::None:
		break;
	case MutabilityModifier::Mutable:
		stream << Keyword( Keywords::mut_ );
		break;
	case MutabilityModifier::Immutable:
		stream << Keyword( Keywords::imut_ );
		break;
	case MutabilityModifier::Constexpr:
		stream << Keyword( Keywords::constexpr_ );
		break;
	}
}

void ElementWrite( const Function& function, std::ostream& stream )
{
	WriteFunctionDeclaration( function, stream );

	if( function.block == nullptr )
		stream << ";\n";
	else
	{} // TODO
}

void ElementWrite( const Class& class_, std::ostream& stream )
{
	stream << Keyword(class_.kind_attribute_ == ClassKindAttribute::Struct ? Keywords::struct_ : Keywords::class_ );
	stream << " " << class_.name;

	switch( class_.kind_attribute_ )
	{
	case ClassKindAttribute::Struct:
	case ClassKindAttribute::Class:
		break;
	case ClassKindAttribute::Final:
		stream << " " << Keyword( Keywords::final_ );
		break;
	case ClassKindAttribute::Polymorph:
		stream << " " << Keyword( Keywords::polymorph_ );
		break;
	case ClassKindAttribute::Interface:
		stream << " " << Keyword( Keywords::interface_ );
		break;
	case ClassKindAttribute::Abstract:
		stream << " " << Keyword( Keywords::abstract_ );
		break;
	};

	if( !class_.parents.empty() )
	{
		stream << " : ";
		for( const ComplexName& parent : class_.parents )
		{
			ElementWrite( parent, stream );
			if( &parent != &class_.parents.back() )
				stream << ", ";
		}
	}

	ElementWrite( class_.non_sync_tag, stream );

	if( class_.keep_fields_order )
		stream << " " << Keyword( Keywords::ordered_ );

	stream << "\n{\n";
	ElementWrite( class_.elements, stream );
	stream << "}\n";
}

void ElementWrite( const NonSyncTag& non_sync_tag, std::ostream& stream )
{
	if( std::holds_alternative<NonSyncTagNone>( non_sync_tag ) )
	{}
	else if( std::holds_alternative<NonSyncTagTrue>( non_sync_tag ) )
		stream << " " << Keyword( Keywords::non_sync_ );
	else if( const auto expression_ptr = std::get_if< std::unique_ptr<const Expression> >( &non_sync_tag ) )
	{
		stream << Keyword( Keywords::non_sync_ ) << " ( ";
		ElementWrite( **expression_ptr, stream );
		stream << " )";
	}
	else U_ASSERT(false);
}

void ElementWrite( const Namespace& namespace_, std::ostream& stream )
{
	stream << Keyword( Keywords::namespace_ ) << " " << namespace_.name << "\n{\n\n";
	ElementWrite( namespace_.elements, stream );
	stream << "\n}\n";
}

void ElementWrite( const VariablesDeclaration& variables_declaration, std::ostream& stream )
{
	stream << Keyword( Keywords::var_ ) << " ";
	ElementWrite( variables_declaration.type, stream );

	if( variables_declaration.variables.size () <= 1 )
	{
		stream << " ";
		for( const VariablesDeclaration::VariableEntry& var : variables_declaration.variables )
		{
			ElementWrite( var.reference_modifier, stream );
			ElementWrite( var.mutability_modifier, stream );
			stream << " " << var.name;

			if( var.initializer != nullptr )
				ElementWrite( *var.initializer, stream );
		}
	}
	else
	{
		stream << "\n";
		for( const VariablesDeclaration::VariableEntry& var : variables_declaration.variables )
		{
			stream << "\t";
			ElementWrite( var.reference_modifier, stream );
			ElementWrite( var.mutability_modifier, stream );
			stream << " " << var.name;

			if( var.initializer != nullptr )
				ElementWrite( *var.initializer, stream );

			if( &var != &variables_declaration.variables.back() )
				stream << ",\n";
		}
	}

	stream << ";\n";
}

void ElementWrite( const AutoVariableDeclaration& auto_variable_declaration, std::ostream& stream )
{
	stream << Keyword( Keywords::auto_ );
	ElementWrite( auto_variable_declaration.reference_modifier, stream );
	stream << " ";
	ElementWrite( auto_variable_declaration.mutability_modifier, stream );
	if( auto_variable_declaration.mutability_modifier != MutabilityModifier::None )
		stream << " ";

	stream << auto_variable_declaration.name;
	stream << " = ";
	ElementWrite( auto_variable_declaration.initializer_expression, stream );
	stream << ";\n";
}

void ElementWrite( const StaticAssert& static_assert_, std::ostream& stream )
{
	U_UNUSED(static_assert_);
	U_UNUSED(stream);
	U_ASSERT(false);
	// Not implemented yet.
}

void ElementWrite( const Enum& enum_, std::ostream& stream )
{
	stream << Keyword( Keywords::enum_ ) << " " << enum_.name;

	if( enum_.underlying_type_name != std::nullopt )
	{
		stream << " : ";
		ElementWrite( *enum_.underlying_type_name, stream );
	}

	stream << "\n{\n";
	for( const Enum::Member& enum_member : enum_.members )
		stream << "\t" << enum_member.name << ",\n";
	stream << "}\n";
}

void ElementWrite( const TypeAlias& type_alias, std::ostream& stream )
{
	stream << Keyword( Keywords::type_ ) << " " << type_alias.name << " = ";
	ElementWrite( type_alias.value, stream );
	stream << ";\n";
}

void ElementWrite( const TypeTemplate& type_template, std::ostream& stream )
{
	U_UNUSED(type_template);
	U_UNUSED(stream);
	U_ASSERT(false);
	// Not implemented yet.
}

void ElementWrite( const FunctionTemplate& function_template, std::ostream& stream )
{
	WriteFunctionTemplate( function_template, stream );
}

void ElementWrite( const ClassField& class_field, std::ostream& stream )
{
	stream << "\t";
	ElementWrite( class_field.type, stream );

	if( !std::holds_alternative< Synt::EmptyVariant >( class_field.inner_reference_tags_expression ) )
	{
		stream << " @(";
		ElementWrite( class_field.inner_reference_tags_expression, stream );
		stream << ") ";
	}

	ElementWrite( class_field.reference_modifier, stream );
	stream << " ";

	ElementWrite( class_field.mutability_modifier, stream );
	if( class_field.mutability_modifier != MutabilityModifier::None )
		stream << " ";

	if( !std::holds_alternative< Synt::EmptyVariant >( class_field.reference_tag_expression ) )
	{
		stream << " @(";
		ElementWrite( class_field.reference_tag_expression, stream );
		stream << ") ";
	}

	stream << class_field.name << ";\n";
}

void ElementWrite( const ClassVisibilityLabel& visibility_label, std::ostream& stream )
{
	switch( visibility_label.visibility )
	{
	case ClassMemberVisibility::Public:
		stream << Keyword( Keywords::public_ ) << ": \n";
		break;
	case ClassMemberVisibility::Protected:
		stream << Keyword( Keywords::protected_ ) << ": \n";
		break;
	case ClassMemberVisibility::Private:
		stream << Keyword( Keywords::private_ ) << ": \n";
		break;
	};
}

void ElementWrite( const Mixin& mixin, std::ostream& stream )
{
	stream << Keyword( Keywords::mixin_ );
	stream << "( ";
	ElementWrite( mixin.expression, stream );
	stream << " );";
}

void ElementWrite( const ClassElementsList& class_elements, std::ostream& stream )
{
	class_elements.Iter( UniversalVisitor( stream ) );
}

void ElementWrite( const ProgramElementsList& elements, std::ostream& stream )
{
	elements.Iter( UniversalVisitor(stream) );
}

} // namespace

void WriteProgram( const ProgramElementsList& program_elements, std::ostream& stream )
{
	ElementWrite( program_elements, stream );
}

void WriteExpression( const Synt::Expression& expression, std::ostream& stream )
{
	ElementWrite( expression, stream );
}

void WriteTypeName( const Synt::TypeName& type_name, std::ostream& stream )
{
	ElementWrite( type_name, stream );
}

void WriteFunctionDeclaration( const Synt::Function& function, std::ostream& stream )
{
	if( function.overloaded_operator == OverloadedOperator::None )
		stream << Keyword( Keywords::fn_ );
	else
		stream << Keyword( Keywords::op_ );
	stream << " ";

	switch( function.virtual_function_kind )
	{
	case VirtualFunctionKind::None:
		break;
	case VirtualFunctionKind::DeclareVirtual:
		stream << Keyword( Keywords::virtual_ ) << " ";
		break;
	case VirtualFunctionKind::VirtualOverride:
		stream << Keyword( Keywords::override_ ) << " ";
		break;
	case VirtualFunctionKind::VirtualFinal:
		stream << Keyword( Keywords::final_ ) << " ";
		break;
	case VirtualFunctionKind::VirtualPure:
		stream << Keyword( Keywords::pure_ ) << " ";
		break;
	};

	if( function.kind == Function::Kind::Generator )
		stream << Keyword( Keywords::generator_ ) << " ";

	ElementWrite( function.coroutine_non_sync_tag, stream );

	if( function.constexpr_ )
		stream << Keyword( Keywords::constexpr_ ) << " ";
	if( function.no_mangle )
		stream << Keyword( Keywords::nomangle_ ) << " ";

	if( !std::holds_alternative<EmptyVariant>(function.condition) )
	{
		stream << Keyword( Keywords::enable_if_ ) << "( ";
		ElementWrite( function.condition, stream );
		stream << " ) ";
	}

	for( const Function::NameComponent& component : function.name )
	{
		stream << component.name;
		if( &component != &function.name.back() )
			stream << "::";
	}

	WriteFunctionParamsList( function.type, stream );

	WriteFunctionTypeEnding( function.type, stream );

	switch( function.body_kind )
	{
	case Function::BodyKind::None:
		break;
	case Function::BodyKind::BodyGenerationRequired:
		stream << "= " << Keyword( Keywords::default_ );
		break;
	case Function::BodyKind::BodyGenerationDisabled:
		stream << "= " << Keyword( Keywords::delete_ );
		break;
	}
}

void WriteFunctionParamsList( const Synt::FunctionType& function_type, std::ostream& stream )
{
	if( function_type.params.empty() )
		stream << "()";
	else
	{
		stream << "( ";
		for( const FunctionParam& param : function_type.params )
		{
			ElementWrite( param, stream );
			if( &param != &function_type.params.back() )
				stream << ", ";
		}
		stream << " )";
	}
	stream << " ";
}

void WriteFunctionTypeEnding( const FunctionType& function_type, std::ostream& stream )
{
	if( function_type.references_pollution_expression != nullptr )
	{
		stream << " @( ";
		ElementWrite( *function_type.references_pollution_expression, stream );
		stream << " )";
	}

	if( function_type.unsafe )
		stream << Keyword( Keywords::unsafe_ ) << " ";

	if( function_type.calling_convention != std::nullopt )
		stream << Keyword( Keywords::call_conv_ ) << "( \"" << *function_type.calling_convention << "\" ) ";

	stream << ": ";
	if( function_type.return_type != nullptr )
		ElementWrite( *function_type.return_type, stream );
	else
		stream << Keyword( Keywords::void_ );

	if( function_type.return_value_inner_references_expression != nullptr )
	{
		stream << " @( ";
		ElementWrite( *function_type.return_value_inner_references_expression, stream );
		stream << " )";
	}

	if( function_type.return_value_reference_modifier != ReferenceModifier::None )
	{
		stream << " ";

		ElementWrite( function_type.return_value_reference_modifier, stream );
		ElementWrite( function_type.return_value_mutability_modifier, stream );

		if( function_type.return_value_reference_expression != nullptr )
		{
			stream << " @( ";
			ElementWrite( *function_type.return_value_reference_expression, stream );
			stream << " )";
		}
	}
}

void WriteFunctionTemplate( const FunctionTemplate& function_template, std::ostream& stream )
{
	stream << Keyword( Keywords::template_ );
	stream << "</ ";

	for( const TemplateBase::Param& param : function_template.params )
	{
		if( const auto variable_param_data= std::get_if<TemplateBase::VariableParamData>( &param.kind_data ) )
		{
			ElementWrite( variable_param_data->type, stream );
			stream << " ";
		}
		else if( std::holds_alternative<TemplateBase::TypeParamData>( param.kind_data ) )
			stream << Keyword( Keywords::type_ ) << " ";
		else if( std::holds_alternative<TemplateBase::TypeTemplateParamData>( param.kind_data ) )
			stream << Keyword( Keywords::type_ ) << " " << Keyword( Keywords::template_ ) << " ";
		else U_ASSERT(false);

		stream << param.name;

		if( &param != &function_template.params.back() )
			stream << ", ";
	}

	stream << " /> ";

	if( function_template.function != nullptr )
		WriteFunctionDeclaration( *function_template.function, stream );
}

} // namespace Synt

} // namespace U
