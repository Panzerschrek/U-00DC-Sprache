#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "code_builder.hpp"
#include "error_reporting.hpp"

namespace U
{

Value CodeBuilder::BuildLambda( NamesScope& names_scope, FunctionContext& function_context, const Synt::Lambda& lambda )
{
	const ClassPtr lambda_class= PrepareLambdaClass( names_scope, function_context, lambda );

	const VariableMutPtr result=
		Variable::Create(
			lambda_class, ValueType::Value, Variable::Location::Pointer, "value of " + Type(lambda_class).ToString() );

	function_context.variables_state.AddNode( result );

	if( !function_context.is_functionless_context )
	{
		result->llvm_value= function_context.alloca_ir_builder.CreateAlloca( lambda_class->llvm_type, nullptr, result->name );
		CreateLifetimeStart( function_context, result->llvm_value );
	}

	// Copy captured values.
	const auto lambda_class_data= std::get_if<LambdaClassData>( &lambda_class->generated_class_data );
	U_ASSERT( lambda_class_data != nullptr );

	llvm::SmallVector<llvm::Constant*, 4> constexpr_initializers;
	if( lambda_class->can_be_constexpr )
		constexpr_initializers.resize( lambda_class->llvm_type->getNumElements() );
	size_t num_constexpr_initializers= 0;

	if( const auto capture_list= std::get_if<Synt::Lambda::CaptureList>( &lambda.capture ) )
	{
		llvm::SmallVector<VariablePtr, 4> temp_initialized_variables;

		// Initialize lambda fields in capture list order.
		for( const Synt::Lambda::CaptureListElement& capture : *capture_list )
		{
			if( const auto field_value= lambda_class->members->GetThisScopeValue( capture.name ) )
			{
				if( const auto field= field_value->value.GetClassField() )
				{
					std::pair<llvm::Value*, llvm::Constant*> init_value( nullptr, nullptr );
					if( std::holds_alternative<Synt::EmptyVariant>( capture.expression ) )
					{
						// Simple capture - lookup variable for it by name.
						if( const auto lookup_value= LookupName( names_scope, capture.name, lambda.src_loc ).value )
							if( const auto variable= lookup_value->value.GetVariable() )
								init_value= InitializeLambdaField( names_scope, function_context, *field, variable, result, lambda.src_loc );
					}
					else
					{
						// Capture with expression specified.
						const VariablePtr variable= BuildExpressionCodeEnsureVariable( capture.expression, names_scope, function_context );
						init_value= InitializeLambdaField( names_scope, function_context, *field, variable, result, lambda.src_loc );
					}

					if( lambda_class->can_be_constexpr && init_value.second != nullptr )
					{
						constexpr_initializers[ field->index ]= init_value.second;
						++num_constexpr_initializers;
					}

					if( !field->is_reference && // No need to destruct reference fields.
						&capture != &capture_list->back() && // No need to register last one.
						field->type.HasDestructor() && // No need to call destructors.
						init_value.first != nullptr )
					{
						// Temportary register field value for destruction,
						// in case if "return" or "await" happens in evaluation of further captrure expressions.
						VariableMutPtr temp_initialized_variable= Variable::Create(
							field->type,
							ValueType::Value,
							Variable::Location::Pointer,
							result->name + "." + capture.name,
							init_value.first );
						temp_initialized_variable->preserve_temporary= true;
						function_context.variables_state.AddNode( temp_initialized_variable );
						RegisterTemporaryVariable( function_context, temp_initialized_variable );
						temp_initialized_variables.push_back( std::move(temp_initialized_variable) );
					}
				}
			}
		}

		for( const VariablePtr& temp_initialized_variable : temp_initialized_variables )
			function_context.variables_state.MoveNode( temp_initialized_variable );
	}
	else
	{
		// Initialize lambda fields in natural order.
		for( const LambdaClassData::Capture& capture : lambda_class_data->captures )
		{
			if( const auto lookup_value= LookupName( names_scope, capture.captured_variable_name, lambda.src_loc ).value )
			{
				if( const auto variable= lookup_value->value.GetVariable() )
				{
					const auto constexpr_value= InitializeLambdaField( names_scope, function_context, *capture.field, variable, result, lambda.src_loc ).second;
					if( lambda_class->can_be_constexpr && constexpr_value != nullptr )
					{
						constexpr_initializers[ capture.field->index ]= constexpr_value;
						++num_constexpr_initializers;
					}
				}
			}
		}
	}

	if( lambda_class->can_be_constexpr && num_constexpr_initializers == constexpr_initializers.size() )
		result->constexpr_value= llvm::ConstantStruct::get( lambda_class->llvm_type, constexpr_initializers );

	if( std::holds_alternative<Synt::Lambda::CaptureList>( lambda.capture ) )
	{
		// Destroy temporaries in lambda captures initializers.
		DestroyUnusedTemporaryVariables( function_context, names_scope.GetErrors(), lambda.src_loc );
	}

	RegisterTemporaryVariable( function_context, result );
	return result;
}

std::pair<llvm::Value*, llvm::Constant*> CodeBuilder::InitializeLambdaField(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const ClassField& field,
	const VariablePtr& variable,
	const VariablePtr& result,
	const SrcLoc& src_loc )
{
	U_ASSERT( variable->type == field.type );

	const auto field_address= CreateClassFieldGEP( function_context, *result, field.index );

	if( field.is_reference )
	{
		if( variable->value_type == ValueType::Value )
		{
			REPORT_ERROR( ExpectedReferenceValue, names_scope.GetErrors(), src_loc );
			return std::make_pair( field_address, nullptr );
		}
		if( field.is_mutable && variable->value_type == ValueType::ReferenceImut )
		{
			REPORT_ERROR( BindingConstReferenceToNonconstReference, names_scope.GetErrors(), src_loc );
			return std::make_pair( field_address, nullptr );
		}

		// Link references.
		U_ASSERT( field.reference_tag < result->inner_reference_nodes.size() );
		function_context.variables_state.TryAddLink( variable, result->inner_reference_nodes[ field.reference_tag ], names_scope.GetErrors(), src_loc );

		CreateTypedReferenceStore( function_context, variable->type, variable->llvm_value, field_address );

		if( variable->constexpr_value != nullptr )
		{
			if( variable != nullptr && llvm::isa<llvm::GlobalVariable>( variable->llvm_value ) )
				return std::make_pair( field_address, llvm::dyn_cast<llvm::Constant>(variable->llvm_value) ); // Return address of existing global constant.

			// We need to store constant somewhere. Create global variable for it.
			const auto global_constant= CreateGlobalConstantVariable( variable->type, "_temp_const", variable->constexpr_value );
			return std::make_pair( field_address, global_constant );
		}
		return std::make_pair( field_address, nullptr );
	}
	else
	{
		// Link references (before performing potential move).
		const size_t reference_tag_count= field.type.ReferenceTagCount();
		U_ASSERT( field.inner_reference_tags.size() == reference_tag_count );
		U_ASSERT( variable->inner_reference_nodes.size() == reference_tag_count );
		for( size_t i= 0; i < reference_tag_count; ++i )
		{
			const size_t dst_node_index= field.inner_reference_tags[i];
			U_ASSERT( dst_node_index < result->inner_reference_nodes.size() );
			function_context.variables_state.TryAddLink( variable->inner_reference_nodes[i], result->inner_reference_nodes[ dst_node_index ], names_scope.GetErrors(), src_loc );
		}

		if( variable->type.GetFundamentalType() != nullptr||
			variable->type.GetEnumType() != nullptr ||
			variable->type.GetRawPointerType() != nullptr ||
			variable->type.GetFunctionPointerType() != nullptr )
		{
			// Just copy simple scalar.
			CreateTypedStore( function_context, variable->type, CreateMoveToLLVMRegisterInstruction( *variable, function_context ), field_address );
		}
		else
		{
			if( variable->value_type == ValueType::Value )
			{
				// Move.
				function_context.variables_state.MoveNode( variable );
				if( !function_context.is_functionless_context )
				{
					CopyBytes( field_address, variable->llvm_value, variable->type, function_context );

					if( variable->location == Variable::Location::Pointer )
						CreateLifetimeEnd( function_context, variable->llvm_value );
				}
			}
			else if( !variable->type.IsCopyConstructible() )
				REPORT_ERROR( CopyConstructValueOfNoncopyableType, names_scope.GetErrors(), src_loc, variable->type );
			else if( !function_context.is_functionless_context )
				BuildCopyConstructorPart( field_address, variable->llvm_value, variable->type, function_context );
		}

		return std::make_pair( field_address, variable->constexpr_value );
	}
}

ClassPtr CodeBuilder::PrepareLambdaClass( NamesScope& names_scope, FunctionContext& function_context, const Synt::Lambda& lambda )
{
	// Use first named namespace as lambda class parent.
	// We can't use namespace of function variables here, because it will be destroyed later.
	// Usually this is a closest global scope that contains this function - some namespace, struct, class or a template args namespace.
	NamesScope* const lambda_class_parent_scope= names_scope.GetClosestNamedSpaceOrRoot();

	LambdaKey key;
	key.parent_scope= lambda_class_parent_scope;
	key.src_loc= lambda.src_loc;

	// Extract tuple-for indices.
	{
		NamesScope* current= &names_scope;
		while( current != nullptr )
		{
			if( const auto tuple_for_index= current->GetThisScopeValue( " tuple for index" ) )
				if( const auto tuple_for_index_variable= tuple_for_index->value.GetVariable() )
					if( tuple_for_index_variable->constexpr_value != nullptr )
						key.tuple_for_indices.push_back( uint32_t( tuple_for_index_variable->constexpr_value->getUniqueInteger().getLimitedValue() ) );
			current= current->GetParent();
		}

		// Since we iterate over name scopes in reverse order reverse the result container.
		std::reverse( key.tuple_for_indices.begin(), key.tuple_for_indices.end() );
	}

	if( completion_request_index_ > 0 )
	{
		// Workaround for language server.
		// On each completion request increase this index and use it for lambdas caching,
		// in order to preprocess same lambdas in completion.
		key.tuple_for_indices.push_back( completion_request_index_ );
	}

	if( const auto it= lambda_classes_table_.find(key); it != lambda_classes_table_.end() )
	{
		// Already generated.
		// This may happen because of expressions preevaluation.
		return it->second.get();
	}

	std::vector<TemplateArg> template_args;
	if( lambda_class_parent_scope->GetThisNamespaceName() == NamesScope::c_template_args_namespace_name )
	{
		// This is a lambda inside template function or in template class declaration expression.
		// Extract template args for later usage for lambda name mangling.
		using NamedTemplateArg= std::pair< std::string_view, TemplateArg >;
		llvm::SmallVector< NamedTemplateArg, 4 > named_template_args;
		lambda_class_parent_scope->ForEachInThisScope(
			[&]( const std::string_view name, const NamesScopeValue& value )
			{
				if( const auto t= value.value.GetTypeName() )
				{
					if( const auto class_= t->GetClassType() )
					{
						if( class_->members->GetThisNamespaceName() == Class::c_template_class_name &&
							class_->members->GetParent() == lambda_class_parent_scope )
							return; // Skip class of this class template.
					}
					named_template_args.emplace_back( name, *t );
				}
				else if( const auto v= value.value.GetVariable() )
					named_template_args.emplace_back( name, TemplateVariableArg( *v ) );
			} );

		// Sort template args by name in order to obtain some stable order.
		// This order should not be the same as order in the template itself, it should only be deterministic.
		std::sort(
			named_template_args.begin(), named_template_args.end(),
			[&]( const NamedTemplateArg& l, const NamedTemplateArg& r ) { return l.first < r.first; } );

		// Populate result template args.
		template_args.reserve( named_template_args.size() );
		for( NamedTemplateArg& template_arg : named_template_args )
			template_args.push_back( std::move( template_arg.second ) );
	}

	// Create the class.
	auto class_ptr= std::make_unique<Class>( GetLambdaBaseName( lambda, key.tuple_for_indices ), lambda_class_parent_scope );
	Class* const class_= class_ptr.get();
	lambda_classes_table_.emplace( std::move(key), std::move(class_ptr) );

	class_->members->SetClass( class_ );
	class_->src_loc= lambda.src_loc;
	class_->kind= Class::Kind::Struct; // Set temporary to struct in order to allow generation of some methods.
	class_->parents_list_prepared= true;
	class_->has_explicit_noncopy_constructors= false;
	class_->is_default_constructible= false;
	class_->can_be_constexpr= false; // Set later.
	class_->generated_class_data= LambdaClassData{ {}, std::move(template_args) };
	class_->llvm_type= llvm::StructType::create( llvm_context_, mangler_->MangleType( class_ ) );

	// Perform some checks.

	// Do not allow to specify reference notation - reference affects are calculated automatically during lambda preprocessing.
	if( lambda.function.type.references_pollution_expression != nullptr )
		REPORT_ERROR( ReferenceNotationForLambda, names_scope.GetErrors(), Synt::GetExpressionSrcLoc( *lambda.function.type.references_pollution_expression ) );
	if( lambda.function.type.return_value_reference_expression != nullptr )
		REPORT_ERROR( ReferenceNotationForLambda, names_scope.GetErrors(), Synt::GetExpressionSrcLoc( *lambda.function.type.return_value_reference_expression ) );
	if( lambda.function.type.return_value_inner_references_expression != nullptr )
		REPORT_ERROR( ReferenceNotationForLambda, names_scope.GetErrors(), Synt::GetExpressionSrcLoc( *lambda.function.type.return_value_inner_references_expression ) );

	const auto call_op_name= OverloadedOperatorToString( OverloadedOperator::Call );

	bool has_preprocessing_errors= false;
	FunctionType::ReturnReferences return_references;
	FunctionType::ReturnInnerReferences return_inner_references;
	FunctionType::ReferencesPollution references_pollution;
	std::optional<Type> return_type;

	// Run preprocessing.
	{
		NamesScope custom_captures_names_scope( "", &names_scope );

		ReferenceNotationDeductionContext reference_notation_deduction_context;

		LambdaPreprocessingContext lambda_preprocessing_context;
		lambda_preprocessing_context.parent= function_context.lambda_preprocessing_context;
		lambda_preprocessing_context.external_variables= CollectCurrentFunctionVariables( function_context );
		lambda_preprocessing_context.capture_by_reference= std::holds_alternative<Synt::Lambda::CaptureAllByReference>( lambda.capture );

		U_ASSERT( !lambda.function.type.params.empty() );
		{
			const Synt::FunctionParam& in_this_param= lambda.function.type.params.front();
			U_ASSERT( in_this_param.name == Keywords::this_ );
			lambda_preprocessing_context.lambda_this_is_mutable= in_this_param.mutability_modifier == Synt::MutabilityModifier::Mutable;
		}

		if( std::holds_alternative<Synt::Lambda::CaptureNothing>( lambda.capture ) )
			lambda_preprocessing_context.explicit_captures= LambdaPreprocessingContext::ExplicitCaptures();
		else if(
			std::holds_alternative<Synt::Lambda::CaptureAllByValue>( lambda.capture ) ||
			std::holds_alternative<Synt::Lambda::CaptureAllByReference>( lambda.capture ) )
			lambda_preprocessing_context.explicit_captures= std::nullopt; // Allow to capture anything.
		else if( const auto capture_list= std::get_if<Synt::Lambda::CaptureList>( &lambda.capture ) )
		{
			LambdaPreprocessingContext::ExplicitCaptures explicit_captures;
			for( const Synt::Lambda::CaptureListElement& capture : *capture_list )
			{
				if( capture.completion_requested )
					NameLookupCompleteImpl( names_scope, capture.name );

				if( std::holds_alternative<Synt::EmptyVariant>( capture.expression ) )
				{
					// Simple capture with only name.
					if( capture.name == Keywords::this_ || capture.name == Keywords::base_ )
					{
						if( function_context.this_ == nullptr )
							REPORT_ERROR( ThisUnavailable, names_scope.GetErrors(), capture.src_loc );
						else
						{
							// Do not allow to capture "this" even via capture list.
							REPORT_ERROR( ExpectedVariable, names_scope.GetErrors(), capture.src_loc, capture.name );
						}
					}
					else if( const auto value= LookupName( names_scope, capture.name, capture.src_loc ).value )
					{
						CollectDefinition( *value, capture.src_loc );
						if( const VariablePtr variable= value->value.GetVariable() )
						{
							if( explicit_captures.count( variable ) > 0 )
								REPORT_ERROR( DuplicatedCapture, names_scope.GetErrors(), capture.src_loc, capture.name );
							else if( lambda_preprocessing_context.external_variables.count( variable ) == 0 )
								REPORT_ERROR( ExpectedVariable, names_scope.GetErrors(), capture.src_loc, "non-local variable" );
							else
							{
								LambdaPreprocessingContext::ExplicitCapture explicit_capture;
								explicit_capture.capture_by_reference= capture.by_reference;
								explicit_captures.emplace( variable, std::move(explicit_capture) );
							}
						}
						else
							REPORT_ERROR( ExpectedVariable, names_scope.GetErrors(), capture.src_loc, value->value.GetKindName() );
					}
				}
				else
				{
					// Capture with initializer expression.

					if( IsKeyword( capture.name ) )
						REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), lambda.src_loc );

					VariablePtr expr_result;
					{
						const bool prev_is_functionless_context= function_context.is_functionless_context;
						function_context.is_functionless_context= true;
						const auto state= SaveFunctionContextState( function_context );
						{
							const StackVariablesStorage dummy_stack_variables_storage( function_context );

							// Do not need to use preevaluation cache here, since lambda preprocessing itself is cached.
							expr_result= BuildExpressionCodeEnsureVariable( capture.expression, names_scope, function_context );
						}

						RestoreFunctionContextState( function_context, state );
						function_context.is_functionless_context= prev_is_functionless_context;
					}

					const VariableMutPtr capture_variable= Variable::Create(
						expr_result->type,
						// Make immediate values in preprocessing constant.
						expr_result->value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut,
						Variable::Location::Pointer,
						expr_result->name,
						expr_result->llvm_value,
						nullptr /* no need to use constexpr in preprocessing */ );

					if( expr_result->location == Variable::Location::LLVMRegister || expr_result->llvm_value == nullptr )
					{
						// Create proper address for this variable in order to avoid crashes in preprocessing.
						// We can't "alloca" in this function, so, create global variable for that.
						const auto llvm_type= expr_result->type.GetLLVMType();
						capture_variable->llvm_value=
							new llvm::GlobalVariable(
								*module_,
								llvm_type,
								false, // is constant
								llvm::GlobalValue::PrivateLinkage,
								llvm::UndefValue::get( llvm_type ) );
					}

					// No need to add this variable into context of current function, since it is not used in it.

					// Make this variable accessible by its name inside preprocessed function.
					custom_captures_names_scope.AddName( capture.name, NamesScopeValue( capture_variable, capture.src_loc ) );
					lambda_preprocessing_context.external_variables.insert( capture_variable );

					LambdaPreprocessingContext::ExplicitCapture explicit_capture;
					explicit_capture.capture_by_reference= capture.by_reference;
					explicit_captures.emplace( capture_variable, std::move(explicit_capture) );
				}
			}
			lambda_preprocessing_context.explicit_captures= std::move(explicit_captures);
		}
		else U_ASSERT(false);

