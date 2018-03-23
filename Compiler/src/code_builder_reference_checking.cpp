#include "assert.hpp"
#include "keywords.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

void CodeBuilder::ProcessFunctionArgReferencesTags(
	const Synt::Function& func,
	Function& function_type,
	const Synt::FunctionArgument& in_arg,
	const Function::Arg& out_arg,
	const size_t arg_number )
{
	if( !in_arg.inner_arg_reference_tags_.empty() &&
		!out_arg.type.IsIncomplete() && // Only generate error for args with complete type. Complete types now required for functions with body.
		in_arg.inner_arg_reference_tags_.size() != out_arg.type.ReferencesTagsCount() )
		errors_.push_back( ReportInvalidReferenceTagCount( in_arg.file_pos_, in_arg.inner_arg_reference_tags_.size(), out_arg.type.ReferencesTagsCount() ) );

	if( function_type.return_value_is_reference && !func.return_value_reference_tag_.empty() )
	{
		// Arg reference to return reference
		if( out_arg.is_reference && !in_arg.reference_tag_.empty() &&
			in_arg.reference_tag_ == func.return_value_reference_tag_ )
			function_type.return_references.args_references.push_back( arg_number );

		// Inner arg references to return reference
		for( const ProgramString& tag : in_arg.inner_arg_reference_tags_ )
		{
			const size_t tag_number= &tag - in_arg.inner_arg_reference_tags_.data();
			if( tag == func.return_value_reference_tag_ )
				function_type.return_references.inner_args_references.emplace_back( arg_number, tag_number );
		}
	}

	if( !function_type.return_value_is_reference && !func.return_value_inner_reference_tags_.empty() &&
		function_type.return_type.ReferencesTagsCount() > 0u )
	{
		// In arg reference to return value references
		if( out_arg.is_reference && !in_arg.reference_tag_.empty() )
		{
			for( const ProgramString& tag : func.return_value_inner_reference_tags_ )
			{
				if( tag == in_arg.reference_tag_ )
					function_type.return_references.args_references.push_back( arg_number );
			}
		}

		// Inner arg references to return value references
		if( !in_arg.inner_arg_reference_tags_.empty() )
		{
			for( const ProgramString& arg_tag : in_arg.inner_arg_reference_tags_ )
			for( const ProgramString& ret_tag : func.return_value_inner_reference_tags_ )
			{
				if( arg_tag == ret_tag )
				{
					const size_t arg_tag_number= &arg_tag - in_arg.inner_arg_reference_tags_.data();
					//const size_t ret_tag_number= &ret_tag - func.return_value_inner_reference_tags_;
					function_type.return_references.inner_args_references.emplace_back( arg_number, arg_tag_number );
				}
			}
		}
	}
}

void CodeBuilder::TryGenerateFunctionReturnReferencesMapping(
	const Synt::Function& func,
	Function& function_type )
{
	// Generate mapping of input references to output references, if reference tags are not specified explicitly.

	if( function_type.return_value_is_reference &&
		( function_type.return_references.args_references.empty() && function_type.return_references.inner_args_references.empty() ) )
	{
		if( !func.return_value_reference_tag_.empty() )
		{
			// Tag exists, but referenced args is empty - means tag apperas only in return value, but not in any argument.
			errors_.push_back( ReportNameNotFound( func.file_pos_, func.return_value_reference_tag_ ) );
		}

		// If there is no tag for return reference, assume, that it may refer to any reference argument, but not inner reference of any argument.
		for( size_t i= 0u; i < function_type.args.size(); ++i )
		{
			if( function_type.args[i].is_reference )
				function_type.return_references.args_references.push_back(i);
		}
	}

	if( !function_type.return_value_is_reference && function_type.return_type.ReferencesTagsCount() > 0u &&
		func.return_value_inner_reference_tags_.empty() )
	{
		// If there is no tag for return reference, assume, that it may refer to any reference argument, but not inner reference of any argument.
		for( size_t i= 0u; i < function_type.args.size(); ++i )
		{
			if( function_type.args[i].is_reference )
				function_type.return_references.args_references.push_back(i);
		}
	}
}

