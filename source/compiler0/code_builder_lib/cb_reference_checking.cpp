#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

void CodeBuilder::ProcessFunctionParamReferencesTags(
	const Synt::FunctionType& func,
	FunctionType& function_type,
	const Synt::FunctionParam& in_param,
	const FunctionType::Param& out_param,
	const size_t arg_number )
{

	if( function_type.return_value_type != ValueType::Value && !func.return_value_reference_tag_.empty() )
	{
		// Arg reference to return reference
		if( out_param.value_type != ValueType::Value && !in_param.reference_tag_.empty() && in_param.reference_tag_ == func.return_value_reference_tag_ )
			function_type.return_references.emplace( uint8_t(arg_number), FunctionType::c_arg_reference_tag_number );

		// Inner arg references to return reference
		if( in_param.inner_arg_reference_tag_ == func.return_value_reference_tag_ )
			function_type.return_references.emplace( uint8_t(arg_number), 0u );
	}

	if( function_type.return_value_type == ValueType::Value && !func.return_value_inner_reference_tag_.empty() )
	{
		// In arg reference to return value references
		if( out_param.value_type != ValueType::Value && !in_param.reference_tag_.empty() && in_param.reference_tag_ == func.return_value_inner_reference_tag_ )
			function_type.return_references.emplace( uint8_t(arg_number), FunctionType::c_arg_reference_tag_number );

		// Inner arg references to return value references
		if( in_param.inner_arg_reference_tag_ == func.return_value_inner_reference_tag_ )
			function_type.return_references.emplace( uint8_t(arg_number), 0u );
	}
}

void CodeBuilder::ProcessFunctionReturnValueReferenceTags(
	CodeBuilderErrorsContainer& errors_container,
	const Synt::FunctionType& func,
	const FunctionType& function_type )
{
	if( function_type.return_value_type == ValueType::Value )
	{
		// Check names of tags, report about unknown tag names.
		if( !func.return_value_inner_reference_tag_.empty() )
		{
			bool found= false;
			for( const Synt::FunctionParam& arg : func.params_ )
			{
				if( func.return_value_inner_reference_tag_ == arg.reference_tag_ ||
					func.return_value_inner_reference_tag_ == arg.inner_arg_reference_tag_ )
				{
					found= true;
					break;
				}
			}
			if( !found )
				REPORT_ERROR( NameNotFound, errors_container, func.src_loc_, func.return_value_inner_reference_tag_ );
		}
	}
}

