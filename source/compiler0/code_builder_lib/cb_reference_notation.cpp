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
		// TODO - use separate error code.
		REPORT_ERROR( NotImplemented, names_scope.GetErrors(), src_loc, "out of range reference tags" );
		return std::nullopt;
	}
}

} // namespace U
