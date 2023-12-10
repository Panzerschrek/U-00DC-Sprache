#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

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
			if( pollution.src.first == 0 && pollution.src.second == FunctionType::c_param_reference_number )
				REPORT_ERROR( ConstructorThisReferencePollution, errors_container, func.src_loc );
		}
	}

	const auto create_copy_pollution=
	[&]
	{
		// Assume, that this function is called during function preparation, which for class is called after determining number of inner reference tags.
		// So, we can request it.
		const size_t reference_tag_count= base_class->inner_references.size();
		for( size_t i= 0u; i < reference_tag_count; ++i )
		{
			FunctionType::ReferencePollution ref_pollution;
			ref_pollution.dst.first= 0u;
			ref_pollution.dst.second= uint8_t(i);
			ref_pollution.src.first= 1u;
			ref_pollution.src.second= uint8_t(i);
			function_type.references_pollution.insert(ref_pollution);
		}
	};

	if( func_name == Keywords::constructor_ && IsCopyConstructor( function_type, base_class ) )
	{
		if( func.type.references_pollution_expression != nullptr )
			REPORT_ERROR( ExplicitReferencePollutionForCopyConstructor, errors_container, func.src_loc );

		// This is copy constructor. Generate reference pollution for it automatically.
		create_copy_pollution();
	}
	else if( func_name == OverloadedOperatorToString( OverloadedOperator::Assign ) && IsCopyAssignmentOperator( function_type, base_class ) )
	{
		if( func.type.references_pollution_expression != nullptr )
			REPORT_ERROR( ExplicitReferencePollutionForCopyAssignmentOperator, errors_container, func.src_loc );

		// This is copy assignment operator. Generate reference pollution for it automatically.
		create_copy_pollution();
	}
	else if( func_name == OverloadedOperatorToString( OverloadedOperator::CompareEqual ) && IsEqualityCompareOperator( function_type, base_class ) )
	{
		if( func.type.references_pollution_expression != nullptr )
			REPORT_ERROR( ExplicitReferencePollutionForEqualityCompareOperator, errors_container, func.src_loc );
	}
}

void CodeBuilder::CheckCompleteFunctionReferenceNotation( const FunctionType& function_type, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	CheckFunctionReferencesNotationInnerReferences( function_type, errors_container, src_loc );
	CheckFunctionReferencesNotationMutabilityCorrectness( function_type, errors_container, src_loc );
}

void CodeBuilder::CheckFunctionReferencesNotationInnerReferences( const FunctionType& function_type, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	const auto check_param_reference=
	[&]( const FunctionType::ParamReference& param_reference )
	{
		if( param_reference.second != FunctionType::c_param_reference_number && param_reference.first < function_type.params.size()  )
		{
			const Type& type= function_type.params[ param_reference.first ].type;
			const auto tag_count= type.ReferenceTagCount();
			if( param_reference.second >= tag_count )
			{
				REPORT_ERROR( ReferenceTagOutOfRange, errors_container, src_loc, param_reference.second, type, tag_count );
			}
		}
	};

	for( const auto& pollution : function_type.references_pollution )
	{
		check_param_reference(pollution.dst);
		check_param_reference(pollution.src);
	}

	for( const FunctionType::ParamReference& param_reference : function_type.return_references )
		check_param_reference(param_reference);

	for( const auto& param_references : function_type.return_inner_references )
		for( const FunctionType::ParamReference& param_reference : param_references )
			check_param_reference(param_reference);

	const auto return_type_tag_count= function_type.return_type.ReferenceTagCount();
	if( !function_type.return_inner_references.empty() &&
		function_type.return_inner_references.size() != return_type_tag_count )
		REPORT_ERROR( InnerReferenceTagCountMismatch, errors_container, src_loc, return_type_tag_count, function_type.return_inner_references.size() );
}

