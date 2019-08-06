#include "assert.hpp"
#include "keywords.hpp"
#include "program_writer.hpp"

namespace U
{

namespace Synt
{

class UniversalVisitor final : public boost::static_visitor<>
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
static void ElementWrite( const ProgramElements& elements, std::ostream& stream );
static void ElementWrite( const EmptyVariant&, std::ostream& ){}
static void ElementWrite( const TypeName& type_name, std::ostream& stream );
static void ElementWrite( const Expression& expression, std::ostream& stream );
static void ElementWrite( const ReferenceModifier& reference_modifier, std::ostream& stream );
static void ElementWrite( const MutabilityModifier& mutability_modifier, std::ostream& stream );
static void ElementWrite( const FunctionArgument& arg, std::ostream& stream );
static void ElementWrite( const ClassElements& class_elements, std::ostream& stream );

// Prototypes end

static void ElementWrite( const ComplexName& complex_name, std::ostream& stream )
{
	for( const ComplexName::Component& component : complex_name.components )
	{
		stream << ToUTF8( component.name );
		if( component.have_template_parameters )
		{
			stream << "</";
			stream << "/>";
		}
		if( &component != &complex_name.components.back() )
			stream << "::";
	}
}

static void ElementWrite( const ArrayTypeName& array_type_name, std::ostream& stream )
{
	stream << "[ ";
	ElementWrite( *array_type_name.element_type, stream );
	stream << ", ";
	ElementWrite( *array_type_name.size, stream );
	stream << " ]";
}

static void ElementWrite( const TypeofTypeName& typeof_type_name, std::ostream& stream )
{
	stream << KeywordAscii( Keywords::typeof_ ) << "( ";
	ElementWrite( *typeof_type_name.expression, stream );
	stream << " )";
}

static void ElementWrite( const NamedTypeName& named_type_name, std::ostream& stream )
{
	ElementWrite( named_type_name.name, stream );
}

static void ElementWriteFunctionTypeEnding( const FunctionType& function_type, std::ostream& stream )
{
	if( function_type.unsafe_ )
		stream << KeywordAscii( Keywords::unsafe_ );

	if( function_type.return_type_ != nullptr )
	{
		stream << " : ";
		ElementWrite( *function_type.return_type_, stream );
	}
	else
		stream << ": " << KeywordAscii( Keywords::void_ );

	if( !function_type.return_value_inner_reference_tags_.empty() )
	{
		stream << "'";
		for( const ProgramString& tag : function_type.return_value_inner_reference_tags_ )
		{
			if( tag.empty() )
				stream << "...";
			else
				stream << ToUTF8( tag );
			if( &tag != &function_type.return_value_inner_reference_tags_.back() && !function_type.return_value_inner_reference_tags_.back().empty() )
				stream << ", ";
		}
		stream << "'";
	}

	ElementWrite( function_type.return_value_reference_modifier_, stream );
	ElementWrite( function_type.return_value_mutability_modifier_, stream );
}

static void ElementWrite( const FunctionType& function_type_name, std::ostream& stream )
{
	stream << "( " << KeywordAscii( Keywords::fn_ ) << "( ";
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

static void ElementWrite( const FunctionArgument& arg, std::ostream& stream )
{
	ElementWrite( arg.type_, stream );
	ElementWrite( arg.reference_modifier_, stream );

	if( !arg.reference_tag_.empty() )
	{
		stream << "'";
		stream << ToUTF8( arg.reference_tag_ ) << " ";
	}

	stream << " ";
	ElementWrite( arg.mutability_modifier_, stream );

	stream << " " << ToUTF8( arg.name_ );

	if( !arg.inner_arg_reference_tags_.empty() )
	{
		stream << "'";
		for( const ProgramString& tag : arg.inner_arg_reference_tags_ )
		{
			if( tag.empty() )
				stream << "...";
			else
				stream << ToUTF8(tag);
			if( &tag != &arg.inner_arg_reference_tags_.back() && !arg.inner_arg_reference_tags_.back().empty() )
				stream << ", ";
		}
		stream << "'";
	}
}

static void ElementWrite( const TypeName& type_name, std::ostream& stream )
{
	boost::apply_visitor( UniversalVisitor(stream), type_name );
}

static void ElementWrite( const Expression& expression, std::ostream& stream )
{
	class Visitor final : public boost::static_visitor<>
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
			stream << " " << ToUTF8( BinaryOperatorToString(binary_operator.operator_type_) ) << " ";
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
			stream << KeywordAscii( Keywords::select_ ) << "( ";
			ElementWrite( *ternary_operator.condition, stream );
			stream << " ? ";
			ElementWrite( *ternary_operator.true_branch, stream );
			stream << " : ";
			ElementWrite( *ternary_operator.false_branch, stream );
			stream << " )";
		}
		void operator()( const NumericConstant& numeric_constant ) const
		{
			stream << numeric_constant.value_;
		}
		void operator()( const StringLiteral& string_literal ) const
		{
			stream << "\"" << ToUTF8( string_literal.value_ ) << "\"" << ToUTF8( string_literal.type_suffix_.data() );
		}
		void operator()( const BooleanConstant& boolean_constant ) const
		{
			stream << ( boolean_constant.value_ ? KeywordAscii( Keywords::true_ ) : KeywordAscii( Keywords::false_ ) );
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
			stream << KeywordAscii( Keywords::move_ ) << "( " << ToUTF8( move_operator.var_name_ ) << " )";
		}
		void operator()( const CastRef& cast_ref ) const
		{
			if( cast_ref.type_ == nullptr || cast_ref.expression_ == nullptr )
				return;
			stream << KeywordAscii( Keywords::cast_ref ) << "</ ";
			ElementWrite( *cast_ref.type_, stream );
			stream << " />( ";
			ElementWrite( *cast_ref.expression_, stream );
			stream << " )";
		}
		void operator()( const CastRefUnsafe& cast_ref_unsafe ) const
		{
			if( cast_ref_unsafe.type_ == nullptr || cast_ref_unsafe.expression_ == nullptr )
				return;
			stream << KeywordAscii( Keywords::cast_ref_unsafe ) << "</ ";
			ElementWrite( *cast_ref_unsafe.type_, stream );
			stream << " />( ";
			ElementWrite( *cast_ref_unsafe.expression_, stream );
			stream << " )";
		}
		void operator()( const CastImut& cast_imut ) const
		{
			if( cast_imut.expression_ == nullptr )
				return;
			stream << KeywordAscii( Keywords::cast_imut ) << "( ";
			ElementWrite( *cast_imut.expression_, stream );
			stream << " )";
		}
		void operator()( const CastMut& cast_mut ) const
		{
			if( cast_mut.expression_ == nullptr )
				return;
			stream << KeywordAscii( Keywords::cast_mut ) << "( ";
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

	boost::apply_visitor( Visitor(stream), expression );

	struct ExpressionWithUnaryOperatorsVisitor final : public boost::static_visitor<const ExpressionComponentWithUnaryOperators*>
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

	if( const auto expression_with_unary_operators= boost::apply_visitor( ExpressionWithUnaryOperatorsVisitor(), expression ) )
	{
		struct PrefixVisitor final : public boost::static_visitor<ProgramString>
		{
			ProgramString operator()( const UnaryMinus& ) const
			{
				return OverloadedOperatorToString( OverloadedOperator::Sub );
			}
			ProgramString operator()( const UnaryPlus& ) const
			{
				return OverloadedOperatorToString( OverloadedOperator::Add );
			}
			ProgramString operator()( const LogicalNot& ) const
			{
				return OverloadedOperatorToString( OverloadedOperator::LogicalNot );
			}
			ProgramString operator()( const BitwiseNot& ) const
			{
				return OverloadedOperatorToString( OverloadedOperator::BitwiseNot );
			}
		};

		class PostifxVisitor final : public boost::static_visitor<>
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
			stream << ToUTF8( boost::apply_visitor( prefix_visitor, prefix_operator ) );

		PostifxVisitor postfix_visitor(stream);
		for( const UnaryPostfixOperator& postfix_operator : expression_with_unary_operators->postfix_operators_ )
			boost::apply_visitor( postfix_visitor, postfix_operator );
	}
}

static void ElementWrite( const ReferenceModifier& reference_modifier, std::ostream& stream )
{
	if( reference_modifier == ReferenceModifier::Reference )
		stream << "&";
}

static void ElementWrite( const MutabilityModifier& mutability_modifier, std::ostream& stream )
{
	switch(mutability_modifier)
	{
	case MutabilityModifier::None:
		break;
	case MutabilityModifier::Mutable:
		stream << KeywordAscii( Keywords::mut_ );
		break;
	case MutabilityModifier::Immutable:
		stream << KeywordAscii( Keywords::imut_ );
		break;
	case MutabilityModifier::Constexpr:
		stream << KeywordAscii( Keywords::constexpr_ );
		break;
	}
}

static void ElementWrite( const Function& function, std::ostream& stream )
{
	if( function.overloaded_operator_ == OverloadedOperator::None )
		stream << KeywordAscii( Keywords::fn_ );
	else
		stream << KeywordAscii( Keywords::op_ );
	stream << " ";

	switch( function.virtual_function_kind_ )
	{
	case VirtualFunctionKind::None:
		break;
	case VirtualFunctionKind::DeclareVirtual:
		stream << KeywordAscii( Keywords::virtual_ ) << " ";
		break;
	case VirtualFunctionKind::VirtualOverride:
		stream << KeywordAscii( Keywords::override_ ) << " ";
		break;
	case VirtualFunctionKind::VirtualFinal:
		stream << KeywordAscii( Keywords::final_ ) << " ";
		break;
	case VirtualFunctionKind::VirtualPure:
		stream << KeywordAscii( Keywords::pure_ ) << " ";
		break;
	};

	if( function.constexpr_ )
		stream << KeywordAscii( Keywords::constexpr_ ) << " ";
	if( function.no_mangle_ )
		stream << KeywordAscii( Keywords::nomangle_ ) << " ";

	if( boost::get<EmptyVariant>(&function.condition_) == nullptr )
	{
		stream << KeywordAscii( Keywords::enable_if_ ) << "( ";
		ElementWrite( function.condition_, stream );
		stream << " ) ";
	}

	ElementWrite( function.name_, stream );

	stream << " ( ";
	for( const FunctionArgument& arg : function.type_.arguments_ )
	{
		ElementWrite( arg, stream );
		if( &arg != &function.type_.arguments_.back() )
			stream << ", ";
	}
	stream << " ) ";

	ElementWriteFunctionTypeEnding( function.type_, stream );

	switch( function.body_kind )
	{
	case Function::BodyKind::None:
		break;
	case Function::BodyKind::BodyGenerationRequired:
		stream << "= " << KeywordAscii( Keywords::default_ );
		break;
	case Function::BodyKind::BodyGenerationDisabled:
		stream << "= " << KeywordAscii( Keywords::break_ );
		break;
	}

	if( function.block_ == nullptr )
		stream << ";\n";
	else
	{} // TODO
}

static void ElementWrite( const Class& class_, std::ostream& stream )
{
	stream << KeywordAscii(class_.kind_attribute_ == ClassKindAttribute::Struct ? Keywords::struct_ : Keywords::class_ );
	stream << " " << ToUTF8( class_.name_ ) << " ";

	switch( class_.kind_attribute_ )
	{
	case ClassKindAttribute::Struct:
	case ClassKindAttribute::Class:
		break;
	case ClassKindAttribute::Final:
		stream << KeywordAscii( Keywords::final_ ) << " ";
		break;
	case ClassKindAttribute::Polymorph:
		stream << KeywordAscii( Keywords::polymorph_ ) << " ";
		break;
	case ClassKindAttribute::Interface:
		stream << KeywordAscii( Keywords::interface_ ) << " ";
		break;
	case ClassKindAttribute::Abstract:
		stream << KeywordAscii( Keywords::abstract_ ) << " ";
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
		stream << KeywordAscii( Keywords::shared_ ) << " ";
	if( class_.keep_fields_order_ )
		stream << KeywordAscii( Keywords::ordered_ ) << " ";

	stream << "\n{\n";
	ElementWrite( class_.elements_, stream );
	stream << "}\n";
}

static void ElementWrite( const Namespace& namespace_, std::ostream& stream )
{
	stream << KeywordAscii( Keywords::namespace_ ) << " " << ToUTF8( namespace_.name_ ) << "\n{\n\n";
	ElementWrite( namespace_.elements_, stream );
	stream << "\n}\n";
}

static void ElementWrite( const VariablesDeclaration& variables_declaration, std::ostream& stream )
{
	U_UNUSED(variables_declaration);
	U_UNUSED(stream);
	U_ASSERT(false);
	// Not implemented yet.
}

static void ElementWrite( const AutoVariableDeclaration& auto_variable_declaration, std::ostream& stream )
{
	U_UNUSED(auto_variable_declaration);
	U_UNUSED(stream);
	U_ASSERT(false);
	// Not implemented yet.
}

static void ElementWrite( const StaticAssert& static_assert_, std::ostream& stream )
{
	U_UNUSED(static_assert_);
	U_UNUSED(stream);
	U_ASSERT(false);
	// Not implemented yet.
}

static void ElementWrite( const Enum& enum_, std::ostream& stream )
{
	U_UNUSED(enum_);
	U_UNUSED(stream);
	U_ASSERT(false);
	// Not implemented yet.
}

static void ElementWrite( const Typedef& typedef_, std::ostream& stream )
{
	stream << KeywordAscii( Keywords::type_ ) << " " << ToUTF8( typedef_.name ) << " = ";
	ElementWrite( typedef_.value, stream );
	stream << ";\n";
}

static void ElementWrite( const TypeTemplateBase& type_template, std::ostream& stream )
{
	U_UNUSED(type_template);
	U_UNUSED(stream);
	U_ASSERT(false);
	// Not implemented yet.
}

static void ElementWrite( const FunctionTemplate& function_template, std::ostream& stream )
{
	U_UNUSED(function_template);
	U_UNUSED(stream);
	U_ASSERT(false);
	// Not implemented yet.
}

static void ElementWrite( const ClassField& class_field, std::ostream& stream )
{
	ElementWrite( class_field.type, stream );
	stream << " ";
	ElementWrite( class_field.reference_modifier, stream );
	ElementWrite( class_field.mutability_modifier, stream );
	stream << " " << ToUTF8( class_field.name ) << ";\n";
}

static void ElementWrite( const ClassVisibilityLabel& visibility_label, std::ostream& stream )
{
	switch( visibility_label.visibility_ )
	{
	case ClassMemberVisibility::Public:
		stream << KeywordAscii( Keywords::public_ ) << ": \n";
		break;
	case ClassMemberVisibility::Protected:
		stream << KeywordAscii( Keywords::protected_ ) << ": \n";
		break;
	case ClassMemberVisibility::Private:
		stream << KeywordAscii( Keywords::private_ ) << ": \n";
		break;
	};
}

static void ElementWrite( const ClassElements& class_elements, std::ostream& stream )
{
	for( const ClassElement& element : class_elements )
		boost::apply_visitor( UniversalVisitor( stream ), element );
}

static void ElementWrite( const ProgramElement& element, std::ostream& stream )
{
	boost::apply_visitor( UniversalVisitor(stream), element );
}

static void ElementWrite( const ProgramElements& elements, std::ostream& stream )
{
	for( const ProgramElement& element : elements )
		ElementWrite( element, stream );
}

void WriteProgram( const ProgramElements& program_elements, std::ostream& stream )
{
	ElementWrite( program_elements, stream );
}

} // namespace Synt

} // namespace U