		ReturnTypeDeductionContext return_type_deduction_context;

		{
			FunctionVariable op_variable;
			op_variable.syntax_element= &lambda.function;
			// It's fine to use incomplete lambda class here, since this class can't be accessed.
			op_variable.type= PrepareFunctionType( names_scope, function_context, lambda.function.type, GetLambdaPreprocessingDummyClass( names_scope ) );
			op_variable.llvm_function= std::make_shared<LazyLLVMFunction>( "" /* The name of temporary function is irrelevant. */ );
			op_variable.is_this_call= true;

			BuildFuncCode(
				op_variable,
				class_,
				custom_captures_names_scope,
				call_op_name,
				lambda.function.type.IsAutoReturn() ? &return_type_deduction_context : nullptr,
				&reference_notation_deduction_context,
				&lambda_preprocessing_context );

			// Remove temp function.
			op_variable.llvm_function->function->eraseFromParent();
		}

		has_preprocessing_errors= lambda_preprocessing_context.has_preprocessing_errors;

		if( lambda.function.type.IsAutoReturn() )
			return_type= return_type_deduction_context.return_type.value_or( void_type_ );

		// Check if explicitly specified captures are not used.
		// It's important to produce such error, becase later unused captures will NOT be actually captured.
		if( const auto capture_list= std::get_if<Synt::Lambda::CaptureList>( &lambda.capture ) )
		{
			for( const Synt::Lambda::CaptureListElement& capture : *capture_list )
			{
				if( lambda_preprocessing_context.captured_external_variables.count( capture.name ) == 0 )
					REPORT_ERROR( UnusedCapture, names_scope.GetErrors(), capture.src_loc, capture.name );
			}
		}