void CodeBuilder::CheckFunctionReferencesNotationMutabilityCorrectness(
	const FunctionType& function_type,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	// Do not allow to specify reference notation where immutable references are linked with mutable references.

	if( function_type.return_value_type == ValueType::ReferenceMut )
		CheckReferenceNotationMutabilityViolationForReturnReferences( function_type, function_type.return_references, errors_container, src_loc );

	for(
		size_t i= 0, i_end= std::min( function_type.return_type.ReferenceTagCount(), function_type.return_inner_references.size() );
		i < i_end;
		++i )
	{
		if( function_type.return_type.GetInnerReferenceType(i) == InnerReferenceType::Mut )
			CheckReferenceNotationMutabilityViolationForReturnReferences( function_type, function_type.return_inner_references[i], errors_container, src_loc );
	}

	for( const auto& pollution : function_type.references_pollution )
	{
		if( pollution.dst.first >= function_type.params.size() )
			continue;
		const FunctionType::Param& dst_param= function_type.params[ pollution.dst.first ];

		if( pollution.dst.second != FunctionType::c_param_reference_number &&
			pollution.dst.second < dst_param.type.ReferenceTagCount() &&
			dst_param.type.GetInnerReferenceType( pollution.dst.second ) == InnerReferenceType::Mut )
			CheckReferenceNotationMutabilityViolationForMutableReference( function_type, pollution.src, errors_container, src_loc );
	}
}

void CodeBuilder::CheckReferenceNotationMutabilityViolationForReturnReferences(
	const FunctionType& function_type,
	const std::set<FunctionType::ParamReference>& return_references,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	for( const auto& return_reference : return_references )
		CheckReferenceNotationMutabilityViolationForMutableReference( function_type, return_reference, errors_container, src_loc );
}

void CodeBuilder::CheckReferenceNotationMutabilityViolationForMutableReference(
	const FunctionType& function_type,
	const FunctionType::ParamReference& param_reference,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	if( param_reference.first >= function_type.params.size() )
		return; // May be in case of error.

	const FunctionType::Param& param= function_type.params[ param_reference.first ];

	if( param_reference.second == FunctionType::c_param_reference_number )
	{
		if( param.value_type == ValueType::ReferenceImut )
			REPORT_ERROR( ReferenceNotationViolatesImMutability, errors_container, src_loc );
	}
	else
	{
		if( param_reference.second < param.type.ReferenceTagCount() &&
			param.type.GetInnerReferenceType( param_reference.second ) != InnerReferenceType::Mut )
			REPORT_ERROR( ReferenceNotationViolatesImMutability, errors_container, src_loc );
	}
}

void CodeBuilder::SetupReferencesInCopyOrMove( FunctionContext& function_context, const VariablePtr& dst_variable, const VariablePtr& src_variable, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	if( dst_variable->type.ReferenceTagCount() == 0u )
		return;

	const size_t reference_tag_count= dst_variable->type.ReferenceTagCount();
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
	std::vector<VariablePtr>& varaibles= function_context.stack_variables_stack.back()->variables_;
	// Try to remove unused nodes (variables and references) until we can't remove anything.
	// Multiple iterations needed to process complex references chains.
	while(true)
	{
		// Perform removal with total order preservation.
		const auto new_end= std::remove_if(
			varaibles.begin(),
			varaibles.end(),
			[&]( const VariablePtr& variable )
			{
				// Destroy variables without links.
				// Destroy all references, because all actual references that holds values should not yet be registered.
				// Preserve variables with "preserve_temporary" flag set, unless they are already moved.
				if( variable->preserve_temporary )
				{
					if( function_context.variables_state.NodeMoved( variable ) )
					{
						function_context.variables_state.RemoveNode( variable );
						return true;
					}
					return false;
				}
				else if( ( variable->value_type != ValueType::Value || !function_context.variables_state.HaveOutgoingLinks( variable ) ) )
				{
					// Emit destructions call code only if it is a non-moved variable.
					if( variable->value_type == ValueType::Value && !function_context.is_functionless_context && !function_context.variables_state.NodeMoved( variable ) )
					{
						if( variable->type.HaveDestructor() )
							CallDestructor( variable->llvm_value, variable->type, function_context, errors_container, src_loc );
						if( variable->location == Variable::Location::Pointer )
							CreateLifetimeEnd( function_context, variable->llvm_value );
					}
					function_context.variables_state.RemoveNode( variable );
					return true;
				}
				return false;
			} );

		if( new_end == varaibles.end() )
			return;
		varaibles.erase( new_end, varaibles.end() );
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
		if( param_and_tag.second == FunctionType::c_param_reference_number && variable_node == function_context.args_nodes[arg_n].first )
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
		if( param_and_tag.second == FunctionType::c_param_reference_number && variable_node == function_context.args_nodes[arg_n].first )
			return true;
		if( param_and_tag.second < function_context.args_nodes[arg_n].second.size() && variable_node == function_context.args_nodes[arg_n].second[param_and_tag.second] )
			return true;
	}

	return false;
}

