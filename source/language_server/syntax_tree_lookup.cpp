#include <algorithm>
#include "../lex_synt_lib_common/assert.hpp"
#include "syntax_tree_lookup.hpp"

namespace U
{

namespace LangServer
{

namespace
{

// Use class in order to avoid writing prototypes.
// Also simplify passing line/column by making these params class fields.
// Also simplfiy prefix/global item setup and result tracking by making these stuff fields.
class Finder
{
public:
	SyntaxTreeLookupResultOpt Find( const uint32_t line, const uint32_t column, const Synt::ProgramElements& program_elements )
	{
		line_= line;
		column_= column;
		global_item_= std::nullopt;
		result_= std::nullopt;

		FindImpl( program_elements );
		return result_;
	}

private:
	uint32_t line_= 0;
	uint32_t column_= 0;
	std::vector<CodeBuilder::CompletionRequestPrefixComponent> prefix_;
	std::optional<GlobalItem> global_item_;
	std::optional<SyntaxTreeLookupResult> result_;

private:

template<typename ... Args>
void FindImplVariant( const std::variant<Args...>& v )
{
	std::visit( [&]( const auto& el ) { FindImpl( el ); }, v );
}

template<typename T>
void FindImpl( const std::unique_ptr<T>& el )
{
	if( el == nullptr )
		return;

	FindImpl( *el );
}

void FindImpl( const Synt::Expression& expression )
{
	FindImplVariant( expression );
}

void FindImpl( const Synt::TypeName& type_name )
{
	FindImplVariant( type_name );
}

void FindImpl( const Synt::NonSyncTag& non_sync_tag )
{
	FindImplVariant( non_sync_tag );
}

void FindImpl( const Synt::EmptyVariant& empty_variant )
{
	(void)empty_variant;
}

void FindImpl( const Synt::CallOperator& call_operator )
{
	FindImpl( call_operator.expression_ );

	for( const Synt::Expression& expression : call_operator.arguments_ )
		FindImpl( expression );
}

void FindImpl( const Synt::IndexationOperator& indexation_operator )
{
	FindImpl( indexation_operator.expression_ );
	FindImpl( indexation_operator.index_ );
}

void FindImpl( const Synt::MemberAccessOperator& member_access_operator )
{
	FindImpl( member_access_operator.expression_ );
	if( member_access_operator.template_parameters != std::nullopt )
	{
		for( const Synt::Expression& template_arg : *member_access_operator.template_parameters )
			FindImpl( template_arg );
	}
}

void FindImpl( const Synt::MemberAccessOperatorCompletion& member_access_operator_completion )
{
	if( member_access_operator_completion.src_loc_.GetLine() == line_ && member_access_operator_completion.src_loc_.GetColumn() == column_ )
	{
		U_ASSERT( global_item_ != std::nullopt );
		result_= SyntaxTreeLookupResult{ prefix_, &member_access_operator_completion, *global_item_ };
	}
}

void FindImpl( const Synt::UnaryPlus& unary_plus )
{
	FindImpl( unary_plus.expression_ );
}

void FindImpl( const Synt::UnaryMinus& unary_minus )
{
	FindImpl( unary_minus.expression_ );
}

void FindImpl( const Synt::LogicalNot& ligical_not )
{
	FindImpl( ligical_not.expression_ );
}

void FindImpl( const Synt::BitwiseNot& bitwise_not )
{
	FindImpl( bitwise_not.expression_ );
}

void FindImpl( const Synt::ComplexName& complex_name )
{
	FindImplVariant( complex_name );
}

void FindImpl( const Synt::BinaryOperator& binary_operator )
{
	FindImpl( binary_operator.left_ );
	FindImpl( binary_operator.right_ );
}

void FindImpl( const Synt::TernaryOperator& ternary_operator )
{
	FindImpl( ternary_operator.condition );
	FindImpl( ternary_operator.false_branch );
	FindImpl( ternary_operator.true_branch );
}

void FindImpl( const Synt::ReferenceToRawPointerOperator& reference_to_raw_pointer_operator )
{
	FindImpl( reference_to_raw_pointer_operator.expression );
}

void FindImpl( const Synt::RawPointerToReferenceOperator& raw_pointer_to_reference_operator )
{
	FindImpl( raw_pointer_to_reference_operator.expression );
}

void FindImpl( const Synt::NumericConstant& numeric_constant )
{
	(void)numeric_constant;
}

void FindImpl( const Synt::BooleanConstant& boolean_constant )
{
	(void)boolean_constant;
}

void FindImpl( const Synt::StringLiteral& string_literal )
{
	(void)string_literal;
}

void FindImpl( const Synt::MoveOperator& move_operator )
{
	// TODO - process it specially
	(void)move_operator;
}

void FindImpl( const Synt::TakeOperator& take_operator )
{
	FindImpl( take_operator.expression_ );
}

void FindImpl( const Synt::CastMut& cast_mut )
{
	FindImpl( cast_mut.expression_ );
}

void FindImpl( const Synt::CastImut& cast_imut )
{
	FindImpl( cast_imut.expression_ );
}

void FindImpl( const Synt::CastRef& cast_ref )
{
	FindImpl( cast_ref.type_ );
	FindImpl( cast_ref.expression_ );
}

void FindImpl( const Synt::CastRefUnsafe& cast_ref_unsafe )
{
	FindImpl( cast_ref_unsafe.type_ );
	FindImpl( cast_ref_unsafe.expression_ );
}

void FindImpl( const Synt::TypeInfo& type_info )
{
	FindImpl( type_info.type_ );
}

void FindImpl( const Synt::NonSyncExpression& non_sync_expression )
{
	FindImpl( non_sync_expression.type_ );
}

void FindImpl( const Synt::SafeExpression& safe_expression )
{
	FindImpl( safe_expression.expression_ );
}

void FindImpl( const Synt::UnsafeExpression& unsafe_expression )
{
	FindImpl( unsafe_expression.expression_ );
}

void FindImpl( const Synt::ArrayTypeName& array_type_name )
{
	FindImpl( array_type_name.element_type );
	FindImpl( array_type_name.size );
}

void FindImpl( const Synt::FunctionType& function_type )
{
	FindImpl( function_type.return_type_ );

	for( const Synt::FunctionParam& param : function_type.params_ )
		FindImpl( param.type_ );
}

void FindImpl( const Synt::TupleType& tuple_type )
{
	for( const Synt::TypeName& element_type_name : tuple_type.element_types_ )
		FindImpl( element_type_name );
}

void FindImpl( const Synt::RawPointerType& raw_pointer_type )
{
	FindImpl( raw_pointer_type.element_type );
}

void FindImpl( const Synt::GeneratorType& generator_type )
{
	FindImpl( generator_type.return_type );
	FindImpl( generator_type.non_sync_tag );
}

void FindImpl( const Synt::TypeofTypeName& typeof_type_name )
{
	FindImpl( typeof_type_name.expression );
}

void FindImpl( const Synt::RootNamespaceNameLookup& root_namespace_name_lookup )
{
	(void)root_namespace_name_lookup;
}

void FindImpl( const Synt::NameLookup& name_lookup )
{
	(void)name_lookup;
}

void FindImpl( const Synt::NameLookupCompletion& name_lookup_completion )
{
	if( line_ == name_lookup_completion.src_loc_.GetLine() && column_ == name_lookup_completion.src_loc_.GetColumn() )
	{
		U_ASSERT( global_item_ != std::nullopt );
		result_= SyntaxTreeLookupResult{ prefix_, &name_lookup_completion, *global_item_ };
	}
}

void FindImpl( const Synt::NamesScopeNameFetch& names_scope_name_fetch )
{
	FindImpl( names_scope_name_fetch.base );
}

void FindImpl( const Synt::NamesScopeNameFetchCompletion& names_scope_name_fetch_completion )
{
	FindImpl( names_scope_name_fetch_completion.base );

	if( line_ == names_scope_name_fetch_completion.src_loc_.GetLine() && column_ == names_scope_name_fetch_completion.src_loc_.GetColumn() )
	{
		U_ASSERT( global_item_ != std::nullopt );
		result_= SyntaxTreeLookupResult{ prefix_, &names_scope_name_fetch_completion, *global_item_ };
	}
}

void FindImpl( const Synt::TemplateParametrization& template_parametrization )
{
	FindImpl( template_parametrization.base );

	for( const Synt::Expression& arg : template_parametrization.template_args )
		FindImpl( arg );
}

void FindImpl( const Synt::Initializer& initializer )
{
	FindImplVariant( initializer );
}

void FindImpl( const Synt::SequenceInitializer& sequence_initializer )
{
	for( const Synt::Initializer& initializer : sequence_initializer.initializers )
		FindImpl( initializer );
}

void FindImpl( const Synt::StructNamedInitializer& struct_named_initializer )
{
	for( const Synt::StructNamedInitializer::MemberInitializer& member_initializer : struct_named_initializer.members_initializers )
	{
		if( line_ == member_initializer.src_loc.GetLine() && column_ == member_initializer.src_loc.GetColumn() )
		{
			U_ASSERT( global_item_ != std::nullopt );
			result_= SyntaxTreeLookupResult{ prefix_, &member_initializer, *global_item_ };
		}

		FindImpl( member_initializer.initializer );
	}
}

void FindImpl( const Synt::ConstructorInitializer& constructor_initializer )
{
	for( const Synt::Expression& expression : constructor_initializer.arguments )
		FindImpl( expression );
}

void FindImpl( const Synt::ZeroInitializer& zero_initializer )
{
	(void)zero_initializer;
}

void FindImpl( const Synt::UninitializedInitializer& uninitialized_initializer )
{
	(void)uninitialized_initializer;
}

void FindImpl( const Synt::ProgramElements& program_elements )
{
	std::optional<GlobalItem> prev_global_item= global_item_;
	for( const Synt::ProgramElement& program_element : program_elements )
	{
		global_item_= GlobalItem(&program_element);
		FindImplVariant( program_element );
	}
	global_item_= prev_global_item;
}

void FindImpl( const Synt::ClassElements& class_elements )
{
	std::optional<GlobalItem> prev_global_item= global_item_;
	for( const Synt::ClassElement& class_element : class_elements )
	{
		global_item_= GlobalItem(&class_element);
		FindImplVariant( class_element );
	}
	global_item_= prev_global_item;
}

void FindImpl( const Synt::VariablesDeclaration& variables_declaration )
{
	FindImpl( variables_declaration.type );

	for( const Synt::VariablesDeclaration::VariableEntry& entry : variables_declaration.variables )
		FindImpl( entry.initializer );
}

void FindImpl( const Synt::AutoVariableDeclaration& auto_variable_declaration )
{
	FindImpl( auto_variable_declaration.initializer_expression );
}

void FindImpl( const Synt::StaticAssert& static_assert_ )
{
	FindImpl( static_assert_.expression );
}

void FindImpl( const Synt::TypeAlias& type_alias )
{
	FindImpl( type_alias.value );
}

void FindImpl( const Synt::Enum& enum_ )
{
	// Nothing to do here.
	(void)enum_;
}

void FindImpl( const Synt::Function& function )
{
	FindImpl( function.type_ );
	FindImpl( function.constructor_initialization_list_ );
	FindImpl( function.coroutine_non_sync_tag );
	FindImpl( function.block_ );
}

void FindImpl( const Synt::Class& class_ )
{
	prefix_.push_back( &class_ );
	FindImpl( class_.elements_ );
	prefix_.pop_back();

	for( const Synt::ComplexName& parent : class_.parents_ )
		FindImpl( parent );

	FindImpl( class_.non_sync_tag_ );
}

void FindImpl( const Synt::TypeTemplate& type_template )
{
	prefix_.push_back( &type_template );
	FindImplVariant( type_template.something_ );
	prefix_.pop_back();

	// TODO - process template arguments and signature arguments.
}

void FindImpl( const Synt::FunctionTemplate& function_template )
{
	// TODO
	(void)function_template;
}

void FindImpl( const Synt::Namespace& namespace_ )
{
	prefix_.push_back( &namespace_ );
	FindImpl( namespace_.elements_ );
	prefix_.pop_back();
}

void FindImpl( const Synt::ClassField& class_field )
{
	FindImpl( class_field.type );
}

void FindImpl( const Synt::ClassVisibilityLabel& visibiliy_label )
{
	(void)visibiliy_label;
}

void FindImpl( const Synt::NonSyncTagNone& non_sync_tag_none )
{
	(void)non_sync_tag_none;
}

void FindImpl( const Synt::NonSyncTagTrue& non_sync_tag_true )
{
	(void)non_sync_tag_true;
}

void FindImpl( const Synt::IfAlternative& if_alternative )
{
	FindImplVariant( if_alternative );
}

void FindImpl( const Synt::Block& block )
{
	for( const Synt::BlockElement& block_element : block.elements_ )
		FindImplVariant( block_element );
}

void FindImpl( const Synt::ScopeBlock& scope_block )
{
	FindImpl( static_cast<const Synt::Block&>(scope_block) );
}

void FindImpl( const Synt::ReturnOperator& return_operator )
{
	FindImpl( return_operator.expression_ );
}

void FindImpl( const Synt::YieldOperator& yield_operator )
{
	FindImpl( yield_operator.expression );
}

void FindImpl( const Synt::WhileOperator& while_operator )
{
	FindImpl( while_operator.condition_ );
	FindImpl( while_operator.block_ );
}

void FindImpl( const Synt::LoopOperator& loop_operator )
{
	FindImpl( loop_operator.block_ );
}

void FindImpl( const Synt::RangeForOperator& range_for_operator )
{
	FindImpl( range_for_operator.sequence_ );
	FindImpl( range_for_operator.block_ );
}

void FindImpl( const Synt::CStyleForOperator& c_style_for_operator )
{
	if( c_style_for_operator.variable_declaration_part_ != nullptr )
		FindImplVariant( *c_style_for_operator.variable_declaration_part_ );

	FindImpl( c_style_for_operator.loop_condition_ );

	for( const auto& element : c_style_for_operator.iteration_part_elements_ )
		FindImplVariant( element );

	FindImpl( c_style_for_operator.block_ );
}

void FindImpl( const Synt::BreakOperator& break_operator )
{
	(void)break_operator;
}

void FindImpl( const Synt::ContinueOperator& continue_operator )
{
	(void)continue_operator;
}

void FindImpl( const Synt::WithOperator& with_operator )
{
	FindImpl( with_operator.expression_ );
	FindImpl( with_operator.block_ );
}

void FindImpl( const Synt::IfOperator& if_operator )
{
	FindImpl( if_operator.condition );
	FindImpl( if_operator.alternative );
	FindImpl( if_operator.block );
}

void FindImpl( const Synt::StaticIfOperator& static_if_operator )
{
	FindImpl( static_if_operator.condition );
	FindImpl( static_if_operator.alternative );
	FindImpl( static_if_operator.block );
}

void FindImpl( const Synt::IfCoroAdvanceOperator& if_coro_advance_operator )
{
 FindImpl( if_coro_advance_operator.expression );
	FindImpl( if_coro_advance_operator.alternative );
	FindImpl( if_coro_advance_operator.block );
}

void FindImpl( const Synt::SwitchOperator& switch_operator )
{
	FindImpl( switch_operator.value );

	for( const Synt::SwitchOperator::Case& case_ : switch_operator.cases )
	{
		if( const auto case_values= std::get_if< std::vector<Synt::SwitchOperator::CaseValue> >( &case_.values ) )
		{
			for( const auto& case_value : *case_values )
			{
				if( const auto single_value= std::get_if< Synt::Expression >( &case_value ) )
					FindImpl( *single_value );
				else if( const auto range= std::get_if< Synt::SwitchOperator::CaseRange >( &case_value ) )
				{
					FindImpl( range->low );
					FindImpl( range->high );
				}
				else U_ASSERT(false);
			}
		}
		else if( std::get_if< Synt::SwitchOperator::DefaultPlaceholder >( &case_.values ) != nullptr )
		{}
		else U_ASSERT(false);

		FindImpl( case_.block );
	}
}

void FindImpl( const Synt::SingleExpressionOperator& single_expression_operator )
{
	FindImpl( single_expression_operator.expression_ );
}

void FindImpl( const Synt::AssignmentOperator& assignment_operator )
{
	FindImpl( assignment_operator.l_value_ );
	FindImpl( assignment_operator.r_value_ );
}

void FindImpl( const Synt::AdditiveAssignmentOperator& additive_assignment_operator )
{
	FindImpl( additive_assignment_operator.l_value_ );
	FindImpl( additive_assignment_operator.r_value_ );
}

void FindImpl( const Synt::IncrementOperator& increment_operator )
{
	FindImpl( increment_operator.expression );
}

void FindImpl( const Synt::DecrementOperator& decrement_operator )
{
	FindImpl( decrement_operator.expression );
}

void FindImpl( const Synt::Halt& halt )
{
	(void)halt;
}

void FindImpl( const Synt::HaltIf& halt_if )
{
	FindImpl( halt_if.condition );
}

}; // Class Finder

} // namespace

SyntaxTreeLookupResultOpt FindCompletionSyntaxElement( const uint32_t line, const uint32_t column, const Synt::ProgramElements& program_elements )
{
	return Finder().Find( line, column, program_elements );
}

} // namespace LangServer

} // namespace U