void CodeBuilder::TryGenerateFunctionReturnReferencesMapping(
	CodeBuilderErrorsContainer& errors_container,
	const Synt::FunctionType& func,
	FunctionType& function_type )
{
	// Generate mapping of input references to output references, if reference tags are not specified explicitly.

	if( function_type.return_value_type != ValueType::Value && function_type.return_references.empty() )
	{
		if( !func.return_value_reference_tag_.empty() )
		{
			bool tag_found= false;
			for( const Synt::FunctionParam& arg : func.params_ )
			{
				if( func.return_value_reference_tag_ == arg.inner_arg_reference_tag_ ||
					func.return_value_reference_tag_ == arg.reference_tag_)
				{
					tag_found= true;
					break;
				}
			}

			if( !tag_found ) // Tag exists, but referenced args is empty - means tag apperas only in return value, but not in any argument.
				REPORT_ERROR( NameNotFound, errors_container, func.src_loc_, func.return_value_reference_tag_ );
		}

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
	const ClassPtr& base_class )
{
	if( func.name_.back() == Keywords::constructor_ && IsCopyConstructor( function_type, base_class ) )
	{
		if( !func.type_.referecnces_pollution_list_.empty() )
			REPORT_ERROR( ExplicitReferencePollutionForCopyConstructor, errors_container, func.src_loc_ );

		// This is copy constructor. Generate reference pollution for it automatically.
		FunctionType::ReferencePollution ref_pollution;
		ref_pollution.dst.first= 0u;
		ref_pollution.dst.second= 0u;
		ref_pollution.src.first= 1u;
		ref_pollution.src.second= 0u;
		function_type.references_pollution.insert(ref_pollution);
	}
	else if( func.name_.back() == OverloadedOperatorToString( OverloadedOperator::Assign ) && IsCopyAssignmentOperator( function_type, base_class ) )
	{
		if( !func.type_.referecnces_pollution_list_.empty() )
			REPORT_ERROR( ExplicitReferencePollutionForCopyAssignmentOperator, errors_container, func.src_loc_ );

		// This is copy assignment operator. Generate reference pollution for it automatically.
		FunctionType::ReferencePollution ref_pollution;
		ref_pollution.dst.first= 0u;
		ref_pollution.dst.second= 0u;
		ref_pollution.src.first= 1u;
		ref_pollution.src.second= 0u;
		function_type.references_pollution.insert(ref_pollution);
	}
	else if( func.name_.back() == OverloadedOperatorToString( OverloadedOperator::CompareEqual ) && IsEqualityCompareOperator( function_type, base_class ) )
	{
		if( !func.type_.referecnces_pollution_list_.empty() )
			REPORT_ERROR( ExplicitReferencePollutionForEqualityCompareOperator, errors_container, func.src_loc_ );
	}
	else
		ProcessFunctionTypeReferencesPollution( errors_container, func.type_, function_type );
}

void CodeBuilder::ProcessFunctionTypeReferencesPollution(
	CodeBuilderErrorsContainer& errors_container,
	const Synt::FunctionType& func,
	FunctionType& function_type )
{
	const auto get_references=
	[&]( const std::string& name ) -> ArgsVector<FunctionType::ParamReference>
	{
		ArgsVector<FunctionType::ParamReference> result;

		for( size_t arg_n= 0u; arg_n < function_type.params.size(); ++arg_n )
		{
			const Synt::FunctionParam& in_arg= func.params_[ arg_n ];

			if( name == in_arg.reference_tag_ )
				result.emplace_back( arg_n, FunctionType::c_arg_reference_tag_number );
			if( name == in_arg.inner_arg_reference_tag_ )
				result.emplace_back( arg_n, 0u );
		}

		if( result.empty() )
			REPORT_ERROR( NameNotFound, errors_container, func.src_loc_, name );

		return result;
	};

	for( const Synt::FunctionReferencesPollution& pollution : func.referecnces_pollution_list_ )
	{
		if( pollution.first == pollution.second )
		{
			REPORT_ERROR( SelfReferencePollution, errors_container, func.src_loc_ );
			continue;
		}

		const ArgsVector<FunctionType::ParamReference> dst_references= get_references( pollution.first );
		const ArgsVector<FunctionType::ParamReference> src_references= get_references( pollution.second );

		for( const FunctionType::ParamReference& dst_ref : dst_references )
		{
			if( dst_ref.second == FunctionType::c_arg_reference_tag_number )
			{
				REPORT_ERROR( ArgReferencePollution, errors_container, func.src_loc_ );
				continue;
			}

			for( const FunctionType::ParamReference& src_ref : src_references )
			{
				FunctionType::ReferencePollution ref_pollution;
				ref_pollution.dst= dst_ref;
				ref_pollution.src= src_ref;
				function_type.references_pollution.emplace(ref_pollution);
			}
		}
	} // for pollution
}

void CodeBuilder::SetupReferencesInCopyOrMove( FunctionContext& function_context, const VariablePtr& dst_variable, const VariablePtr& src_variable, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	const VariablePtr& src_node= src_variable;
	const VariablePtr& dst_node= dst_variable;
	if( src_node == nullptr || dst_node == nullptr || dst_variable->type.ReferencesTagsCount() == 0u )
		return;

	const ReferencesGraph::NodesSet src_node_inner_references= function_context.variables_state.GetAccessibleVariableNodesInnerReferences( src_node );
	const ReferencesGraph::NodesSet dst_variable_nodes= function_context.variables_state.GetAllAccessibleVariableNodes( dst_node );

	if( src_node_inner_references.empty() || dst_variable_nodes.empty() )
		return;

	bool node_is_mutable= false;
	for( const VariablePtr& src_node_inner_reference : src_node_inner_references )
		node_is_mutable= node_is_mutable || src_node_inner_reference->node_kind == ReferencesGraphNodeKind::ReferenceMut;

	for( const VariablePtr& dst_variable_node : dst_variable_nodes )
	{
		VariablePtr dst_node_inner_reference= function_context.variables_state.GetNodeInnerReference( dst_variable_node );
		if( dst_node_inner_reference == nullptr )
		{
			dst_node_inner_reference=
				function_context.variables_state.CreateNodeInnerReference( dst_node, node_is_mutable ? ReferencesGraphNodeKind::ReferenceMut : ReferencesGraphNodeKind::ReferenceImut );
		}

		if( ( dst_node_inner_reference->node_kind == ReferencesGraphNodeKind::ReferenceMut  && !node_is_mutable ) ||
			( dst_node_inner_reference->node_kind == ReferencesGraphNodeKind::ReferenceImut &&  node_is_mutable ) )
			REPORT_ERROR( InnerReferenceMutabilityChanging, errors_container, src_loc, dst_node_inner_reference->name );

		for( const VariablePtr& src_node_inner_reference : src_node_inner_references )
		{
			if( !function_context.variables_state.TryAddLink( src_node_inner_reference, dst_node_inner_reference ) )
				REPORT_ERROR( ReferenceProtectionError, errors_container, src_loc, src_node_inner_reference->name );
		}
	}
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
		for( const VariablePtr& variable_ptr : temporary_variables_storage.variables_ )
		{
			const Variable& variable= *variable_ptr;

			// Destroy variables without links.
			// Destroy all references, because all actual references that holds values should not yet be registered.
			if( !function_context.variables_state.NodeMoved( variable_ptr ) &&
				( variable.node_kind != ReferencesGraphNodeKind::Variable ||
					!function_context.variables_state.HaveOutgoingLinks( variable_ptr ) ) )
			{
				if( variable.node_kind == ReferencesGraphNodeKind::Variable && !function_context.is_functionless_context )
				{
					if( variable.type.HaveDestructor() )
						CallDestructor( variable.llvm_value, variable.type, function_context, errors_container, src_loc );
					if( variable.location == Variable::Location::Pointer )
						CreateLifetimeEnd( function_context, variable.llvm_value );
				}
				function_context.variables_state.MoveNode( variable_ptr );
				any_node_moved= true;
			}
		}

		if(!any_node_moved)
			return;
	}
}

