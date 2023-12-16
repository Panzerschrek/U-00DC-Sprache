#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "code_builder.hpp"
#include "error_reporting.hpp"

namespace U
{

Value CodeBuilder::BuildLambda( NamesScope& names, FunctionContext& function_context, const Synt::Lambda& lambda )
{
	const ClassPtr lambda_class= PrepareLambdaClass( names, function_context, lambda );
	// TODO
	(void)lambda_class;

	return ErrorValue();
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
	class_->generated_class_data= LambdaClassData{};

	// TODO - fill fields.
	llvm::SmallVector<llvm::Type*, 16> fields_llvm_types;

	class_->can_be_constexpr= false; // TODO - allow some lambda classes to be constexpr.

	class_->llvm_type= llvm::StructType::create( llvm_context_ ); // TODO - mangle name?
	class_->llvm_type->setBody( fields_llvm_types );
	class_->is_complete= true;

	// Try generate importan methods.
	TryGenerateCopyConstructor( class_ );
	TryGenerateCopyAssignmentOperator( class_ );
	TryGenerateDestructor( class_ );

	// Create () operator.
	{
		const auto call_op_name= OverloadedOperatorToString( OverloadedOperator::Call );

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
			*lambda.function.block, nullptr );

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

} // namespace U
