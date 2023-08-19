#include "syntax_tree_lookup.hpp"

namespace U
{

namespace LangServer
{

namespace
{

NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::Expression& expression );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ExpressionPtr& expression_ptr );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TypeName& type_name );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NonSyncTag& non_sync_tag );

NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::EmptyVariant& empty_variant );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::CallOperator& call_operator );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::IndexationOperator& indexation_operator );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::MemberAccessOperator& member_access_operator );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::UnaryPlus& unary_plus );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::UnaryMinus& unary_minus );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::LogicalNot& ligical_not );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::BitwiseNot& bitwise_not );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ComplexName& complex_name );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::BinaryOperator& binary_operator );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TernaryOperator& ternary_operator );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ReferenceToRawPointerOperator& reference_to_raw_pointer_operator );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::RawPointerToReferenceOperator& raw_pointer_to_reference_operator );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NumericConstant& numeric_constant );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::BooleanConstant& boolean_constant );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::StringLiteral& string_literal );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::MoveOperator& move_operator );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TakeOperator& take_operator );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::CastMut& cast_mut );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::CastImut& cast_imut );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::CastRef& cast_ref );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::CastRefUnsafe& cast_ref_unsafe );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TypeInfo& type_info );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NonSyncExpression& non_sync_expression );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::SafeExpression& safe_expression );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::UnsafeExpression& unsafe_expression );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ArrayTypeName& array_type_name );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::FunctionTypePtr& function_type_ptr );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TupleType& tuple_type );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::RawPointerType& raw_pointer_type );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::GeneratorTypePtr& generator_type_ptr );

NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TypeofTypeName& typeof_type_name );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::RootNamespaceNameLookup& root_namespace_name_lookup );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NameLookup& name_lookup );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NamesScopeNameFetch& names_scope_names_fetch );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TemplateParametrization& template_parametrization );

NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ProgramElements& program_elements );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::VariablesDeclaration& variables_declaration );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::AutoVariableDeclaration& auto_variable_declaration );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::StaticAssert& static_assert_ );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TypeAlias& type_alias );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::Enum& enum_ );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::FunctionPtr& function_ptr );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ClassPtr& class_ptr );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::TypeTemplate& type_template );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::FunctionTemplate& function_template );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NamespacePtr& namespace_ptr );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ClassField& class_field );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::ClassVisibilityLabel& visibiliy_label );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NonSyncTagNone& non_sync_tag_none );
NamedSyntaxElement FindSyntaxElementForPositionImpl( uint32_t line, uint32_t column, const Synt::NonSyncTagTrue& non_sync_tag_true );

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::Expression& expression )
{
	return std::visit( [&]( const auto& el ) { return FindSyntaxElementForPositionImpl( line, column, el ); }, expression );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ExpressionPtr& expression_ptr )
{
	return FindSyntaxElementForPositionImpl( line, column, *expression_ptr );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TypeName& type_name )
{
	return std::visit( [&]( const auto& el ) { return FindSyntaxElementForPositionImpl( line, column, el ); }, type_name );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NonSyncTag& non_sync_tag )
{
	return std::visit( [&]( const auto& el ) { return FindSyntaxElementForPositionImpl( line, column, el ); }, non_sync_tag );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::EmptyVariant& empty_variant )
{
	(void)line;
	(void)column;
	return empty_variant;
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::CallOperator& call_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *call_operator.expression_ );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}

	for( const Synt::Expression& expression : call_operator.arguments_ )
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, expression );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}

	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::IndexationOperator& indexation_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *indexation_operator.expression_ );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *indexation_operator.index_ );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}

	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::MemberAccessOperator& member_access_operator )
{
	// TODO - check also "src_loc" of name itself.
	if( member_access_operator.src_loc_.GetLine() == line && member_access_operator.src_loc_.GetColumn() == column )
		return &member_access_operator;

	if( member_access_operator.template_parameters != std::nullopt )
	{
		// TODO
	}

	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::UnaryPlus& unary_plus )
{
	return FindSyntaxElementForPositionImpl( line, column, *unary_plus.expression_ );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::UnaryMinus& unary_minus )
{
	return FindSyntaxElementForPositionImpl( line, column, *unary_minus.expression_ );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::LogicalNot& ligical_not )
{
	return FindSyntaxElementForPositionImpl( line, column, *ligical_not.expression_ );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::BitwiseNot& bitwise_not )
{
	return FindSyntaxElementForPositionImpl( line, column, *bitwise_not.expression_ );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ComplexName& complex_name )
{
	return std::visit( [&]( const auto& el ) { return FindSyntaxElementForPositionImpl( line, column, el ); }, complex_name );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::BinaryOperator& binary_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *binary_operator.left_ );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *binary_operator.right_ );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TernaryOperator& ternary_operator )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *ternary_operator.condition );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *ternary_operator.false_branch );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *ternary_operator.true_branch );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ReferenceToRawPointerOperator& reference_to_raw_pointer_operator )
{
	return FindSyntaxElementForPositionImpl( line, column, *reference_to_raw_pointer_operator.expression );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::RawPointerToReferenceOperator& raw_pointer_to_reference_operator )
{
	return FindSyntaxElementForPositionImpl( line, column, *raw_pointer_to_reference_operator.expression );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NumericConstant& numeric_constant )
{
	(void)line;
	(void)column;
	(void)numeric_constant;
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::BooleanConstant& boolean_constant )
{
	(void)line;
	(void)column;
	(void)boolean_constant;
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::StringLiteral& string_literal )
{
	(void)line;
	(void)column;
	(void)string_literal;
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::MoveOperator& move_operator )
{
	// TOOD - process it specially
	(void)line;
	(void)column;
	(void)move_operator;
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TakeOperator& take_operator )
{
	return FindSyntaxElementForPositionImpl( line, column, *take_operator.expression_ );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::CastMut& cast_mut )
{
	return FindSyntaxElementForPositionImpl( line, column, *cast_mut.expression_ );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::CastImut& cast_imut )
{
	return FindSyntaxElementForPositionImpl( line, column, *cast_imut.expression_ );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::CastRef& cast_ref )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *cast_ref.type_ );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *cast_ref.expression_ );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::CastRefUnsafe& cast_ref_unsafe )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *cast_ref_unsafe.type_ );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *cast_ref_unsafe.expression_ );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TypeInfo& type_info )
{
	return FindSyntaxElementForPositionImpl( line, column,* type_info.type_ );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NonSyncExpression& non_sync_expression )
{
	return FindSyntaxElementForPositionImpl( line, column,* non_sync_expression.type_ );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::SafeExpression& safe_expression )
{
	return FindSyntaxElementForPositionImpl( line, column,* safe_expression.expression_ );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::UnsafeExpression& unsafe_expression )
{
	return FindSyntaxElementForPositionImpl( line, column,* unsafe_expression.expression_ );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ArrayTypeName& array_type_name )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *array_type_name.element_type );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *array_type_name.size );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::FunctionTypePtr& function_type_ptr )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, *function_type_ptr->return_type_ );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}

	for( const Synt::FunctionParam& param : function_type_ptr->params_ )
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, param.type_ );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}

	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TupleType& tuple_type )
{
	for( const Synt::TypeName& element_type_name : tuple_type.element_types_ )
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, element_type_name );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}

	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::RawPointerType& raw_pointer_type )
{
	return FindSyntaxElementForPositionImpl( line, column, *raw_pointer_type.element_type );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::GeneratorTypePtr& generator_type_ptr )
{
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, generator_type_ptr->return_type );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, generator_type_ptr->non_sync_tag );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}

	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TypeofTypeName& typeof_type_name )
{
	return FindSyntaxElementForPositionImpl( line, column, *typeof_type_name.expression );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::RootNamespaceNameLookup& root_namespace_name_lookup )
{
	if( line == root_namespace_name_lookup.src_loc_.GetLine() && column == root_namespace_name_lookup.src_loc_.GetColumn() )
		return &root_namespace_name_lookup;
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NameLookup& name_lookup )
{
	if( line == name_lookup.src_loc_.GetLine() && column == name_lookup.src_loc_.GetColumn() )
		return &name_lookup;
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NamesScopeNameFetch& names_scope_names_fetch )
{
	if( line == names_scope_names_fetch.src_loc_.GetLine() && column == names_scope_names_fetch.src_loc_.GetColumn() )
		return &names_scope_names_fetch;
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TemplateParametrization& template_parametrization )
{
	for( const Synt::Expression& arg : template_parametrization.template_args )
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, arg );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}

	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ProgramElements& program_elements )
{
	for( const Synt::ProgramElement& program_element : program_elements )
	{
		NamedSyntaxElement res= std::visit( [&]( const auto& el ) { return FindSyntaxElementForPositionImpl( line, column, el ); }, program_element );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}

	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::VariablesDeclaration& variables_declaration )
{
	// TODO
	(void)line;
	(void)column;
	(void)variables_declaration;
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::AutoVariableDeclaration& auto_variable_declaration )
{
	// TODO
	(void)line;
	(void)column;
	(void)auto_variable_declaration;
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::StaticAssert& static_assert_ )
{
	return FindSyntaxElementForPositionImpl( line, column, static_assert_.expression );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TypeAlias& type_alias )
{
	return FindSyntaxElementForPositionImpl( line, column, type_alias.value );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::Enum& enum_ )
{
	// TODO
	(void)line;
	(void)column;
	(void)enum_;
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::FunctionPtr& function_ptr )
{
	// TODO
	(void)line;
	(void)column;
	(void)function_ptr;
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ClassPtr& class_ptr )
{
	for( const Synt::ClassElement& class_element : class_ptr->elements_ )
	{
		NamedSyntaxElement res= std::visit( [&]( const auto& el ) { return FindSyntaxElementForPositionImpl( line, column, el ); }, class_element );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}

	for( const Synt::ComplexName& parent : class_ptr->parents_ )
	{
		auto res= FindSyntaxElementForPositionImpl( line, column, parent );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}

	{
		auto res= FindSyntaxElementForPositionImpl( line, column, class_ptr->non_sync_tag_ );
		if( std::get_if<Synt::EmptyVariant>( &res ) == nullptr )
			return res;
	}

	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TypeTemplate& type_template )
{
	// TODO
	(void)line;
	(void)column;
	(void)type_template;
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::FunctionTemplate& function_template )
{
	// TODO
	(void)line;
	(void)column;
	(void)function_template;
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NamespacePtr& namespace_ptr )
{
	return FindSyntaxElementForPositionImpl( line, column, namespace_ptr->elements_ );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ClassField& class_field )
{
	return FindSyntaxElementForPositionImpl( line, column, class_field.type );
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ClassVisibilityLabel& visibiliy_label )
{
	(void)line;
	(void)column;
	(void)visibiliy_label;
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NonSyncTagNone& non_sync_tag_none )
{
	(void)line;
	(void)column;
	(void)non_sync_tag_none;
	return Synt::EmptyVariant{};
}

NamedSyntaxElement FindSyntaxElementForPositionImpl(const uint32_t line, const uint32_t column, const Synt::NonSyncTagTrue& non_sync_tag_true )
{
	(void)line;
	(void)column;
	(void)non_sync_tag_true;
	return Synt::EmptyVariant{};
}

} // namespace

NamedSyntaxElement FindSyntaxElementForPosition( const uint32_t line, const uint32_t column, const Synt::ProgramElements& program_elements )
{
	return FindSyntaxElementForPositionImpl( line, column, program_elements );
}

} // namespace LangServer

} // namespace U
