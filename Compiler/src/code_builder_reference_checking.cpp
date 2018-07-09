#include "assert.hpp"
#include "keywords.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

static const Class& GetClassForIncompleteType( const Type& type )
{
	U_ASSERT( type.GetClassType() != nullptr || type.GetArrayType() != nullptr );

	const Type* lower_type= &type;
	while( lower_type->GetClassType() == nullptr )
	{
		U_ASSERT( lower_type->GetArrayType() != nullptr ); // If not class - must be array
		lower_type= &lower_type->GetArrayType()->type;
	}

	return *lower_type->GetClassType();
}

void CodeBuilder::ProcessFunctionArgReferencesTags(
	const Synt::FunctionType& func,
	Function& function_type,
	const Synt::FunctionArgument& in_arg,
	const Function::Arg& out_arg,
	const size_t arg_number )
{
	const bool has_continuous_tag= !in_arg.inner_arg_reference_tags_.empty() && in_arg.inner_arg_reference_tags_.back().empty();
	const size_t regular_tag_count= has_continuous_tag ? ( in_arg.inner_arg_reference_tags_.size() - 2u ) : in_arg.inner_arg_reference_tags_.size();
	const size_t arg_reference_tag_count= out_arg.type.ReferencesTagsCount();

	if( !in_arg.inner_arg_reference_tags_.empty() )
	{
		if( out_arg.type.IsIncomplete() )
		{
			const Class& class_= GetClassForIncompleteType( out_arg.type );
			if( class_.completeness < Class::Completeness::ReferenceTagsComplete )
				errors_.push_back( ReportUsingIncompleteType( in_arg.file_pos_, class_.members.GetThisNamespaceName() ) );
		}

		if( has_continuous_tag )
		{
			if( regular_tag_count > arg_reference_tag_count )
				errors_.push_back( ReportInvalidReferenceTagCount( in_arg.file_pos_, regular_tag_count, arg_reference_tag_count ) );
		}
		else if( regular_tag_count != arg_reference_tag_count )
			errors_.push_back( ReportInvalidReferenceTagCount( in_arg.file_pos_, regular_tag_count, arg_reference_tag_count ) );
	}

	if( function_type.return_value_is_reference && !func.return_value_reference_tag_.empty() )
	{
		// Arg reference to return reference
		if( out_arg.is_reference && !in_arg.reference_tag_.empty() &&
			in_arg.reference_tag_ == func.return_value_reference_tag_ )
			function_type.return_references.args_references.push_back( arg_number );

		// Inner arg references to return reference
		for( size_t tag_number= 0u; tag_number < regular_tag_count; ++tag_number )
		{
			if( in_arg.inner_arg_reference_tags_[tag_number] == func.return_value_reference_tag_ )
				function_type.return_references.inner_args_references.emplace_back( arg_number, tag_number );
		}
		if( has_continuous_tag )
		{
			for( size_t tag_number= regular_tag_count; tag_number < arg_reference_tag_count; ++tag_number )
			{
				if( in_arg.inner_arg_reference_tags_[regular_tag_count] == func.return_value_reference_tag_ )
					function_type.return_references.inner_args_references.emplace_back( arg_number, tag_number );
			}
		}
	}

	const bool return_value_has_continuous_tag= !func.return_value_inner_reference_tags_.empty() && func.return_value_inner_reference_tags_.back().empty();
	const size_t return_value_regular_tag_count= return_value_has_continuous_tag ? ( func.return_value_inner_reference_tags_.size() - 2u ) : func.return_value_inner_reference_tags_.size();
	const size_t return_value_reference_tag_count= function_type.return_type.ReferencesTagsCount();

	if( !function_type.return_value_is_reference && !func.return_value_inner_reference_tags_.empty() &&
		return_value_reference_tag_count > 0u )
	{
		// In arg reference to return value references
		if( out_arg.is_reference && !in_arg.reference_tag_.empty() )
		{
			for( size_t ret_tag_number= 0u; ret_tag_number < return_value_regular_tag_count; ++ ret_tag_number )
			{
				if( func.return_value_inner_reference_tags_[ret_tag_number] == in_arg.reference_tag_ )
					function_type.return_references.args_references.push_back( arg_number );
			}
			if( return_value_has_continuous_tag )
			{
				for( size_t ret_tag_number= return_value_regular_tag_count; ret_tag_number < return_value_reference_tag_count; ++ret_tag_number )
				{
					if( func.return_value_inner_reference_tags_[return_value_regular_tag_count] == in_arg.reference_tag_ )
						function_type.return_references.args_references.push_back( arg_number );
				}
			}
		}

		// Inner arg references to return value references
		if( !in_arg.inner_arg_reference_tags_.empty() )
		{
			for( size_t arg_tag_number= 0u; arg_tag_number < regular_tag_count; ++arg_tag_number )
			{
				const ProgramString& arg_tag= in_arg.inner_arg_reference_tags_[arg_tag_number];
				for( size_t ret_tag_number= 0u; ret_tag_number < return_value_regular_tag_count; ++ ret_tag_number )
				{
					if( arg_tag == func.return_value_inner_reference_tags_[ret_tag_number] )
						function_type.return_references.inner_args_references.emplace_back( arg_number, arg_tag_number );
				}
				if( return_value_has_continuous_tag )
				{
					for( size_t ret_tag_number= return_value_regular_tag_count; ret_tag_number < return_value_reference_tag_count; ++ret_tag_number )
					{
						if( arg_tag == func.return_value_inner_reference_tags_[return_value_regular_tag_count] )
							function_type.return_references.inner_args_references.emplace_back( arg_number, arg_tag_number );
					}
				}
			}
			if( has_continuous_tag )
			{
				for( size_t arg_tag_number= regular_tag_count; arg_tag_number < arg_reference_tag_count; ++arg_tag_number )
				{
					const ProgramString& arg_tag= in_arg.inner_arg_reference_tags_[regular_tag_count];
					for( size_t ret_tag_number= 0u; ret_tag_number < return_value_regular_tag_count; ++ ret_tag_number )
					{
						if( arg_tag == func.return_value_inner_reference_tags_[ret_tag_number] )
							function_type.return_references.inner_args_references.emplace_back( arg_number, arg_tag_number );
					}
					if( return_value_has_continuous_tag )
					{
						for( size_t ret_tag_number= return_value_regular_tag_count; ret_tag_number < return_value_reference_tag_count; ++ret_tag_number )
						{
							if( arg_tag == func.return_value_inner_reference_tags_[return_value_regular_tag_count] )
								function_type.return_references.inner_args_references.emplace_back( arg_number, arg_tag_number );
						}
					}
				}
			}
		}
	}
}