void CodeBuilder::ProcessFunctionReferencesPollution(
	const Synt::Function& func,
	Function& function_type,
	const ClassProxyPtr& base_class )
{
	const bool first_arg_is_implicit_this=
		( func.name_.components.back().name == Keywords::destructor_ ) ||
		( func.name_.components.back().name == Keywords::constructor_ && ( func.arguments_.empty() || func.arguments_.front()->name_ != Keywords::this_ ) );

	const auto get_references=
	[&]( const ProgramString& name ) -> std::vector<Function::ArgReference>
	{
		std::vector<Function::ArgReference> result;

		for( size_t arg_n= 0u; arg_n < function_type.args.size(); ++arg_n )
		{
			if( arg_n == 0u && first_arg_is_implicit_this )
				continue;

			const Synt::FunctionArgument& in_arg= *func.arguments_[ arg_n - ( first_arg_is_implicit_this ? 1u : 0u ) ];

			if( !in_arg.reference_tag_.empty() && in_arg.reference_tag_ == name )
				result.emplace_back( arg_n, Function::c_arg_reference_tag_number );

			for( const ProgramString& inner_tag : in_arg.inner_arg_reference_tags_ )
				if( inner_tag == name )
					result.emplace_back( arg_n, &inner_tag - in_arg.inner_arg_reference_tags_.data() );
		}

		return result;
	};

	if( func.name_.components.size() == 1u && func.name_.components.front().name == Keywords::constructor_ &&
		function_type.args.size() == 2u &&
		function_type.args.back().type == base_class && !function_type.args.back().is_mutable && function_type.args.back().is_reference )
	{
		if( !func.referecnces_pollution_list_.empty() )
			errors_.push_back( ReportExplicitReferencePollutionForCopyConstructor( func.file_pos_ ) );

		if( base_class->class_->references_tags_count > 0u )
		{
			// This is copy constructor. Generate reference pollution for it automatically.
			Function::ReferencePollution ref_pollution;
			ref_pollution.dst.first= 0u;
			ref_pollution.dst.second= 0u;
			ref_pollution.src.first= 1u;
			ref_pollution.src.second= 0u;
			ref_pollution.src_is_mutable= true; // TODO - set correct mutability.
			function_type.references_pollution.insert(ref_pollution);
		}
	}
	else if( func.name_.components.back().name == OverloadedOperatorToString( OverloadedOperator::Assign ) &&
		function_type.args.size() == 2u &&
		function_type.args[0u].type == base_class &&  function_type.args[0u].is_mutable && function_type.args[0u].is_reference &&
		function_type.args[1u].type == base_class && !function_type.args[1u].is_mutable && function_type.args[1u].is_reference )
	{
		if( !func.referecnces_pollution_list_.empty() )
			errors_.push_back( ReportExplicitReferencePollutionForCopyAssignmentOperator( func.file_pos_ ) );

		if( base_class->class_->references_tags_count > 0u )
		{
			// This is copy assignment operator. Generate reference pollution for it automatically.
			Function::ReferencePollution ref_pollution;
			ref_pollution.dst.first= 0u;
			ref_pollution.dst.second= 0u;
			ref_pollution.src.first= 1u;
			ref_pollution.src.second= 0u;
			ref_pollution.src_is_mutable= true; // TODO - set correct mutability.
			function_type.references_pollution.insert(ref_pollution);
		}
	}
	else
	{
		for( const Synt::FunctionReferencesPollution& pollution : func.referecnces_pollution_list_ )
		{
			if( pollution.first == pollution.second.name )
			{
				errors_.push_back( ReportSelfReferencePollution( func.file_pos_ ) );
				continue;
			}

			const std::vector<Function::ArgReference> dst_references= get_references( pollution.first );
			const std::vector<Function::ArgReference> src_references= get_references( pollution.second.name );
			if( dst_references.empty() )
				errors_.push_back( ReportNameNotFound( func.file_pos_, pollution.first ) );
			if( src_references.empty() )
				errors_.push_back( ReportNameNotFound( func.file_pos_, pollution.second.name ) );

			for( const Function::ArgReference& dst_ref : dst_references )
			{
				if( dst_ref.second == Function::c_arg_reference_tag_number )
				{
					errors_.push_back( ReportArgReferencePollution( func.file_pos_ ) );
					continue;
				}

				for( const Function::ArgReference& src_ref : src_references )
				{
					Function::ReferencePollution ref_pollution;
					ref_pollution.dst= dst_ref;
					ref_pollution.src= src_ref;
					ref_pollution.src_is_mutable= pollution.second.is_mutable;
					function_type.references_pollution.emplace(ref_pollution);
				}
			}
		} // for pollution
	}
}

