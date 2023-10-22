#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

void CodeBuilder::TryGenerateFunctionReturnReferencesMapping(
	const Synt::FunctionType& func,
	FunctionType& function_type )
{
	if( func.return_value_reference_expression != nullptr || func.return_value_inner_references_expression != nullptr )
		return;

	// Generate mapping of input references to output references, if reference tags are not specified explicitly.

	if( function_type.return_value_type != ValueType::Value && function_type.return_references.empty() )
	{
		// If there is no tag for return reference, assume, that it may refer to any reference argument, but not inner reference of any argument.
		for( size_t i= 0u; i < function_type.params.size(); ++i )
		{
			if( function_type.params[i].value_type != ValueType::Value )
				function_type.return_references.emplace( i, FunctionType::c_arg_reference_tag_number );
		}
	}
}

void CodeBuilder::ProcessFunctionReferencesPollution(
	CodeBuilderErrorsContainer& errors_container,
	const Synt::Function& func,
	FunctionType& function_type,
	const ClassPtr base_class )
{
	const std::string& func_name= func.name.back().name;

	if( func_name == Keywords::constructor_ )
	{
		for( const FunctionType::ReferencePollution& pollution : function_type.references_pollution )
		{
			if( pollution.src.first == 0 && pollution.src.second == FunctionType::c_arg_reference_tag_number )
				REPORT_ERROR( ConstructorThisReferencePollution, errors_container, func.src_loc );
		}
	}

	// TODO - fix this.
	// We need to know exact number of inner reference tags in order to generate proper pollution for copy constructor and copy assignment operators.
	if( func_name == Keywords::constructor_ && IsCopyConstructor( function_type, base_class ) )
	{
		if( func.type.references_pollution_expression != nullptr )
			REPORT_ERROR( ExplicitReferencePollutionForCopyConstructor, errors_container, func.src_loc );

		// This is copy constructor. Generate reference pollution for it automatically.
		FunctionType::ReferencePollution ref_pollution;
		ref_pollution.dst.first= 0u;
		ref_pollution.dst.second= 0u;
		ref_pollution.src.first= 1u;
		ref_pollution.src.second= 0u;
		function_type.references_pollution.insert(ref_pollution);
	}
	else if( func_name == OverloadedOperatorToString( OverloadedOperator::Assign ) && IsCopyAssignmentOperator( function_type, base_class ) )
	{
		if( func.type.references_pollution_expression != nullptr )
			REPORT_ERROR( ExplicitReferencePollutionForCopyAssignmentOperator, errors_container, func.src_loc );

		// This is copy assignment operator. Generate reference pollution for it automatically.
		FunctionType::ReferencePollution ref_pollution;
		ref_pollution.dst.first= 0u;
		ref_pollution.dst.second= 0u;
		ref_pollution.src.first= 1u;
		ref_pollution.src.second= 0u;
		function_type.references_pollution.insert(ref_pollution);
	}
	else if( func_name == OverloadedOperatorToString( OverloadedOperator::CompareEqual ) && IsEqualityCompareOperator( function_type, base_class ) )
	{
		if( func.type.references_pollution_expression != nullptr )
			REPORT_ERROR( ExplicitReferencePollutionForEqualityCompareOperator, errors_container, func.src_loc );
	}
}

void CodeBuilder::SetupReferencesInCopyOrMove( FunctionContext& function_context, const VariablePtr& dst_variable, const VariablePtr& src_variable, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	if( dst_variable->type.ReferencesTagsCount() == 0u )
		return;

	const size_t reference_tag_count= dst_variable->type.ReferencesTagsCount();
	U_ASSERT( src_variable->inner_reference_nodes.size() >= reference_tag_count );
	U_ASSERT( dst_variable->inner_reference_nodes.size() >= reference_tag_count );
	for( size_t i= 0; i < reference_tag_count; ++i )
		function_context.variables_state.TryAddLinkToAllAccessibleVariableNodesInnerReferences(
			src_variable->inner_reference_nodes[i],
			dst_variable->inner_reference_nodes[i],
			errors_container,
			src_loc );
}

void CodeBuilder::RegisterTemporaryVariable( FunctionContext& function_context, VariablePtr variable )
{
	function_context.stack_variables_stack.back()->RegisterVariable( std::move(variable) );
}

void CodeBuilder::DestroyUnusedTemporaryVariables( FunctionContext& function_context, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	StackVariablesStorage& temporary_variables_storage= *function_context.stack_variables_stack.back();
	// Try to move unused nodes (variables and references) until we can't move anything.
	// Multiple iterations needed to process complex references chains.
	while(true)
	{
		bool any_node_moved= false;
		for( const VariablePtr& variable : temporary_variables_storage.variables_ )
		{
			// Destroy variables without links.
			// Destroy all references, because all actual references that holds values should not yet be registered.
			if( !function_context.variables_state.NodeMoved( variable ) &&
				( variable->value_type != ValueType::Value ||
					!function_context.variables_state.HaveOutgoingLinks( variable ) ) )
			{
				if( variable->value_type == ValueType::Value && !function_context.is_functionless_context )
				{
					if( variable->type.HaveDestructor() )
						CallDestructor( variable->llvm_value, variable->type, function_context, errors_container, src_loc );
					if( variable->location == Variable::Location::Pointer )
						CreateLifetimeEnd( function_context, variable->llvm_value );
				}
				function_context.variables_state.MoveNode( variable );
				any_node_moved= true;
			}
		}

		if(!any_node_moved)
			return;
	}
}

