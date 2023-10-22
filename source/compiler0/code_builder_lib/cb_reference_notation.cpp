#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

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

std::set<FunctionType::ReferencePollution> CodeBuilder::EvaluateFunctionReferencePollution( NamesScope& names_scope, const Synt::Expression& expression )
{
	std::set<FunctionType::ReferencePollution> result;

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
		const llvm::Constant* pollution_constant= variable->constexpr_value->getAggregateElement( uint32_t(i) );

		const auto dst_reference= ParseEvaluatedParamReference( pollution_constant->getAggregateElement( uint32_t(0) ), names_scope, src_loc );
		const auto src_reference= ParseEvaluatedParamReference( pollution_constant->getAggregateElement( uint32_t(1) ), names_scope, src_loc );
		if( dst_reference == std::nullopt || src_reference == std::nullopt )
			continue;

		FunctionType::ReferencePollution pollution;
		pollution.dst= *dst_reference;
		pollution.src= *src_reference;

		if( pollution.dst.second == FunctionType::c_arg_reference_tag_number )
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

std::set<FunctionType::ParamReference> CodeBuilder::EvaluateFunctionReturnReferences( NamesScope& names_scope, const Synt::Expression& expression )
{
	std::set<FunctionType::ParamReference> result;

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
		if( const auto param_reference= ParseEvaluatedParamReference( variable->constexpr_value->getAggregateElement( uint32_t(i) ), names_scope, src_loc ) )
			result.insert( *param_reference );
	}

	return result;
}

std::vector<std::set<FunctionType::ParamReference>> CodeBuilder::EvaluateFunctionReturnInnerReferences( NamesScope& names_scope, const Synt::Expression& expression )
{
	std::vector<std::set<FunctionType::ParamReference>> result;

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
			REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, "array of " + reference_notation_param_reference_description_type_.ToString(), variable->type );
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
			if( const auto param_reference= ParseEvaluatedParamReference( tag_constant->getAggregateElement( uint32_t(j) ), names_scope, src_loc ) )
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

std::optional<FunctionType::ParamReference> CodeBuilder::ParseEvaluatedParamReference( const llvm::Constant* const constant, NamesScope& names_scope, const SrcLoc& src_loc )
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
	param_reference.second= ref == '_' ? FunctionType::c_arg_reference_tag_number : uint8_t( ref - 'a' );
	return param_reference;
}

} // namespace U
