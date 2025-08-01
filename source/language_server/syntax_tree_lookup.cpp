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
	SyntaxTreeLookupResultOpt Find( const uint32_t line, const uint32_t column, const Synt::ProgramElementsList& program_elements )
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
	FindImpl( call_operator.expression );

	for( const Synt::Expression& expression : call_operator.arguments )
		FindImpl( expression );
}

void FindImpl( const Synt::CallOperatorSignatureHelp& call_operator_signature_help )
{
	FindImpl( call_operator_signature_help.expression );

	if( call_operator_signature_help.src_loc.GetLine() == line_ && call_operator_signature_help.src_loc.GetColumn() == column_ )
	{
		U_ASSERT( global_item_ != std::nullopt );
		result_= SyntaxTreeLookupResult{ prefix_, &call_operator_signature_help, *global_item_ };
	}
}

void FindImpl( const Synt::IndexationOperator& indexation_operator )
{
	FindImpl( indexation_operator.expression );
	FindImpl( indexation_operator.index );
}

void FindImpl( const Synt::MemberAccessOperator& member_access_operator )
{
	FindImpl( member_access_operator.expression );
	if( member_access_operator.has_template_args )
	{
		for( const Synt::Expression& template_arg : member_access_operator.template_args )
			FindImpl( template_arg );
	}
}

void FindImpl( const Synt::MemberAccessOperatorCompletion& member_access_operator_completion )
{
	if( member_access_operator_completion.src_loc.GetLine() == line_ && member_access_operator_completion.src_loc.GetColumn() == column_ )
	{
		U_ASSERT( global_item_ != std::nullopt );
		result_= SyntaxTreeLookupResult{ prefix_, &member_access_operator_completion, *global_item_ };
	}
}

void FindImpl( const Synt::VariableInitialization& variable_initialization )
{
	FindImpl( variable_initialization.type );
	FindImpl( variable_initialization.initializer );
}

void FindImpl( const Synt::AwaitOperator& await_operator )
{
	FindImpl( await_operator.expression );
}

void FindImpl( const Synt::UnaryMinus& unary_minus )
{
	FindImpl( unary_minus.expression );
}

void FindImpl( const Synt::LogicalNot& ligical_not )
{
	FindImpl( ligical_not.expression );
}

void FindImpl( const Synt::BitwiseNot& bitwise_not )
{
	FindImpl( bitwise_not.expression );
}

void FindImpl( const Synt::ComplexName& complex_name )
{
	FindImplVariant( complex_name );
}

void FindImpl( const Synt::BinaryOperator& binary_operator )
{
	FindImpl( binary_operator.left );
	FindImpl( binary_operator.right );
}

void FindImpl( const Synt::TernaryOperator& ternary_operator )
{
	FindImpl( ternary_operator.condition );
	FindImpl( ternary_operator.branches[0] );
	FindImpl( ternary_operator.branches[1] );
}

void FindImpl( const Synt::ReferenceToRawPointerOperator& reference_to_raw_pointer_operator )
{
	FindImpl( reference_to_raw_pointer_operator.expression );
}

void FindImpl( const Synt::RawPointerToReferenceOperator& raw_pointer_to_reference_operator )
{
	FindImpl( raw_pointer_to_reference_operator.expression );
}

void FindImpl( const Synt::IntegerNumericConstant& numeric_constant )
{
	(void)numeric_constant;
}

void FindImpl( const Synt::FloatingPointNumericConstant& numeric_constant )
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

void FindImpl( const Synt::CharLiteral& char_literal )
{
	(void)char_literal;
}

void FindImpl( const Synt::MoveOperator& move_operator )
{
	(void)move_operator;
}

void FindImpl( const Synt::MoveOperatorCompletion& move_operator_completion )
{
	if( move_operator_completion.src_loc.GetLine() == line_ && move_operator_completion.src_loc.GetColumn() == column_ )
	{
		U_ASSERT( global_item_ != std::nullopt );
		result_= SyntaxTreeLookupResult{ prefix_, &move_operator_completion, *global_item_ };
	}
}

