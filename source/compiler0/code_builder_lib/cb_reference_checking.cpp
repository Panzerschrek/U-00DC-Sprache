#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

void CodeBuilder::ProcessFunctionArgReferencesTags(
	CodeBuilderErrorsContainer& errors_container,
	const Synt::FunctionType& func,
	Function& function_type,
	const Synt::FunctionArgument& in_arg,
	const Function::Arg& out_arg,
	const size_t arg_number )
{
	U_UNUSED(errors_container); // TODO - remove it.

	if( function_type.return_value_is_reference && !func.return_value_reference_tag_.empty() )
	{
		// Arg reference to return reference
		if( out_arg.is_reference && !in_arg.reference_tag_.empty() && in_arg.reference_tag_ == func.return_value_reference_tag_ )
			function_type.return_references.emplace( arg_number, Function::c_arg_reference_tag_number );

		// Inner arg references to return reference
		if( in_arg.inner_arg_reference_tag_ == func.return_value_reference_tag_ )
			function_type.return_references.emplace( arg_number, 0u );
	}

	if( !function_type.return_value_is_reference && !func.return_value_inner_reference_tag_.empty() )
	{
		// In arg reference to return value references
		if( out_arg.is_reference && !in_arg.reference_tag_.empty() && in_arg.reference_tag_ == func.return_value_inner_reference_tag_ )
			function_type.return_references.emplace( arg_number, Function::c_arg_reference_tag_number );

		// Inner arg references to return value references
		if( in_arg.inner_arg_reference_tag_ == func.return_value_inner_reference_tag_ )
			function_type.return_references.emplace( arg_number, 0u );
	}
}

