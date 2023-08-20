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

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::Expression& expression )
{
	return std::visit( [&]( const auto& el ) { return FindSyntaxElementForPositionImpl( line, column, el ); }, expression );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ExpressionPtr& expression_ptr )
{
	return FindSyntaxElementForPositionImpl( line, column, *expression_ptr );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TypeName& type_name )
{
	return std::visit( [&]( const auto& el ) { return FindSyntaxElementForPositionImpl( line, column, el ); }, type_name );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NonSyncTag& non_sync_tag )
{
	return std::visit( [&]( const auto& el ) { return FindSyntaxElementForPositionImpl( line, column, el ); }, non_sync_tag );
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
	// TODO - check also "src_loc" of name itself.
	if( member_access_operator.src_loc_.GetLine() == line && member_access_operator.src_loc_.GetColumn() == column )
		return SyntaxTreeLookupResult{ {}, &member_access_operator };

	if( member_access_operator.template_parameters != std::nullopt )
	{
		// TODO
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
	return std::visit( [&]( const auto& el ) { return FindSyntaxElementForPositionImpl( line, column, el ); }, complex_name );
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
		return SyntaxTreeLookupResult{ {}, &root_namespace_name_lookup };
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NameLookup& name_lookup )
{
	if( line == name_lookup.src_loc_.GetLine() && column == name_lookup.src_loc_.GetColumn() )
		return SyntaxTreeLookupResult{ {}, &name_lookup };
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::NamesScopeNameFetch& names_scope_names_fetch )
{
	if( line == names_scope_names_fetch.src_loc_.GetLine() && column == names_scope_names_fetch.src_loc_.GetColumn() )
		return SyntaxTreeLookupResult{ {}, &names_scope_names_fetch };
	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TemplateParametrization& template_parametrization )
{
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
	return std::visit( [&]( const auto& el ) { return FindSyntaxElementForPositionImpl( line, column, el ); }, initializer );
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
		SyntaxTreeLookupResultOpt res= std::visit( [&]( const auto& el ) { return FindSyntaxElementForPositionImpl( line, column, el ); }, program_element );
		if( res != std::nullopt )
			return res;
	}

	return std::nullopt;
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::ClassElements& class_elements )
{
	for( const Synt::ClassElement& class_element : class_elements )
	{
		SyntaxTreeLookupResultOpt res= std::visit( [&]( const auto& el ) { return FindSyntaxElementForPositionImpl( line, column, el ); }, class_element );
		if( res != std::nullopt )
			return res;
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

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::TypeAlias& type_alias )
{
	return FindSyntaxElementForPositionImpl( line, column, type_alias.value );
}

SyntaxTreeLookupResultOpt FindSyntaxElementForPositionImpl( const uint32_t line, const uint32_t column, const Synt::Enum& enum_ )
{
	// TODO
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

	// TODO - perform lookup from function body.
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
	// TODO
	(void)line;
	(void)column;
	(void)type_template;
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