		// Extract captured variables and sort them to ensure stable order.
		struct CapturedVariableForSorting
		{
			std::string name;
			LambdaPreprocessingContext::CapturedVariableData data;
			uint64_t alignment= 0;
			bool capture_by_reference= false;
		};

		llvm::SmallVector<CapturedVariableForSorting, 4> captured_variables;

		for( auto& captured_variable_pair : lambda_preprocessing_context.captured_external_variables )
		{
			CapturedVariableForSorting v;
			v.name= captured_variable_pair.first;
			v.data= std::move(captured_variable_pair.second);

			v.capture_by_reference= lambda_preprocessing_context.capture_by_reference;
			if( lambda_preprocessing_context.explicit_captures != std::nullopt )
			{
				// If has explicit captures - use individual by reference capture flags.
				if( const auto it= lambda_preprocessing_context.explicit_captures->find( v.data.source_variable ); it != lambda_preprocessing_context.explicit_captures->end() )
					v.capture_by_reference= it->second.capture_by_reference;
			}

			const auto llvm_type= v.data.source_variable->type.GetLLVMType();
			v.alignment=
				v.capture_by_reference
					? data_layout_.getABITypeAlignment( llvm_type->getPointerTo() )
					: data_layout_.getABITypeAlignment( llvm_type );

			captured_variables.push_back( std::move(v) );
		};

