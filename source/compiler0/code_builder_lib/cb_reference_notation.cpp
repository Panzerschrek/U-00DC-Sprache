#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

namespace
{

std::optional<FunctionType::ParamReference> ParseEvaluatedParamReference(
	const llvm::Constant* const constant,
	const size_t num_params,
	NamesScope& names_scope,
	const SrcLoc& src_loc )
{
	const uint64_t param= constant->getAggregateElement( uint32_t(0) )->getUniqueInteger().getLimitedValue();
	const uint64_t ref= constant->getAggregateElement( uint32_t(1) )->getUniqueInteger().getLimitedValue();

	if( !( param >= '0' && param <= '9' ) )
	{
		REPORT_ERROR( InvalidParamNumber, names_scope.GetErrors(), src_loc, param );
		return std::nullopt;
	}
	if( !( ( ref >= 'a' && ref <= 'z' ) || ref == '_' ) )
	{
		REPORT_ERROR( InvalidInnerReferenceTagName, names_scope.GetErrors(), src_loc, param );
		return std::nullopt;
	}

	FunctionType::ParamReference param_reference;
	param_reference.first= uint8_t(param - '0');
	param_reference.second= ref == '_' ? FunctionType::c_param_reference_number : uint8_t( ref - 'a' );

	if( param_reference.first >= num_params )
	{
		REPORT_ERROR( ParamNumberOutOfRange, names_scope.GetErrors(), src_loc, param_reference.first, num_params );
		return std::nullopt;
	}

	return param_reference;
}

} // namespace

std::optional<uint8_t> CodeBuilder::EvaluateReferenceFieldTag( NamesScope& names_scope, const Synt::Expression& expression )
{
	const VariablePtr variable= EvaluateReferenceNotationExpression( names_scope, expression );
	const SrcLoc src_loc= Synt::GetExpressionSrcLoc( expression );

	const Type expected_type= FundamentalType( U_FundamentalType::char8_ );
	if( variable->type != expected_type )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, expected_type, variable->type );
		return std::nullopt;
	}
	if( variable->constexpr_value == nullptr )
	{
		REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), src_loc );
		return std::nullopt;
	}

	const uint64_t value= variable->constexpr_value->getUniqueInteger().getLimitedValue();
	if( value >= 'a' && value <= 'z' )
		return uint8_t( value - 'a' );
	else
	{
		REPORT_ERROR( InvalidInnerReferenceTagName, names_scope.GetErrors(), src_loc, value );
		return std::nullopt;
	}
}

std::optional< llvm::SmallVector<uint8_t, 4> > CodeBuilder::EvaluateReferenceFieldInnerTags( NamesScope& names_scope, const Synt::Expression& expression )
{
	const VariablePtr variable= EvaluateReferenceNotationExpression( names_scope, expression );
	const SrcLoc src_loc= Synt::GetExpressionSrcLoc( expression );

	const auto array_type= variable->type.GetArrayType();
	if( array_type == nullptr )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, "array of char8", variable->type );
		return std::nullopt;
	}
	const Type expected_element_type= FundamentalType( U_FundamentalType::char8_ );
	if( array_type->element_type != expected_element_type )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, expected_element_type, array_type->element_type );
		return std::nullopt;
	}
	if( variable->constexpr_value == nullptr )
	{
		REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), src_loc );
		return std::nullopt;
	}

	llvm::SmallVector<uint8_t, 4> result;
	for( uint64_t i= 0; i < array_type->element_count; ++i )
	{
		const uint64_t value= variable->constexpr_value->getAggregateElement( uint32_t(i) )->getUniqueInteger().getLimitedValue();
		if( value >= 'a' && value <= 'z' )
			result.push_back( uint8_t(value - 'a') );
		else
		{
			REPORT_ERROR( InvalidInnerReferenceTagName, names_scope.GetErrors(), src_loc, value );
			return std::nullopt;
		}
	}

	return std::move(result);
}