void FindImpl( const Synt::TakeOperator& take_operator )
{
	FindImpl( take_operator.expression );
}

void FindImpl( const Synt::Lambda& lambda )
{
	if( const auto capture_list= std::get_if<Synt::Lambda::CaptureList>( &lambda.capture ) )
	{
		for( const Synt::Lambda::CaptureListElement& capture : *capture_list )
		{
			FindImpl( capture.expression );

			if( capture.src_loc.GetLine() == line_ && capture.src_loc.GetColumn() == column_ )
			{
				U_ASSERT( global_item_ != std::nullopt );
				result_= SyntaxTreeLookupResult{ prefix_, &capture, *global_item_ };
			}
		}
	}

	FindImpl( lambda.function );
}

void FindImpl( const Synt::CastMut& cast_mut )
{
	FindImpl( cast_mut.expression );
}

void FindImpl( const Synt::CastImut& cast_imut )
{
	FindImpl( cast_imut.expression );
}

void FindImpl( const Synt::CastRef& cast_ref )
{
	FindImpl( cast_ref.type );
	FindImpl( cast_ref.expression );
}

void FindImpl( const Synt::CastRefUnsafe& cast_ref_unsafe )
{
	FindImpl( cast_ref_unsafe.type );
	FindImpl( cast_ref_unsafe.expression );
}

void FindImpl( const Synt::Embed& embed )
{
	if( embed.element_type != std::nullopt )
		FindImpl( *embed.element_type );

	FindImpl( embed.expression );
}

void FindImpl( const Synt::ExternalFunctionAccess& external_function_access )
{
	FindImpl( external_function_access.type );
}

void FindImpl( const Synt::ExternalVariableAccess& external_variable_access )
{
	FindImpl( external_variable_access.type );
}

void FindImpl( const Synt::TypeInfo& type_info )
{
	FindImpl( type_info.type );
}

void FindImpl( const Synt::SameType& same_type )
{
	FindImpl( same_type.l );
	FindImpl( same_type.r );
}

void FindImpl( const Synt::NonSyncExpression& non_sync_expression )
{
	FindImpl( non_sync_expression.type );
}

void FindImpl( const Synt::SafeExpression& safe_expression )
{
	FindImpl( safe_expression.expression );
}

void FindImpl( const Synt::UnsafeExpression& unsafe_expression )
{
	FindImpl( unsafe_expression.expression );
}

void FindImpl( const Synt::ArrayTypeName& array_type_name )
{
	FindImpl( array_type_name.element_type );
	FindImpl( array_type_name.size );
}

void FindImpl( const Synt::FunctionType& function_type )
{
	FindImpl( function_type.return_type );

	for( const Synt::FunctionParam& param : function_type.params )
		FindImpl( param.type );

	FindImpl( function_type.calling_convention );
	FindImpl( function_type.references_pollution_expression );
	FindImpl( function_type.return_value_reference_expression );
	FindImpl( function_type.return_value_inner_references_expression );
}

void FindImpl( const Synt::TupleType& tuple_type )
{
	for( const Synt::TypeName& element_type_name : tuple_type.element_types )
		FindImpl( element_type_name );
}

void FindImpl( const Synt::RawPointerType& raw_pointer_type )
{
	FindImpl( raw_pointer_type.element_type );
}

void FindImpl( const Synt::CoroutineType& coroutine_type )
{
	FindImpl( coroutine_type.return_type );
	FindImpl( coroutine_type.non_sync_tag );
}

void FindImpl( const Synt::TypeofTypeName& typeof_type_name )
{
	FindImpl( typeof_type_name.expression );
}

void FindImpl( const Synt::RootNamespaceNameLookup& root_namespace_name_lookup )
{
	(void)root_namespace_name_lookup;
}