		// We must sort fields in order to avoid creating fields list in hash-map iteration order (which is non-deterministic).
		// There should be no equal elements here, because this can prevent sorting to be stable.
		std::sort(
			captured_variables.begin(), captured_variables.end(),
			[]( const CapturedVariableForSorting& l, const CapturedVariableForSorting& r )
			{
				// Sort in alignment descending order (to minimize padding).
				if( l.alignment != r.alignment )
					return l.alignment > r.alignment;
				// If alignment is the same - use name for ordering (it should be unique).
				return l.name < r.name;
			} );

		std::unordered_map<VariablePtr, uint8_t> captured_variable_to_lambda_inner_reference_tag;

		// Iterate over sorted captured variables, create fields for them, setup reference notation.
		for( const CapturedVariableForSorting& captured_variable : captured_variables )
		{
			const Type& type= captured_variable.data.source_variable->type;

			const auto index= uint32_t( class_->fields_order.size() );

			auto field= std::make_shared<ClassField>( class_, type, index, true, false );
			class_->fields_order.push_back( field );

			const auto reference_tag_cout= type.ReferenceTagCount();

			if( captured_variable.capture_by_reference )
			{
				field->is_reference= true;
				// Captured reference mutability is determined by mutability of source variable.
				field->is_mutable= captured_variable.data.source_variable->value_type == ValueType::ReferenceMut;

				// Create a reference tag for captured reference.
				field->reference_tag= uint8_t( class_->inner_references.size() );
				class_->inner_references.push_back( field->is_mutable ? InnerReferenceKind::Mut : InnerReferenceKind::Imut );

				if( reference_tag_cout > 0 )
					REPORT_ERROR( ReferenceFieldOfTypeWithReferencesInside, names_scope.GetErrors(), lambda.src_loc, captured_variable.name );

				// Captured by reference variable points to one of the inner reference tags of lambda this.
				captured_variable_to_lambda_inner_reference_tag.emplace( captured_variable.data.variable_node, field->reference_tag );
			}
			else
			{
				// Captured by value variable points to lambda this param ptself.
				captured_variable_to_lambda_inner_reference_tag.emplace( captured_variable.data.variable_node, FunctionType::c_param_reference_number );

				// Each reference tag of each captured variable get its own reference tag in result lambda class.
				U_ASSERT( captured_variable.data.accessible_variables.size() == reference_tag_cout );
				field->inner_reference_tags.reserve( reference_tag_cout );
				for( size_t i= 0; i < reference_tag_cout; ++i )
				{
					const uint8_t reference_tag= uint8_t( class_->inner_references.size() );
					field->inner_reference_tags.push_back( reference_tag );
					class_->inner_references.push_back( type.GetInnerReferenceKind(i) );

					captured_variable_to_lambda_inner_reference_tag.emplace( captured_variable.data.accessible_variables[i], reference_tag );
				}
			}

			class_->members->AddName( captured_variable.name, NamesScopeValue( field, lambda.src_loc ) );

			// Make captured variable fields private.
			class_->members_visibility.insert_or_assign( captured_variable.name, ClassMemberVisibility::Private );

			LambdaClassData::Capture capture;
			capture.captured_variable_name= captured_variable.name;
			capture.field= std::move(field);
			std::get_if<LambdaClassData>( &class_->generated_class_data )->captures.push_back( std::move(capture) );
		}