void CodeBuilder::ProcessFunctionReturnValueReferenceTags( const Synt::FunctionType& func, const Function& function_type )
{
	if( !function_type.return_value_is_reference && !func.return_value_inner_reference_tags_.empty() )
	{
		const bool has_continuous_tag= !func.return_value_inner_reference_tags_.empty() && func.return_value_inner_reference_tags_.back().empty();
		const size_t regular_tag_count= has_continuous_tag ? ( func.return_value_inner_reference_tags_.size() - 2u ) : func.return_value_inner_reference_tags_.size();
		const size_t reference_tag_count= function_type.return_type.ReferencesTagsCount();

		if( function_type.return_type.IsIncomplete() )
		{
			const Class& class_= GetClassForIncompleteType( function_type.return_type );
			if( class_.completeness < Class::Completeness::ReferenceTagsComplete )
				errors_.push_back( ReportUsingIncompleteType( func.file_pos_, class_.members.GetThisNamespaceName() ) );
		}

		if( has_continuous_tag )
		{
			if( regular_tag_count > reference_tag_count )
				errors_.push_back( ReportInvalidReferenceTagCount( func.file_pos_, regular_tag_count, reference_tag_count ) );
		}
		else if( regular_tag_count != reference_tag_count )
			errors_.push_back( ReportInvalidReferenceTagCount( func.file_pos_, regular_tag_count, reference_tag_count ) );

		// Check names of tags, report about unknown tag names.
		for( size_t i= 0u; i < regular_tag_count; ++i )
		{
			const ProgramString& tag = func.return_value_inner_reference_tags_[i];

			bool found= false;
			for( const Synt::FunctionArgumentPtr& arg : func.arguments_ )
			{
				if( tag == arg->reference_tag_ )
				{
					found= true;
					break;
				}
				for( const ProgramString& inner_arg_tag : arg->inner_arg_reference_tags_ )
				{
					if( tag == inner_arg_tag )
					{
						found= true;
						break;
					}
				}
			}
			if( !found )
				errors_.push_back( ReportNameNotFound( func.file_pos_, tag ) );
		}
	}
}

