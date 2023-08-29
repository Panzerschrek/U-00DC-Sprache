#include <algorithm>
#include "syntax_tree_lookup.hpp"

namespace U
{

namespace LangServer
{

namespace
{

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::Expression& expression );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ExpressionPtr& expression_ptr );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TypeName& type_name );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NonSyncTag& non_sync_tag );

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::EmptyVariant& empty_variant );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::CallOperator& call_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::IndexationOperator& indexation_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::MemberAccessOperator& member_access_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::UnaryPlus& unary_plus );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::UnaryMinus& unary_minus );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::LogicalNot& ligical_not );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::BitwiseNot& bitwise_not );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ComplexName& complex_name );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::BinaryOperator& binary_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TernaryOperator& ternary_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ReferenceToRawPointerOperator& reference_to_raw_pointer_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::RawPointerToReferenceOperator& raw_pointer_to_reference_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NumericConstant& numeric_constant );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::BooleanConstant& boolean_constant );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::StringLiteral& string_literal );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::MoveOperator& move_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TakeOperator& take_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::CastMut& cast_mut );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::CastImut& cast_imut );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::CastRef& cast_ref );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::CastRefUnsafe& cast_ref_unsafe );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TypeInfo& type_info );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NonSyncExpression& non_sync_expression );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::SafeExpression& safe_expression );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::UnsafeExpression& unsafe_expression );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ArrayTypeName& array_type_name );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::FunctionTypePtr& function_type_ptr );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::FunctionType& function_type );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TupleType& tuple_type );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::RawPointerType& raw_pointer_type );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::GeneratorTypePtr& generator_type_ptr );

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TypeofTypeName& typeof_type_name );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::RootNamespaceNameLookup& root_namespace_name_lookup );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NameLookup& name_lookup );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NameLookupCompletion& name_lookup_completion );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NamesScopeNameFetch& names_scope_names_fetch );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TemplateParametrization& template_parametrization );

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::Initializer& initializer );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::SequenceInitializer& sequence_initializer );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::StructNamedInitializer& struct_named_initializer );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ConstructorInitializer& constructor_initializer );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ZeroInitializer& zero_initializer );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::UninitializedInitializer& uninitialized_initializer );

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ProgramElements& program_elements );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ClassElements& class_elements );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::VariablesDeclaration& variables_declaration );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::AutoVariableDeclaration& auto_variable_declaration );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::StaticAssert& static_assert_ );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const std::unique_ptr<const Synt::TypeAlias>& type_alias_ptr );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TypeAlias& type_alias );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::Enum& enum_ );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::FunctionPtr& function_ptr );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ClassPtr& class_ptr );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TypeTemplate& type_template );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::FunctionTemplate& function_template );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NamespacePtr& namespace_ptr );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ClassField& class_field );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ClassVisibilityLabel& visibiliy_label );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NonSyncTagNone& non_sync_tag_none );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NonSyncTagTrue& non_sync_tag_true );

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::IfAlternative& if_alternative );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::Block& block );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ScopeBlock& scope_block );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ReturnOperator& return_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::YieldOperator& yield_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::WhileOperator& while_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::LoopOperator& loop_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::RangeForOperator& range_for_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::CStyleForOperator& c_style_for_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::BreakOperator& break_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ContinueOperator& continue_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::WithOperator& with_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::IfOperator& if_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::StaticIfOperator& static_if_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::IfCoroAdvanceOperator& if_coro_advance_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::SwitchOperator& switch_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::SingleExpressionOperator& single_expression_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::AssignmentOperator& assignment_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::AdditiveAssignmentOperator& additive_assignment_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::IncrementOperator& increment_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::DecrementOperator& decrement_operator );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::Halt& halt );
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::HaltIf& halt_if );