		const uint8_t this_param_index= 0; // Lambda is "this" (argument 0).

		// Process return references.
		return_references= std::move(reference_notation_deduction_context.return_references);
		for( const VariablePtr& captured_variable_return_reference : lambda_preprocessing_context.captured_variables_return_references )
		{
			if( const auto it= captured_variable_to_lambda_inner_reference_tag.find( captured_variable_return_reference ); it != captured_variable_to_lambda_inner_reference_tag.end() )
				return_references.emplace( this_param_index, it->second );
		}

		// Process return inner references.
		return_inner_references= std::move(reference_notation_deduction_context.return_inner_references);
		if( return_inner_references.size() < lambda_preprocessing_context.captured_variables_return_inner_references.size() )
			return_inner_references.resize( lambda_preprocessing_context.captured_variables_return_inner_references.size() );
		for( size_t tag_n= 0; tag_n < lambda_preprocessing_context.captured_variables_return_inner_references.size(); ++tag_n )
		{
			for( const VariablePtr& captured_variable_return_reference : lambda_preprocessing_context.captured_variables_return_inner_references[ tag_n ] )
			{
				if( const auto it= captured_variable_to_lambda_inner_reference_tag.find( captured_variable_return_reference ); it != captured_variable_to_lambda_inner_reference_tag.end() )
					return_inner_references[tag_n].emplace( this_param_index, it->second );
			}
		}