FunctionType::ReferencesPollution CodeBuilder::EvaluateFunctionReferencePollution(
	NamesScope& names_scope,
	const Synt::Expression& expression,
	const size_t num_params )
{
	FunctionType::ReferencesPollution result;

	const VariablePtr variable= EvaluateReferenceNotationExpression( names_scope, expression );
	const SrcLoc src_loc= Synt::GetExpressionSrcLoc( expression );

	const auto array_type= variable->type.GetArrayType();
	if( array_type == nullptr )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, "array of " + reference_notation_pollution_element_type_.ToString(), variable->type );
		return result;
	}
	if( array_type->element_type != reference_notation_pollution_element_type_ )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, reference_notation_pollution_element_type_, array_type->element_type );
		return result;
	}
	if( variable->constexpr_value == nullptr )
	{
		REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), src_loc );
		return result;
	}

	for( uint64_t i= 0; i < array_type->element_count; ++i )
	{
		const llvm::Constant* const pollution_constant= variable->constexpr_value->getAggregateElement( uint32_t(i) );
		const auto dst_reference= ParseEvaluatedParamReference( pollution_constant->getAggregateElement( uint32_t(0) ), num_params, names_scope, src_loc );
		const auto src_reference= ParseEvaluatedParamReference( pollution_constant->getAggregateElement( uint32_t(1) ), num_params, names_scope, src_loc );
		if( dst_reference == std::nullopt || src_reference == std::nullopt )
			continue;

		FunctionType::ReferencePollution pollution;
		pollution.dst= *dst_reference;
		pollution.src= *src_reference;

		if( pollution.dst.second == FunctionType::c_param_reference_number )
		{
			REPORT_ERROR( ArgReferencePollution, names_scope.GetErrors(), src_loc );
			continue;
		}

		if( pollution.dst == pollution.src )
		{
			REPORT_ERROR( SelfReferencePollution, names_scope.GetErrors(), src_loc );
			continue;
		}

		result.insert( pollution );
	}
	return result;
}

FunctionType::ReturnReferences CodeBuilder::EvaluateFunctionReturnReferences(
	NamesScope& names_scope,
	const Synt::Expression& expression,
	const size_t num_params )
{
	FunctionType::ReturnReferences result;

	const VariablePtr variable= EvaluateReferenceNotationExpression( names_scope, expression );
	const SrcLoc src_loc= Synt::GetExpressionSrcLoc( expression );

	const auto array_type= variable->type.GetArrayType();
	if( array_type == nullptr )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, "array of " + reference_notation_param_reference_description_type_.ToString(), variable->type );
		return result;
	}
	if( array_type->element_type != reference_notation_param_reference_description_type_ )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, reference_notation_param_reference_description_type_, array_type->element_type );
		return result;
	}
	if( variable->constexpr_value == nullptr )
	{
		REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), src_loc );
		return result;
	}

	for( uint64_t i= 0; i < array_type->element_count; ++i )
	{
		if( const auto param_reference= ParseEvaluatedParamReference( variable->constexpr_value->getAggregateElement( uint32_t(i) ), num_params, names_scope, src_loc ) )
			result.insert( *param_reference );
	}

	return result;
}

FunctionType::ReturnInnerReferences CodeBuilder::EvaluateFunctionReturnInnerReferences(
	NamesScope& names_scope,
	const Synt::Expression& expression,
	const size_t num_params )
{
	FunctionType::ReturnInnerReferences result;

	const VariablePtr variable= EvaluateReferenceNotationExpression( names_scope, expression );
	const SrcLoc src_loc= Synt::GetExpressionSrcLoc( expression );

	const auto tuple_type= variable->type.GetTupleType();
	if( tuple_type == nullptr )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, "tuple of " + reference_notation_param_reference_description_type_.ToString() + " arrays", variable->type );
		return result;
	}
	if( variable->constexpr_value == nullptr )
	{
		REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), src_loc );
		return result;
	}

	result.resize( tuple_type->element_types.size() );
	for( size_t i= 0; i < tuple_type->element_types.size(); ++i )
	{
		const auto array_type= tuple_type->element_types[i].GetArrayType();
		if( array_type == nullptr )
		{
			REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, "array of " + reference_notation_param_reference_description_type_.ToString(), tuple_type->element_types[i] );
			continue;
		}
		if( array_type->element_type != reference_notation_param_reference_description_type_ )
		{
			REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, reference_notation_param_reference_description_type_, array_type->element_type );
			continue;
		}

		const auto tag_constant= variable->constexpr_value->getAggregateElement( uint32_t(i) );
		for( uint64_t j= 0; j < array_type->element_count; ++j )
		{
			if( const auto param_reference= ParseEvaluatedParamReference( tag_constant->getAggregateElement( uint32_t(j) ), num_params, names_scope, src_loc ) )
				result[i].insert( *param_reference );
		}
	}

	return result;
}