void FindImpl( const Synt::RootNamespaceNameLookupCompletion& root_namespace_name_lookup_completion )
{
	if( line_ == root_namespace_name_lookup_completion.src_loc.GetLine() && column_ == root_namespace_name_lookup_completion.src_loc.GetColumn() )
	{
		U_ASSERT( global_item_ != std::nullopt );
		result_= SyntaxTreeLookupResult{ prefix_, &root_namespace_name_lookup_completion, *global_item_ };
	}
}

void FindImpl( const Synt::NameLookup& name_lookup )
{
	(void)name_lookup;
}

void FindImpl( const Synt::NameLookupCompletion& name_lookup_completion )
{
	if( line_ == name_lookup_completion.src_loc.GetLine() && column_ == name_lookup_completion.src_loc.GetColumn() )
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

	if( line_ == names_scope_name_fetch_completion.src_loc.GetLine() && column_ == names_scope_name_fetch_completion.src_loc.GetColumn() )
	{
		U_ASSERT( global_item_ != std::nullopt );
		result_= SyntaxTreeLookupResult{ prefix_, &names_scope_name_fetch_completion, *global_item_ };
	}
}

void FindImpl( const Synt::TemplateParameterization& template_parameterization )
{
	FindImpl( template_parameterization.base );

	for( const Synt::Expression& arg : template_parameterization.template_args )
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

void FindImpl( const Synt::ConstructorInitializerSignatureHelp& constructor_initializer_signature_help )
{
	if( constructor_initializer_signature_help.src_loc.GetLine() == line_ && constructor_initializer_signature_help.src_loc.GetColumn() == column_ )
	{
		U_ASSERT( global_item_ != std::nullopt );
		result_= SyntaxTreeLookupResult{ prefix_, &constructor_initializer_signature_help, *global_item_ };
	}
}

void FindImpl( const Synt::ZeroInitializer& zero_initializer )
{
	(void)zero_initializer;
}

void FindImpl( const Synt::UninitializedInitializer& uninitialized_initializer )
{
	(void)uninitialized_initializer;
}

void FindImpl( const Synt::ProgramElementsList& program_elements )
{
	std::optional<GlobalItem> prev_global_item= global_item_;
	program_elements.Iter(
		[&]( const auto & el )
		{
			global_item_= GlobalItem(&el);
			FindImpl( el );
		} );
	global_item_= prev_global_item;
}

void FindImpl( const Synt::ClassElementsList& class_elements )
{
	std::optional<GlobalItem> prev_global_item= global_item_;
	class_elements.Iter(
		[&]( const auto & el )
		{
			global_item_= GlobalItem(&el);
			FindImpl( el );
		} );
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

void FindImpl( const Synt::DecomposeDeclaration& decompose_declaration )
{
	FindImpl( decompose_declaration.root_component );
	FindImpl( decompose_declaration.initializer_expression );
}

void FindImpl( const Synt::DecomposeDeclarationComponent& component )
{
	FindImplVariant( component );
}

void FindImpl( const Synt::DecomposeDeclarationNamedComponent& component )
{
	U_UNUSED( component );
}

void FindImpl( const Synt::DecomposeDeclarationSequenceComponent& component )
{
	for( const auto& c : component.sub_components )
		FindImpl( c );
}

void FindImpl( const Synt::DecomposeDeclarationStructComponent& component )
{
	for( const auto& entry : component.entries )
	{
		FindImpl( entry.component );

		if( entry.src_loc.GetLine() == line_ && entry.src_loc.GetColumn() == column_ )
		{
			U_ASSERT( global_item_ != std::nullopt );
			result_= SyntaxTreeLookupResult{ prefix_, &entry, *global_item_ };
		}
	}
}

void FindImpl( const Synt::AllocaDeclaration& alloca_declaration )
{
	FindImpl( alloca_declaration.type );
	FindImpl( alloca_declaration.size );
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
	for( const Synt::Function::NameComponent& name_component : function.name )
	{
		if( line_ == name_component.src_loc.GetLine() && column_ == name_component.src_loc.GetColumn() )
		{
			U_ASSERT( global_item_ != std::nullopt );
			result_= SyntaxTreeLookupResult{ prefix_, &name_component, *global_item_ };
		}
	}

	FindImpl( function.condition );
	FindImpl( function.type );
	FindImpl( function.constructor_initialization_list );
	FindImpl( function.block );
}

void FindImpl( const Synt::Class& class_ )
{
	prefix_.push_back( &class_ );
	FindImpl( class_.elements );
	prefix_.pop_back();

	for( const Synt::ComplexName& parent : class_.parents )
		FindImpl( parent );

	FindImpl( class_.non_sync_tag );
}

void FindImpl( const Synt::TypeTemplate& type_template )
{
	prefix_.push_back( &type_template );

	std::optional<GlobalItem> prev_global_item= global_item_;
	std::visit(
		[&]( const auto& el )
		{
			global_item_= GlobalItem( el.get() );
			FindImpl( el );
		},
		type_template.something );

	global_item_= prev_global_item;

	prefix_.pop_back();

	// TODO - process template arguments and signature arguments.
}

void FindImpl( const Synt::TemplateParam::TypeParamData& ){}

void FindImpl( const Synt::TemplateParam::TypeTemplateParamData& ){}

void FindImpl( const Synt::TemplateParam::VariableParamData& variable_param_data )
{
	FindImpl( variable_param_data.type );
}

void FindImpl( const Synt::FunctionTemplate& function_template )
{
	for( const auto& param : function_template.params )
		FindImplVariant( param.kind_data );

	FindImpl( function_template.function );
}

void FindImpl( const Synt::Namespace& namespace_ )
{
	prefix_.push_back( &namespace_ );
	FindImpl( namespace_.elements );
	prefix_.pop_back();
}

void FindImpl( const Synt::Mixin& mixin )
{
	FindImpl( mixin.expression );
}

void FindImpl( const Synt::ClassField& class_field )
{
	FindImpl( class_field.type );
	FindImpl( class_field.reference_tag_expression );
	FindImpl( class_field.inner_reference_tags_expression );
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
	block.elements.Iter( [&]( const auto& el ) { FindImpl(el); } );
}

void FindImpl( const Synt::ScopeBlock& scope_block )
{
	FindImpl( scope_block.block );
}

void FindImpl( const Synt::ReturnOperator& return_operator )
{
	FindImpl( return_operator.expression );
}

void FindImpl( const Synt::YieldOperator& yield_operator )
{
	FindImpl( yield_operator.expression );
}

void FindImpl( const Synt::WhileOperator& while_operator )
{
	FindImpl( while_operator.condition );
	FindImpl( while_operator.block );
}

void FindImpl( const Synt::LoopOperator& loop_operator )
{
	FindImpl( loop_operator.block );
}

void FindImpl( const Synt::RangeForOperator& range_for_operator )
{
	FindImpl( range_for_operator.sequence );
	FindImpl( range_for_operator.block );
}

void FindImpl( const Synt::CStyleForOperator& c_style_for_operator )
{
	FindImplVariant( c_style_for_operator.variable_declaration_part );

	FindImpl( c_style_for_operator.loop_condition );

	c_style_for_operator.iteration_part_elements.Iter( [&]( const auto& el ){ FindImpl( el ); } );

	FindImpl( c_style_for_operator.block );
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
	FindImpl( with_operator.expression );
	FindImpl( with_operator.block );
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
	FindImpl( single_expression_operator.expression );
}

void FindImpl( const Synt::AssignmentOperator& assignment_operator )
{
	FindImpl( assignment_operator.l_value );
	FindImpl( assignment_operator.r_value );
}

void FindImpl( const Synt::CompoundAssignmentOperator& compound_assignment_operator )
{
	FindImpl( compound_assignment_operator.l_value );
	FindImpl( compound_assignment_operator.r_value );
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

SyntaxTreeLookupResultOpt FindCompletionSyntaxElement( const uint32_t line, const uint32_t column, const Synt::ProgramElementsList& program_elements )
{
	return Finder().Find( line, column, program_elements );
}

} // namespace LangServer

} // namespace U