ReferencesGraph CodeBuilder::MergeVariablesStateAfterIf(
	const std::vector<ReferencesGraph>& bracnhes_variables_state,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	ReferencesGraph::MergeResult res= ReferencesGraph::MergeVariablesStateAfterIf( bracnhes_variables_state, src_loc );
	errors_container.insert( errors_container.end(), res.second.begin(), res.second.end() );
	return std::move(res.first);
}

bool CodeBuilder::IsReferenceAllowedForReturn( FunctionContext& function_context, const VariablePtr& variable_node )
{
	for( const FunctionType::ParamReference& param_and_tag : function_context.function_type.return_references )
	{
		const size_t arg_n= param_and_tag.first;
		U_ASSERT( arg_n < function_context.args_nodes.size() );
		if( param_and_tag.second == FunctionType::c_arg_reference_tag_number && variable_node == function_context.args_nodes[arg_n].first )
			return true;
		if( param_and_tag.second == 0u && variable_node == function_context.args_nodes[arg_n].second )
			return true;
	}

	if( IsGlobalVariable(variable_node) )
	{
		//  Allow to return global variables.
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

		const VariablePtr inner_reference= function_context.variables_state.GetNodeInnerReference( node_pair.first );
		if( inner_reference == nullptr )
			continue;

		for( const VariablePtr& accesible_variable : function_context.variables_state.GetAllAccessibleVariableNodes( inner_reference ) )
		{
			if( accesible_variable == node_pair.second )
				continue;

			if( IsGlobalVariable( accesible_variable ) )
				continue;

			std::optional<FunctionType::ParamReference> reference;
			for( size_t j= 0u; j < function_context.function_type.params.size(); ++j )
			{
				if( accesible_variable == function_context.args_nodes[j].first )
					reference= FunctionType::ParamReference( uint8_t(j), FunctionType::c_arg_reference_tag_number );
				if( accesible_variable == function_context.args_nodes[j].second )
					reference= FunctionType::ParamReference( uint8_t(j), 0u );
			}

			if( reference != std::nullopt )
			{
				FunctionType::ReferencePollution pollution;
				pollution.src= *reference;
				pollution.dst.first= uint8_t(i);
				pollution.dst.second= 0u;
				if( function_context.function_type.references_pollution.count( pollution ) != 0u )
					continue;
			}
			REPORT_ERROR( UnallowedReferencePollution, errors_container, src_loc );
		}
	}
}

} // namespace U