void CodeBuilder::CheckReferencedVariables( const Variable& reference, const FilePos& file_pos )
{
	for( const StoredVariablePtr& referenced_variable : reference.referenced_variables )
		CheckVariableReferences( *referenced_variable, file_pos );
}

void CodeBuilder::CheckVariableReferences( const StoredVariable& var, const FilePos& file_pos )
{
	if(
		var. mut_use_counter.use_count() <= 2u &&
		var.imut_use_counter.use_count() == 1u)
	{} // All ok - one mutable reference.
	else if( var. mut_use_counter.use_count() == 1u )
	{} // All ok - 0-infinity immutable references.
	else
		errors_.push_back( ReportReferenceProtectionError( file_pos, var.name ) );
}

std::vector<VariableStorageUseCounter> CodeBuilder::LockReferencedVariables( const Variable& reference )
{
	std::vector<VariableStorageUseCounter> locks;
	for( const StoredVariablePtr& referenced_variable : reference.referenced_variables )
		locks.push_back( reference.value_type == ValueType::Reference ? referenced_variable->mut_use_counter : referenced_variable->imut_use_counter );

	return locks;
}

VariablesState CodeBuilder::MergeVariablesStateAfterIf( const std::vector<VariablesState>& bracnhes_variables_state, const FilePos& file_pos )
{
	U_UNUSED( file_pos );
	U_ASSERT( !bracnhes_variables_state.empty() );

	VariablesState::VariablesContainer result;

	// SPRACHE_TODO - check moving. Disallow conditional moving.

	for( const VariablesState& branch_state : bracnhes_variables_state )
	{
		U_ASSERT( branch_state.GetVariables().size() == bracnhes_variables_state.front().GetVariables().size() );
		for (const auto& variable_pair : branch_state.GetVariables() )
		{
			VariablesState::VariableEntry& result_entry= result[variable_pair.first];
			for( auto& reference : variable_pair.second.inner_references )
			{
				const auto it= result_entry.inner_references.find( reference.first );
				if( it == result_entry.inner_references.end() )
				{
					// Ok, inserted one time.
					result_entry.inner_references.insert( reference );
				}
				else
				{
					// If linked as mutable and as immutable in different branches - result is mutable.
					if( !it->second.IsMutable() && reference.second.IsMutable() )
						it->second= reference.second;
				}
			}
		}
	} // for branches.

	return VariablesState(std::move(result));
}

void CodeBuilder::CheckWhileBlokVariablesState( const VariablesState& state_before, const VariablesState& state_after, const FilePos& file_pos )
{
	U_ASSERT( state_before.GetVariables().size() == state_after.GetVariables().size() );

	// SPRACHE_TODO - detect also moving of outer variables inside loop.

	for( const auto& var_before : state_before.GetVariables() )
	{
		U_ASSERT( state_after.GetVariables().find( var_before.first ) != state_after.GetVariables().end() );
		const auto& var_after= *state_after.GetVariables().find( var_before.first );

		U_ASSERT( var_before.second.inner_references.size() <= var_after.second.inner_references.size() ); // Currently, can only add references.

		for( const auto& reference_after : var_after.second.inner_references )
		{
			const auto reference_before_it= var_before.second.inner_references.find( reference_after.first );
			if( reference_before_it == var_before.second.inner_references.end() )
			{
				//add reference in while loop
				if( reference_after.second.IsMutable() )
					errors_.push_back( ReportMutableReferencePollutionOfOuterLoopVariable( file_pos, var_before.first->name, reference_after.first->name ) );
			}
		}
	}
}

} // namespace CodeBuilderPrivate

} // namespace U