void CodeBuilder::ProcessFunctionReturnValueReferenceTags(
	CodeBuilderErrorsContainer& errors_container,
	const Synt::FunctionType& func,
	const Function& function_type )
{
	if( !function_type.return_value_is_reference )
	{
		// Check names of tags, report about unknown tag names.
		if( !func.return_value_inner_reference_tag_.empty() )
		{
			bool found= false;
			for( const Synt::FunctionArgument& arg : func.arguments_ )
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
	Function& function_type )
{
	// Generate mapping of input references to output references, if reference tags are not specified explicitly.

	if( function_type.return_value_is_reference && function_type.return_references.empty() )
	{
		if( !func.return_value_reference_tag_.empty() )
		{
			bool tag_found= false;
			for( const Synt::FunctionArgument& arg : func.arguments_ )
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
		for( size_t i= 0u; i < function_type.args.size(); ++i )
		{
			if( function_type.args[i].is_reference )
				function_type.return_references.emplace( i, Function::c_arg_reference_tag_number );
		}
	}
}

void CodeBuilder::ProcessFunctionReferencesPollution(
	CodeBuilderErrorsContainer& errors_container,
	const Synt::Function& func,
	Function& function_type,
	const ClassProxyPtr& base_class )
{
	if( func.name_.back() == Keywords::constructor_ && IsCopyConstructor( function_type, base_class ) )
	{
		if( !func.type_.referecnces_pollution_list_.empty() )
			REPORT_ERROR( ExplicitReferencePollutionForCopyConstructor, errors_container, func.src_loc_ );

		// This is copy constructor. Generate reference pollution for it automatically.
		Function::ReferencePollution ref_pollution;
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
		Function::ReferencePollution ref_pollution;
		ref_pollution.dst.first= 0u;
		ref_pollution.dst.second= 0u;
		ref_pollution.src.first= 1u;
		ref_pollution.src.second= 0u;
		function_type.references_pollution.insert(ref_pollution);
	}
	else
		ProcessFunctionTypeReferencesPollution( errors_container, func.type_, function_type );
}

void CodeBuilder::ProcessFunctionTypeReferencesPollution(
	CodeBuilderErrorsContainer& errors_container,
	const Synt::FunctionType& func,
	Function& function_type )
{
	const auto get_references=
	[&]( const std::string& name ) -> ArgsVector<Function::ArgReference>
	{
		ArgsVector<Function::ArgReference> result;

		for( size_t arg_n= 0u; arg_n < function_type.args.size(); ++arg_n )
		{
			const Synt::FunctionArgument& in_arg= func.arguments_[ arg_n ];

			if( name == in_arg.reference_tag_ )
				result.emplace_back( arg_n, Function::c_arg_reference_tag_number );
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

		const ArgsVector<Function::ArgReference> dst_references= get_references( pollution.first );
		const ArgsVector<Function::ArgReference> src_references= get_references( pollution.second );

		for( const Function::ArgReference& dst_ref : dst_references )
		{
			if( dst_ref.second == Function::c_arg_reference_tag_number )
			{
				REPORT_ERROR( ArgReferencePollution, errors_container, func.src_loc_ );
				continue;
			}

			for( const Function::ArgReference& src_ref : src_references )
			{
				Function::ReferencePollution ref_pollution;
				ref_pollution.dst= dst_ref;
				ref_pollution.src= src_ref;
				function_type.references_pollution.emplace(ref_pollution);
			}
		}
	} // for pollution
}

void CodeBuilder::SetupReferencesInCopyOrMove( FunctionContext& function_context, const Variable& dst_variable, const Variable& src_variable, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	const ReferencesGraphNodePtr& src_node= src_variable.node;
	const ReferencesGraphNodePtr& dst_node= dst_variable.node;
	if( src_node == nullptr || dst_node == nullptr || dst_variable.type.ReferencesTagsCount() == 0u )
		return;

	const ReferencesGraph::NodesSet src_node_inner_references= function_context.variables_state.GetAccessibleVariableNodesInnerReferences( src_node );
	const ReferencesGraph::NodesSet dst_variable_nodes= function_context.variables_state.GetAllAccessibleVariableNodes( dst_node );

	if( src_node_inner_references.empty() || dst_variable_nodes.empty() )
		return;

	bool node_is_mutable= false;
	for( const ReferencesGraphNodePtr& src_node_inner_reference : src_node_inner_references )
		node_is_mutable= node_is_mutable || src_node_inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut;

	for( const ReferencesGraphNodePtr& dst_variable_node : dst_variable_nodes )
	{
		ReferencesGraphNodePtr dst_node_inner_reference= function_context.variables_state.GetNodeInnerReference( dst_variable_node );
		if( dst_node_inner_reference == nullptr )
		{
			dst_node_inner_reference= std::make_shared<ReferencesGraphNode>( dst_node->name + " inner variable", node_is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
			function_context.variables_state.SetNodeInnerReference( dst_node, dst_node_inner_reference );
		}

		if( ( dst_node_inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut  && !node_is_mutable ) ||
			( dst_node_inner_reference->kind == ReferencesGraphNode::Kind::ReferenceImut &&  node_is_mutable ) )
			REPORT_ERROR( InnerReferenceMutabilityChanging, errors_container, src_loc, dst_node_inner_reference->name );

		for( const ReferencesGraphNodePtr& src_node_inner_reference : src_node_inner_references )
		{
			if( !function_context.variables_state.TryAddLink( src_node_inner_reference, dst_node_inner_reference ) )
				REPORT_ERROR( ReferenceProtectionError, errors_container, src_loc, src_node_inner_reference->name );
		}
	}
}

void CodeBuilder::DestroyUnusedTemporaryVariables( FunctionContext& function_context, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	StackVariablesStorage& temporary_variables_storage= *function_context.stack_variables_stack.back();
	for( const Variable& variable : temporary_variables_storage.variables_ )
	{
		if( !function_context.variables_state.HaveOutgoingLinks( variable.node ) &&
			!function_context.variables_state.NodeMoved( variable.node ) )
		{
			if( variable.node->kind == ReferencesGraphNode::Kind::Variable && variable.type.HaveDestructor() )
				CallDestructor( variable.llvm_value, variable.type, function_context, errors_container, src_loc );
			function_context.variables_state.MoveNode( variable.node );
		}
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

bool CodeBuilder::IsReferenceAllowedForReturn( FunctionContext& function_context, const ReferencesGraphNodePtr& variable_node )
{
	for( const Function::ArgReference& arg_and_tag : function_context.function_type.return_references )
	{
		const size_t arg_n= arg_and_tag.first;
		U_ASSERT( arg_n < function_context.args_nodes.size() );
		if( arg_and_tag.second == Function::c_arg_reference_tag_number && variable_node == function_context.args_nodes[arg_n].first )
			return true;
		if( arg_and_tag.second == 0u && variable_node == function_context.args_nodes[arg_n].second )
			return true;
	}
	return false;
}

void CodeBuilder::CheckReferencesPollutionBeforeReturn(
	FunctionContext& function_context,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	for( size_t i= 0u; i < function_context.function_type.args.size(); ++i )
	{
		if( !function_context.function_type.args[i].is_reference )
			continue;

		const auto& node_pair= function_context.args_nodes[i];

		const ReferencesGraphNodePtr inner_reference= function_context.variables_state.GetNodeInnerReference( node_pair.first );
		if( inner_reference == nullptr )
			continue;

		for( const ReferencesGraphNodePtr& accesible_variable : function_context.variables_state.GetAllAccessibleVariableNodes( inner_reference ) )
		{
			if( accesible_variable == node_pair.second )
				continue;

			std::optional<Function::ArgReference> reference;
			for( size_t j= 0u; j < function_context.function_type.args.size(); ++j )
			{
				if( accesible_variable == function_context.args_nodes[j].first )
					reference= Function::ArgReference( j, Function::c_arg_reference_tag_number );
				if( accesible_variable == function_context.args_nodes[j].second )
					reference= Function::ArgReference( j, 0u );
			}

			if( reference != std::nullopt )
			{
				Function::ReferencePollution pollution;
				pollution.src= *reference;
				pollution.dst.first= i;
				pollution.dst.second= 0u;
				if( function_context.function_type.references_pollution.count( pollution ) != 0u )
					continue;
			}
			REPORT_ERROR( UnallowedReferencePollution, errors_container, src_loc );
		}
	}
}

} // namespace CodeBuilderPrivate

} // namespace U
