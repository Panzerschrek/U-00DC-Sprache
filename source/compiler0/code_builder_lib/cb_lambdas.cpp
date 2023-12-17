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
						// TODO - link references.
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
	auto class_ptr= std::make_unique<Class>( "_lambda_TODO_name", &names );
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

	// Run preprocessing.
	if( !std::holds_alternative<Synt::Lambda::CaptureNothing>( lambda.capture ) )
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

		// TODO - order fields to minimize padding.
		for( const auto& captured_varaible_pair : lambda_preprocessing_context.captured_external_variables )
		{
			const auto& name= captured_varaible_pair.first;

			const auto index= uint32_t( class_->fields_order.size() );

			auto field= std::make_shared<ClassField>( class_, captured_varaible_pair.second->type, index, true, false );
			class_->fields_order.push_back( field );

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

} // namespace U