		// Process pollution.
		references_pollution= std::move(reference_notation_deduction_context.references_pollution);
		for( const LambdaPreprocessingContext::ReferencePollution& pollution : lambda_preprocessing_context.references_pollution )
		{
			FunctionType::ReferencePollution result_pollution;
			if( const auto dst_param_reference= std::get_if<FunctionType::ParamReference>( &pollution.dst ) )
				result_pollution.dst= *dst_param_reference;
			else if( const auto dst_variable_ptr= std::get_if<VariablePtr>( &pollution.dst ) )
			{
				if( const auto it= captured_variable_to_lambda_inner_reference_tag.find( *dst_variable_ptr ); it != captured_variable_to_lambda_inner_reference_tag.end() )
					result_pollution.dst= std::make_pair( this_param_index, it->second );
				else
					continue;
			}
			else U_ASSERT(false);

			if( const auto src_param_reference= std::get_if<FunctionType::ParamReference>( &pollution.src ) )
				result_pollution.src= *src_param_reference;
			else if( const auto src_variable_ptr= std::get_if<VariablePtr>( &pollution.src ) )
			{
				if( const auto it= captured_variable_to_lambda_inner_reference_tag.find( *src_variable_ptr ); it != captured_variable_to_lambda_inner_reference_tag.end() )
					result_pollution.src= std::make_pair( this_param_index, it->second );
				else
					continue;
			}
			else U_ASSERT(false);

			references_pollution.insert( result_pollution );
		}
	}

	{
		llvm::SmallVector<llvm::Type*, 16> fields_llvm_types;
		fields_llvm_types.reserve( class_->fields_order.size() );
		for( const auto& field : class_->fields_order )
		{
			llvm::Type* const llvm_type= field->type.GetLLVMType();
			fields_llvm_types.push_back( field->is_reference ? llvm_type->getPointerTo() : llvm_type );
		}
		class_->llvm_type->setBody( fields_llvm_types );
	}

	class_->field_count= uint32_t( class_->fields_order.size() );
	class_->is_complete= true;

	// Calculate constexpr property.
	class_->can_be_constexpr= true;
	for( const auto& field : class_->fields_order )
	{
		// Disable constexpr, if field can not be constexpr, or if field is mutable reference.
		if( !field->type.CanBeConstexpr() || ( field->is_reference && field->is_mutable ) )
		{
			class_->can_be_constexpr= false;
			break;
		}
	}

	// Try to generate important methods.
	TryGenerateCopyConstructor( class_ );
	TryGenerateCopyAssignmentOperator( class_ );
	TryGenerateDestructor( class_ );
	// Equality compare operator is not needed for lambdas.
	class_->kind= Class::Kind::NonPolymorph; // Set to class after methods generation.

	// Create () operator.
	{
		FunctionVariable op_variable;

		op_variable.type= PrepareFunctionType( names_scope, function_context, lambda.function.type, class_ );
		op_variable.type.return_references= std::move(return_references);
		op_variable.type.return_inner_references= std::move(return_inner_references);
		op_variable.type.references_pollution= std::move(references_pollution);

		if( return_type != std::nullopt )
			op_variable.type.return_type= *return_type;

		op_variable.syntax_element= &lambda.function;
		op_variable.llvm_function= std::make_shared<LazyLLVMFunction>( mangler_->MangleFunction( *class_->members, call_op_name, op_variable.type ) );
		op_variable.is_this_call= true;
		op_variable.prototype_src_loc= op_variable.body_src_loc= lambda.function.src_loc;

		// Use auto-constexpr for () operator.
		op_variable.constexpr_kind= FunctionVariable::ConstexprKind::ConstexprAuto;

		if( !has_preprocessing_errors ) // Avoid building body in case of preprocessing errors.
			BuildFuncCode( op_variable, class_, names_scope, call_op_name );

		auto functions_set= std::make_shared<OverloadedFunctionsSet>();
		functions_set->functions.push_back( op_variable );
		functions_set->base_class= class_;

		class_->members->AddName(
			call_op_name,
			NamesScopeValue( std::move(functions_set), lambda.src_loc ) );
	}

	return class_;
}

