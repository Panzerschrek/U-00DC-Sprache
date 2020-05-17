#include <ostream>

#include "assert.hpp"
#include "keywords.hpp"
#include "program_string.hpp"
#include "program_writer.hpp"

namespace U
{

namespace Synt
{

class UniversalVisitor final
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

// Prototypes start
void ElementWrite( const ProgramElements& elements, std::ostream& stream );
void ElementWrite( const EmptyVariant&, std::ostream& ){}
void ElementWrite( const TypeName& type_name, std::ostream& stream );
void ElementWrite( const Expression& expression, std::ostream& stream );
void ElementWrite( const ReferenceModifier& reference_modifier, std::ostream& stream );
void ElementWrite( const MutabilityModifier& mutability_modifier, std::ostream& stream );
void ElementWrite( const FunctionArgument& arg, std::ostream& stream );
void ElementWrite( const ClassElements& class_elements, std::ostream& stream );
void ElementWrite( const TypeofTypeName& typeof_type_name, std::ostream& stream );

// Prototypes end

void ElementWrite( const ComplexName& complex_name, std::ostream& stream )
{
	if( std::get_if<EmptyVariant>(&complex_name.start_value) != nullptr )
	{}
	else if( const auto typeof_type_name= std::get_if<TypeofTypeName>(&complex_name.start_value) )
		ElementWrite( *typeof_type_name, stream );
	else if(const auto simple_name= std::get_if<std::string>(&complex_name.start_value) )
		stream << *simple_name;
	else U_ASSERT(false);

	auto tail= complex_name.tail.get();
	while(tail != nullptr)
	{
		if( const auto name= std::get_if<std::string>( &tail->name_or_template_paramenters ) )
			stream << "::" << *name;
		else if( const auto template_prameters= std::get_if< std::vector<Expression> >( &tail->name_or_template_paramenters ) )
		{
			stream << "</ ";
			for( const Expression& expr : *template_prameters )
			{
				ElementWrite( expr, stream );
				if( &expr != &template_prameters->back() )
					stream << ", ";
			}
			stream << " />";
		}
		else
			U_ASSERT( false );

		tail= tail->next.get();
	}
}

void ElementWrite( const ArrayTypeName& array_type_name, std::ostream& stream )
{
	stream << "[ ";
	ElementWrite( *array_type_name.element_type, stream );
	stream << ", ";
	ElementWrite( *array_type_name.size, stream );
	stream << " ]";
}

void ElementWrite( const TupleType& tuple_type_name, std::ostream& stream )
{
	stream << Keyword( Keywords::tup_ ) << "( ";
	for( const TypeName& element_type : tuple_type_name.element_types_ )
	{
		ElementWrite( element_type, stream );
		if( &element_type != &tuple_type_name.element_types_.back() )
			stream << ", ";
	}
	stream << " )";
}

void ElementWrite( const TypeofTypeName& typeof_type_name, std::ostream& stream )
{
	stream << Keyword( Keywords::typeof_ ) << "( ";
	ElementWrite( *typeof_type_name.expression, stream );
	stream << " )";
}

void ElementWrite( const NamedTypeName& named_type_name, std::ostream& stream )
{
	ElementWrite( named_type_name.name, stream );
}

void ElementWriteFunctionTypeEnding( const FunctionType& function_type, std::ostream& stream )
{
	if( function_type.unsafe_ )
		stream << " " << Keyword( Keywords::unsafe_ );

	stream << " : ";
	if( function_type.return_type_ != nullptr )
		ElementWrite( *function_type.return_type_, stream );
	else
		stream << Keyword( Keywords::void_ );

	if( !function_type.return_value_inner_reference_tags_.empty() )
	{
		stream << "'";
		for( const std::string& tag : function_type.return_value_inner_reference_tags_ )
		{
			if( tag.empty() )
				stream << "...";
			else
				stream << tag;
			if( &tag != &function_type.return_value_inner_reference_tags_.back() && !function_type.return_value_inner_reference_tags_.back().empty() )
				stream << ", ";
		}
		stream << "'";
	}

	ElementWrite( function_type.return_value_reference_modifier_, stream );
	if( !function_type.return_value_reference_tag_.empty() )
		stream << "'" << function_type.return_value_reference_tag_;

	if( function_type.return_value_mutability_modifier_ != MutabilityModifier::None )
	{
		stream << " ";
		ElementWrite( function_type.return_value_mutability_modifier_, stream );
	}
}

void ElementWrite( const FunctionType& function_type_name, std::ostream& stream )
{
	stream << "( " << Keyword( Keywords::fn_ ) << "( ";
	for( const FunctionArgument& arg : function_type_name.arguments_ )
	{
		ElementWrite( arg, stream );
		if( &arg != &function_type_name.arguments_.back() )
			stream << ", ";
	}
	stream << " ) ";
	ElementWriteFunctionTypeEnding( function_type_name, stream );
	stream << " )";
}

void ElementWrite( const FunctionArgument& arg, std::ostream& stream )
{
	ElementWrite( arg.type_, stream );
	ElementWrite( arg.reference_modifier_, stream );

	if( !arg.reference_tag_.empty() )
	{
		stream << "'";
		stream << arg.reference_tag_;
	}

	stream << " ";
	ElementWrite( arg.mutability_modifier_, stream );

	if( arg.mutability_modifier_ != MutabilityModifier::None )
		stream << " ";
	stream << arg.name_;

	if( !arg.inner_arg_reference_tags_.empty() )
	{
		stream << "'";
		for( const std::string& tag : arg.inner_arg_reference_tags_ )
		{
			if( tag.empty() )
				stream << "...";
			else
				stream << tag;
			if( &tag != &arg.inner_arg_reference_tags_.back() && !arg.inner_arg_reference_tags_.back().empty() )
				stream << ", ";
		}
		stream << "'";
	}
}

void ElementWrite( const TypeName& type_name, std::ostream& stream )
{
	std::visit( UniversalVisitor(stream), type_name );
}

void ElementWrite( const Expression& expression, std::ostream& stream )
{
	class Visitor final
	{
	public:
		explicit Visitor( std::ostream& in_stream ) : stream(in_stream) {}

		void operator()( const EmptyVariant& ) const
		{
			return;
		}
		void operator()( const BinaryOperator& binary_operator ) const
		{
			if( binary_operator.left_ == nullptr || binary_operator.right_ == nullptr )
				return;
			ElementWrite( *binary_operator.left_ , stream );
			stream << " " << BinaryOperatorToString(binary_operator.operator_type_) << " ";
			ElementWrite( *binary_operator.right_, stream );
		}
		void operator()( const NamedOperand& named_operand ) const
		{
			ElementWrite( named_operand.name_, stream );
		}
		void operator()( const TernaryOperator& ternary_operator ) const
		{
			if( ternary_operator.condition == nullptr || ternary_operator.true_branch == nullptr || ternary_operator.false_branch == nullptr )
				return;
			stream << Keyword( Keywords::select_ ) << "( ";
			ElementWrite( *ternary_operator.condition, stream );
			stream << " ? ";
			ElementWrite( *ternary_operator.true_branch, stream );
			stream << " : ";
			ElementWrite( *ternary_operator.false_branch, stream );
			stream << " )";
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
		void operator()( const StringLiteral& string_literal ) const
		{
			std::string escaped;
			for( const char c : string_literal.value_ )
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
						for( unsigned int i= 0u; i < 4u; ++i )
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
			stream << "\"" << escaped << "\"" << string_literal.type_suffix_.data();
		}
		void operator()( const BooleanConstant& boolean_constant ) const
		{
			stream << ( boolean_constant.value_ ? Keyword( Keywords::true_ ) : Keyword( Keywords::false_ ) );
		}
		void operator()( const BracketExpression& bracket_expression ) const
		{
			if( bracket_expression.expression_ == nullptr )
				return;
			stream << "( ";
			ElementWrite( *bracket_expression.expression_, stream );
			stream << " )";
		}
		void operator()( const TypeNameInExpression& type_name_in_expression ) const
		{
			ElementWrite( type_name_in_expression.type_name, stream );
		}
		void operator()( const MoveOperator& move_operator ) const
		{
			stream << Keyword( Keywords::move_ ) << "( " << move_operator.var_name_ << " )";
		}
		void operator()( const TakeOperator& take_operator ) const
		{
			stream << Keyword( Keywords::take_ ) << "( ";
			ElementWrite( *take_operator.expression_, stream );
			stream << " )";
		}
		void operator()( const CastRef& cast_ref ) const
		{
			if( cast_ref.type_ == nullptr || cast_ref.expression_ == nullptr )
				return;
			stream << Keyword( Keywords::cast_ref_ ) << "</ ";
			ElementWrite( *cast_ref.type_, stream );
			stream << " />( ";
			ElementWrite( *cast_ref.expression_, stream );
			stream << " )";
		}
		void operator()( const CastRefUnsafe& cast_ref_unsafe ) const
		{
			if( cast_ref_unsafe.type_ == nullptr || cast_ref_unsafe.expression_ == nullptr )
				return;
			stream << Keyword( Keywords::cast_ref_unsafe_ ) << "</ ";
			ElementWrite( *cast_ref_unsafe.type_, stream );
			stream << " />( ";
			ElementWrite( *cast_ref_unsafe.expression_, stream );
			stream << " )";
		}
		void operator()( const CastImut& cast_imut ) const
		{
			if( cast_imut.expression_ == nullptr )
				return;
			stream << Keyword( Keywords::cast_imut_ ) << "( ";
			ElementWrite( *cast_imut.expression_, stream );
			stream << " )";
		}
		void operator()( const CastMut& cast_mut ) const
		{
			if( cast_mut.expression_ == nullptr )
				return;
			stream << Keyword( Keywords::cast_mut_ ) << "( ";
			ElementWrite( *cast_mut.expression_, stream );
			stream << " )";
		}
		void operator()( const TypeInfo& typeinfo_ ) const
		{
			if( typeinfo_.type_ == nullptr )
				return;
			stream << "</ ";
			ElementWrite( *typeinfo_.type_, stream );
			stream << " />";
		}

	private:
		std::ostream& stream;
	};

	std::visit( Visitor(stream), expression );

	struct ExpressionWithUnaryOperatorsVisitor final
	{
		const ExpressionComponentWithUnaryOperators* operator()( const ExpressionComponentWithUnaryOperators& expression_with_unary_operators ) const
		{
			return &expression_with_unary_operators;
		}
		const ExpressionComponentWithUnaryOperators* operator()( const BinaryOperator& ) const
		{
			return nullptr;
		}
		const ExpressionComponentWithUnaryOperators* operator()( const EmptyVariant& ) const
		{
			return nullptr;
		}
	};

	if( const auto expression_with_unary_operators= std::visit( ExpressionWithUnaryOperatorsVisitor(), expression ) )
	{
		struct PrefixVisitor final
		{
			std::string operator()( const UnaryMinus& ) const
			{
				return OverloadedOperatorToString( OverloadedOperator::Sub );
			}
			std::string operator()( const UnaryPlus& ) const
			{
				return OverloadedOperatorToString( OverloadedOperator::Add );
			}
			std::string operator()( const LogicalNot& ) const
			{
				return OverloadedOperatorToString( OverloadedOperator::LogicalNot );
			}
			std::string operator()( const BitwiseNot& ) const
			{
				return OverloadedOperatorToString( OverloadedOperator::BitwiseNot );
			}
		};

		class PostifxVisitor final
		{
		public:
			explicit PostifxVisitor( std::ostream& in_stream ) : stream(in_stream) {}

			void operator()( const IndexationOperator& indexation_operator )
			{
				stream << "[ ";
				ElementWrite( indexation_operator.index_, stream );
				stream << " ]";
			}
			void operator()( const MemberAccessOperator& member_access_operator ) const
			{
				stream << ".";
				if( member_access_operator.have_template_parameters )
				{
					stream << "</";
					for( const Expression& template_param : member_access_operator.template_parameters )
					{
						ElementWrite( template_param, stream );
						if( &template_param != &member_access_operator.template_parameters.back() )
							stream<< ", ";
					}
					stream << "/>";
				}
			}
			void operator()( const CallOperator& call_operator ) const
			{
				stream << "( ";
				for( const Expression& arg : call_operator.arguments_ )
				{
					ElementWrite( arg, stream );

					if( &arg != &call_operator.arguments_.back() )
						stream << ", ";
				}
				stream << ")";
			}

		private:
			std::ostream& stream;
		};

		PrefixVisitor prefix_visitor;
		for( const UnaryPrefixOperator& prefix_operator : expression_with_unary_operators->prefix_operators_ )
			stream << std::visit( prefix_visitor, prefix_operator );

		PostifxVisitor postfix_visitor(stream);
		for( const UnaryPostfixOperator& postfix_operator : expression_with_unary_operators->postfix_operators_ )
			std::visit( postfix_visitor, postfix_operator );
	}
}

void ElementWrite( const Initializer& initializer, std::ostream& stream )
{
	if( const auto constructor_initializer= std::get_if<ConstructorInitializer>( &initializer ) )
	{
		if( constructor_initializer->call_operator.arguments_.empty() )
		{
			stream << "()";
		}
		else
		{
			stream << "( ";
			for( const Expression& arg :constructor_initializer->call_operator.arguments_ )
			{
				ElementWrite( arg, stream );
				if( &arg != &constructor_initializer->call_operator.arguments_.back() )
					stream << ", ";
			}
			stream << " )";
		}
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
	if( function.overloaded_operator_ == OverloadedOperator::None )
		stream << Keyword( Keywords::fn_ );
	else
		stream << Keyword( Keywords::op_ );
	stream << " ";

	switch( function.virtual_function_kind_ )
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

	if( function.constexpr_ )
		stream << Keyword( Keywords::constexpr_ ) << " ";
	if( function.no_mangle_ )
		stream << Keyword( Keywords::nomangle_ ) << " ";

	if( std::get_if<EmptyVariant>(&function.condition_) == nullptr )
	{
		stream << Keyword( Keywords::enable_if_ ) << "( ";
		ElementWrite( function.condition_, stream );
		stream << " ) ";
	}

	for( const std::string& component : function.name_ )
	{
		stream << component;
		if( &component != &function.name_.back() )
			stream << "::";
	}

	if( function.type_.arguments_.empty() )
		stream << "()";
	else
	{
		stream << "( ";
		for( const FunctionArgument& arg : function.type_.arguments_ )
		{
			ElementWrite( arg, stream );
			if( &arg != &function.type_.arguments_.back() )
				stream << ", ";
		}
		stream << " )";
	}

	ElementWriteFunctionTypeEnding( function.type_, stream );

	switch( function.body_kind )
	{
	case Function::BodyKind::None:
		break;
	case Function::BodyKind::BodyGenerationRequired:
		stream << "= " << Keyword( Keywords::default_ );
		break;
	case Function::BodyKind::BodyGenerationDisabled:
		stream << "= " << Keyword( Keywords::break_ );
		break;
	}

	if( function.block_ == nullptr )
		stream << ";\n";
	else
	{} // TODO
}

void ElementWrite( const Class& class_, std::ostream& stream )
{
	stream << Keyword(class_.kind_attribute_ == ClassKindAttribute::Struct ? Keywords::struct_ : Keywords::class_ );
	stream << " " << class_.name_ << " ";

	switch( class_.kind_attribute_ )
	{
	case ClassKindAttribute::Struct:
	case ClassKindAttribute::Class:
		break;
	case ClassKindAttribute::Final:
		stream << Keyword( Keywords::final_ ) << " ";
		break;
	case ClassKindAttribute::Polymorph:
		stream << Keyword( Keywords::polymorph_ ) << " ";
		break;
	case ClassKindAttribute::Interface:
		stream << Keyword( Keywords::interface_ ) << " ";
		break;
	case ClassKindAttribute::Abstract:
		stream << Keyword( Keywords::abstract_ ) << " ";
		break;
	};

	if( !class_.parents_.empty() )
	{
		stream << " : ";
		for( const ComplexName& parent : class_.parents_ )
		{
			ElementWrite( parent, stream );
			if( &parent != &class_.parents_.back() )
				stream << ", ";
		}
	}

	if( class_.have_shared_state_ )
		stream << Keyword( Keywords::shared_ ) << " ";
	if( class_.keep_fields_order_ )
		stream << Keyword( Keywords::ordered_ ) << " ";

	if( class_.is_forward_declaration_ )
		stream << ";\n";
	else
	{
		stream << "\n{\n";
		ElementWrite( class_.elements_, stream );
		stream << "}\n";
	}
}

void ElementWrite( const Namespace& namespace_, std::ostream& stream )
{
	stream << Keyword( Keywords::namespace_ ) << " " << namespace_.name_ << "\n{\n\n";
	ElementWrite( namespace_.elements_, stream );
	stream << "\n}\n";
}

void ElementWrite( const VariablesDeclaration& variables_declaration, std::ostream& stream )
{
	stream << Keyword( Keywords::var_ ) << " ";
	ElementWrite( variables_declaration.type, stream );
	stream << "\n";

	for( const VariablesDeclaration::VariableEntry& var : variables_declaration.variables )
	{
		stream << "\t";
		ElementWrite( var.reference_modifier, stream );
		ElementWrite( var.mutability_modifier, stream );
		stream << " "  << var.name;

		if( var.initializer != nullptr )
			ElementWrite( *var.initializer, stream );

		if( &var != &variables_declaration.variables.back() )
			stream << ",\n";
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
	/*
	if( !enum_.underlaying_type_name.components.empty() )
	{
		stream << " : ";
		ElementWrite( enum_.underlaying_type_name, stream );
	}
	*/
	// TODO
	stream << "\n{\n";
	for( const Enum::Member& enum_member : enum_.members )
		stream << "\t" << enum_member.name << ",\n";
	stream << "}\n";
}

void ElementWrite( const Typedef& typedef_, std::ostream& stream )
{
	stream << Keyword( Keywords::type_ ) << " " << typedef_.name << " = ";
	ElementWrite( typedef_.value, stream );
	stream << ";\n";
}

void ElementWrite( const TypeTemplateBase& type_template, std::ostream& stream )
{
	U_UNUSED(type_template);
	U_UNUSED(stream);
	U_ASSERT(false);
	// Not implemented yet.
}

void ElementWrite( const FunctionTemplate& function_template, std::ostream& stream )
{
	U_UNUSED(function_template);
	U_UNUSED(stream);
	U_ASSERT(false);
	// Not implemented yet.
}

void ElementWrite( const ClassField& class_field, std::ostream& stream )
{
	stream << "\t";
	ElementWrite( class_field.type, stream );
	ElementWrite( class_field.reference_modifier, stream );
	stream << " ";
	ElementWrite( class_field.mutability_modifier, stream );
	if( class_field.mutability_modifier != MutabilityModifier::None )
		stream << " ";
	stream << class_field.name << ";\n";
}

void ElementWrite( const ClassVisibilityLabel& visibility_label, std::ostream& stream )
{
	switch( visibility_label.visibility_ )
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

void ElementWrite( const ClassElements& class_elements, std::ostream& stream )
{
	for( const ClassElement& element : class_elements )
		std::visit( UniversalVisitor( stream ), element );
}

void ElementWrite( const ProgramElement& element, std::ostream& stream )
{
	std::visit( UniversalVisitor(stream), element );
}

void ElementWrite( const ProgramElements& elements, std::ostream& stream )
{
	for( const ProgramElement& element : elements )
		ElementWrite( element, stream );
}

void WriteProgramElement( const ComplexName& complex_name, std::ostream& stream )
{
	ElementWrite( complex_name, stream );
}

void WriteProgram( const ProgramElements& program_elements, std::ostream& stream )
{
	ElementWrite( program_elements, stream );
}

} // namespace Synt

} // namespace U