template<typename ... Args>
SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImplVariant( uint32_t line, uint32_t column, const std::variant<Args...>& v )
{
	return std::visit( [&]( const auto& el ) { return FindSyntaxElementForPositionImpl( line, column, el ); }, v );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::Expression& expression )
{
	return FindSyntaxElementForPositionImplVariant( line, column, expression );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ExpressionPtr& expression_ptr )
{
	return FindSyntaxElementForPositionImpl( line, column, *expression_ptr );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TypeName& type_name )
{
	return FindSyntaxElementForPositionImplVariant( line, column, type_name );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NonSyncTag& non_sync_tag )
{
	return FindSyntaxElementForPositionImplVariant( line, column, non_sync_tag );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::EmptyVariant& empty_variant )
{
	(void)line;
	(void)column;
	(void)empty_variant;
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::CallOperator& call_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *call_operator.expression_ );
		if( res != std::nullopt )
			return res;
	}

	for( const Synt::Expression& expression : call_operator.arguments_ )
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, expression );
		if( res != std::nullopt )
			return res;
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::IndexationOperator& indexation_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *indexation_operator.expression_ );
		if( res != std::nullopt )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *indexation_operator.index_ );
		if( res != std::nullopt )
			return res;
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::MemberAccessOperator& member_access_operator )
{
	if( member_access_operator.src_loc_.GetLine() == line && member_access_operator.src_loc_.GetColumn() == column )
		return SyntaxTreeLookupResult{ {}, &member_access_operator, std::nullopt };

	if( member_access_operator.template_parameters != std::nullopt )
	{
		for( const Synt::Expression& template_arg : *member_access_operator.template_parameters )
		{
			auto res= FindSyntaxElementForPositionImpl( line, column, template_arg );
			if( res != std::nullopt )
				return res;
		}
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::UnaryPlus& unary_plus )
{
	return FindSyntaxElementForPositionImpl( line, column, *unary_plus.expression_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::UnaryMinus& unary_minus )
{
	return FindSyntaxElementForPositionImpl( line, column, *unary_minus.expression_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::LogicalNot& ligical_not )
{
	return FindSyntaxElementForPositionImpl( line, column, *ligical_not.expression_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::BitwiseNot& bitwise_not )
{
	return FindSyntaxElementForPositionImpl( line, column, *bitwise_not.expression_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ComplexName& complex_name )
{
	return FindSyntaxElementForPositionImplVariant( line, column, complex_name );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::BinaryOperator& binary_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *binary_operator.left_ );
		if( res != std::nullopt )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *binary_operator.right_ );
		if( res != std::nullopt )
			return res;
	}
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TernaryOperator& ternary_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *ternary_operator.condition );
		if( res != std::nullopt )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *ternary_operator.false_branch );
		if( res != std::nullopt )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *ternary_operator.true_branch );
		if( res != std::nullopt )
			return res;
	}
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ReferenceToRawPointerOperator& reference_to_raw_pointer_operator )
{
	return FindSyntaxElementForPositionImpl( line, column, *reference_to_raw_pointer_operator.expression );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::RawPointerToReferenceOperator& raw_pointer_to_reference_operator )
{
	return FindSyntaxElementForPositionImpl( line, column, *raw_pointer_to_reference_operator.expression );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NumericConstant& numeric_constant )
{
	(void)line;
	(void)column;
	(void)numeric_constant;
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::BooleanConstant& boolean_constant )
{
	(void)line;
	(void)column;
	(void)boolean_constant;
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::StringLiteral& string_literal )
{
	(void)line;
	(void)column;
	(void)string_literal;
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::MoveOperator& move_operator )
{
	// TOOD - process it specially
	(void)line;
	(void)column;
	(void)move_operator;
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TakeOperator& take_operator )
{
	return FindSyntaxElementForPositionImpl( line, column, *take_operator.expression_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::CastMut& cast_mut )
{
	return FindSyntaxElementForPositionImpl( line, column, *cast_mut.expression_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::CastImut& cast_imut )
{
	return FindSyntaxElementForPositionImpl( line, column, *cast_imut.expression_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::CastRef& cast_ref )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *cast_ref.type_ );
		if( res != std::nullopt )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *cast_ref.expression_ );
		if( res != std::nullopt )
			return res;
	}
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::CastRefUnsafe& cast_ref_unsafe )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *cast_ref_unsafe.type_ );
		if( res != std::nullopt )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *cast_ref_unsafe.expression_ );
		if( res != std::nullopt )
			return res;
	}
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TypeInfo& type_info )
{
	return FindSyntaxElementForPositionImpl( line, column,* type_info.type_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NonSyncExpression& non_sync_expression )
{
	return FindSyntaxElementForPositionImpl( line, column,* non_sync_expression.type_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::SafeExpression& safe_expression )
{
	return FindSyntaxElementForPositionImpl( line, column,* safe_expression.expression_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::UnsafeExpression& unsafe_expression )
{
	return FindSyntaxElementForPositionImpl( line, column,* unsafe_expression.expression_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ArrayTypeName& array_type_name )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *array_type_name.element_type );
		if( res != std::nullopt )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *array_type_name.size );
		if( res != std::nullopt )
			return res;
	}
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::FunctionTypePtr& function_type_ptr )
{
	return FindSyntaxElementForPositionImpl( line, column, *function_type_ptr );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::FunctionType& function_type )
{
	if( function_type.return_type_ != nullptr )
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *function_type.return_type_ );
		if( res != std::nullopt )
			return res;
	}

	for( const Synt::FunctionParam& param : function_type.params_ )
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, param.type_ );
		if( res != std::nullopt )
			return res;
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TupleType& tuple_type )
{
	for( const Synt::TypeName& element_type_name : tuple_type.element_types_ )
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, element_type_name );
		if( res != std::nullopt )
			return res;
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::RawPointerType& raw_pointer_type )
{
	return FindSyntaxElementForPositionImpl( line, column, *raw_pointer_type.element_type );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::GeneratorTypePtr& generator_type_ptr )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, generator_type_ptr->return_type );
		if( res != std::nullopt )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, generator_type_ptr->non_sync_tag );
		if( res != std::nullopt )
			return res;
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TypeofTypeName& typeof_type_name )
{
	return FindSyntaxElementForPositionImpl( line, column, *typeof_type_name.expression );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::RootNamespaceNameLookup& root_namespace_name_lookup )
{
	if( line == root_namespace_name_lookup.src_loc_.GetLine() && column == root_namespace_name_lookup.src_loc_.GetColumn() )
		return SyntaxTreeLookupResult{ {}, &root_namespace_name_lookup, std::nullopt };
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NameLookup& name_lookup )
{
	if( line == name_lookup.src_loc_.GetLine() && column == name_lookup.src_loc_.GetColumn() )
		return SyntaxTreeLookupResult{ {}, &name_lookup, std::nullopt };
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NameLookupCompletion& name_lookup_completion )
{
	if( line == name_lookup_completion.src_loc_.GetLine() && column == name_lookup_completion.src_loc_.GetColumn() )
		return SyntaxTreeLookupResult{ {}, &name_lookup_completion, std::nullopt };
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NamesScopeNameFetch& names_scope_names_fetch )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *names_scope_names_fetch.base );
		if( res != std::nullopt )
			return res;
	}

	if( line == names_scope_names_fetch.src_loc_.GetLine() && column == names_scope_names_fetch.src_loc_.GetColumn() )
		return SyntaxTreeLookupResult{ {}, &names_scope_names_fetch, std::nullopt };
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TemplateParametrization& template_parametrization )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *template_parametrization.base );
		if( res != std::nullopt )
			return res;
	}

	for( const Synt::Expression& arg : template_parametrization.template_args )
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, arg );
		if( res != std::nullopt )
			return res;
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::Initializer& initializer )
{
	return FindSyntaxElementForPositionImplVariant( line, column, initializer );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::SequenceInitializer& sequence_initializer )
{
	for( const Synt::Initializer& initializer : sequence_initializer.initializers )
	{
		SyntaxTreeLookupResultOpt res= FindSyntaxElementForPositionImpl( line, column, initializer );
		if( res != std::nullopt )
			return res;
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::StructNamedInitializer& struct_named_initializer )
{
	for( const Synt::StructNamedInitializer::MemberInitializer& member_initializer : struct_named_initializer.members_initializers )
	{
		// TODO - process also name of member itself.
		SyntaxTreeLookupResultOpt res= FindSyntaxElementForPositionImpl( line, column, member_initializer.initializer );
		if( res != std::nullopt )
			return res;
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ConstructorInitializer& constructor_initializer )
{
	for( const Synt::Expression& expression : constructor_initializer.arguments )
	{
		SyntaxTreeLookupResultOpt res= FindSyntaxElementForPositionImpl( line, column, expression );
		if( res != std::nullopt )
			return res;
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ZeroInitializer& zero_initializer )
{
	(void)line;
	(void)column;
	(void)zero_initializer;
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::UninitializedInitializer& uninitialized_initializer )
{
	(void)line;
	(void)column;
	(void)uninitialized_initializer;
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ProgramElements& program_elements )
{
	for( const Synt::ProgramElement& program_element : program_elements )
	{
		SyntaxTreeLookupResultOpt res= FindSyntaxElementForPositionImplVariant( line, column, program_element );
		if( res != std::nullopt )
		{
			if( res->global_item == std::nullopt )
				res->global_item= GlobalItem(&program_element);
			return res;
		}
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ClassElements& class_elements )
{
	for( const Synt::ClassElement& class_element : class_elements )
	{
		SyntaxTreeLookupResultOpt res= FindSyntaxElementForPositionImplVariant( line, column, class_element );
		if( res != std::nullopt )
		{
			if( res->global_item == std::nullopt )
				res->global_item= GlobalItem(&class_element);
			return res;
		}
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::VariablesDeclaration& variables_declaration )
{
	{
		SyntaxTreeLookupResultOpt res= FindSyntaxElementForPositionImpl( line, column, variables_declaration.type );
		if( res != std::nullopt )
			return res;
	}

	for( const Synt::VariablesDeclaration::VariableEntry& entry : variables_declaration.variables )
	{
		if( entry.initializer != nullptr )
		{
			SyntaxTreeLookupResultOpt res= FindSyntaxElementForPositionImpl( line, column, *entry.initializer );
			if( res != std::nullopt )
				return res;
		}
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::AutoVariableDeclaration& auto_variable_declaration )
{
	return FindSyntaxElementForPositionImpl( line, column, auto_variable_declaration.initializer_expression );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::StaticAssert& static_assert_ )
{
	return FindSyntaxElementForPositionImpl( line, column, static_assert_.expression );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const std::unique_ptr<const Synt::TypeAlias>& type_alias_ptr )
{
	return FindSyntaxElementForPositionImpl( line, column, *type_alias_ptr );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TypeAlias& type_alias )
{
	return FindSyntaxElementForPositionImpl( line, column, type_alias.value );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::Enum& enum_ )
{
	// Nothing to do here.
	(void)line;
	(void)column;
	(void)enum_;
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::FunctionPtr& function_ptr )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, function_ptr->type_ );
		if( res != std::nullopt )
			return res;
	}

	{
		auto res= FindSyntaxElementForPositionImpl( line, column, function_ptr->coroutine_non_sync_tag );
		if( res != std::nullopt )
			return res;
	}

	if( function_ptr->block_ != nullptr )
		return FindSyntaxElementForPositionImpl( line, column, *function_ptr->block_ );

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ClassPtr& class_ptr )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, class_ptr->elements_ );
		if( res != std::nullopt )
		{
			res->prefix.push_back( class_ptr.get() );
			return res;
		}
	}

	for( const Synt::ComplexName& parent : class_ptr->parents_ )
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, parent );
		if( res != std::nullopt )
			return res;
	}

	{
		auto res= FindSyntaxElementForPositionImpl( line, column, class_ptr->non_sync_tag_ );
		if( res != std::nullopt )
			return res;
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TypeTemplate& type_template )
{
	auto res= FindSyntaxElementForPositionImplVariant( line, column, type_template.something_ );
	if( res != std::nullopt )
	{
		res->prefix.push_back( &type_template );
		return res;
	}

	// TODO - process template arguments and signature arguments.

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::FunctionTemplate& function_template )
{
	// TODO
	(void)line;
	(void)column;
	(void)function_template;
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NamespacePtr& namespace_ptr )
{
	auto res= FindSyntaxElementForPositionImpl( line, column, namespace_ptr->elements_ );
	if( res != std::nullopt )
	{
		res->prefix.push_back( namespace_ptr.get() );
		return res;
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ClassField& class_field )
{
	return FindSyntaxElementForPositionImpl( line, column, class_field.type );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ClassVisibilityLabel& visibiliy_label )
{
	(void)line;
	(void)column;
	(void)visibiliy_label;
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NonSyncTagNone& non_sync_tag_none )
{
	(void)line;
	(void)column;
	(void)non_sync_tag_none;
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl(const uint32_t line, const uint32_t column, const Synt::NonSyncTagTrue& non_sync_tag_true )
{
	(void)line;
	(void)column;
	(void)non_sync_tag_true;
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::IfAlternative& if_alternative )
{
	return FindSyntaxElementForPositionImplVariant( line, column, if_alternative );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::Block& block )
{
	for( const Synt::BlockElement& block_element : block.elements_ )
	{
		auto res= FindSyntaxElementForPositionImplVariant( line, column, block_element );
		if( res != std::nullopt )
			return res;
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ScopeBlock& scope_block )
{
	return FindSyntaxElementForPositionImpl( line, column, static_cast<const Synt::Block&>(scope_block) );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ReturnOperator& return_operator )
{
	return FindSyntaxElementForPositionImpl( line, column, return_operator.expression_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::YieldOperator& yield_operator )
{
	return FindSyntaxElementForPositionImpl( line, column, yield_operator.expression );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::WhileOperator& while_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, while_operator.condition_ );
		if( res != std::nullopt )
			return res;
	}

	return FindSyntaxElementForPositionImpl( line, column, *while_operator.block_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::LoopOperator& loop_operator )
{
	return FindSyntaxElementForPositionImpl( line, column, *loop_operator.block_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::RangeForOperator& range_for_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, range_for_operator.sequence_ );
		if( res != std::nullopt )
			return res;
	}

	return FindSyntaxElementForPositionImpl( line, column, *range_for_operator.block_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::CStyleForOperator& c_style_for_operator )
{
	if( c_style_for_operator.variable_declaration_part_ != nullptr )
	{
		auto res= FindSyntaxElementForPositionImplVariant( line, column, *c_style_for_operator.variable_declaration_part_ );
		if( res != std::nullopt )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, c_style_for_operator.loop_condition_ );
		if( res != std::nullopt )
			return res;
	}
	for( const auto& element : c_style_for_operator.iteration_part_elements_ )
	{
		auto res= FindSyntaxElementForPositionImplVariant( line, column, element );
		if( res != std::nullopt )
			return res;
	}

	return FindSyntaxElementForPositionImpl( line, column, *c_style_for_operator.block_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::BreakOperator& break_operator )
{
	(void)line;
	(void)column;
	(void)break_operator;
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ContinueOperator& continue_operator )
{
	(void)line;
	(void)column;
	(void)continue_operator;
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::WithOperator& with_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, with_operator.expression_ );
		if( res != std::nullopt )
			return res;
	}

	return FindSyntaxElementForPositionImpl( line, column, with_operator.block_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::IfOperator& if_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, if_operator.condition );
		if( res != std::nullopt )
			return res;
	}
	if( if_operator.alternative != nullptr )
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *if_operator.alternative );
		if( res != std::nullopt )
			return res;
	}

	return FindSyntaxElementForPositionImpl( line, column, if_operator.block );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::StaticIfOperator& static_if_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, static_if_operator.condition );
		if( res != std::nullopt )
			return res;
	}
	if( static_if_operator.alternative != nullptr )
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *static_if_operator.alternative );
		if( res != std::nullopt )
			return res;
	}

	return FindSyntaxElementForPositionImpl( line, column, static_if_operator.block );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::IfCoroAdvanceOperator& if_coro_advance_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, if_coro_advance_operator.expression );
		if( res != std::nullopt )
			return res;
	}
	if( if_coro_advance_operator.alternative != nullptr )
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *if_coro_advance_operator.alternative );
		if( res != std::nullopt )
			return res;
	}

	return FindSyntaxElementForPositionImpl( line, column, if_coro_advance_operator.block );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::SwitchOperator& switch_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, switch_operator.value );
		if( res != std::nullopt )
			return res;
	}

	for( const Synt::SwitchOperator::Case& case_ : switch_operator.cases )
	{
		if( const auto case_values= std::get_if< std::vector<Synt::SwitchOperator::CaseValue> >( &case_.values ) )
		{
			for( const auto& case_value : *case_values )
			{
				if( const auto single_value= std::get_if< Synt::Expression >( &case_value ) )
				{
					auto res= FindSyntaxElementForPositionImpl( line, column, *single_value );
					if( res != std::nullopt )
						return res;
				}
				else if( const auto range= std::get_if< Synt::SwitchOperator::CaseRange >( &case_value ) )
				{
					{
						auto res= FindSyntaxElementForPositionImpl( line, column, range->low );
						if( res != std::nullopt )
							return res;
					}
					{
						auto res= FindSyntaxElementForPositionImpl( line, column, range->high );
						if( res != std::nullopt )
							return res;
					}
				}
				// else U_ASSERT(false);
			}
		}
		else if( std::get_if< Synt::SwitchOperator::DefaultPlaceholder >( &case_.values ) != nullptr )
		{}
		// else U_ASSERT(false);

		{
			auto res= FindSyntaxElementForPositionImpl( line, column, case_.block );
			if( res != std::nullopt )
				return res;
		}
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::SingleExpressionOperator& single_expression_operator )
{
	return FindSyntaxElementForPositionImpl( line, column, single_expression_operator.expression_ );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::AssignmentOperator& assignment_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, assignment_operator.l_value_ );
		if( res != std::nullopt )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, assignment_operator.r_value_ );
		if( res != std::nullopt )
			return res;
	}
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::AdditiveAssignmentOperator& additive_assignment_operator )
{

	{
		auto res= FindSyntaxElementForPositionImpl( line, column, additive_assignment_operator.l_value_ );
		if( res != std::nullopt )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, additive_assignment_operator.r_value_ );
		if( res != std::nullopt )
			return res;
	}
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::IncrementOperator& increment_operator )
{
	return FindSyntaxElementForPositionImpl( line, column, increment_operator.expression );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::DecrementOperator& decrement_operator )
{
	return FindSyntaxElementForPositionImpl( line, column, decrement_operator.expression );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::Halt& halt )
{
	(void)line;
	(void)column;
	(void)halt;
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::HaltIf& halt_if )
{
	return FindSyntaxElementForPositionImpl( line, column, halt_if.condition );
}

} // namespace

SyntaxTreeLookupResultOpt FindSyntaxElementForPosition( const uint32_t line, const uint32_t column, const Synt::ProgramElements& program_elements )
{
	auto res= FindSyntaxElementForPositionImpl( line, column, program_elements );
	if( res != std::nullopt )
	{
		// Prefix elements are pushed in reverse order. So, reverse order.
		std::reverse( res->prefix.begin(), res->prefix.end() );
		return res;
	}

	return std::nullopt;
}

} // namespace LangServer

} // namespace U