VariablePtr CodeBuilder::EvaluateReferenceNotationExpression( NamesScope& names_scope, const Synt::Expression& expression )
{
	const StackVariablesStorage dummy_stack_variables_storage( *global_function_context_ );
	return BuildExpressionCodeEnsureVariable( expression, names_scope, *global_function_context_ );
}

CodeBuilder::ReferenceNotationConstant CodeBuilder::GetReturnReferencesConstant( const FunctionType::ReturnReferences& return_references )
{
	llvm::SmallVector<llvm::Constant*, 8> constant_initializers;
	constant_initializers.reserve( return_references.size() );
	for( const FunctionType::ParamReference& return_reference : return_references )
		constant_initializers.push_back( GetParamReferenceConstant( return_reference ) );

	ArrayType array_type;
	array_type.element_type= reference_notation_param_reference_description_type_;
	array_type.element_count= constant_initializers.size();
	const auto array_llvm_type= llvm::ArrayType::get( array_type.element_type.GetLLVMType(), array_type.element_count );
	array_type.llvm_type= array_llvm_type;

	return std::make_pair( std::move(array_type), llvm::ConstantArray::get( array_llvm_type, constant_initializers ) );
}

CodeBuilder::ReferenceNotationConstant CodeBuilder::GetReturnInnerReferencesConstant( const FunctionType::ReturnInnerReferences& return_inner_references )
{
	TupleType tuple_type;
	tuple_type.element_types.reserve( return_inner_references.size() );

	llvm::SmallVector<llvm::Type*, 8> elements_llvm_types;
	elements_llvm_types.reserve( return_inner_references.size() );

	llvm::SmallVector<llvm::Constant*, 8> constant_initializers;
	constant_initializers.reserve( return_inner_references.size() );

	for( const auto& return_references : return_inner_references )
	{
		auto return_references_contant= GetReturnReferencesConstant( return_references );
		elements_llvm_types.push_back( return_references_contant.first.GetLLVMType() );
		tuple_type.element_types.push_back( std::move(return_references_contant.first) );
		constant_initializers.push_back( return_references_contant.second );
	}

	const auto tuple_llvm_type= llvm::StructType::get( llvm_context_, elements_llvm_types );
	tuple_type.llvm_type= tuple_llvm_type;

	return std::make_pair( std::move(tuple_type), llvm::ConstantStruct::get( tuple_llvm_type, constant_initializers ) );
}

CodeBuilder::ReferenceNotationConstant CodeBuilder::GetReferencesPollutionConstant( const FunctionType::ReferencesPollution& references_pollution )
{
	llvm::SmallVector<llvm::Constant*, 8> constant_initializers;
	constant_initializers.reserve( references_pollution.size() );

	const auto element_llvm_type= llvm::dyn_cast<llvm::ArrayType>( reference_notation_pollution_element_type_.GetLLVMType() );
	for( const FunctionType::ReferencePollution& pollution : references_pollution )
	{
		llvm::Constant* const initializer[2]
		{
			GetParamReferenceConstant( pollution.dst ),
			GetParamReferenceConstant( pollution.src ),
		};
		constant_initializers.push_back( llvm::ConstantArray::get( element_llvm_type, initializer ) );
	}

	ArrayType array_type;
	array_type.element_type= reference_notation_pollution_element_type_;
	array_type.element_count= constant_initializers.size();
	const auto array_llvm_type= llvm::ArrayType::get( element_llvm_type, array_type.element_count );
	array_type.llvm_type= array_llvm_type;

	return std::make_pair( std::move(array_type), llvm::ConstantArray::get( array_llvm_type, constant_initializers ) );
}

llvm::Constant* CodeBuilder::GetParamReferenceConstant( const FunctionType::ParamReference& param_reference )
{
	const char initializer[2]
	{
		char( '0' + param_reference.first ),
		param_reference.second == FunctionType::c_param_reference_number ? '_' : char( 'a' + param_reference.second ),
	};

	return llvm::ConstantDataArray::get( llvm_context_, initializer );
}

} // namespace U