ClassPtr CodeBuilder::GetLambdaPreprocessingDummyClass( NamesScope& names_scope )
{
	if( lambda_preprocessing_dummy_class_ != nullptr )
		return lambda_preprocessing_dummy_class_.get();

	auto class_ptr= std::make_unique<Class>( "_lambda_preprocessing_dummy", names_scope.GetRoot() );
	Class* const class_= class_ptr.get();
	lambda_preprocessing_dummy_class_= std::move(class_ptr);

	class_->members->SetClass( class_ );
	class_->kind= Class::Kind::Struct;
	class_->parents_list_prepared= true;
	class_->has_explicit_noncopy_constructors= false;
	class_->is_default_constructible= false;
	class_->can_be_constexpr= true;
	class_->generated_class_data= LambdaClassData{};
	class_->field_count= 0;
	class_->is_complete= true;
	class_->llvm_type= llvm::StructType::create( llvm_context_, mangler_->MangleType( class_ ) );
	class_->llvm_type->setBody( llvm::ArrayRef<llvm::Type*>() );

	// Try to generate important methods.
	TryGenerateCopyConstructor( class_ );
	TryGenerateCopyAssignmentOperator( class_ );
	TryGenerateDestructor( class_ );

	return class_;
}

std::string CodeBuilder::GetLambdaBaseName( const Synt::Lambda& lambda, const llvm::ArrayRef<uint32_t> tuple_for_indices )
{
	std::string name;
	name+= "_lambda_"; // Start with "_" in order to avoid collisions with user names.

	SrcLoc src_loc= lambda.src_loc;
	while(true)
	{
		// Encode file.
		U_ASSERT( source_graph_ != nullptr );
		U_ASSERT( src_loc.GetFileIndex() < source_graph_->nodes_storage.size() );
		// Use file contenst hash instead of file index, because we need to use stable identifier independent on from which main file this file is imported.
		name+= source_graph_->nodes_storage[ src_loc.GetFileIndex() ].contents_hash;
		name+= "_";

		// Encode line and column into the name to produce different names for different lambdas in the same file.
		name+= std::to_string( src_loc.GetLine() );
		name+= "_";

		name+= std::to_string( src_loc.GetColumn() );
		name+= "_";

		const auto macro_expansion_index= src_loc.GetMacroExpansionIndex();
		if( macro_expansion_index != SrcLoc::c_max_macro_expanison_index )
		{
			// If this is a lambda defined via macro - add macro expansion context to the name.
			src_loc= (*macro_expansion_contexts_)[ macro_expansion_index ].src_loc;
			continue;
		}
		else
			break; // Not a macro expansion context.
	}

	if( !tuple_for_indices.empty() )
	{
		name+= "tf_";

		for( const uint32_t tuple_for_index : tuple_for_indices )
		{
			name+= std::to_string(tuple_for_index);
			name+= "_";
		}
	}

	return name;
}

std::unordered_set<VariablePtr> CodeBuilder::CollectCurrentFunctionVariables( FunctionContext& function_context )
{
	std::unordered_set<VariablePtr> result;
	for( const StackVariablesStorage* const variable_frame : function_context.stack_variables_stack )
	{
		for( const VariablePtr& variable : variable_frame->variables_ )
			result.insert( variable );
	}

	return result;
}

void CodeBuilder::LambdaPreprocessingCheckVariableUsage(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const VariablePtr& variable,
	const std::string& name,
	const SrcLoc& src_loc )
{
	U_ASSERT( function_context.lambda_preprocessing_context != nullptr );
	LambdaPreprocessingContext& lambda_preprocessing_context= *function_context.lambda_preprocessing_context;

	if( lambda_preprocessing_context.external_variables.count( variable ) > 0 &&
		lambda_preprocessing_context.explicit_captures != std::nullopt &&
		lambda_preprocessing_context.explicit_captures->count( variable ) == 0 )
	{
		REPORT_ERROR( VariableIsNotCapturedByLambda, names_scope.GetErrors(), src_loc, name );
		lambda_preprocessing_context.has_preprocessing_errors= true;
	}

	// Do not allow to capture variables through another lambda.
	const LambdaPreprocessingContext* current_context= function_context.lambda_preprocessing_context->parent;
	while( current_context != nullptr )
	{
		if( current_context->external_variables.count( variable ) > 0 )
		{
			REPORT_ERROR( VariableIsNotCapturedByLambda, names_scope.GetErrors(), src_loc, name );
			function_context.variables_state.AddNode( variable ); // Prevent further errors.
			lambda_preprocessing_context.has_preprocessing_errors= true;
		}

		current_context= current_context->parent;
	}
}

