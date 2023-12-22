#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "code_builder.hpp"
#include "error_reporting.hpp"

namespace U
{

Value CodeBuilder::BuildLambda( NamesScope& names, FunctionContext& function_context, const Synt::Lambda& lambda )
{
	const ClassPtr lambda_class= PrepareLambdaClass( names, function_context, lambda );

	const VariableMutPtr result=
		Variable::Create(
			lambda_class, ValueType::Value, Variable::Location::Pointer, "TODO - lambda name" );

	function_context.variables_state.AddNode( result );

	if( !function_context.is_functionless_context )
	{
		result->llvm_value= function_context.alloca_ir_builder.CreateAlloca( lambda_class->llvm_type, nullptr, result->name );
		CreateLifetimeStart( function_context, result->llvm_value );
	}

	// Copy captured values.
	if( const auto lambda_class_data= std::get_if<LambdaClassData>( &lambda_class->generated_class_data ) )
	{
		for( const LambdaClassData::Capture& capture : lambda_class_data->captures )
		{
			const NameLookupResult lookup_result= LookupName( names, capture.captured_variable_name, lambda.src_loc );
			if( lookup_result.value != nullptr )
			{
				if( const auto variable= lookup_result.value->value.GetVariable() )
				{
					const auto field_value= CreateClassFieldGEP( function_context, *result, capture.field->index );

					if( capture.field->is_reference )
					{
						// TODO
					}
					else
					{
						U_ASSERT( variable->type == capture.field->type );
						if( !variable->type.IsCopyConstructible() )
							REPORT_ERROR( CopyConstructValueOfNoncopyableType, names.GetErrors(), lambda.src_loc, variable->type );
						else
						{
							BuildCopyConstructorPart(
								field_value, variable->llvm_value,
								variable->type,
								function_context );
						}

						// Link references.
						const size_t reference_tag_count= capture.field->type.ReferenceTagCount();
						U_ASSERT( capture.field->inner_reference_tags.size() == reference_tag_count );
						U_ASSERT( variable->inner_reference_nodes.size() == reference_tag_count );
						for( size_t i= 0; i < reference_tag_count; ++i )
						{
							const size_t dst_node_index= capture.field->inner_reference_tags[i];
							U_ASSERT( dst_node_index < result->inner_reference_nodes.size() );
							function_context.variables_state.TryAddLink( variable->inner_reference_nodes[i], result->inner_reference_nodes[ dst_node_index ], names.GetErrors(), lambda.src_loc );
						}
					}
				}
			}
		}
	}

	RegisterTemporaryVariable( function_context, result );
	return result;
}

ClassPtr CodeBuilder::PrepareLambdaClass( NamesScope& names, FunctionContext& function_context, const Synt::Lambda& lambda )
{
	LambdaKey key;
	key.template_args_namespace= nullptr; // TODO - extract closest template args namespace.
	key.src_loc= lambda.src_loc;

	if( const auto it= lambda_classes_table_.find(key); it != lambda_classes_table_.end() )
	{
		// Already generated.
		// This may happens because of expressions preevaluation.
		return it->second.get();
	}

	// TODO - preprocess lambda to build capture list.

	// Create the class.

	// Use some stable namespace as parent for class members namespace.
	// We can't use namespace of function variables here, because it will be destroyed later.
	// TODO - use closest global namespace.

	auto class_ptr= std::make_unique<Class>( "_lambda_TODO_name", names.GetRoot() );
	Class* const class_= class_ptr.get();
	lambda_classes_table_.emplace( std::move(key), std::move(class_ptr) );

	class_->src_loc= lambda.src_loc;
	class_->kind= Class::Kind::NonPolymorph;
	class_->parents_list_prepared= true;
	class_->have_explicit_noncopy_constructors= false;
	class_->is_default_constructible= false;
	class_->can_be_constexpr= false; // TODO - allow some lambda classes to be constexpr.
	class_->generated_class_data= LambdaClassData{};
	class_->llvm_type= llvm::StructType::create( llvm_context_ ); // TODO - mangle name?

	const auto call_op_name= OverloadedOperatorToString( OverloadedOperator::Call );

	std::set<FunctionType::ParamReference> return_references;

	// Run preprocessing.
	{
		LambdaPreprocessingContext lambda_preprocessing_context;
		lambda_preprocessing_context.external_variables= CallectCurrentFunctionVariables( function_context );

		FunctionVariable op_variable;
		// It's fine to use incomplete lambda class here, since this class can't be accessed.
		op_variable.type= PrepareLambdaCallOperatorType( names, function_context, lambda.function.type, class_ );
		op_variable.llvm_function= std::make_shared<LazyLLVMFunction>( mangler_->MangleFunction( names, call_op_name, op_variable.type ) );
		op_variable.is_this_call= true;

		BuildFuncCode(
			op_variable,
			class_,
			names,
			call_op_name,
			lambda.function.type.params,
			*lambda.function.block,
			nullptr,
			&lambda_preprocessing_context );

		// Remove temp function.
		op_variable.llvm_function->function->eraseFromParent();

		// Collect actual return references in lambdas.
		return_references= std::move(lambda_preprocessing_context.return_references);

		// TODO - order fields to minimize padding.
		for( const auto& captured_variable_pair : lambda_preprocessing_context.captured_external_variables )
		{
			const auto& name= captured_variable_pair.first;
			const Type& type= captured_variable_pair.second.source_variable->type;

			const auto index= uint32_t( class_->fields_order.size() );

			auto field= std::make_shared<ClassField>( class_, type, index, true, false );
			class_->fields_order.push_back( field );

			// Each reference tag of each captured variable get its own reference tag in result lambda class.
			const auto reference_tag_cout= type.ReferenceTagCount();
			field->inner_reference_tags.reserve( reference_tag_cout );
			for( size_t i= 0; i < reference_tag_cout; ++i )
			{
				field->inner_reference_tags.push_back( uint8_t( class_->inner_references.size() ) );
				class_->inner_references.push_back( type.GetInnerReferenceType( i ) );
			}

			// Check if references to this variable or its inner references are returned and populate return references container.
			for( const VariablePtr& captured_variable_return_reference : lambda_preprocessing_context.captured_variables_return_references )
			{
				// Lambda is "this" (argument 0).
				const uint8_t param_index= 0;

				// A reference to captured variable itself - it bacame a reference to lamba "this" itself.
				if( captured_variable_return_reference == captured_variable_pair.second.variable_node )
					return_references.emplace( param_index, FunctionType::c_param_reference_number );

				for( size_t i= 0; i < captured_variable_pair.second.accessible_variables.size(); ++i )
				{
					// An inner reference of a captured by value variable - it became an inner reference to "this".
					if( captured_variable_return_reference == captured_variable_pair.second.accessible_variables[i] )
						return_references.emplace( param_index, field->inner_reference_tags[i] );
				}
			}

			class_->members->AddName( name, NamesScopeValue( field, lambda.src_loc ) );

			LambdaClassData::Capture capture;
			capture.captured_variable_name= name;
			capture.field= std::move(field);
			std::get_if<LambdaClassData>( &class_->generated_class_data )->captures.push_back( std::move(capture) );
		}
	}

	llvm::SmallVector<llvm::Type*, 16> fields_llvm_types;
	fields_llvm_types.reserve( class_->fields_order.size() );
	for( const auto& field : class_->fields_order )
	{
		llvm::Type* llvm_type= field->type.GetLLVMType();
		if( field->is_reference )
			llvm_type= llvm_type->getPointerTo();

		fields_llvm_types.push_back( llvm_type );
	}

	class_->llvm_type->setBody( fields_llvm_types );
	class_->is_complete= true;

	// Try generate importan methods.
	TryGenerateCopyConstructor( class_ );
	TryGenerateCopyAssignmentOperator( class_ );
	TryGenerateDestructor( class_ );

	// Create () operator.
	{
		FunctionVariable op_variable;
		op_variable.type= PrepareLambdaCallOperatorType( names, function_context, lambda.function.type, class_ );
		op_variable.type.return_references= std::move(return_references);
		op_variable.llvm_function= std::make_shared<LazyLLVMFunction>( mangler_->MangleFunction( names, call_op_name, op_variable.type ) );
		op_variable.is_this_call= true;

		auto functions_set= std::make_shared<OverloadedFunctionsSet>();
		functions_set->functions.push_back( op_variable );

		BuildFuncCode(
			functions_set->functions.back(),
			class_,
			names,
			call_op_name,
			lambda.function.type.params,
			*lambda.function.block,
			nullptr );

		class_->members->AddName(
			call_op_name,
			NamesScopeValue( std::move(functions_set), lambda.src_loc ) );
	}

	return class_;
}

FunctionType CodeBuilder::PrepareLambdaCallOperatorType(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::FunctionType& lambda_function_type,
	const ClassPtr lambda_class_type )
{
	FunctionType function_type;

	if( lambda_function_type.return_type == nullptr )
		function_type.return_type= void_type_;
	else
		function_type.return_type= PrepareType( *lambda_function_type.return_type, names, function_context );

	if( lambda_function_type.return_value_reference_modifier == ReferenceModifier::None )
		function_type.return_value_type= ValueType::Value;
	else
		function_type.return_value_type= lambda_function_type.return_value_mutability_modifier == MutabilityModifier::Mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut;

	function_type.params.reserve( lambda_function_type.params.size() + 1 );

	// First param is always "this" of the lambda type.
	const llvm::ArrayRef<Synt::FunctionParam> synt_params= lambda_function_type.params;
	U_ASSERT( ! synt_params.empty() && synt_params.front().name == Keywords::this_ );
	{
		FunctionType::Param this_param;
		this_param.value_type= ValueType::ReferenceImut; // TODO - allow to specify it.
		this_param.type= lambda_class_type;

		function_type.params.push_back( std::move( this_param ) );
	}

	for( const Synt::FunctionParam& in_param : synt_params.drop_front() )
	{
		FunctionType::Param out_param;

		if( IsKeyword( in_param.name ) )
			REPORT_ERROR( UsingKeywordAsName, names.GetErrors(), in_param.src_loc );

		out_param.type= PrepareType( in_param.type, names, function_context );

		if( in_param.reference_modifier == Synt::ReferenceModifier::None )
			out_param.value_type= ValueType::Value;
		else if( in_param.mutability_modifier == Synt::MutabilityModifier::Mutable )
			out_param.value_type= ValueType::ReferenceMut;
		else
			out_param.value_type= ValueType::ReferenceImut;


		function_type.params.push_back( std::move(out_param) );
	}

	function_type.unsafe= lambda_function_type.unsafe;
	function_type.calling_convention= GetLLVMCallingConvention( lambda_function_type.calling_convention, lambda_function_type.src_loc, names.GetErrors() );

	// TODO - somehow deal with reference notation.
	return function_type;
}

std::unordered_set<VariablePtr> CodeBuilder::CallectCurrentFunctionVariables( FunctionContext& function_context )
{
	std::unordered_set<VariablePtr> result;
	for( const StackVariablesStorage* const variable_frame : function_context.stack_variables_stack )
	{
		for( const VariablePtr& variable : variable_frame->variables_ )
			result.insert( variable );
	}

	return result;
}

VariablePtr CodeBuilder::LambdaPreprocessingAccessExternalVariable( FunctionContext& function_context, const VariablePtr& variable, const std::string& name )
{
	U_ASSERT( function_context.lambda_preprocessing_context != nullptr );
	U_ASSERT( function_context.lambda_preprocessing_context->external_variables.count( variable ) > 0 );

	// Create special temporary reference grap nodes for captured external variable.

	LambdaPreprocessingContext::CapturedVariableData& captured_variable= function_context.lambda_preprocessing_context->captured_external_variables[name];

	if( captured_variable.source_variable == nullptr )
		captured_variable.source_variable= variable;

	// TODO - what should we do with contexpr values?

	const auto reference_tag_count= variable->type.ReferenceTagCount();

	if( captured_variable.variable_node == nullptr && captured_variable.reference_node == nullptr )
	{
		captured_variable.variable_node=
			Variable::Create(
				variable->type,
				ValueType::Value,
				Variable::Location::Pointer,
				variable->name + " lambda copy",
				variable->llvm_value );

		captured_variable.reference_node=
			Variable::Create(
				variable->type,
				variable->value_type,
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

	function_context.variables_state.AddNodeIfNotExists( captured_variable.variable_node );
	function_context.variables_state.AddNodeIfNotExists( captured_variable.reference_node );
	function_context.variables_state.AddLink( captured_variable.variable_node, captured_variable.reference_node );

	for( size_t i= 0; i < reference_tag_count; ++i )
	{
		function_context.variables_state.AddNodeIfNotExists( captured_variable.accessible_variables[i] );
		function_context.variables_state.AddLink( captured_variable.accessible_variables[i], captured_variable.variable_node->inner_reference_nodes[i] );
		function_context.variables_state.AddLink( captured_variable.variable_node->inner_reference_nodes[i], captured_variable.reference_node->inner_reference_nodes[i] );
	}

	return captured_variable.reference_node;
}

} // namespace U