void CodeBuilder::TryGenerateFunctionReturnReferencesMapping(
	const Synt::FunctionType& func,
	Function& function_type )
{
	// Generate mapping of input references to output references, if reference tags are not specified explicitly.

	if( function_type.return_value_is_reference &&
		( function_type.return_references.args_references.empty() && function_type.return_references.inner_args_references.empty() ) )
	{
		if( !func.return_value_reference_tag_.empty() )
		{
			bool tag_found= false;
			for( const Synt::FunctionArgumentPtr& arg : func.arguments_ )
			{
				for( const ProgramString& tag : arg->inner_arg_reference_tags_ )
					if( tag == func.return_value_reference_tag_ )
						tag_found= true;
				if( arg->reference_tag_ == func.return_value_reference_tag_ )
					tag_found= true;
				if( tag_found )
					break;
			}

			if( !tag_found ) // Tag exists, but referenced args is empty - means tag apperas only in return value, but not in any argument.
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
		( func.name_.components.back().name == Keywords::constructor_ && ( func.type_.arguments_.empty() || func.type_.arguments_.front()->name_ != Keywords::this_ ) );

	if( func.name_.components.back().name == Keywords::constructor_ && IsCopyConstructor( function_type, base_class ) )
	{
		if( !func.type_.referecnces_pollution_list_.empty() )
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
	else if( func.name_.components.back().name == OverloadedOperatorToString( OverloadedOperator::Assign ) && IsCopyAssignmentOperator( function_type, base_class ) )
	{
		if( !func.type_.referecnces_pollution_list_.empty() )
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
		ProcessFunctionTypeReferencesPollution( func.type_, function_type, first_arg_is_implicit_this );
}

void CodeBuilder::ProcessFunctionTypeReferencesPollution(
	const Synt::FunctionType& func,
	Function& function_type,
	const bool first_arg_is_implicit_this )
{
	const auto get_references=
	[&]( const ProgramString& name ) -> std::vector<Function::ArgReference>
	{
		std::vector<Function::ArgReference> result;
		bool any_ref_found= false;

		for( size_t arg_n= 0u; arg_n < function_type.args.size(); ++arg_n )
		{
			if( arg_n == 0u && first_arg_is_implicit_this )
				continue;

			const Synt::FunctionArgument& in_arg= *func.arguments_[ arg_n - ( first_arg_is_implicit_this ? 1u : 0u ) ];

			if( !in_arg.reference_tag_.empty() && in_arg.reference_tag_ == name )
				result.emplace_back( arg_n, Function::c_arg_reference_tag_number );

			const bool has_continuous_tag= !in_arg.inner_arg_reference_tags_.empty() && in_arg.inner_arg_reference_tags_.back().empty();
			const size_t regular_tag_count= has_continuous_tag ? ( in_arg.inner_arg_reference_tags_.size() - 2u ) : in_arg.inner_arg_reference_tags_.size();
			const size_t arg_reference_tag_count= function_type.args[arg_n].type.ReferencesTagsCount();

			for( size_t tag_number= 0u; tag_number < regular_tag_count; ++tag_number )
				if( in_arg.inner_arg_reference_tags_[tag_number] == name )
					result.emplace_back( arg_n, tag_number );

			if( has_continuous_tag )
			{
				for( size_t tag_number= regular_tag_count; tag_number < arg_reference_tag_count; ++tag_number )
					if( in_arg.inner_arg_reference_tags_[regular_tag_count] == name )
						result.emplace_back( arg_n, tag_number );
			}

			if( has_continuous_tag && in_arg.inner_arg_reference_tags_[regular_tag_count] == name )
				any_ref_found= true;
		}

		if( !any_ref_found && result.empty() )
			errors_.push_back( ReportNameNotFound( func.file_pos_, name ) );

		return result;
	};

	for( const Synt::FunctionReferencesPollution& pollution : func.referecnces_pollution_list_ )
	{
		if( pollution.first == pollution.second.name )
		{
			errors_.push_back( ReportSelfReferencePollution( func.file_pos_ ) );
			continue;
		}

		const std::vector<Function::ArgReference> dst_references= get_references( pollution.first );
		const std::vector<Function::ArgReference> src_references= get_references( pollution.second.name );

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

	for( const VariablesState& branch_state : bracnhes_variables_state )
	{
		U_ASSERT( branch_state.GetVariables().size() == bracnhes_variables_state.front().GetVariables().size() );
		for (const auto& variable_pair : branch_state.GetVariables() )
		{
			if( result.find( variable_pair.first ) == result.end() )
				result[variable_pair.first].is_moved= variable_pair.second.is_moved;

			VariablesState::VariableEntry& result_entry= result[variable_pair.first];

			if( result_entry.is_moved != variable_pair.second.is_moved )
				errors_.push_back( ReportConditionalMove( file_pos, variable_pair.first->name ) );

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

	for( const auto& var_before : state_before.GetVariables() )
	{
		U_ASSERT( state_after.GetVariables().find( var_before.first ) != state_after.GetVariables().end() );
		const auto& var_after= *state_after.GetVariables().find( var_before.first );

		U_ASSERT( var_before.second.inner_references.size() <= var_after.second.inner_references.size() ); // Currently, can only add references.

		if( !var_before.second.is_moved && var_after.second.is_moved )
			errors_.push_back( ReportOuterVariableMoveInsideLoop( file_pos, var_before.first->name ) );

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