VariablePtr CodeBuilder::LambdaPreprocessingAccessExternalVariable(
	FunctionContext& function_context,
	const VariablePtr& variable,
	const std::string& name )
{
	U_ASSERT( function_context.lambda_preprocessing_context != nullptr );
	auto& lambda_preprocessing_context= *function_context.lambda_preprocessing_context;
	U_ASSERT( lambda_preprocessing_context.external_variables.count( variable ) > 0 );

	// Create special temporary reference graph nodes for captured external variable.

	LambdaPreprocessingContext::CapturedVariableData& captured_variable= lambda_preprocessing_context.captured_external_variables[name];

	if( captured_variable.source_variable == nullptr )
		captured_variable.source_variable= variable;

	const auto reference_tag_count= variable->type.ReferenceTagCount();

	if( captured_variable.variable_node == nullptr && captured_variable.reference_node == nullptr )
	{
		// Do not set "constexpr" values - make "constexpr" captured variables non-constexpr in lambdas.

		captured_variable.variable_node=
			Variable::Create(
				variable->type,
				ValueType::Value,
				Variable::Location::Pointer,
				variable->name + " lambda copy",
				variable->llvm_value );

		ValueType value_type= ValueType::ReferenceImut;

		bool capture_by_reference= lambda_preprocessing_context.capture_by_reference;
		if( lambda_preprocessing_context.explicit_captures != std::nullopt )
		{
			// If has explicit captures - use individual by reference capture flags.
			if( const auto it= lambda_preprocessing_context.explicit_captures->find( variable ); it != lambda_preprocessing_context.explicit_captures->end() )
				capture_by_reference= it->second.capture_by_reference;
		}

		if( capture_by_reference )
		{
			// While capturing by reference capture mutable values as mutable references, immutable values as immutable references.
			value_type= variable->value_type;
		}
		else
		{
			// If a variable is captured by value and lambda "this" is immutable captured variable can't be modified.
			value_type= lambda_preprocessing_context.lambda_this_is_mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut;
		}

		captured_variable.reference_node=
			Variable::Create(
				variable->type,
				value_type,
				Variable::Location::Pointer,
				variable->name + " lambda copy ref",
				variable->llvm_value );

		captured_variable.accessible_variables.resize( reference_tag_count );
		for( size_t i= 0; i < reference_tag_count; ++i )
		{
			captured_variable.accessible_variables[i]=
				Variable::Create(
					invalid_type_,
					ValueType::Value,
					Variable::Location::Pointer,
					"referenced variable " + std::to_string(i) + " of captured lambda variable " + variable->name );
		}
	}

	LambdaPreprocessingEnsureCapturedVariableRegistered( function_context, captured_variable );

	return captured_variable.reference_node;
}

void CodeBuilder::LambdaPreprocessingEnsureCapturedVariableRegistered(
	FunctionContext& function_context,
	const LambdaPreprocessingContext::CapturedVariableData& captured_variable )
{
	function_context.variables_state.AddNodeIfNotExists( captured_variable.variable_node );
	function_context.variables_state.AddNodeIfNotExists( captured_variable.reference_node );
	function_context.variables_state.AddLink( captured_variable.variable_node, captured_variable.reference_node );

	const auto reference_tag_count= captured_variable.source_variable->type.ReferenceTagCount();
	for( size_t i= 0; i < reference_tag_count; ++i )
	{
		function_context.variables_state.AddNodeIfNotExists( captured_variable.accessible_variables[i] );
		function_context.variables_state.AddLink( captured_variable.accessible_variables[i], captured_variable.variable_node->inner_reference_nodes[i] );
		function_context.variables_state.AddLink( captured_variable.variable_node->inner_reference_nodes[i], captured_variable.reference_node->inner_reference_nodes[i] );
	}
}

Value CodeBuilder::LambdaPreprocessingHandleCapturedVariableMove(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const VariablePtr& variable,
	const std::string& name,
	const SrcLoc& src_loc )
{
	U_ASSERT( function_context.lambda_preprocessing_context != nullptr );
	U_ASSERT( function_context.lambda_preprocessing_context->external_variables.count( variable ) > 0 );

	const VariablePtr resolved_variable= LambdaPreprocessingAccessExternalVariable( function_context, variable, name );

	if( resolved_variable->value_type != ValueType::ReferenceMut )
	{
		REPORT_ERROR( ExpectedReferenceValue, names_scope.GetErrors(), src_loc );
		return ErrorValue();
	}
	if( function_context.variables_state.HasOutgoingLinks( resolved_variable ) )
	{
		REPORT_ERROR( MovedVariableHasReferences, names_scope.GetErrors(), src_loc, resolved_variable->name );
		return ErrorValue();
	}
	if( function_context.variables_state.NodeMoved( resolved_variable ) )
	{
		REPORT_ERROR( AccessingMovedVariable, names_scope.GetErrors(), src_loc, resolved_variable->name );
		return ErrorValue();
	}

	const VariablePtr result=
		Variable::Create(
			resolved_variable->type,
			ValueType::Value,
			resolved_variable->location,
			"_moved_" + resolved_variable->name,
			resolved_variable->llvm_value,
			resolved_variable->constexpr_value );
	function_context.variables_state.AddNode( result );

	function_context.variables_state.TryAddInnerLinks( resolved_variable, result, names_scope.GetErrors(), src_loc );

	function_context.variables_state.MoveNode( resolved_variable );

	RegisterTemporaryVariable( function_context, result );
	return result;
}

} // namespace U