void CodeBuilder::CheckAsyncReturnReferenceIsAllowed(
	NamesScope& names,
	FunctionContext& function_context,
	const CoroutineTypeDescription& coroutine_type_description,
	const VariablePtr& node,
	const SrcLoc& src_loc )
{
	for( const VariablePtr& var_node : function_context.variables_state.GetAllAccessibleVariableNodes( node ) )
	{
		const auto coroutine_inner_reference= GetCoroutineInnerReferenceForParamNode( function_context, var_node );

		if( coroutine_inner_reference == std::nullopt ||
			coroutine_type_description.return_references.count( *coroutine_inner_reference ) == 0 )
			REPORT_ERROR( ReturningUnallowedReference, names.GetErrors(), src_loc );
	}
}

void CodeBuilder::CheckAsyncReturnInnerReferencesAreAllowed(
	NamesScope& names,
	FunctionContext& function_context,
	const CoroutineTypeDescription& coroutine_type_description,
	const VariablePtr& node,
	const SrcLoc& src_loc )
{
	for( size_t i= 0; i < node->inner_reference_nodes.size(); ++i )
	{
		for( const VariablePtr& var_node : function_context.variables_state.GetAllAccessibleVariableNodes( node->inner_reference_nodes[i] ) )
		{
			const auto coroutine_inner_reference= GetCoroutineInnerReferenceForParamNode( function_context, var_node );

			if( coroutine_inner_reference == std::nullopt ||
				i >= coroutine_type_description.return_inner_references.size() ||
				coroutine_type_description.return_inner_references[i].count( *coroutine_inner_reference ) == 0 )
				REPORT_ERROR( ReturningUnallowedReference, names.GetErrors(), src_loc );
		}
	}
}

std::optional<FunctionType::ParamReference> CodeBuilder::GetCoroutineInnerReferenceForParamNode( FunctionContext& function_context, const VariablePtr& node )
{
	// Map coroutine function input references to returned coroutine inner references.
	// If this changed, "TransformCoroutineFunctionType" function must be changed too!

	size_t coroutine_inner_reference_index= 0;
	for( size_t i= 0; i < function_context.function_type.params.size(); ++i )
	{
		const FunctionType::Param& param= function_context.function_type.params[i];

		if( param.value_type == ValueType::Value )
		{
			for( size_t j= 0; j < function_context.args_nodes[i].second.size(); ++j )
			{
				if( node == function_context.args_nodes[i].second[j] )
					return FunctionType::ParamReference{ uint8_t(0), uint8_t(coroutine_inner_reference_index + j) };
			}

			coroutine_inner_reference_index+= param.type.ReferenceTagCount();
		}
		else
		{
			if( node == function_context.args_nodes[i].first )
				return FunctionType::ParamReference{ uint8_t(0), uint8_t(coroutine_inner_reference_index) };

			++coroutine_inner_reference_index;
		}
	}

	return std::nullopt;
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
						reference= FunctionType::ParamReference( uint8_t(j), FunctionType::c_param_reference_number );

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