ReferencesGraph CodeBuilder::MergeVariablesStateAfterIf(
	const llvm::ArrayRef<ReferencesGraph> bracnhes_variables_state,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	ReferencesGraph::MergeResult res= ReferencesGraph::MergeVariablesStateAfterIf( bracnhes_variables_state, src_loc );
	errors_container.insert( errors_container.end(), res.second.begin(), res.second.end() );
	return std::move(res.first);
}

void CodeBuilder::CheckReturnedReferenceIsAllowed( NamesScope& names, FunctionContext& function_context, const VariablePtr& return_reference_node, const SrcLoc& src_loc )
{
	for( const VariablePtr& var_node : function_context.variables_state.GetAllAccessibleVariableNodes( return_reference_node ) )
		if( !IsReferenceAllowedForReturn( function_context, var_node ) )
			REPORT_ERROR( ReturningUnallowedReference, names.GetErrors(), src_loc );
}

bool CodeBuilder::IsReferenceAllowedForReturn( FunctionContext& function_context, const VariablePtr& variable_node )
{
	U_ASSERT( variable_node != nullptr );
	U_ASSERT( variable_node->value_type == ValueType::Value );

	for( const FunctionType::ParamReference& param_and_tag : function_context.function_type.return_references )
	{
		const size_t arg_n= param_and_tag.first;
		U_ASSERT( arg_n < function_context.args_nodes.size() );
		if( param_and_tag.second == FunctionType::c_arg_reference_tag_number && variable_node == function_context.args_nodes[arg_n].first )
			return true;
		else if( param_and_tag.second < function_context.args_nodes[arg_n].second.size() && variable_node == function_context.args_nodes[arg_n].second[ param_and_tag.second ] )
			return true;
	}

	return false;
}

void CodeBuilder::CheckReturnedInnerReferenceIsAllowed( NamesScope& names, FunctionContext& function_context, const VariablePtr& return_reference_node, const SrcLoc& src_loc )
{
	for( size_t i= 0; i < return_reference_node->inner_reference_nodes.size(); ++i )
		for( const VariablePtr& var_node : function_context.variables_state.GetAllAccessibleVariableNodes( return_reference_node->inner_reference_nodes[i] ) )
			if( !IsReferenceAllowedForInnerReturn( function_context, var_node, i ) )
				REPORT_ERROR( ReturningUnallowedReference, names.GetErrors(), src_loc );
}

bool CodeBuilder::IsReferenceAllowedForInnerReturn( FunctionContext& function_context, const VariablePtr& variable_node, const size_t index )
{
	U_ASSERT( variable_node != nullptr );
	U_ASSERT( variable_node->value_type == ValueType::Value );

	if( index >= function_context.function_type.return_inner_references.size() )
		return false;

	for( const FunctionType::ParamReference& param_and_tag : function_context.function_type.return_inner_references[index] )
	{
		const size_t arg_n= param_and_tag.first;
		U_ASSERT( arg_n < function_context.args_nodes.size() );
		if( param_and_tag.second == FunctionType::c_arg_reference_tag_number && variable_node == function_context.args_nodes[arg_n].first )
			return true;
		if( param_and_tag.second < function_context.args_nodes[arg_n].second.size() && variable_node == function_context.args_nodes[arg_n].second[param_and_tag.second] )
			return true;
	}

	return false;
}

void CodeBuilder::CheckReferencesPollutionBeforeReturn(
	FunctionContext& function_context,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	for( size_t i= 0u; i < function_context.function_type.params.size(); ++i )
	{
		if( function_context.function_type.params[i].value_type == ValueType::Value )
			continue;

		const auto& node_pair= function_context.args_nodes[i];

		for( size_t j= 0; j < node_pair.first->inner_reference_nodes.size(); ++j )
		{
			const VariablePtr& inner_reference= node_pair.first->inner_reference_nodes[j];
			for( const VariablePtr& accesible_variable : function_context.variables_state.GetAllAccessibleVariableNodes( inner_reference ) )
			{
				if( j < node_pair.second.size() && accesible_variable == node_pair.second[j] )
					continue;

				std::optional<FunctionType::ParamReference> reference;
				for( size_t j= 0u; j < function_context.function_type.params.size(); ++j )
				{
					if( accesible_variable == function_context.args_nodes[j].first )
						reference= FunctionType::ParamReference( uint8_t(j), FunctionType::c_arg_reference_tag_number );

					for( size_t k= 0; k < function_context.args_nodes[j].second.size(); ++k )
					if( accesible_variable == function_context.args_nodes[j].second[k] )
						reference= FunctionType::ParamReference( uint8_t(j), uint8_t(k) );
				}

				if( reference != std::nullopt )
				{
					FunctionType::ReferencePollution pollution;
					pollution.src= *reference;
					pollution.dst.first= uint8_t(i);
					pollution.dst.second= uint8_t(j);
					if( function_context.function_type.references_pollution.count( pollution ) != 0u )
						continue;
				}
				REPORT_ERROR( UnallowedReferencePollution, errors_container, src_loc );
			}
		}
	}
}

} // namespace U
