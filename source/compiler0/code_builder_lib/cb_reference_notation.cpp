#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

std::optional<uint8_t> CodeBuilder::EvaluateReferenceFieldTag( NamesScope& names_scope, const Synt::Expression& expression )
{
	VariablePtr variable;
	{
		const StackVariablesStorage dummy_stack_variables_storage( *global_function_context_ );
		variable= BuildExpressionCodeEnsureVariable( expression, names_scope, *global_function_context_ );
	}

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
	VariablePtr variable;
	{
		const StackVariablesStorage dummy_stack_variables_storage( *global_function_context_ );
		variable= BuildExpressionCodeEnsureVariable( expression, names_scope, *global_function_context_ );
	}

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

	VariablePtr variable;
	{
		const StackVariablesStorage dummy_stack_variables_storage( *global_function_context_ );
		variable= BuildExpressionCodeEnsureVariable( expression, names_scope, *global_function_context_ );
	}

	const SrcLoc src_loc= Synt::GetExpressionSrcLoc( expression );

	Type expected_element_type;
	{
		ArrayType reference_name;
		reference_name.element_type= FundamentalType( U_FundamentalType::char8_ );
		reference_name.element_count= 2;

		ArrayType reference_pair;
		reference_pair.element_type= std::move(reference_name);
		reference_pair.element_count= 2;

		expected_element_type= std::move(reference_pair);
	}

	const auto array_type= variable->type.GetArrayType();
	if( array_type == nullptr )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, "array of " + expected_element_type.ToString(), variable->type );
		return result;
	}
	if( array_type->element_type != expected_element_type )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, expected_element_type, array_type->element_type );
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
		const llvm::Constant* const dst= pollution_constant->getAggregateElement( uint32_t(0) );
		const llvm::Constant* const src= pollution_constant->getAggregateElement( uint32_t(1) );

		const uint64_t dst_param= dst->getAggregateElement( uint32_t(0) )->getUniqueInteger().getLimitedValue();
		const uint64_t dst_ref= dst->getAggregateElement( uint32_t(1) )->getUniqueInteger().getLimitedValue();
		const uint64_t src_param= src->getAggregateElement( uint32_t(0) )->getUniqueInteger().getLimitedValue();
		const uint64_t src_ref= src->getAggregateElement( uint32_t(1) )->getUniqueInteger().getLimitedValue();

		if( !( dst_param >= '0' && dst_param <= '9' ) )
		{
			REPORT_ERROR( InvalidParamNumber, names_scope.GetErrors(), src_loc, dst_param );
			continue;
		}
		if( !( src_param >= '0' && src_param <= '9' ) )
		{
			REPORT_ERROR( InvalidParamNumber, names_scope.GetErrors(), src_loc, src_param );
			continue;
		}

		if( !( ( dst_ref >= 'a' && dst_ref <= 'z' ) || dst_ref == '_' ) )
		{
			REPORT_ERROR( InvalidInnerReferenceTagName, names_scope.GetErrors(), src_loc, dst_param );
			continue;
		}
		if( !( ( src_ref >= 'a' && src_ref <= 'z' ) || src_ref == '_' ) )
		{
			REPORT_ERROR( InvalidInnerReferenceTagName, names_scope.GetErrors(), src_loc, src_param );
			continue;
		}

		FunctionType::ReferencePollution pollution;
		pollution.dst.first= uint8_t(dst_param - '0');
		pollution.dst.second= dst_ref == '_' ? FunctionType::c_arg_reference_tag_number : uint8_t( dst_ref - 'a' );
		pollution.src.first= uint8_t(src_param - '0');
		pollution.src.second= src_ref == '_' ? FunctionType::c_arg_reference_tag_number : uint8_t( src_ref - 'a' );

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

} // namespace U
