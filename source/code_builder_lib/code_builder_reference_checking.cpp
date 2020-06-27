#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"
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
				REPORT_ERROR( NameNotFound, errors_container, func.file_pos_, func.return_value_inner_reference_tag_ );
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
				REPORT_ERROR( NameNotFound, errors_container, func.file_pos_, func.return_value_reference_tag_ );
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
			REPORT_ERROR( ExplicitReferencePollutionForCopyConstructor, errors_container, func.file_pos_ );

		if( base_class->class_->inner_reference_type > InnerReferenceType::None )
		{
			// This is copy constructor. Generate reference pollution for it automatically.
			Function::ReferencePollution ref_pollution;
			ref_pollution.dst.first= 0u;
			ref_pollution.dst.second= 0u;
			ref_pollution.src.first= 1u;
			ref_pollution.src.second= 0u;
			function_type.references_pollution.insert(ref_pollution);
		}
	}
	else if( func.name_.back() == OverloadedOperatorToString( OverloadedOperator::Assign ) && IsCopyAssignmentOperator( function_type, base_class ) )
	{
		if( !func.type_.referecnces_pollution_list_.empty() )
			REPORT_ERROR( ExplicitReferencePollutionForCopyAssignmentOperator, errors_container, func.file_pos_ );

		if( base_class->class_->inner_reference_type > InnerReferenceType::None )
		{
			// This is copy assignment operator. Generate reference pollution for it automatically.
			Function::ReferencePollution ref_pollution;
			ref_pollution.dst.first= 0u;
			ref_pollution.dst.second= 0u;
			ref_pollution.src.first= 1u;
			ref_pollution.src.second= 0u;
			function_type.references_pollution.insert(ref_pollution);
		}
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
		bool any_ref_found= false;

		for( size_t arg_n= 0u; arg_n < function_type.args.size(); ++arg_n )
		{
			const Synt::FunctionArgument& in_arg= func.arguments_[ arg_n ];

			if( name == in_arg.reference_tag_ )
			{
				any_ref_found= true;
				result.emplace_back( arg_n, Function::c_arg_reference_tag_number );
			}
			if( name == in_arg.inner_arg_reference_tag_ )
			{
				any_ref_found= true;
				result.emplace_back( arg_n, 0u );
			}
		}

		if( !any_ref_found && result.empty() )
			REPORT_ERROR( NameNotFound, errors_container, func.file_pos_, name );

		return result;
	};

	for( const Synt::FunctionReferencesPollution& pollution : func.referecnces_pollution_list_ )
	{
		if( pollution.first == pollution.second )
		{
			REPORT_ERROR( SelfReferencePollution, errors_container, func.file_pos_ );
			continue;
		}

		const ArgsVector<Function::ArgReference> dst_references= get_references( pollution.first );
		const ArgsVector<Function::ArgReference> src_references= get_references( pollution.second );

		for( const Function::ArgReference& dst_ref : dst_references )
		{
			if( dst_ref.second == Function::c_arg_reference_tag_number )
			{
				REPORT_ERROR( ArgReferencePollution, errors_container, func.file_pos_ );
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

void CodeBuilder::DestroyUnusedTemporaryVariables( FunctionContext& function_context, CodeBuilderErrorsContainer& errors_container, const FilePos& file_pos )
{
	StackVariablesStorage& temporary_variables_storage= *function_context.stack_variables_stack.back();
	for( const StackVariablesStorage::NodeAndVariable& variable : temporary_variables_storage.variables_ )
	{
		if( !function_context.variables_state.HaveOutgoingLinks( variable.first ) &&
			!function_context.variables_state.NodeMoved( variable.first ) )
		{
			if( variable.first->kind == ReferencesGraphNode::Kind::Variable && variable.second.type.HaveDestructor() )
				CallDestructor( variable.second.llvm_value, variable.second.type, function_context, errors_container, file_pos );
			function_context.variables_state.MoveNode( variable.first );
		}
	}
}

ReferencesGraph CodeBuilder::MergeVariablesStateAfterIf(
	const std::vector<ReferencesGraph>& bracnhes_variables_state,
	CodeBuilderErrorsContainer& errors_container,
	const FilePos& file_pos )
{
	ReferencesGraph::MergeResult res= ReferencesGraph::MergeVariablesStateAfterIf( bracnhes_variables_state, file_pos );
	errors_container.insert( errors_container.end(), res.second.begin(), res.second.end() );
	return std::move(res.first);
}

} // namespace CodeBuilderPrivate

} // namespace U
